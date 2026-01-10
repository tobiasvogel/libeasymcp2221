#include "mcp2221.h"
#include "mcp2221_internal.h"

#include <libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "constants.h"
#include "i2c_slave.h"
#include "mcp2221_flash.h"

struct MCP2221 {
	libusb_device_handle *handle;
	uint8_t ep_in;
	uint8_t ep_out;
	int iface;
	uint8_t bus;
	uint8_t addr;
	int refcount;

	int read_timeout_ms;
	int cmd_retries;
	int debug_messages;
	int trace_packets;

	int i2c_dirty;

	// Cache of GPIO settings bytes as used by EasyMCP2221 (SRAM-style GP0..GP3 bytes).
	// Python keeps an internal status because GPIO_write does not alter SRAM and should not be overwritten
	// by subsequent SRAM_config calls.
	uint8_t gpio_status[4];
	int gpio_status_valid;
};

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

// Simple catalog (no thread safety)
typedef struct {
	uint8_t bus;
	uint8_t addr;
	char serial[64];
	MCP2221 *dev;
} catalog_entry_t;

#define CATALOG_MAX 16
static catalog_entry_t g_catalog[CATALOG_MAX];

static MCP2221 *catalog_find(uint8_t bus, uint8_t addr, const char *serial) {
	for (int i = 0; i < CATALOG_MAX; i++) {
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

static void catalog_add(MCP2221 *dev, const char *serial) {
	for (int i = 0; i < CATALOG_MAX; i++) {
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

static void catalog_remove(MCP2221 *dev) {
	for (int i = 0; i < CATALOG_MAX; i++) {
		if (g_catalog[i].dev == dev) {
			memset(&g_catalog[i], 0, sizeof(g_catalog[i]));
			return;
		}
	}
}

// Minimal UTF16LE -> UTF8 (BMP only), best effort (used for flash-based serial scan)
static void utf16le_to_utf8(const uint8_t *in, size_t in_len, char *out, size_t out_len) {
	size_t o = 0;
	for (size_t i = 0; i + 1 < in_len && o + 1 < out_len; i += 2) {
		uint16_t code = (uint16_t)(in[i] | (in[i + 1] << 8));
		if (code == 0)
			break;
		if (code < 0x80) {
			out[o++] = (char)code;
		} else if (code < 0x800 && o + 2 < out_len) {
			out[o++] = (char)(0xC0 | (code >> 6));
			out[o++] = (char)(0x80 | (code & 0x3F));
		} else if (o + 3 < out_len) {
			out[o++] = (char)(0xE0 | (code >> 12));
			out[o++] = (char)(0x80 | ((code >> 6) & 0x3F));
			out[o++] = (char)(0x80 | (code & 0x3F));
		} else {
			break;
		}
	}
	out[o] = '\0';
}

static void parse_wchar_structure(const uint8_t *buf, char *out, size_t out_len) {
	size_t declared = (buf[2] >= 2) ? (size_t)(buf[2] - 2) : 0;
	if (declared > 56)
		declared = 56;
	utf16le_to_utf8(&buf[4], declared, out, out_len);
}

// Helper-function (for Timeouts)
static double now_seconds(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + ts.tv_nsec / 1e9;
}

// --- Internal GPIO status helpers (Python compatibility) ---

mcp_err_t mcp2221_internal_ensure_gpio_status(MCP2221 *dev) {
	if (!dev)
		return MCP_ERR_INVALID;
	if (dev->gpio_status_valid)
		return MCP_ERR_OK;

	uint8_t cmd = CMD_GET_SRAM_SETTINGS;
	uint8_t resp[PACKET_SIZE];
	mcp_err_t err = mcp2221_send_cmd(dev, &cmd, 1, resp);
	if (err != MCP_ERR_OK)
		return err;

	// EasyMCP2221 v1.8.4: settings[22..25] are GP0..GP3 config bytes.
	dev->gpio_status[0] = resp[22];
	dev->gpio_status[1] = resp[23];
	dev->gpio_status[2] = resp[24];
	dev->gpio_status[3] = resp[25];
	dev->gpio_status_valid = 1;

	return MCP_ERR_OK;
}

mcp_err_t mcp2221_internal_gpio_status_get(MCP2221 *dev, uint8_t out_gp[4]) {
	if (!dev || !out_gp)
		return MCP_ERR_INVALID;
	if (!dev->gpio_status_valid)
		return MCP_ERR_INVALID;
	memcpy(out_gp, dev->gpio_status, 4);
	return MCP_ERR_OK;
}

void mcp2221_internal_gpio_status_set(MCP2221 *dev, const uint8_t gp[4]) {
	if (!dev || !gp)
		return;
	memcpy(dev->gpio_status, gp, 4);
	dev->gpio_status_valid = 1;
}

void mcp2221_internal_gpio_status_update_out(MCP2221 *dev, int pin, int out_value) {
	if (!dev || pin < 0 || pin > 3)
		return;
	if (!dev->gpio_status_valid)
		return;
	if (out_value)
		dev->gpio_status[pin] |= GPIO_OUT_VAL_1;
	else
		dev->gpio_status[pin] &= (uint8_t)~GPIO_OUT_VAL_1;
}

// Open usb device (optionally scan flash serial if usbserial not enumerated)
static libusb_device_handle *open_by_vid_pid(uint16_t vid, uint16_t pid, int devnum, const char *usbserial,
											 int scan_serial, int *iface, uint8_t *ep_in, uint8_t *ep_out, uint8_t *bus,
											 uint8_t *addr, char *found_serial, size_t found_serial_len) {
	libusb_device **list = NULL;
	ssize_t cnt = libusb_get_device_list(NULL, &list);
	if (cnt < 0)
		return NULL;

	libusb_device_handle *found = NULL;
	int index = 0;

	for (ssize_t i = 0; i < cnt; ++i) {
		struct libusb_device_descriptor desc;
		if (libusb_get_device_descriptor(list[i], &desc) != 0)
			continue;
		if (desc.idVendor != vid || desc.idProduct != pid)
			continue;

		struct libusb_config_descriptor *cfg;
		if (libusb_get_active_config_descriptor(list[i], &cfg) != 0)
			continue;
		int ifnum = 0;
		uint8_t in = MCP2221_DEFAULT_EP_IN, out = MCP_DEFAULT_EP_OUT; /* default */

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
			if (libusb_open(list[i], &h) != 0) {
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
				if (libusb_claim_interface(h, ifnum) == 0) {
					MCP2221 tmp = {0};
					tmp.handle = h;
					tmp.ep_in = in ? in : MCP2221_DEFAULT_EP_IN;
					tmp.ep_out = out ? out : MCP2221_DEFAULT_EP_OUT;
					tmp.iface = ifnum;
					tmp.read_timeout_ms = 500;
					tmp.cmd_retries = 0;

					uint8_t raw[60];
					if (mcp2221_flash_read(&tmp, FLASH_DATA_USB_SERIALNUM, raw) == MCP_ERR_OK) {
						char parsed[128] = {0};
						parse_wchar_structure(raw, parsed, sizeof(parsed));
						if (parsed[0] && strcmp(parsed, usbserial) == 0) {
							found = h;
							if (found_serial && found_serial_len > 0)
								strncpy(found_serial, parsed, found_serial_len - 1);
						}
					}
					libusb_release_interface(h, ifnum);
				}
				if (!found) {
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
			if (libusb_open(list[i], &found) != 0) {
				libusb_free_config_descriptor(cfg);
				found = NULL;
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
	if (found && bus && addr) {
		libusb_device *d = libusb_get_device(found);
		*bus = libusb_get_bus_number(d);
		*addr = libusb_get_device_address(d);
	}
	return found;
}

MCP2221 *mcp2221_open(uint16_t vid, uint16_t pid, int devnum, const char *usbserial, int read_timeout_ms,
					  int cmd_retries, int debug_messages, int trace_packets) {
	return mcp2221_open_scan(vid, pid, devnum, usbserial, read_timeout_ms, cmd_retries, debug_messages, trace_packets,
							 0);
}

MCP2221 *mcp2221_open_scan(uint16_t vid, uint16_t pid, int devnum, const char *usbserial, int read_timeout_ms,
						   int cmd_retries, int debug_messages, int trace_packets, int scan_serial) {
	if (libusb_init(NULL) != 0)
		return NULL;

	char found_serial[128] = {0};

	int iface = 0;
	uint8_t ep_in = 0, ep_out = 0;
	uint8_t bus = 0, addr = 0;
	libusb_device_handle *h =
		open_by_vid_pid(vid, pid, devnum, usbserial, scan_serial, &iface, &ep_in, &ep_out, &bus, &addr, found_serial,
						sizeof(found_serial));
	if (!h) {
		libusb_exit(NULL);
		return NULL;
	}

	const char *match_serial = (usbserial && usbserial[0]) ? usbserial : found_serial;
	MCP2221 *existing = catalog_find(bus, addr, match_serial);
	if (existing) {
		libusb_close(h);
		existing->refcount++;
		libusb_exit(NULL);
		return existing;
	}

	if (libusb_kernel_driver_active(h, iface) == 1) {
		libusb_detach_kernel_driver(h, iface);
	}

	if (libusb_claim_interface(h, iface) != 0) {
		libusb_close(h);
		libusb_exit(NULL);
		return NULL;
	}

	MCP2221 *dev = calloc(1, sizeof(MCP2221));
	if (!dev) {
		libusb_release_interface(h, iface);
		libusb_close(h);
		libusb_exit(NULL);
		return NULL;
	}

	dev->handle = h;
	dev->ep_in = ep_in ? ep_in : MCP2221_DEFAULT_EP_IN;
	dev->ep_out = ep_out ? ep_out : MCP2221_DEFAULT_EP_OUT;
	dev->iface = iface;
	dev->read_timeout_ms = (read_timeout_ms < 0) ? 0 : read_timeout_ms;
	dev->cmd_retries = (cmd_retries < 0) ? 0 : cmd_retries;
	dev->debug_messages = debug_messages;
	dev->trace_packets = trace_packets;
	dev->i2c_dirty = 0;
	dev->bus = bus;
	dev->addr = addr;
	dev->refcount = 1;
	dev->gpio_status_valid = 0;

	catalog_add(dev, match_serial);
	return dev;
}

MCP2221 *mcp2221_open_simple(uint16_t vid, uint16_t pid, int devnum, const char *usbserial, int speed_hz) {
	return mcp2221_open_simple_scan(vid, pid, devnum, usbserial, speed_hz, 0);
}

MCP2221 *mcp2221_open_simple_scan(uint16_t vid, uint16_t pid, int devnum, const char *usbserial, int speed_hz,
								  int scan_serial) {
	// Default-values as in Python module
	int read_timeout_ms = 500;
	int cmd_retries = 3;
	int debug = 0;
	int trace = 0;

	MCP2221 *dev =
		mcp2221_open_scan(vid, pid, devnum, usbserial, read_timeout_ms, cmd_retries, debug, trace, scan_serial);

	if (!dev)
		return NULL;

	// Best effort: release any stale I2C state (mirrors Python __init__ post-open behavior)
	(void)mcp2221_i2c_release(dev);

	// I2C-Speed setting (default to 100kHz if caller passes 0 or negative)
	int target_speed = (speed_hz > 0) ? speed_hz : 100000;
	mcp2221_i2c_speed(dev, target_speed);

	// Preload GPIO status cache (so later SRAM/save_config uses current values)
	(void)mcp2221_internal_ensure_gpio_status(dev);

	return dev;
}

void mcp2221_close(MCP2221 *dev) {
	if (!dev)
		return;
	if (dev->refcount > 1) {
		dev->refcount--;
		return;
	}
	if (dev->handle) {
		libusb_release_interface(dev->handle, dev->iface);
		libusb_close(dev->handle);
	}
	catalog_remove(dev);
	free(dev);
	/* Note: we intentionally do not call libusb_exit(NULL) here because libusb_init(NULL)
	 * is process-global; callers managing multiple handles may still rely on it. */
}

mcp_err_t mcp2221_create_i2c_slave(MCP2221 *dev, I2C_Slave *slave, uint8_t addr, int force, uint32_t speed_hz,
								   int reg_bytes, const char *reg_byteorder) {
	return i2c_slave_init(slave, dev, addr, force, speed_hz, reg_bytes > 0 ? reg_bytes : 1,
						  reg_byteorder ? reg_byteorder : "big");
}

static mcp_err_t usb_write_report(MCP2221 *dev, const uint8_t *data, size_t len) {
	if (!dev || !dev->handle)
		return MCP_ERR_USB;
	if (len > PACKET_SIZE)
		return MCP_ERR_INVALID;

	uint8_t buf[PACKET_SIZE];
	memset(buf, 0, sizeof(buf));
	memcpy(buf, data, len);

	int transferred = 0;
	int r = libusb_interrupt_transfer(dev->handle, dev->ep_out, buf, PACKET_SIZE, &transferred, 500);
	if (r != 0)
		return MCP_ERR_USB;
	return MCP_ERR_OK;
}

static mcp_err_t usb_read_report(MCP2221 *dev, uint8_t *data) {
	if (!dev || !dev->handle)
		return MCP_ERR_USB;

	int transferred = 0;
	int timeout = dev->read_timeout_ms <= 0 ? 0 : dev->read_timeout_ms;
	int r = libusb_interrupt_transfer(dev->handle, dev->ep_in, data, PACKET_SIZE, &transferred, timeout ? timeout : 0);
	if (r == LIBUSB_ERROR_TIMEOUT || transferred == 0)
		return MCP_ERR_TIMEOUT;
	if (r != 0)
		return MCP_ERR_USB;

	return MCP_ERR_OK;
}

// send_cmd: Port of Device.send_cmd()

mcp_err_t mcp2221_send_cmd(MCP2221 *dev, const uint8_t *buf, size_t len, uint8_t *response) {
	if (!dev || !buf || len == 0 || len > PACKET_SIZE)
		return MCP_ERR_INVALID;

	uint8_t out[PACKET_SIZE];
	memcpy(out, buf, len);
	memset(out + len, 0, PACKET_SIZE - len);

	if (dev->trace_packets) {
		printf("CMD:");
		for (size_t i = 0; i < len; ++i)
			printf(" %02X", buf[i]);
		printf("\n");
	}

	for (int retry = 0; retry <= dev->cmd_retries; ++retry) {
		if (dev->debug_messages && retry > 0)
			printf("Command re-try %d\n", retry);

		mcp_err_t err = usb_write_report(dev, out, PACKET_SIZE);
		if (err != MCP_ERR_OK) {
			if (retry < dev->cmd_retries)
				continue;
			return err;
		}

		// Reset
		if (buf[0] == CMD_RESET_CHIP) {
			return MCP_ERR_OK;
		}

		uint8_t in[PACKET_SIZE];
		err = usb_read_report(dev, in);
		if (err != MCP_ERR_OK) {
			if (retry < dev->cmd_retries)
				continue;
			return err;
		}

		if (dev->trace_packets) {
			printf("RES:");
			for (size_t i = 0; i < PACKET_SIZE; ++i)
				printf(" %02X", in[i]);
			printf("\n");
		}

		if (!response) {
			// Caller will ignore payload
			return MCP_ERR_OK;
		}

		/* As in Python-Code:
		 * Some commands can be resent others should raise errors immediately
		 */
		uint8_t cmd = buf[0];

		int non_idempotent =
			(cmd != CMD_READ_FLASH_DATA && cmd != CMD_POLL_STATUS_SET_PARAMETERS && cmd != CMD_SET_GPIO_OUTPUT_VALUES &&
			 cmd != CMD_SET_SRAM_SETTINGS && cmd != CMD_GET_SRAM_SETTINGS && cmd != CMD_READ_FLASH_DATA &&
			 cmd != CMD_WRITE_FLASH_DATA && cmd != CMD_RESET_CHIP);

		if (non_idempotent) {
			memcpy(response, in, PACKET_SIZE);
			return MCP_ERR_OK;
		}

		if (in[RESPONSE_STATUS_BYTE] == RESPONSE_RESULT_OK) {
			memcpy(response, in, PACKET_SIZE);
			return MCP_ERR_OK;
		} else {
			if (retry < dev->cmd_retries) {
				continue;
			} else {
				memcpy(response, in, PACKET_SIZE);
				return MCP_ERR_I2C;	 // I2C Error
			}
		}
	}

	return MCP_ERR_I2C;
}

// _i2c_status

mcp_err_t mcp2221_i2c_status(MCP2221 *dev, mcp2221_i2c_status_t *st) {
	if (!dev || !st)
		return MCP_ERR_INVALID;
	uint8_t rbuf[PACKET_SIZE];
	uint8_t cmd = CMD_POLL_STATUS_SET_PARAMETERS;

	mcp_err_t err = mcp2221_send_cmd(dev, &cmd, 1, rbuf);
	if (err != MCP_ERR_OK)
		return err;

	memset(st, 0, sizeof(*st));

	st->rlen = (rbuf[I2C_POLL_RESP_REQ_LEN_H] << 8) + rbuf[I2C_POLL_RESP_REQ_LEN_L];
	st->txlen = (rbuf[I2C_POLL_RESP_TX_LEN_H] << 8) + rbuf[I2C_POLL_RESP_TX_LEN_L];

	st->div = rbuf[I2C_POLL_RESP_CLKDIV];
	st->ack = rbuf[I2C_POLL_RESP_ACK] & (1 << 6);
	st->st = rbuf[I2C_POLL_RESP_STATUS];
	st->scl = rbuf[I2C_POLL_RESP_SCL];
	st->sda = rbuf[I2C_POLL_RESP_SDA];

	// heuristics "confused" and "initialized" (?)
	// Match EasyMCP2221 v1.8.4 heuristic:
	// confused when byte 18 == 8 and we're not in END_NOSTOP.
	st->confused =
		(rbuf[I2C_POLL_RESP_UNDOCUMENTED_18] == 8 && rbuf[I2C_POLL_RESP_STATUS] != I2C_ST_WRITEDATA_END_NOSTOP);
	st->initialized = (rbuf[I2C_POLL_RESP_UNDOCUMENTED_21] != 0);

	return MCP_ERR_OK;
}

// _i2c_release

mcp_err_t mcp2221_i2c_release(MCP2221 *dev) {
	if (!dev)
		return MCP_ERR_INVALID;

	mcp2221_i2c_status_t st;
	mcp_err_t err = mcp2221_i2c_status(dev, &st);
	if (err != MCP_ERR_OK)
		return err;

	if (st.initialized) {
		uint8_t buf[3] = {0};
		buf[0] = CMD_POLL_STATUS_SET_PARAMETERS;
		buf[1] = 0;
		buf[2] = I2C_CMD_CANCEL_CURRENT_TRANSFER;

		for (int i = 0; i < 3; ++i) {
			uint8_t rbuf[PACKET_SIZE];
			(void)mcp2221_send_cmd(dev, buf, 3, rbuf);

			mcp2221_i2c_status_t st2;
			err = mcp2221_i2c_status(dev, &st2);
			if (err != MCP_ERR_OK)
				return err;

			if (st2.st == 0 && st2.sda == 1 && st2.scl == 1) {
				dev->i2c_dirty = 0;
				return MCP_ERR_OK;
			}

			struct timespec ts = {0, 10 * 1000 * 1000};	 // 10 ms
			nanosleep(&ts, NULL);
		}
	}

	// ultimate try
	err = mcp2221_i2c_status(dev, &st);
	if (err != MCP_ERR_OK)
		return err;

	if (st.st == 0 && st.sda == 1 && st.scl == 1) {
		dev->i2c_dirty = 0;
		return MCP_ERR_OK;
	}

	if (st.scl == 0) {
		dev->i2c_dirty = 1;
		return MCP_ERR_LOW_SCL;
	}

	if (st.sda == 0) {
		dev->i2c_dirty = 1;
		return MCP_ERR_LOW_SDA;
	}

	dev->i2c_dirty = 1;
	return MCP_ERR_I2C; /* Unable to cancel. I2C crashed. */
}

// I2C_speed

mcp_err_t mcp2221_i2c_speed(MCP2221 *dev, uint32_t speed_hz) {
	if (!dev)
		return MCP_ERR_INVALID;

	// bus_speed = round(12_000_000 / speed) - 2
	if (speed_hz == 0)
		return MCP_ERR_INVALID;
	long rounded = round_ties_to_even_pos(12000000.0 / (double)speed_hz);
	int bus_speed = (int)(rounded - 2);

	if (bus_speed < 0 || bus_speed > 255)
		return MCP_ERR_INVALID;

	uint8_t buf[5] = {0};
	uint8_t rbuf[PACKET_SIZE];

	buf[0] = CMD_POLL_STATUS_SET_PARAMETERS;
	buf[1] = 0;
	buf[2] = 0;
	buf[3] = I2C_CMD_SET_BUS_SPEED;
	buf[4] = (uint8_t)bus_speed;

	mcp_err_t err = mcp2221_send_cmd(dev, buf, 5, rbuf);
	if (err != MCP_ERR_OK) {
		dev->i2c_dirty = 1;
		return err;
	}

	if (rbuf[I2C_POLL_RESP_NEWSPEED_STATUS] != 0x20) {
		if (dev->i2c_dirty) {
			mcp2221_i2c_release(dev);
			err = mcp2221_send_cmd(dev, buf, 5, rbuf);
			if (err != MCP_ERR_OK) {
				dev->i2c_dirty = 1;
				return err;
			}
		}
	}

	if (rbuf[I2C_POLL_RESP_NEWSPEED_STATUS] != 0x20) {
		dev->i2c_dirty = 1;
		return MCP_ERR_I2C;
	}

	return MCP_ERR_OK;
}

// I2C_write

mcp_err_t mcp2221_i2c_write(MCP2221 *dev, uint8_t addr, const uint8_t *data, size_t len, int kind, int timeout_ms) {
	if (!dev || !data || len == 0)
		return MCP_ERR_INVALID;
	if (addr > I2C_ADDR_7BIT_MAX)
		return MCP_ERR_INVALID;
	if (len > 0xFFFF)
		return MCP_ERR_INVALID;

	uint8_t cmd;
	if (kind == 0)
		cmd = CMD_I2C_WRITE_DATA;
	else if (kind == 1)
		cmd = CMD_I2C_WRITE_DATA_REPEATED_START;
	else if (kind == 2)
		cmd = CMD_I2C_WRITE_DATA_NO_STOP;
	else
		return MCP_ERR_INVALID;

	// Clear previous state
	mcp2221_i2c_status_t st;
	if (dev->i2c_dirty || (mcp2221_i2c_status(dev, &st) == MCP_ERR_OK && st.confused)) {
		mcp_err_t r = mcp2221_i2c_release(dev);
		if (r != MCP_ERR_OK && r != MCP_ERR_LOW_SCL && r != MCP_ERR_LOW_SDA)
			return r;
	}

	uint8_t header[4];
	header[0] = cmd;
	header[1] = (uint8_t)(len & 0xFF);
	header[2] = (uint8_t)((len >> 8) & 0xFF);
	header[3] = (uint8_t)((addr << 1) & 0xFF);

	size_t offset = 0;
	int chunk_timeout = timeout_ms > 0 ? timeout_ms : 20;

	while (offset < len) {
		size_t chunk = len - offset;
		if (chunk > I2C_CHUNK_SIZE)
			chunk = I2C_CHUNK_SIZE;

		double watchdog = now_seconds() + (chunk_timeout / 1000.0);

		while (1) {
			if (now_seconds() > watchdog) {
				mcp2221_i2c_release(dev);
				return MCP_ERR_TIMEOUT;
			}

			uint8_t out[PACKET_SIZE];
			uint8_t rbuf[PACKET_SIZE];

			memcpy(out, header, 4);
			memcpy(out + 4, data + offset, chunk);

			mcp_err_t err = mcp2221_send_cmd(dev, out, 4 + chunk, rbuf);
			if (err != MCP_ERR_OK) {
				dev->i2c_dirty = 1;
				return err;
			}

			if (rbuf[RESPONSE_STATUS_BYTE] == RESPONSE_RESULT_OK) {
				break; /* next Chunk */
			} else {
				uint8_t ist = rbuf[I2C_INTERNAL_STATUS_BYTE];

				if (ist == I2C_ST_WRADDRL || ist == I2C_ST_WRADDRL_WAITSEND || ist == I2C_ST_WRADDRL_ACK ||
					ist == I2C_ST_WRADDRL_NACK_STOP_PEND || ist == I2C_ST_WRITEDATA ||
					ist == I2C_ST_WRITEDATA_WAITSEND || ist == I2C_ST_WRITEDATA_ACK) {
					continue; /* still busy */
				} else if (ist == I2C_ST_WRITEDATA_TOUT || ist == I2C_ST_STOP_TOUT) {
					mcp2221_i2c_release(dev);
					return MCP_ERR_I2C;
				} else if (ist == I2C_ST_WRADDRL_NACK_STOP) {
					mcp2221_i2c_release(dev);
					return MCP_ERR_NOT_ACK;
				} else if (ist == I2C_ST_WRITEDATA_END_NOSTOP) {
					mcp2221_i2c_release(dev);
					return MCP_ERR_I2C; /* "restart" required */
				} else {
					mcp2221_i2c_release(dev);
					return MCP_ERR_I2C;
				}
			}
		}

		offset += chunk;
	}

	double watchdog = now_seconds() + (chunk_timeout / 1000.0);

	while (1) {
		if (now_seconds() > watchdog) {
			mcp2221_i2c_release(dev);
			return MCP_ERR_TIMEOUT;
		}

		mcp2221_i2c_status_t s;
		mcp_err_t err = mcp2221_i2c_status(dev, &s);
		if (err != MCP_ERR_OK) {
			dev->i2c_dirty = 1;
			return err;
		}

		if (s.st == I2C_ST_IDLE || s.st == I2C_ST_WRITEDATA_END_NOSTOP)
			return MCP_ERR_OK;

		if (s.st == I2C_ST_WRADDRL || s.st == I2C_ST_WRADDRL_WAITSEND || s.st == I2C_ST_WRADDRL_ACK ||
			s.st == I2C_ST_WRADDRL_NACK_STOP_PEND || s.st == I2C_ST_WRITEDATA || s.st == I2C_ST_WRITEDATA_WAITSEND ||
			s.st == I2C_ST_WRITEDATA_ACK || s.st == I2C_ST_STOP || s.st == I2C_ST_STOP_WAIT) {
			continue;
		} else if (s.st == I2C_ST_WRITEDATA_TOUT || s.st == I2C_ST_STOP_TOUT) {
			mcp2221_i2c_release(dev);
			return MCP_ERR_I2C;
		} else if (s.st == I2C_ST_WRADDRL_NACK_STOP) {
			mcp2221_i2c_release(dev);
			return MCP_ERR_NOT_ACK;
		} else if (s.st == I2C_ST_WRITEDATA_END_NOSTOP) {
			mcp2221_i2c_release(dev);
			return MCP_ERR_I2C;
		} else {
			mcp2221_i2c_release(dev);
			return MCP_ERR_I2C;
		}
	}
}

mcp_err_t mcp2221_i2c_write_simple(MCP2221 *dev, uint8_t addr, const uint8_t *data, size_t len, int kind) {
	if (!dev || !data || len == 0)
		return MCP_ERR_INVALID;
	if (addr > I2C_ADDR_7BIT_MAX)
		return MCP_ERR_INVALID;
	if (len > 0xFFFF)
		return MCP_ERR_INVALID;

	uint8_t cmd;
	if (kind == 0)
		cmd = CMD_I2C_WRITE_DATA;
	else if (kind == 1)
		cmd = CMD_I2C_WRITE_DATA_REPEATED_START;
	else if (kind == 2)
		cmd = CMD_I2C_WRITE_DATA_NO_STOP;
	else
		return MCP_ERR_INVALID;

	// clear previous state
	mcp2221_i2c_status_t st;
	if (dev->i2c_dirty || (mcp2221_i2c_status(dev, &st) == MCP_ERR_OK && st.confused)) {
		mcp_err_t r = mcp2221_i2c_release(dev);
		if (r != MCP_ERR_OK && r != MCP_ERR_LOW_SCL && r != MCP_ERR_LOW_SDA)
			return r;
	}

	uint8_t header[4];
	header[0] = cmd;
	header[1] = (uint8_t)(len & 0xFF);
	header[2] = (uint8_t)((len >> 8) & 0xFF);
	header[3] = (uint8_t)((addr << 1) & 0xFF);

	size_t offset = 0;

	// Default-Timeout: dev->read_timeout_ms or 20ms
	int chunk_timeout = dev->read_timeout_ms > 0 ? dev->read_timeout_ms : 20;

	while (offset < len) {
		size_t chunk = len - offset;
		if (chunk > I2C_CHUNK_SIZE)
			chunk = I2C_CHUNK_SIZE;

		double watchdog = now_seconds() + (chunk_timeout / 1000.0);

		while (1) {
			if (now_seconds() > watchdog) {
				mcp2221_i2c_release(dev);
				return MCP_ERR_TIMEOUT;
			}

			uint8_t out[PACKET_SIZE];
			uint8_t rbuf[PACKET_SIZE];

			memcpy(out, header, 4);
			memcpy(out + 4, data + offset, chunk);

			mcp_err_t err = mcp2221_send_cmd(dev, out, 4 + chunk, rbuf);
			if (err != MCP_ERR_OK) {
				dev->i2c_dirty = 1;
				return err;
			}

			if (rbuf[RESPONSE_STATUS_BYTE] == RESPONSE_RESULT_OK) {
				break;	// next Chunk
			} else {
				uint8_t ist = rbuf[I2C_INTERNAL_STATUS_BYTE];

				if (ist == I2C_ST_WRADDRL || ist == I2C_ST_WRADDRL_WAITSEND || ist == I2C_ST_WRADDRL_ACK ||
					ist == I2C_ST_WRADDRL_NACK_STOP_PEND || ist == I2C_ST_WRITEDATA ||
					ist == I2C_ST_WRITEDATA_WAITSEND || ist == I2C_ST_WRITEDATA_ACK) {
					continue;  // busy
				} else if (ist == I2C_ST_WRITEDATA_TOUT || ist == I2C_ST_STOP_TOUT) {
					mcp2221_i2c_release(dev);
					return MCP_ERR_I2C;
				} else if (ist == I2C_ST_WRADDRL_NACK_STOP) {
					mcp2221_i2c_release(dev);
					return MCP_ERR_NOT_ACK;
				} else if (ist == I2C_ST_WRITEDATA_END_NOSTOP) {
					mcp2221_i2c_release(dev);
					return MCP_ERR_I2C;
				} else {
					mcp2221_i2c_release(dev);
					return MCP_ERR_I2C;
				}
			}
		}

		offset += chunk;
	}

	double watchdog = now_seconds() + (chunk_timeout / 1000.0);

	while (1) {
		if (now_seconds() > watchdog) {
			mcp2221_i2c_release(dev);
			return MCP_ERR_TIMEOUT;
		}

		mcp2221_i2c_status_t s;
		mcp_err_t err = mcp2221_i2c_status(dev, &s);
		if (err != MCP_ERR_OK) {
			dev->i2c_dirty = 1;
			return err;
		}

		if (s.st == I2C_ST_IDLE || s.st == I2C_ST_WRITEDATA_END_NOSTOP)
			return MCP_ERR_OK;

		if (s.st == I2C_ST_WRADDRL || s.st == I2C_ST_WRADDRL_WAITSEND || s.st == I2C_ST_WRADDRL_ACK ||
			s.st == I2C_ST_WRADDRL_NACK_STOP_PEND || s.st == I2C_ST_WRITEDATA || s.st == I2C_ST_WRITEDATA_WAITSEND ||
			s.st == I2C_ST_WRITEDATA_ACK || s.st == I2C_ST_STOP || s.st == I2C_ST_STOP_WAIT) {
			continue;
		} else if (s.st == I2C_ST_WRITEDATA_TOUT || s.st == I2C_ST_STOP_TOUT) {
			mcp2221_i2c_release(dev);
			return MCP_ERR_I2C;
		} else if (s.st == I2C_ST_WRADDRL_NACK_STOP) {
			mcp2221_i2c_release(dev);
			return MCP_ERR_NOT_ACK;
		} else if (s.st == I2C_ST_WRITEDATA_END_NOSTOP) {
			mcp2221_i2c_release(dev);
			return MCP_ERR_I2C;
		} else {
			mcp2221_i2c_release(dev);
			return MCP_ERR_I2C;
		}
	}
}

// I2C_read

mcp_err_t mcp2221_i2c_read(MCP2221 *dev, uint8_t addr, uint8_t *data, size_t len, int kind, int timeout_ms) {
	if (!dev || !data || len == 0)
		return MCP_ERR_INVALID;
	if (addr > I2C_ADDR_7BIT_MAX)
		return MCP_ERR_INVALID;
	if (len > 0xFFFF)
		return MCP_ERR_INVALID;

	uint8_t cmd;
	if (kind == 0)
		cmd = CMD_I2C_READ_DATA;
	else if (kind == 1)
		cmd = CMD_I2C_READ_DATA_REPEATED_START;
	else
		return MCP_ERR_INVALID;

	mcp2221_i2c_status_t st;
	if (dev->i2c_dirty || (mcp2221_i2c_status(dev, &st) == MCP_ERR_OK && st.confused)) {
		mcp_err_t r = mcp2221_i2c_release(dev);
		if (r != MCP_ERR_OK && r != MCP_ERR_LOW_SCL && r != MCP_ERR_LOW_SDA)
			return r;
	}

	uint8_t buf[4];
	uint8_t rbuf[PACKET_SIZE];

	buf[0] = cmd;
	buf[1] = (uint8_t)(len & 0xFF);
	buf[2] = (uint8_t)((len >> 8) & 0xFF);
	buf[3] = (uint8_t)((addr << 1) & 0xFF) + 1;

	mcp_err_t err = mcp2221_send_cmd(dev, buf, 4, rbuf);
	if (err != MCP_ERR_OK) {
		dev->i2c_dirty = 1;
		return err;
	}

	if (rbuf[RESPONSE_STATUS_BYTE] != RESPONSE_RESULT_OK) {
		mcp2221_i2c_release(dev);

		uint8_t ist = rbuf[I2C_INTERNAL_STATUS_BYTE];
		if (ist == I2C_ST_WRADDRL_NACK_STOP)
			return MCP_ERR_NOT_ACK;
		else if (ist == I2C_ST_WRITEDATA_END_NOSTOP)
			return MCP_ERR_I2C;
		else
			return MCP_ERR_I2C;
	}

	double watchdog = now_seconds() + ((timeout_ms > 0 ? timeout_ms : 20) / 1000.0);
	size_t offset = 0;

	while (1) {
		if (now_seconds() > watchdog) {
			mcp2221_i2c_release(dev);
			return MCP_ERR_TIMEOUT;
		}

		uint8_t rbuf2[PACKET_SIZE];
		uint8_t cmd2 = CMD_I2C_READ_DATA_GET_I2C_DATA;
		err = mcp2221_send_cmd(dev, &cmd2, 1, rbuf2);
		if (err != MCP_ERR_OK) {
			dev->i2c_dirty = 1;
			return err;
		}

		uint8_t ist = rbuf2[I2C_INTERNAL_STATUS_BYTE];

		if (dev->debug_messages) {
			printf("Internal status: %02X\n", ist);
		}

		if (ist == I2C_ST_WRADDRL || ist == I2C_ST_WRADDRL_WAITSEND || ist == I2C_ST_WRADDRL_ACK ||
			ist == I2C_ST_WRADDRL_NACK_STOP_PEND || ist == I2C_ST_READDATA || ist == I2C_ST_READDATA_ACK ||
			ist == I2C_ST_STOP_WAIT) {
			continue;
		} else if (ist == I2C_ST_READDATA_WAIT || ist == I2C_ST_READDATA_WAITGET) {
			uint8_t chunk_size = rbuf2[3];
			size_t to_copy = chunk_size;
			if (offset + to_copy > len)
				to_copy = len - offset;
			memcpy(data + offset, &rbuf2[4], to_copy);
			offset += to_copy;

			if (ist == I2C_ST_READDATA_WAIT) {
				watchdog = now_seconds() + ((timeout_ms > 0 ? timeout_ms : 20) / 1000.0);
				continue;
			} else {
				return MCP_ERR_OK;
			}
		} else if (ist == I2C_ST_WRADDRL_NACK_STOP || ist == I2C_ST_WRADDRL_TOUT) {
			mcp2221_i2c_release(dev);
			return MCP_ERR_NOT_ACK;
		} else {
			mcp2221_i2c_release(dev);
			return MCP_ERR_I2C;
		}
	}
}

mcp_err_t mcp2221_i2c_read_simple(MCP2221 *dev, uint8_t addr, uint8_t *data, size_t len, int kind) {
	if (!dev || !data || len == 0)
		return MCP_ERR_INVALID;
	if (addr > I2C_ADDR_7BIT_MAX)
		return MCP_ERR_INVALID;
	if (len > 0xFFFF)
		return MCP_ERR_INVALID;

	uint8_t cmd;
	if (kind == 0)
		cmd = CMD_I2C_READ_DATA;
	else if (kind == 1)
		cmd = CMD_I2C_READ_DATA_REPEATED_START;
	else
		return MCP_ERR_INVALID;

	mcp2221_i2c_status_t st;
	if (dev->i2c_dirty || (mcp2221_i2c_status(dev, &st) == MCP_ERR_OK && st.confused)) {
		mcp_err_t r = mcp2221_i2c_release(dev);
		if (r != MCP_ERR_OK && r != MCP_ERR_LOW_SCL && r != MCP_ERR_LOW_SDA)
			return r;
	}

	uint8_t buf[4];
	uint8_t rbuf[PACKET_SIZE];

	buf[0] = cmd;
	buf[1] = (uint8_t)(len & 0xFF);
	buf[2] = (uint8_t)((len >> 8) & 0xFF);
	buf[3] = (uint8_t)((addr << 1) & 0xFF) + 1;

	mcp_err_t err = mcp2221_send_cmd(dev, buf, 4, rbuf);
	if (err != MCP_ERR_OK) {
		dev->i2c_dirty = 1;
		return err;
	}

	if (rbuf[RESPONSE_STATUS_BYTE] != RESPONSE_RESULT_OK) {
		mcp2221_i2c_release(dev);

		uint8_t ist = rbuf[I2C_INTERNAL_STATUS_BYTE];
		if (ist == I2C_ST_WRADDRL_NACK_STOP)
			return MCP_ERR_NOT_ACK;
		else if (ist == I2C_ST_WRITEDATA_END_NOSTOP)
			return MCP_ERR_I2C;
		else
			return MCP_ERR_I2C;
	}

	int tout = dev->read_timeout_ms > 0 ? dev->read_timeout_ms : 20;
	double watchdog = now_seconds() + (tout / 1000.0);
	size_t offset = 0;

	while (1) {
		if (now_seconds() > watchdog) {
			mcp2221_i2c_release(dev);
			return MCP_ERR_TIMEOUT;
		}

		uint8_t rbuf2[PACKET_SIZE];
		uint8_t cmd2 = CMD_I2C_READ_DATA_GET_I2C_DATA;
		err = mcp2221_send_cmd(dev, &cmd2, 1, rbuf2);
		if (err != MCP_ERR_OK) {
			dev->i2c_dirty = 1;
			return err;
		}

		uint8_t ist = rbuf2[I2C_INTERNAL_STATUS_BYTE];

		if (dev->debug_messages) {
			printf("Internal status: %02X\n", ist);
		}

		if (ist == I2C_ST_WRADDRL || ist == I2C_ST_WRADDRL_WAITSEND || ist == I2C_ST_WRADDRL_ACK ||
			ist == I2C_ST_WRADDRL_NACK_STOP_PEND || ist == I2C_ST_READDATA || ist == I2C_ST_READDATA_ACK ||
			ist == I2C_ST_STOP_WAIT) {
			continue;
		} else if (ist == I2C_ST_READDATA_WAIT || ist == I2C_ST_READDATA_WAITGET) {
			uint8_t chunk_size = rbuf2[3];
			size_t to_copy = chunk_size;
			if (offset + to_copy > len)
				to_copy = len - offset;
			memcpy(data + offset, &rbuf2[4], to_copy);
			offset += to_copy;

			if (ist == I2C_ST_READDATA_WAIT) {
				watchdog = now_seconds() + (tout / 1000.0);
				continue;
			} else {
				return MCP_ERR_OK;
			}
		} else if (ist == I2C_ST_WRADDRL_NACK_STOP || ist == I2C_ST_WRADDRL_TOUT) {
			mcp2221_i2c_release(dev);
			return MCP_ERR_NOT_ACK;
		} else {
			mcp2221_i2c_release(dev);
			return MCP_ERR_I2C;
		}
	}
}
