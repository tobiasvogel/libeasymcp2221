#include "mcp2221.h"
#include "mcp2221_internal.h"
#include "mcp2221_internal_analog.h"
#include "mcp2221_internal_usb.h"

#include <libusb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mcp2221_internal_constants.h"
#include "mcp2221_i2c_slave.h"
#include "mcp2221_flash.h"

struct mcp2221_device {
	libusb_device_handle *handle;
	uint8_t ep_in;
	uint8_t ep_out;
	int iface;
	uint8_t bus;
	uint8_t addr;
	int refcount;
	int kernel_driver_detached;

	int usb_read_timeout_ms;
	int cmd_retries;
	int debug_messages;
	int trace_packets;

	int i2c_dirty;

	// Cache of GPIO settings bytes as used by EasyMCP2221 (SRAM-style GP0..GP3 bytes).
	// Python keeps an internal status because GPIO_write does not alter SRAM and should not be overwritten
	// by subsequent SRAM_config calls.
	uint8_t gpio_status[4];
	int gpio_status_valid;

	// Application-supplied supply voltage used when ADC or DAC reference is VDD.
	mcp2221_internal_analog_state_t analog;

	// Enumeration-time USB settings that cannot be changed through the normal
	// SRAM configuration command. Persisted by mcp2221_flash_save_config().
	mcp2221_internal_usb_state_t usb;
};

mcp2221_internal_usb_state_t *mcp2221_internal_usb_get_state(mcp2221_t *dev) {
	return dev ? &dev->usb : NULL;
}

const mcp2221_internal_usb_state_t *mcp2221_internal_usb_get_state_const(const mcp2221_t *dev) {
	return dev ? &dev->usb : NULL;
}

// Match Python's round() behaviour for non-negative values: ties-to-even.
// Python: round(x) rounds halves to the nearest even integer.
static long round_ties_to_even_pos(double x) {
	long f = (long)x;  // truncation == floor() for x >= 0
	double frac = x - (double)f;
	if (frac < 0.5) {
		return f;
	}
	if (frac > 0.5) {
		return f + 1;
	}
	return (f % 2 == 0) ? f : f + 1;
}

// Simple catalog; protected by g_global_state_mutex.
typedef struct {
	uint8_t bus;
	uint8_t addr;
	char serial[64];
	mcp2221_t *dev;
} catalog_entry_t;

#define MCP2221_CATALOG_MAX 16
static catalog_entry_t g_catalog[MCP2221_CATALOG_MAX];

static mcp2221_t *catalog_find(uint8_t bus, uint8_t addr, const char *serial) {
	for (int i = 0; i < MCP2221_CATALOG_MAX; i++) {
		if (!g_catalog[i].dev)
			continue;
		if (g_catalog[i].bus != bus || g_catalog[i].addr != addr)
			continue;
		if (serial && serial[0] && g_catalog[i].serial[0]) {
			if (strcmp(g_catalog[i].serial, serial) != 0)
				continue;
		}
		return g_catalog[i].dev;
	}
	return NULL;
}

static void catalog_add(mcp2221_t *dev, const char *serial) {
	for (int i = 0; i < MCP2221_CATALOG_MAX; i++) {
		if (!g_catalog[i].dev) {
			g_catalog[i].dev = dev;
			g_catalog[i].bus = dev->bus;
			g_catalog[i].addr = dev->addr;
			if (serial && serial[0])
				strncpy(g_catalog[i].serial, serial, sizeof(g_catalog[i].serial) - 1);
			else
				g_catalog[i].serial[0] = 0;
			return;
		}
	}
}

static void catalog_remove(mcp2221_t *dev) {
	for (int i = 0; i < MCP2221_CATALOG_MAX; i++) {
		if (g_catalog[i].dev == dev) {
			memset(&g_catalog[i], 0, sizeof(g_catalog[i]));
			return;
		}
	}
}

static libusb_context *g_libusb_ctx = NULL;
static int g_libusb_refcount = 0;

/* Protects global libusb context/refcount and the device catalog.
 * It intentionally does not serialize I2C/GPIO/Flash operations on an already
 * opened handle; callers must still avoid concurrent operations on the same
 * MCP2221 instance.
 */
static pthread_mutex_t g_global_state_mutex = PTHREAD_MUTEX_INITIALIZER;

static void mcp2221_global_state_lock(void) {
	pthread_mutex_lock(&g_global_state_mutex);
}

static void mcp2221_global_state_unlock(void) {
	pthread_mutex_unlock(&g_global_state_mutex);
}

static mcp2221_error_code_t libusb_context_acquire(void) {
	if (g_libusb_refcount == 0) {
		int err = libusb_init(&g_libusb_ctx);
		if (err != 0)
			return err == LIBUSB_ERROR_NO_MEM ? MCP2221_ERR_NO_MEMORY : MCP2221_ERR_USB_INIT;
	}
	g_libusb_refcount++;
	return MCP2221_ERR_OK;
}

static void libusb_context_release(void) {
	if (g_libusb_refcount <= 0)
		return;

	g_libusb_refcount--;
	if (g_libusb_refcount == 0) {
		libusb_exit(g_libusb_ctx);
		g_libusb_ctx = NULL;
	}
}

// Timeout helper
static double now_seconds(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + ts.tv_nsec / 1e9;
}

static void i2c_poll_delay(void) {
	struct timespec ts = {0, 1000 * 1000}; /* 1 ms */
	nanosleep(&ts, NULL);
}

// --- Internal GPIO status helpers (Python compatibility) ---

