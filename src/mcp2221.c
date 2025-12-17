#include "mcp2221.h"

#include <libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "constants.h"
#include "i2c_slave.h"

struct MCP2221 {
	libusb_device_handle *handle;
	uint8_t ep_in;
	uint8_t ep_out;
	int iface;

	int read_timeout_ms;
	int cmd_retries;
	int debug_messages;
	int trace_packets;

	int i2c_dirty;
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

// Helper-function (for Timeouts)
static double now_seconds(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + ts.tv_nsec / 1e9;
}

// Open usb device
static libusb_device_handle *open_by_vid_pid(uint16_t vid, uint16_t pid, int devnum, const char *usbserial, int *iface,
											 uint8_t *ep_in, uint8_t *ep_out) {
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

		if (usbserial) {
			// check if serial was provided
			libusb_device_handle *h;
			if (libusb_open(list[i], &h) != 0)
				continue;
			unsigned char s[256];
			int r = libusb_get_string_descriptor_ascii(h, desc.iSerialNumber, s, sizeof(s));
			if (r > 0 && strcmp((char *)s, usbserial) == 0) {
				found = h;
			} else {
				libusb_close(h);
				continue;
			}
		} else {
			// else choose by index
			if (index++ != devnum)
				continue;
			if (libusb_open(list[i], &found) != 0)
				found = NULL;
		}

		if (found) {
			// looking for endpoints
			struct libusb_config_descriptor *cfg;
			if (libusb_get_active_config_descriptor(list[i], &cfg) != 0) {
				libusb_close(found);
				found = NULL;
				break;
			}
			int ifnum = 0;
			uint8_t in = 0x81, out = 0x01; /* default */

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
			libusb_free_config_descriptor(cfg);

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
	return found;
}

MCP2221 *mcp2221_open(uint16_t vid, uint16_t pid, int devnum, const char *usbserial, int read_timeout_ms,
					  int cmd_retries, int debug_messages, int trace_packets) {
	if (libusb_init(NULL) != 0)
		return NULL;

	int iface = 0;
	uint8_t ep_in = 0, ep_out = 0;
	libusb_device_handle *h = open_by_vid_pid(vid, pid, devnum, usbserial, &iface, &ep_in, &ep_out);
	if (!h) {
		libusb_exit(NULL);
		return NULL;
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
	dev->ep_in = ep_in ? ep_in : 0x81;
	dev->ep_out = ep_out ? ep_out : 0x01;
	dev->iface = iface;
	dev->read_timeout_ms = (read_timeout_ms < 0) ? 0 : read_timeout_ms;
	dev->cmd_retries = (cmd_retries < 0) ? 0 : cmd_retries;
	dev->debug_messages = debug_messages;
	dev->trace_packets = trace_packets;
	dev->i2c_dirty = 0;

	return dev;
}

MCP2221 *mcp2221_open_simple(uint16_t vid, uint16_t pid, int devnum, const char *usbserial, int speed_hz) {
	// Default-values as in Python-module
	int read_timeout_ms = 500;
	int cmd_retries = 3;
	int debug = 0;
	int trace = 0;

	MCP2221 *dev = mcp2221_open(vid, pid, devnum, usbserial, read_timeout_ms, cmd_retries, debug, trace);

	if (!dev)
		return NULL;

	// I2C-Speed setting
	mcp2221_i2c_speed(dev, speed_hz);

	return dev;
}

void mcp2221_close(MCP2221 *dev) {
	if (!dev)
		return;
	if (dev->handle) {
		libusb_release_interface(dev->handle, dev->iface);
		libusb_close(dev->handle);
	}
	free(dev);
	/* TODO: libusb_exit(NULL) ? */
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
	st->confused = (rbuf[I2C_POLL_RESP_UNDOCUMENTED_18] != 0);
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
	if (addr > 127)
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
	if (addr > 127)
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
	if (addr > 127)
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
	if (addr > 127)
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
