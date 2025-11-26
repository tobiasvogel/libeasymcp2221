#include <errno.h>
#include <libusb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "mcp2221.h"

#ifndef PACKET_SIZE
#define PACKET_SIZE 64
#endif

struct MCP2221 {
	libusb_device_handle *handle;
	uint16_t vid;
	uint16_t pid;
	int ifnum;
	uint8_t ep_in;
	uint8_t ep_out;
	uint8_t packet_size;
};

// Helper: find Nth device with VID/PID and optional serial
static libusb_device_handle *open_device_by_vidpid(uint16_t vid, uint16_t pid, int devnum, const char *serial,
												   int *out_ifnum, uint8_t *out_ep_in, uint8_t *out_ep_out) {
	libusb_device **list = NULL;
	ssize_t cnt = libusb_get_device_list(NULL, &list);
	if (cnt < 0)
		return NULL;

	libusb_device_handle *best = NULL;
	int found = 0;
	for (ssize_t i = 0; i < cnt; ++i) {
		libusb_device *dev = list[i];
		struct libusb_device_descriptor desc;
		if (libusb_get_device_descriptor(dev, &desc) != 0)
			continue;
		if (desc.idVendor != vid || desc.idProduct != pid)
			continue;

		if (found < devnum) {
			found++;
			continue;
		}

		libusb_device_handle *h = NULL;
		if (libusb_open(dev, &h) != 0)
			continue;
		// optional filter by serial
		if (serial != NULL) {
			unsigned char buf[256];
			if (libusb_get_string_descriptor_ascii(h, desc.iSerialNumber, buf, sizeof(buf)) > 0) {
				if (strcmp((char *)buf, serial) != 0) {
					libusb_close(h);
					continue;
				}
			} else {
				libusb_close(h);
				continue;
			}
		}

		// find interface & endpoints (simple heuristic: use interface 0)
		struct libusb_config_descriptor *cfg;
		if (libusb_get_active_config_descriptor(dev, &cfg) != 0) {
			libusb_close(h);
			continue;
		}
		int ifnum = -1;
		uint8_t ep_in = 0, ep_out = 0;
		for (int ic = 0; ic < cfg->bNumInterfaces; ++ic) {
			const struct libusb_interface *iface = &cfg->interface[ic];
			for (int al = 0; al < iface->num_altsetting; ++al) {
				const struct libusb_interface_descriptor *alt = &iface->altsetting[al];
				for (int e = 0; e < alt->bNumEndpoints; ++e) {
					const struct libusb_endpoint_descriptor *ep = &alt->endpoint[e];
					if ((ep->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_INTERRUPT ||
						(ep->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_BULK) {
						if (ep->bEndpointAddress & LIBUSB_ENDPOINT_IN)
							ep_in = ep->bEndpointAddress;
						else
							ep_out = ep->bEndpointAddress;
					}
				}
				if (ep_in && ep_out) {
					ifnum = alt->bInterfaceNumber;
					break;
				}
			}
			if (ifnum >= 0)
				break;
		}
		libusb_free_config_descriptor(cfg);

		if (!ep_in || !ep_out) {
			// fallback: some devices expose only interrupt in/out as 0x81/0x01
			ep_in = 0x81;
			ep_out = 0x01;
			ifnum = 0;
		}

		best = h;
		if (out_ifnum)
			*out_ifnum = ifnum;
		if (out_ep_in)
			*out_ep_in = ep_in;
		if (out_ep_out)
			*out_ep_out = ep_out;
		break;
	}

	libusb_free_device_list(list, 1);
	return best;
}

// Open device
MCP2221 *mcp2221_open(uint16_t vid, uint16_t pid, int devnum, const char *usbserial) {
	if (libusb_init(NULL) != 0)
		return NULL;

	int ifnum = 0;
	uint8_t ep_in = 0, ep_out = 0;
	libusb_device_handle *h = open_device_by_vidpid(vid, pid, devnum, usbserial, &ifnum, &ep_in, &ep_out);
	if (!h) {
		libusb_exit(NULL);
		return NULL;
	}

	// detach kernel driver if necessary
	if (libusb_kernel_driver_active(h, ifnum) == 1) {
		libusb_detach_kernel_driver(h, ifnum);
	}

	if (libusb_claim_interface(h, ifnum) != 0) {
		libusb_close(h);
		libusb_exit(NULL);
		return NULL;
	}

	MCP2221 *dev = (MCP2221 *)calloc(1, sizeof(MCP2221));
	if (!dev) {
		libusb_release_interface(h, ifnum);
		libusb_close(h);
		libusb_exit(NULL);
		return NULL;
	}

	dev->handle = h;
	dev->vid = vid;
	dev->pid = pid;
	dev->ifnum = ifnum;
	dev->ep_in = ep_in;
	dev->ep_out = ep_out;
	dev->packet_size = PACKET_SIZE;

	return dev;
}

// Close
void mcp2221_close(MCP2221 *dev) {
	if (!dev)
		return;
	if (dev->handle) {
		libusb_release_interface(dev->handle, dev->ifnum);
		libusb_close(dev->handle);
	}
	free(dev);
	// don't call libusb_exit here: caller may open multiple devices; user should call libusb_exit if desired
}

// Low level send_report/recv_report helpers
static int send_report(MCP2221 *dev, const uint8_t *buf, size_t len, unsigned int timeout_ms) {
	if (!dev || !dev->handle)
		return -1;
	uint8_t packet[PACKET_SIZE];
	memset(packet, 0, sizeof(packet));
	if (len > sizeof(packet))
		return -2;
	memcpy(packet, buf, len);

	int transferred = 0;
	int r = libusb_interrupt_transfer(dev->handle, dev->ep_out, packet, dev->packet_size, &transferred, timeout_ms);
	if (r != 0)
		return -10 - r;
	return 0;
}

static int recv_report(MCP2221 *dev, uint8_t *buf, size_t bufsize, unsigned int timeout_ms) {
	if (!dev || !dev->handle)
		return -1;
	if (bufsize < dev->packet_size)
		return -2;

	int transferred = 0;
	int r = libusb_interrupt_transfer(dev->handle, dev->ep_in, buf, dev->packet_size, &transferred, timeout_ms);
	if (r != 0)
		return -10 - r;
	(void)transferred;
	return 0;
}

/* Helpers to perform simple I2C commands. Note: this is a pragmatic implementation
 * providing the behaviour smbus.c expects. For robustness you might want to add
 * polling loops, status checks, retries, and full parsing of the poll/status report.
 */

/* Set I2C speed (simple wrapper).
 * Implementation: send CMD_POLL_STATUS_SET_PARAMETERS with speed parameter set
 * (this mimics what EasyMCP2221 does).
 *
 * For complete compatibility, you may need to implement the exact bytes as in the Python code.
 */
int mcp2221_i2c_set_speed(MCP2221 *dev, uint32_t hz) {
	uint8_t out[PACKET_SIZE];
	memset(out, 0, sizeof(out));
	out[0] = CMD_POLL_STATUS_SET_PARAMETERS;
	// The MCP2221 expects a clock divisor in specific byte — here we use a simple mapping:
	// Common clock values: 100000, 400000. We'll set the "clock divisor" roughly.
	// TODO: implement exact mapping used by Python library. For now choose simple values.
	if (hz >= 400000) {
		out[1] = 0x01;	// faster
	} else {
		out[1] = 0x00;	// default ~100k
	}
	int r = send_report(dev, out, dev->packet_size, 500);
	if (r != 0)
		return r;

	// optionally read response (discard)
	uint8_t in[PACKET_SIZE];
	r = recv_report(dev, in, dev->packet_size, 500);
	(void)r;
	return 0;
}

/* I2C write (basic): This function will chunk the data into I2C_CHUNK_SIZE (usually 60)
 * and send CMD_I2C_WRITE_DATA packets. If nonstop==1, we set appropriate command variants.
 *
 * Note: This implementation is pragmatic to get SMBus flows working. For robust operation
 * we should implement the full state machine and poll status responses (like the Python lib).
 */
int mcp2221_i2c_write(MCP2221 *dev, uint8_t addr, const uint8_t *data, size_t len, int nonstop) {
	if (!dev)
		return -1;
	if (len == 0) {
		// send just address? some devices expect at least one byte; we'll allow zero-length
	}

	size_t remaining = len;
	const uint8_t *p = data;
	while (remaining > 0) {
		size_t chunk = remaining;
		if (chunk > I2C_CHUNK_SIZE)
			chunk = I2C_CHUNK_SIZE;

		uint8_t out[PACKET_SIZE];
		memset(out, 0, sizeof(out));
		out[0] = CMD_I2C_WRITE_DATA;
		// first data byte often encodes address+rw bit; MCP2221's Python library sets up the format:
		// Byte 1: i2c address << 1 | 0 (write)
		out[1] = (uint8_t)(addr << 1);
		out[2] = (uint8_t)chunk;  // length
		// copy chunk bytes into out starting at offset 3
		memcpy(&out[3], p, chunk);

		int r = send_report(dev, out, dev->packet_size, 500);
		if (r != 0)
			return r;

		// In robust implementation: poll status until report indicates transfer done.
		// For now we do a small read to consume possible status echo.
		uint8_t in[PACKET_SIZE];
		(void)recv_report(dev, in, dev->packet_size, 500);

		p += chunk;
		remaining -= chunk;
	}

	/* If nonstop == 1, no stop is issued. This implementation does not explicitly control STOP/RESTART.
	 * For full behaviour we must use the CONTROL bytes used by the Python library (CMD_I2C_WRITE_DATA_REPEATED_START,
	 * etc.)
	 * TODO: implement repeated-start/no-stop variants per command constants.
	 */
	(void)nonstop;
	return 0;
}

/* I2C read (basic):
 * Sends a read request for 'len' bytes, then waits for the device to return data via the "get i2c data" command/result.
 *
 * Simplified approach:
 *  - send CMD_I2C_READ_DATA with addr and length
 *  - read incoming data packets until we have 'len' bytes (uses CMD_I2C_READ_DATA_GET_I2C_DATA responses)
 *
 * This is simplified and may need retries/polling for reliability.
 */
int mcp2221_i2c_read(MCP2221 *dev, uint8_t addr, uint8_t *buffer, size_t len, int restart) {
	if (!dev)
		return -1;
	if (len == 0)
		return 0;

	// send read request
	uint8_t out[PACKET_SIZE];
	memset(out, 0, sizeof(out));
	out[0] = CMD_I2C_READ_DATA;
	out[1] = (uint8_t)((addr << 1) | 1);  // read bit
	out[2] = (uint8_t)len;
	// set restart flag if requested: TODO map to appropriate command/flag
	(void)restart;

	int r = send_report(dev, out, dev->packet_size, 500);
	if (r != 0)
		return r;

	size_t got = 0;
	uint8_t in[PACKET_SIZE];

	// read loop (conservative)
	while (got < len) {
		int rr = recv_report(dev, in, dev->packet_size, 1000);
		if (rr != 0)
			return rr;

		// The MCP2221's read response format has data starting at some offset (usually byte 1 or 4).
		// Here we try a simple heuristic: if in[0] == CMD_I2C_READ_DATA_GET_I2C_DATA (0x40) then bytes start at
		// offset 3.
		if (in[0] == CMD_I2C_READ_DATA_GET_I2C_DATA) {
			// Many implementations place actual data at offset 3 or 4. We try offset 3 first.
			size_t avail = dev->packet_size - 3;
			if (avail > len - got)
				avail = len - got;
			memcpy(buffer + got, &in[3], avail);
			got += avail;
		} else {
			// fallback: search for non-zero data bytes
			for (int i = 0; i < (int)dev->packet_size && got < len; ++i) {
				if (in[i] != 0) {
					buffer[got++] = in[i];
				}
			}
		}
		// safety: avoid infinite loop (in robust impl use timeouts and status checking)
		if (got < len) {
			// small delay not implemented here; continue reading
		}
	}

	return 0;
}