mcp2221_error_code_t mcp2221_internal_ensure_gpio_status(mcp2221_t *dev) {
	if (!dev)
		return MCP2221_ERR_INVALID;
	if (dev->gpio_status_valid)
		return MCP2221_ERR_OK;

	uint8_t cmd = MCP2221_CMD_GET_SRAM_SETTINGS;
	uint8_t resp[MCP2221_PACKET_SIZE];
	mcp2221_error_code_t err = mcp2221_send_cmd(dev, &cmd, 1, resp);
	if (err != MCP2221_ERR_OK)
		return err;

	// EasyMCP2221 v1.8.4: settings[22..25] are GP0..GP3 config bytes.
	dev->gpio_status[0] = resp[22];
	dev->gpio_status[1] = resp[23];
	dev->gpio_status[2] = resp[24];
	dev->gpio_status[3] = resp[25];
	dev->gpio_status_valid = 1;

	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_internal_gpio_status_get(mcp2221_t *dev, uint8_t out_gp[4]) {
	if (!dev || !out_gp)
		return MCP2221_ERR_INVALID;
	if (!dev->gpio_status_valid)
		return MCP2221_ERR_INVALID;
	memcpy(out_gp, dev->gpio_status, 4);
	return MCP2221_ERR_OK;
}

void mcp2221_internal_gpio_status_set(mcp2221_t *dev, const uint8_t gp[4]) {
	if (!dev || !gp)
		return;
	memcpy(dev->gpio_status, gp, 4);
	dev->gpio_status_valid = 1;
}

void mcp2221_internal_gpio_status_update_out(mcp2221_t *dev, int pin, int out_value) {
	if (!dev || pin < 0 || pin > 3)
		return;
	if (!dev->gpio_status_valid)
		return;
	if (out_value)
		dev->gpio_status[pin] |= MCP2221_GPIO_OUT_VAL_1;
	else
		dev->gpio_status[pin] &= (uint8_t)~MCP2221_GPIO_OUT_VAL_1;
}

// --- Internal analog state helpers ---

mcp2221_error_code_t mcp2221_internal_analog_set_vdd(
	mcp2221_t *dev,
	double volts) {
	if (!dev)
		return MCP2221_ERR_INVALID;

	return mcp2221_internal_analog_state_set_vdd(
		&dev->analog,
		volts);
}

mcp2221_error_code_t mcp2221_internal_analog_get_vdd(
	const mcp2221_t *dev,
	double *volts) {
	if (!dev)
		return MCP2221_ERR_INVALID;

	return mcp2221_internal_analog_state_get_vdd(
		&dev->analog,
		volts);
}

static mcp2221_error_code_t map_libusb_discovery_error(int libusb_error, mcp2221_error_code_t fallback) {
	switch (libusb_error) {
		case LIBUSB_ERROR_NO_MEM:
			return MCP2221_ERR_NO_MEMORY;
		case LIBUSB_ERROR_ACCESS:
			return MCP2221_ERR_ACCESS;
		case LIBUSB_ERROR_BUSY:
			return MCP2221_ERR_BUSY;
		case LIBUSB_ERROR_NO_DEVICE:
			return MCP2221_ERR_NOT_FOUND;
		default:
			return fallback;
	}
}

static int open_error_priority(mcp2221_error_code_t error) {
	switch (error) {
		case MCP2221_ERR_NO_MEMORY:
			return 5;
		case MCP2221_ERR_ACCESS:
			return 4;
		case MCP2221_ERR_BUSY:
			return 3;
		case MCP2221_ERR_USB_CLAIM:
		case MCP2221_ERR_USB_OPEN:
		case MCP2221_ERR_USB_ENUM:
			return 2;
		case MCP2221_ERR_NOT_FOUND:
		default:
			return 1;
	}
}

static void remember_open_error(mcp2221_error_code_t *best_error, mcp2221_error_code_t candidate) {
	if (best_error && open_error_priority(candidate) > open_error_priority(*best_error))
		*best_error = candidate;
}

// Open usb device (optionally scan flash serial if usbserial not enumerated)
static mcp2221_error_code_t open_by_vid_pid(uint16_t vid, uint16_t pid, int devnum, const char *usbserial,
											  int scan_serial, int *iface, uint8_t *ep_in, uint8_t *ep_out, uint8_t *bus,
											  uint8_t *addr, char *found_serial, size_t found_serial_len,
											  int *kernel_driver_detached, libusb_device_handle **out_handle) {
	if (!out_handle)
		return MCP2221_ERR_INVALID;
	*out_handle = NULL;

	libusb_device **list = NULL;
	ssize_t cnt = libusb_get_device_list(g_libusb_ctx, &list);
	if (cnt < 0)
		return map_libusb_discovery_error((int)cnt, MCP2221_ERR_USB_ENUM);

	libusb_device_handle *found = NULL;
	mcp2221_error_code_t best_error = MCP2221_ERR_NOT_FOUND;
	int index = 0;

	for (ssize_t i = 0; i < cnt; ++i) {
		struct libusb_device_descriptor desc;
		if (libusb_get_device_descriptor(list[i], &desc) != 0)
			continue;
		if (desc.idVendor != vid || desc.idProduct != pid)
			continue;

		struct libusb_config_descriptor *cfg;
		int config_err = libusb_get_active_config_descriptor(list[i], &cfg);
		if (config_err != 0) {
			remember_open_error(&best_error, map_libusb_discovery_error(config_err, MCP2221_ERR_USB_ENUM));
			continue;
		}
		int ifnum = 0;
		uint8_t in = MCP2221_DEFAULT_EP_IN, out = MCP2221_DEFAULT_EP_OUT; /* default */

		for (int ic = 0; ic < cfg->bNumInterfaces; ++ic) {
			const struct libusb_interface *iface_desc = &cfg->interface[ic];
			for (int al = 0; al < iface_desc->num_altsetting; ++al) {
				const struct libusb_interface_descriptor *alt = &iface_desc->altsetting[al];
				for (int e = 0; e < alt->bNumEndpoints; ++e) {
					const struct libusb_endpoint_descriptor *ep = &alt->endpoint[e];
					if ((ep->bmAttributes & 0x3) == LIBUSB_TRANSFER_TYPE_INTERRUPT ||
						(ep->bmAttributes & 0x3) == LIBUSB_TRANSFER_TYPE_BULK) {
						if (ep->bEndpointAddress & LIBUSB_ENDPOINT_IN)
							in = ep->bEndpointAddress;
						else
							out = ep->bEndpointAddress;
					}
				}
				if (in && out) {
					ifnum = alt->bInterfaceNumber;
					break;
				}
			}
		}

		if (usbserial) {
			// check if serial was provided
			libusb_device_handle *h;
			int open_err = libusb_open(list[i], &h);
			if (open_err != 0) {
				remember_open_error(&best_error, map_libusb_discovery_error(open_err, MCP2221_ERR_USB_OPEN));
				libusb_free_config_descriptor(cfg);
				continue;
			}
			unsigned char s[256];
			int r = libusb_get_string_descriptor_ascii(h, desc.iSerialNumber, s, sizeof(s));
			if (r > 0 && strcmp((char *)s, usbserial) == 0) {
				found = h;
				if (found_serial && found_serial_len > 0)
					strncpy(found_serial, (char *)s, found_serial_len - 1);
			} else if (scan_serial) {
				// Flash-based serial scan (best-effort)
				int detached = 0;
				if (libusb_kernel_driver_active(h, ifnum) == 1) {
					if (libusb_detach_kernel_driver(h, ifnum) == 0)
						detached = 1;
				}
				int claim_err = libusb_claim_interface(h, ifnum);
				if (claim_err == 0) {
					mcp2221_t tmp = {0};
					tmp.handle = h;
					tmp.ep_in = in ? in : MCP2221_DEFAULT_EP_IN;
					tmp.ep_out = out ? out : MCP2221_DEFAULT_EP_OUT;
					tmp.iface = ifnum;
					tmp.usb_read_timeout_ms = 500;
					tmp.cmd_retries = 0;

					uint8_t raw[60];
					if (mcp2221_flash_read(&tmp, MCP2221_FLASH_DATA_USB_SERIALNUM, raw) == MCP2221_ERR_OK) {
						char parsed[128] = {0};
						mcp2221_internal_parse_wchar_structure(raw, parsed, sizeof(parsed));
						if (parsed[0] && strcmp(parsed, usbserial) == 0) {
							found = h;
							if (found_serial && found_serial_len > 0)
								strncpy(found_serial, parsed, found_serial_len - 1);
						}
					}
					libusb_release_interface(h, ifnum);
				} else {
					remember_open_error(&best_error, map_libusb_discovery_error(claim_err, MCP2221_ERR_USB_CLAIM));
				}
				if (found) {
					if (kernel_driver_detached)
						*kernel_driver_detached = detached;
				} else {
					if (detached)
						libusb_attach_kernel_driver(h, ifnum);
					libusb_close(h);
				}
			} else {
				libusb_close(h);
			}
		} else {
			// else choose by index
			if (index++ != devnum) {
				libusb_free_config_descriptor(cfg);
				continue;
			}
			int open_err = libusb_open(list[i], &found);
			if (open_err != 0) {
				best_error = map_libusb_discovery_error(open_err, MCP2221_ERR_USB_OPEN);
				libusb_free_config_descriptor(cfg);
				found = NULL;
				break;
			}
		}

		libusb_free_config_descriptor(cfg);

		if (found) {
			if (iface)
				*iface = ifnum;
			if (ep_in)
				*ep_in = in;
			if (ep_out)
				*ep_out = out;
			break;
		}
	}

	libusb_free_device_list(list, 1);
	if (!found)
		return best_error;

	if (bus && addr) {
		libusb_device *d = libusb_get_device(found);
		*bus = libusb_get_bus_number(d);
		*addr = libusb_get_device_address(d);
	}
	*out_handle = found;
	return MCP2221_ERR_OK;
}

static void close_open_handle(libusb_device_handle *handle, int iface, int kernel_driver_detached, int release_interface) {
	if (!handle)
		return;

	if (release_interface)
		libusb_release_interface(handle, iface);
	if (kernel_driver_detached)
		libusb_attach_kernel_driver(handle, iface);
	libusb_close(handle);
}

static int claim_open_handle(libusb_device_handle *handle, int iface, int *kernel_driver_detached) {
	if (!handle || !kernel_driver_detached)
		return LIBUSB_ERROR_INVALID_PARAM;

	if (!*kernel_driver_detached && libusb_kernel_driver_active(handle, iface) == 1) {
		if (libusb_detach_kernel_driver(handle, iface) == 0)
			*kernel_driver_detached = 1;
	}

	return libusb_claim_interface(handle, iface);
}

static mcp2221_t *allocate_device_context(libusb_device_handle *handle, int iface, uint8_t ep_in, uint8_t ep_out,
										 uint8_t bus, uint8_t addr, int kernel_driver_detached,
										 int usb_read_timeout_ms, int cmd_retries, int debug_messages,
										 int trace_packets) {
	mcp2221_t *dev = calloc(1, sizeof(mcp2221_t));
	if (!dev)
		return NULL;

	dev->handle = handle;
	dev->ep_in = ep_in ? ep_in : MCP2221_DEFAULT_EP_IN;
	dev->ep_out = ep_out ? ep_out : MCP2221_DEFAULT_EP_OUT;
	dev->iface = iface;
	dev->usb_read_timeout_ms = (usb_read_timeout_ms < 0) ? 0 : usb_read_timeout_ms;
	dev->cmd_retries = (cmd_retries < 0) ? 0 : cmd_retries;
	dev->debug_messages = debug_messages;
	dev->trace_packets = trace_packets;
	dev->i2c_dirty = 0;
	dev->bus = bus;
	dev->addr = addr;
	dev->refcount = 1;
	dev->kernel_driver_detached = kernel_driver_detached;
	dev->gpio_status_valid = 0;

	/* Analog state */
	dev->analog.vdd = 0.0;
	dev->analog.vdd_valid = 0;

	return dev;
}

static mcp2221_error_code_t mcp2221_open_core(uint16_t vid, uint16_t pid, int devnum, const char *usbserial,
										  int usb_read_timeout_ms, int cmd_retries, int debug_messages,
										  int trace_packets, int scan_serial, mcp2221_t **out_dev) {
	if (!out_dev)
		return MCP2221_ERR_INVALID;
	*out_dev = NULL;

	mcp2221_error_code_t err;
	mcp2221_global_state_lock();

	err = libusb_context_acquire();
	if (err != MCP2221_ERR_OK)
		goto out;

	char found_serial[128] = {0};

	int iface = 0;
	int kernel_driver_detached = 0;
	uint8_t ep_in = 0, ep_out = 0;
	uint8_t bus = 0, addr = 0;
	libusb_device_handle *h = NULL;

	err = open_by_vid_pid(vid, pid, devnum, usbserial, scan_serial, &iface, &ep_in, &ep_out, &bus, &addr, found_serial,
						  sizeof(found_serial), &kernel_driver_detached, &h);
	if (err != MCP2221_ERR_OK) {
		libusb_context_release();
		goto out;
	}

	const char *match_serial = (usbserial && usbserial[0]) ? usbserial : found_serial;
	mcp2221_t *existing = catalog_find(bus, addr, match_serial);
	if (existing) {
		close_open_handle(h, iface, kernel_driver_detached, 0);
		existing->refcount++;
		libusb_context_release();
		*out_dev = existing;
		err = MCP2221_ERR_OK;
		goto out;
	}

	int claim_err = claim_open_handle(h, iface, &kernel_driver_detached);
	if (claim_err != 0) {
		err = map_libusb_discovery_error(claim_err, MCP2221_ERR_USB_CLAIM);
		close_open_handle(h, iface, kernel_driver_detached, 0);
		libusb_context_release();
		goto out;
	}

	mcp2221_t *dev = allocate_device_context(h, iface, ep_in, ep_out, bus, addr, kernel_driver_detached,
										 usb_read_timeout_ms, cmd_retries, debug_messages, trace_packets);
	if (!dev) {
		err = MCP2221_ERR_NO_MEMORY;
		close_open_handle(h, iface, kernel_driver_detached, 1);
		libusb_context_release();
		goto out;
	}

	catalog_add(dev, match_serial);
	*out_dev = dev;
	err = MCP2221_ERR_OK;

out:
	mcp2221_global_state_unlock();
	return err;
}

mcp2221_t *mcp2221_open(uint16_t vid, uint16_t pid, int devnum, const char *usbserial, int usb_read_timeout_ms,
					  int cmd_retries, int debug_messages, int trace_packets) {
	return mcp2221_open_scan(vid, pid, devnum, usbserial, usb_read_timeout_ms, cmd_retries, debug_messages, trace_packets,
							 0);
}

mcp2221_t *mcp2221_open_scan(uint16_t vid, uint16_t pid, int devnum, const char *usbserial, int usb_read_timeout_ms,
						   int cmd_retries, int debug_messages, int trace_packets, int scan_serial) {
	mcp2221_t *dev = NULL;
	(void)mcp2221_open_core(vid, pid, devnum, usbserial, usb_read_timeout_ms, cmd_retries, debug_messages, trace_packets,
							 scan_serial, &dev);
	return dev;
}

mcp2221_t *mcp2221_open_simple(uint16_t vid, uint16_t pid, int devnum, const char *usbserial, int i2c_speed_hz) {
	return mcp2221_open_simple_scan(vid, pid, devnum, usbserial, i2c_speed_hz, 0);
}

mcp2221_t *mcp2221_open_simple_scan(uint16_t vid, uint16_t pid, int devnum, const char *usbserial, int i2c_speed_hz,
								  int scan_serial) {
	// Default values as in the Python module
	int usb_read_timeout_ms = 500;
	int cmd_retries = 3;
	int debug = 0;
	int trace = 0;

	mcp2221_t *dev =
		mcp2221_open_scan(vid, pid, devnum, usbserial, usb_read_timeout_ms, cmd_retries, debug, trace, scan_serial);

	if (!dev)
		return NULL;

	// Best effort: release any stale I2C state (mirrors Python __init__ post-open behavior)
	(void)mcp2221_i2c_release(dev);

	/* Match EasyMCP2221's initialization sequence:
	 * Device.__init__() first sets the bus to the safer 100 kHz value because
	 * some device revisions may power up at 500 kHz. Helpers that accept an
	 * explicit speed then apply the requested speed afterwards.
	 */
	mcp2221_error_code_t err = mcp2221_i2c_set_speed(dev, 100000);
	if (err != MCP2221_ERR_OK) {
		mcp2221_close(dev);
		return NULL;
	}

	// Apply requested I2C speed if it differs from the safe initialization value.
	int target_i2c_speed_hz = (i2c_speed_hz > 0) ? i2c_speed_hz : 100000;
	if (target_i2c_speed_hz != 100000) {
		err = mcp2221_i2c_set_speed(dev, target_i2c_speed_hz);
		if (err != MCP2221_ERR_OK) {
			mcp2221_close(dev);
			return NULL;
		}
	}

	// Preload GPIO status cache (so later SRAM/save_config uses current values)
	(void)mcp2221_internal_ensure_gpio_status(dev);

	return dev;
}

void mcp2221_close(mcp2221_t *dev) {
	if (!dev)
		return;

	mcp2221_global_state_lock();
	if (dev->refcount > 1) {
		dev->refcount--;
		mcp2221_global_state_unlock();
		return;
	}
	if (dev->handle) {
		libusb_release_interface(dev->handle, dev->iface);
		if (dev->kernel_driver_detached)
			libusb_attach_kernel_driver(dev->handle, dev->iface);
		libusb_close(dev->handle);
	}
	catalog_remove(dev);
	free(dev);
	libusb_context_release();
	mcp2221_global_state_unlock();
}

static mcp2221_error_code_t usb_write_report(mcp2221_t *dev, const uint8_t *data, size_t len) {
	if (!dev || !dev->handle)
		return MCP2221_ERR_USB;
	if (len > MCP2221_PACKET_SIZE)
		return MCP2221_ERR_INVALID;

	uint8_t buf[MCP2221_PACKET_SIZE];
	memset(buf, 0, sizeof(buf));
	memcpy(buf, data, len);

	int transferred = 0;
	int r = libusb_interrupt_transfer(dev->handle, dev->ep_out, buf, MCP2221_PACKET_SIZE, &transferred, 500);
	if (r != 0)
		return MCP2221_ERR_USB;
	if (transferred != MCP2221_PACKET_SIZE)
		return MCP2221_ERR_USB;
	return MCP2221_ERR_OK;
}

static mcp2221_error_code_t usb_read_report(mcp2221_t *dev, uint8_t *data) {
	if (!dev || !dev->handle)
		return MCP2221_ERR_USB;

	int transferred = 0;
	int usb_timeout_ms = dev->usb_read_timeout_ms <= 0 ? 0 : dev->usb_read_timeout_ms;
	int r = libusb_interrupt_transfer(dev->handle, dev->ep_in, data, MCP2221_PACKET_SIZE, &transferred, usb_timeout_ms ? usb_timeout_ms : 0);
	if (r == LIBUSB_ERROR_TIMEOUT || transferred == 0)
		return MCP2221_ERR_TIMEOUT;
	if (r != 0)
		return MCP2221_ERR_USB;
	if (transferred != MCP2221_PACKET_SIZE)
		return MCP2221_ERR_USB;

	return MCP2221_ERR_OK;
}

// send_cmd: Port of Device.send_cmd()

mcp2221_error_code_t mcp2221_send_cmd(mcp2221_t *dev, const uint8_t *buf, size_t len, uint8_t *response) {
	if (!dev || !buf || len == 0 || len > MCP2221_PACKET_SIZE)
		return MCP2221_ERR_INVALID;

	uint8_t out[MCP2221_PACKET_SIZE];
	memcpy(out, buf, len);
	memset(out + len, 0, MCP2221_PACKET_SIZE - len);

	if (dev->trace_packets) {
		printf("CMD:");
		for (size_t i = 0; i < len; ++i)
			printf(" %02X", buf[i]);
		printf("\n");
	}

	for (int retry = 0; retry <= dev->cmd_retries; ++retry) {
		if (dev->debug_messages && retry > 0)
			printf("Command re-try %d\n", retry);

		mcp2221_error_code_t err = usb_write_report(dev, out, MCP2221_PACKET_SIZE);
		if (err != MCP2221_ERR_OK) {
			if (retry < dev->cmd_retries)
				continue;
			return err;
		}

		// Reset
		if (buf[0] == MCP2221_CMD_RESET_CHIP) {
			return MCP2221_ERR_OK;
		}

		uint8_t in[MCP2221_PACKET_SIZE];
		err = usb_read_report(dev, in);
		if (err != MCP2221_ERR_OK) {
			if (retry < dev->cmd_retries)
				continue;
			return err;
		}

		if (dev->trace_packets) {
			printf("RES:");
			for (size_t i = 0; i < MCP2221_PACKET_SIZE; ++i)
				printf(" %02X", in[i]);
			printf("\n");
		}

		if (!response) {
			// Caller will ignore payload
			return MCP2221_ERR_OK;
		}

		/* Match the Python implementation:
		 * some commands can be retried, while others should fail immediately.
		 */
		uint8_t cmd = buf[0];

		int non_idempotent =
			(cmd != MCP2221_CMD_READ_FLASH_DATA && cmd != MCP2221_CMD_POLL_STATUS_SET_PARAMETERS && cmd != MCP2221_CMD_SET_GPIO_OUTPUT_VALUES &&
			 cmd != MCP2221_CMD_SET_SRAM_SETTINGS && cmd != MCP2221_CMD_GET_SRAM_SETTINGS && cmd != MCP2221_CMD_READ_FLASH_DATA &&
			 cmd != MCP2221_CMD_WRITE_FLASH_DATA && cmd != MCP2221_CMD_RESET_CHIP);

		if (non_idempotent) {
			memcpy(response, in, MCP2221_PACKET_SIZE);
			return MCP2221_ERR_OK;
		}

		if (in[MCP2221_RESPONSE_STATUS_BYTE] == MCP2221_RESPONSE_RESULT_OK) {
			memcpy(response, in, MCP2221_PACKET_SIZE);
			return MCP2221_ERR_OK;
		} else {
			if (retry < dev->cmd_retries) {
				continue;
			} else {
				memcpy(response, in, MCP2221_PACKET_SIZE);
				return MCP2221_ERR_I2C;	 // I2C Error
			}
		}
	}

	return MCP2221_ERR_I2C;
}

// _i2c_status

mcp2221_error_code_t mcp2221_i2c_status(mcp2221_t *dev, mcp2221_i2c_status_t *st) {
	if (!dev || !st)
		return MCP2221_ERR_INVALID;
	uint8_t rbuf[MCP2221_PACKET_SIZE];
	uint8_t cmd = MCP2221_CMD_POLL_STATUS_SET_PARAMETERS;

	mcp2221_error_code_t err = mcp2221_send_cmd(dev, &cmd, 1, rbuf);
	if (err != MCP2221_ERR_OK)
		return err;

	memset(st, 0, sizeof(*st));

	st->rlen = (rbuf[MCP2221_I2C_POLL_RESP_REQ_LEN_H] << 8) + rbuf[MCP2221_I2C_POLL_RESP_REQ_LEN_L];
	st->txlen = (rbuf[MCP2221_I2C_POLL_RESP_TX_LEN_H] << 8) + rbuf[MCP2221_I2C_POLL_RESP_TX_LEN_L];

	st->div = rbuf[MCP2221_I2C_POLL_RESP_CLKDIV];
	st->ack = rbuf[MCP2221_I2C_POLL_RESP_ACK] & (1 << 6);
	st->st = rbuf[MCP2221_I2C_POLL_RESP_STATUS];
	st->scl = rbuf[MCP2221_I2C_POLL_RESP_SCL];
	st->sda = rbuf[MCP2221_I2C_POLL_RESP_SDA];

	// heuristics "confused" and "initialized" (?)
	// Match EasyMCP2221 v1.8.4 heuristic:
	// confused when byte 18 == 8 and we're not in END_NOSTOP.
	st->confused =
		(rbuf[MCP2221_I2C_POLL_RESP_UNDOCUMENTED_18] == 8 && rbuf[MCP2221_I2C_POLL_RESP_STATUS] != MCP2221_I2C_ST_WRITEDATA_END_NOSTOP);
	st->initialized = (rbuf[MCP2221_I2C_POLL_RESP_UNDOCUMENTED_21] != 0);

	return MCP2221_ERR_OK;
}

// _i2c_release

mcp2221_error_code_t mcp2221_i2c_release(mcp2221_t *dev) {
	if (!dev)
		return MCP2221_ERR_INVALID;

	mcp2221_i2c_status_t st;
	mcp2221_error_code_t err = mcp2221_i2c_status(dev, &st);
	if (err != MCP2221_ERR_OK)
		return err;

	if (st.initialized) {
		uint8_t buf[3] = {0};
		buf[0] = MCP2221_CMD_POLL_STATUS_SET_PARAMETERS;
		buf[1] = 0;
		buf[2] = MCP2221_I2C_CMD_CANCEL_CURRENT_TRANSFER;

		for (int i = 0; i < 3; ++i) {
			uint8_t rbuf[MCP2221_PACKET_SIZE];
			(void)mcp2221_send_cmd(dev, buf, 3, rbuf);

			mcp2221_i2c_status_t st2;
			err = mcp2221_i2c_status(dev, &st2);
			if (err != MCP2221_ERR_OK)
				return err;

			if (st2.st == 0 && st2.sda == 1 && st2.scl == 1) {
				dev->i2c_dirty = 0;
				return MCP2221_ERR_OK;
			}

			struct timespec ts = {0, 10 * 1000 * 1000};	 // 10 ms
			nanosleep(&ts, NULL);
		}
	}

	// ultimate try
	err = mcp2221_i2c_status(dev, &st);
	if (err != MCP2221_ERR_OK)
		return err;

	if (st.st == 0 && st.sda == 1 && st.scl == 1) {
		dev->i2c_dirty = 0;
		return MCP2221_ERR_OK;
	}

	if (st.scl == 0) {
		dev->i2c_dirty = 1;
		return MCP2221_ERR_LOW_SCL;
	}

	if (st.sda == 0) {
		dev->i2c_dirty = 1;
		return MCP2221_ERR_LOW_SDA;
	}

	dev->i2c_dirty = 1;
	return MCP2221_ERR_I2C; /* Unable to cancel. I2C crashed. */
}

// I2C_speed

mcp2221_error_code_t mcp2221_i2c_set_speed(mcp2221_t *dev, uint32_t i2c_speed_hz) {
	if (!dev)
		return MCP2221_ERR_INVALID;

	// bus_speed = round(12_000_000 / speed) - 2
	if (i2c_speed_hz == 0)
		return MCP2221_ERR_INVALID;
	long rounded = round_ties_to_even_pos(12000000.0 / (double)i2c_speed_hz);
	int bus_speed = (int)(rounded - 2);

	if (bus_speed < 0 || bus_speed > 255)
		return MCP2221_ERR_INVALID;

	uint8_t buf[5] = {0};
	uint8_t rbuf[MCP2221_PACKET_SIZE];

	buf[0] = MCP2221_CMD_POLL_STATUS_SET_PARAMETERS;
	buf[1] = 0;
	buf[2] = 0;
	buf[3] = MCP2221_I2C_CMD_SET_BUS_SPEED;
	buf[4] = (uint8_t)bus_speed;

	mcp2221_error_code_t err = mcp2221_send_cmd(dev, buf, 5, rbuf);
	if (err != MCP2221_ERR_OK) {
		dev->i2c_dirty = 1;
		return err;
	}

	if (rbuf[MCP2221_I2C_POLL_RESP_NEWSPEED_STATUS] != 0x20) {
		if (dev->i2c_dirty) {
			mcp2221_i2c_release(dev);
			err = mcp2221_send_cmd(dev, buf, 5, rbuf);
			if (err != MCP2221_ERR_OK) {
				dev->i2c_dirty = 1;
				return err;
			}
		}
	}

	if (rbuf[MCP2221_I2C_POLL_RESP_NEWSPEED_STATUS] != 0x20) {
		dev->i2c_dirty = 1;
		return MCP2221_ERR_I2C;
	}

	return MCP2221_ERR_OK;
}

// I2C_write

mcp2221_error_code_t mcp2221_i2c_write_ex(mcp2221_t *dev, uint8_t addr, const uint8_t *data, size_t len, mcp2221_i2c_kind_t kind, int i2c_timeout_ms) {
	if (!dev || !data || len == 0)
		return MCP2221_ERR_INVALID;
	if (addr > MCP2221_I2C_ADDR_7BIT_MAX)
		return MCP2221_ERR_INVALID;
	if (len > 0xFFFF)
		return MCP2221_ERR_INVALID;

	uint8_t cmd;
	switch (kind) {
		case MCP2221_I2C_KIND_NORMAL:
			cmd = MCP2221_CMD_I2C_WRITE_DATA;
			break;
		case MCP2221_I2C_KIND_REPEATED_START:
			cmd = MCP2221_CMD_I2C_WRITE_DATA_REPEATED_START;
			break;
		case MCP2221_I2C_KIND_NO_STOP:
			cmd = MCP2221_CMD_I2C_WRITE_DATA_NO_STOP;
			break;
		default:
			return MCP2221_ERR_INVALID;
	}

	// Clear previous state
	mcp2221_i2c_status_t st;
	if (dev->i2c_dirty || (mcp2221_i2c_status(dev, &st) == MCP2221_ERR_OK && st.confused)) {
		mcp2221_error_code_t r = mcp2221_i2c_release(dev);
		if (r != MCP2221_ERR_OK && r != MCP2221_ERR_LOW_SCL && r != MCP2221_ERR_LOW_SDA)
			return r;
	}

	uint8_t header[4];
	header[0] = cmd;
	header[1] = (uint8_t)(len & 0xFF);
	header[2] = (uint8_t)((len >> 8) & 0xFF);
	header[3] = (uint8_t)((addr << 1) & 0xFF);

	size_t offset = 0;
	int chunk_timeout_ms = i2c_timeout_ms > 0 ? i2c_timeout_ms : 20;

	while (offset < len) {
		size_t chunk = len - offset;
		if (chunk > MCP2221_I2C_CHUNK_SIZE)
			chunk = MCP2221_I2C_CHUNK_SIZE;

		double watchdog = now_seconds() + (chunk_timeout_ms / 1000.0);

		while (1) {
			if (now_seconds() > watchdog) {
				mcp2221_i2c_release(dev);
				return MCP2221_ERR_TIMEOUT;
			}

			uint8_t out[MCP2221_PACKET_SIZE];
			uint8_t rbuf[MCP2221_PACKET_SIZE];

			memcpy(out, header, 4);
			memcpy(out + 4, data + offset, chunk);

			mcp2221_error_code_t err = mcp2221_send_cmd(dev, out, 4 + chunk, rbuf);
			if (err != MCP2221_ERR_OK) {
				dev->i2c_dirty = 1;
				return err;
			}

			if (rbuf[MCP2221_RESPONSE_STATUS_BYTE] == MCP2221_RESPONSE_RESULT_OK) {
				break; /* next Chunk */
			} else {
				uint8_t ist = rbuf[MCP2221_I2C_INTERNAL_STATUS_BYTE];

				if (ist == MCP2221_I2C_ST_WRADDRL || ist == MCP2221_I2C_ST_WRADDRL_WAITSEND || ist == MCP2221_I2C_ST_WRADDRL_ACK ||
					ist == MCP2221_I2C_ST_WRADDRL_NACK_STOP_PEND || ist == MCP2221_I2C_ST_WRITEDATA ||
					ist == MCP2221_I2C_ST_WRITEDATA_WAITSEND || ist == MCP2221_I2C_ST_WRITEDATA_ACK) {
					i2c_poll_delay();
					continue; /* still busy */
				} else if (ist == MCP2221_I2C_ST_WRITEDATA_TOUT || ist == MCP2221_I2C_ST_STOP_TOUT) {
					mcp2221_i2c_release(dev);
					return MCP2221_ERR_I2C;
				} else if (ist == MCP2221_I2C_ST_WRADDRL_NACK_STOP) {
					mcp2221_i2c_release(dev);
					return MCP2221_ERR_NOT_ACK;
				} else if (ist == MCP2221_I2C_ST_WRITEDATA_END_NOSTOP) {
					mcp2221_i2c_release(dev);
					return MCP2221_ERR_I2C; /* "restart" required */
				} else {
					mcp2221_i2c_release(dev);
					return MCP2221_ERR_I2C;
				}
			}
		}

		offset += chunk;
	}

	double watchdog = now_seconds() + (chunk_timeout_ms / 1000.0);

	while (1) {
		if (now_seconds() > watchdog) {
			mcp2221_i2c_release(dev);
			return MCP2221_ERR_TIMEOUT;
		}

		mcp2221_i2c_status_t s;
		mcp2221_error_code_t err = mcp2221_i2c_status(dev, &s);
		if (err != MCP2221_ERR_OK) {
			dev->i2c_dirty = 1;
			return err;
		}

		if (s.st == MCP2221_I2C_ST_IDLE || s.st == MCP2221_I2C_ST_WRITEDATA_END_NOSTOP)
			return MCP2221_ERR_OK;

		if (s.st == MCP2221_I2C_ST_WRADDRL || s.st == MCP2221_I2C_ST_WRADDRL_WAITSEND || s.st == MCP2221_I2C_ST_WRADDRL_ACK ||
			s.st == MCP2221_I2C_ST_WRADDRL_NACK_STOP_PEND || s.st == MCP2221_I2C_ST_WRITEDATA || s.st == MCP2221_I2C_ST_WRITEDATA_WAITSEND ||
			s.st == MCP2221_I2C_ST_WRITEDATA_ACK || s.st == MCP2221_I2C_ST_STOP || s.st == MCP2221_I2C_ST_STOP_WAIT) {
			i2c_poll_delay();
			continue;
		} else if (s.st == MCP2221_I2C_ST_WRITEDATA_TOUT || s.st == MCP2221_I2C_ST_STOP_TOUT) {
			mcp2221_i2c_release(dev);
			return MCP2221_ERR_I2C;
		} else if (s.st == MCP2221_I2C_ST_WRADDRL_NACK_STOP) {
			mcp2221_i2c_release(dev);
			return MCP2221_ERR_NOT_ACK;
		} else if (s.st == MCP2221_I2C_ST_WRITEDATA_END_NOSTOP) {
			mcp2221_i2c_release(dev);
			return MCP2221_ERR_I2C;
		} else {
			mcp2221_i2c_release(dev);
			return MCP2221_ERR_I2C;
		}
	}
}

mcp2221_error_code_t mcp2221_i2c_write_simple(mcp2221_t *dev, uint8_t addr, const uint8_t *data, size_t len, mcp2221_i2c_kind_t kind) {
	int i2c_timeout_ms = (dev && dev->usb_read_timeout_ms > 0) ? dev->usb_read_timeout_ms : 20;
	return mcp2221_i2c_write_ex(dev, addr, data, len, kind, i2c_timeout_ms);
}

// I2C_read

mcp2221_error_code_t mcp2221_i2c_read_ex(mcp2221_t *dev, uint8_t addr, uint8_t *data, size_t len, mcp2221_i2c_kind_t kind, int i2c_timeout_ms) {
	if (!dev || !data || len == 0)
		return MCP2221_ERR_INVALID;
	if (addr > MCP2221_I2C_ADDR_7BIT_MAX)
		return MCP2221_ERR_INVALID;
	if (len > 0xFFFF)
		return MCP2221_ERR_INVALID;

	uint8_t cmd;
	switch (kind) {
		case MCP2221_I2C_KIND_NORMAL:
			cmd = MCP2221_CMD_I2C_READ_DATA;
			break;
		case MCP2221_I2C_KIND_REPEATED_START:
			cmd = MCP2221_CMD_I2C_READ_DATA_REPEATED_START;
			break;
		default:
			return MCP2221_ERR_INVALID;
	}

	mcp2221_i2c_status_t st;
	if (dev->i2c_dirty || (mcp2221_i2c_status(dev, &st) == MCP2221_ERR_OK && st.confused)) {
		mcp2221_error_code_t r = mcp2221_i2c_release(dev);
		if (r != MCP2221_ERR_OK && r != MCP2221_ERR_LOW_SCL && r != MCP2221_ERR_LOW_SDA)
			return r;
	}

	uint8_t buf[4];
	uint8_t rbuf[MCP2221_PACKET_SIZE];

	buf[0] = cmd;
	buf[1] = (uint8_t)(len & 0xFF);
	buf[2] = (uint8_t)((len >> 8) & 0xFF);
	buf[3] = (uint8_t)((addr << 1) & 0xFF) + 1;

	mcp2221_error_code_t err = mcp2221_send_cmd(dev, buf, 4, rbuf);
	if (err != MCP2221_ERR_OK) {
		dev->i2c_dirty = 1;
		return err;
	}

	if (rbuf[MCP2221_RESPONSE_STATUS_BYTE] != MCP2221_RESPONSE_RESULT_OK) {
		mcp2221_i2c_release(dev);

		uint8_t ist = rbuf[MCP2221_I2C_INTERNAL_STATUS_BYTE];
		if (ist == MCP2221_I2C_ST_WRADDRL_NACK_STOP)
			return MCP2221_ERR_NOT_ACK;
		else if (ist == MCP2221_I2C_ST_WRITEDATA_END_NOSTOP)
			return MCP2221_ERR_I2C;
		else
			return MCP2221_ERR_I2C;
	}

	double watchdog = now_seconds() + ((i2c_timeout_ms > 0 ? i2c_timeout_ms : 20) / 1000.0);
	size_t offset = 0;

	while (1) {
		if (now_seconds() > watchdog) {
			mcp2221_i2c_release(dev);
			return MCP2221_ERR_TIMEOUT;
		}

		uint8_t rbuf2[MCP2221_PACKET_SIZE];
		uint8_t cmd2 = MCP2221_CMD_I2C_READ_DATA_GET_I2C_DATA;
		err = mcp2221_send_cmd(dev, &cmd2, 1, rbuf2);
		if (err != MCP2221_ERR_OK) {
			dev->i2c_dirty = 1;
			return err;
		}

		uint8_t ist = rbuf2[MCP2221_I2C_INTERNAL_STATUS_BYTE];

		if (dev->debug_messages) {
			printf("Internal status: %02X\n", ist);
		}

		if (ist == MCP2221_I2C_ST_WRADDRL || ist == MCP2221_I2C_ST_WRADDRL_WAITSEND || ist == MCP2221_I2C_ST_WRADDRL_ACK ||
			ist == MCP2221_I2C_ST_WRADDRL_NACK_STOP_PEND || ist == MCP2221_I2C_ST_READDATA || ist == MCP2221_I2C_ST_READDATA_ACK ||
			ist == MCP2221_I2C_ST_STOP_WAIT) {
			i2c_poll_delay();
			continue;
		} else if (ist == MCP2221_I2C_ST_READDATA_WAIT || ist == MCP2221_I2C_ST_READDATA_WAITGET) {
			uint8_t chunk_size = rbuf2[3];
			size_t to_copy = chunk_size;
			if (offset + to_copy > len)
				to_copy = len - offset;
			memcpy(data + offset, &rbuf2[4], to_copy);
			offset += to_copy;

			if (ist == MCP2221_I2C_ST_READDATA_WAIT) {
				watchdog = now_seconds() + ((i2c_timeout_ms > 0 ? i2c_timeout_ms : 20) / 1000.0);
				i2c_poll_delay();
				continue;
			} else {
				return (offset == len) ? MCP2221_ERR_OK : MCP2221_ERR_I2C_SHORT_READ;
			}
		} else if (ist == MCP2221_I2C_ST_WRADDRL_NACK_STOP || ist == MCP2221_I2C_ST_WRADDRL_TOUT) {
			mcp2221_i2c_release(dev);
			return MCP2221_ERR_NOT_ACK;
		} else {
			mcp2221_i2c_release(dev);
			return MCP2221_ERR_I2C;
		}
	}
}

mcp2221_error_code_t mcp2221_i2c_read_simple(mcp2221_t *dev, uint8_t addr, uint8_t *data, size_t len, mcp2221_i2c_kind_t kind) {
	int i2c_timeout_ms = (dev && dev->usb_read_timeout_ms > 0) ? dev->usb_read_timeout_ms : 20;
	return mcp2221_i2c_read_ex(dev, addr, data, len, kind, i2c_timeout_ms);
}
