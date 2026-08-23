/*
 * Scripted libusb backend for black-box regression tests.
 *
 * This file deliberately understands USB transport only. MCP2221 command
 * semantics remain in the test cases so the fake cannot accidentally become
 * a second implementation of the protocol under test.
 */

#include <libusb.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fake_libusb.h"

struct libusb_context {
	int unused;
};
struct libusb_device {
	int unused;
};
struct libusb_device_handle {
	int unused;
};

static struct libusb_context g_context;
static struct libusb_device g_device;
static struct libusb_device_handle g_handle;

enum {
	FAKE_CDC_COMM_EP_IN = 0x81u,
	FAKE_CDC_DATA_EP_OUT = 0x02u,
	FAKE_CDC_DATA_EP_IN = 0x82u,
	FAKE_HID_INTERFACE = 2,
};

static const struct libusb_endpoint_descriptor g_cdc_comm_endpoints[] = {
	{
		.bEndpointAddress = FAKE_CDC_COMM_EP_IN,
		.bmAttributes = LIBUSB_TRANSFER_TYPE_INTERRUPT,
		.wMaxPacketSize = 8,
	},
};

static const struct libusb_endpoint_descriptor g_cdc_data_endpoints[] = {
	{
		.bEndpointAddress = FAKE_CDC_DATA_EP_OUT,
		.bmAttributes = LIBUSB_TRANSFER_TYPE_BULK,
		.wMaxPacketSize = 16,
	},
	{
		.bEndpointAddress = FAKE_CDC_DATA_EP_IN,
		.bmAttributes = LIBUSB_TRANSFER_TYPE_BULK,
		.wMaxPacketSize = 16,
	},
};

static const struct libusb_endpoint_descriptor g_hid_endpoints[] = {
	{
		.bEndpointAddress = FAKE_LIBUSB_HID_EP_IN,
		.bmAttributes = LIBUSB_TRANSFER_TYPE_INTERRUPT,
		.wMaxPacketSize = MCP2221_PACKET_SIZE,
	},
	{
		.bEndpointAddress = FAKE_LIBUSB_HID_EP_OUT,
		.bmAttributes = LIBUSB_TRANSFER_TYPE_INTERRUPT,
		.wMaxPacketSize = MCP2221_PACKET_SIZE,
	},
};

static const struct libusb_interface_descriptor g_altsettings[] = {
	{
		.bInterfaceNumber = 0,
		.bNumEndpoints = 1,
		.bInterfaceClass = LIBUSB_CLASS_COMM,
		.endpoint = g_cdc_comm_endpoints,
	},
	{
		.bInterfaceNumber = 1,
		.bNumEndpoints = 2,
		.bInterfaceClass = LIBUSB_CLASS_DATA,
		.endpoint = g_cdc_data_endpoints,
	},
	{
		.bInterfaceNumber = FAKE_HID_INTERFACE,
		.bNumEndpoints = 2,
		.bInterfaceClass = LIBUSB_CLASS_HID,
		.endpoint = g_hid_endpoints,
	},
};

static const struct libusb_interface g_interfaces[] = {
	{.altsetting = &g_altsettings[0], .num_altsetting = 1},
	{.altsetting = &g_altsettings[1], .num_altsetting = 1},
	{.altsetting = &g_altsettings[2], .num_altsetting = 1},
};

static const struct libusb_config_descriptor g_config_template = {
	.bNumInterfaces = 3,
	.bConfigurationValue = 1,
	.interface = g_interfaces,
};

typedef enum {
	FAKE_TRANSFER_OUT,
	FAKE_TRANSFER_IN,
} fake_transfer_direction_t;

typedef struct {
	fake_transfer_direction_t direction;
	unsigned char endpoint;
	uint8_t report[MCP2221_PACKET_SIZE];
	int libusb_result;
	int actual_length;
} fake_transfer_t;

typedef struct {
	int device_configured;
	uint16_t vid;
	uint16_t pid;
	char serial[64];

	fake_transfer_t transfers[FAKE_LIBUSB_MAX_TRANSFERS];
	size_t transfer_head;
	size_t transfer_count;
} fake_state_t;

static fake_state_t g_fake;

static void fake_fail(const char *message) {
	fprintf(stderr, "fake_libusb: %s\n", message);
	abort();
}

static void validate_transfer_setup(
	const uint8_t *bytes, size_t len, int actual_length) {
	if (!bytes && len != 0)
		fake_fail("NULL transfer data with non-zero length");
	if (len > MCP2221_PACKET_SIZE)
		fake_fail("scripted transfer data exceeds 64-byte MCP2221 report");
	if (actual_length < 0 || actual_length > MCP2221_PACKET_SIZE)
		fake_fail("scripted actual_length is outside 0..64");
}

static fake_transfer_t *queue_transfer(void) {
	if (g_fake.transfer_count >= FAKE_LIBUSB_MAX_TRANSFERS)
		fake_fail("transfer queue full");

	size_t slot =
		(g_fake.transfer_head + g_fake.transfer_count) % FAKE_LIBUSB_MAX_TRANSFERS;
	fake_transfer_t *transfer = &g_fake.transfers[slot];
	memset(transfer, 0, sizeof(*transfer));
	g_fake.transfer_count++;
	return transfer;
}

void fake_libusb_reset(void) {
	memset(&g_fake, 0, sizeof(g_fake));
}

void fake_libusb_configure_device(uint16_t vid, uint16_t pid, const char *serial) {
	g_fake.device_configured = 1;
	g_fake.vid = vid;
	g_fake.pid = pid;
	if (serial) {
		strncpy(g_fake.serial, serial, sizeof(g_fake.serial) - 1);
		g_fake.serial[sizeof(g_fake.serial) - 1] = '\0';
	}
}

void fake_libusb_expect_write(const uint8_t *bytes, size_t len) {
	fake_libusb_expect_write_result(
		bytes, len, LIBUSB_SUCCESS, MCP2221_PACKET_SIZE);
}

void fake_libusb_expect_write_result(
	const uint8_t *bytes, size_t len, int libusb_result, int actual_length) {
	validate_transfer_setup(bytes, len, actual_length);
	fake_transfer_t *transfer = queue_transfer();
	transfer->direction = FAKE_TRANSFER_OUT;
	transfer->endpoint = FAKE_LIBUSB_HID_EP_OUT;
	transfer->libusb_result = libusb_result;
	transfer->actual_length = actual_length;
	if (len != 0)
		memcpy(transfer->report, bytes, len);
}

void fake_libusb_queue_read(const uint8_t response[MCP2221_PACKET_SIZE]) {
	fake_libusb_queue_read_result(
		response, MCP2221_PACKET_SIZE,
		LIBUSB_SUCCESS, MCP2221_PACKET_SIZE);
}

void fake_libusb_queue_read_result(
	const uint8_t *response, size_t response_len,
	int libusb_result, int actual_length) {
	validate_transfer_setup(response, response_len, actual_length);
	fake_transfer_t *transfer = queue_transfer();
	transfer->direction = FAKE_TRANSFER_IN;
	transfer->endpoint = FAKE_LIBUSB_HID_EP_IN;
	transfer->libusb_result = libusb_result;
	transfer->actual_length = actual_length;
	if (response_len != 0)
		memcpy(transfer->report, response, response_len);
}

int fake_libusb_all_expectations_met(void) {
	return g_fake.transfer_count == 0;
}

int LIBUSB_CALL libusb_init(libusb_context **ctx) {
	if (ctx)
		*ctx = &g_context;
	return LIBUSB_SUCCESS;
}

void LIBUSB_CALL libusb_exit(libusb_context *ctx) {
	(void)ctx;
}

ssize_t LIBUSB_CALL libusb_get_device_list(
	libusb_context *ctx, libusb_device ***list) {
	(void)ctx;
	if (!list)
		return LIBUSB_ERROR_INVALID_PARAM;

	libusb_device **devices = calloc(2, sizeof(*devices));
	if (!devices) {
		*list = NULL;
		return LIBUSB_ERROR_NO_MEM;
	}

	if (g_fake.device_configured)
		devices[0] = &g_device;
	*list = devices;
	return g_fake.device_configured ? 1 : 0;
}

void LIBUSB_CALL libusb_free_device_list(
	libusb_device **list, int unref_devices) {
	(void)unref_devices;
	free(list);
}

int LIBUSB_CALL libusb_get_device_descriptor(
	libusb_device *dev, struct libusb_device_descriptor *desc) {
	if (dev != &g_device || !desc)
		return LIBUSB_ERROR_INVALID_PARAM;

	memset(desc, 0, sizeof(*desc));
	desc->bLength = LIBUSB_DT_DEVICE_SIZE;
	desc->bDescriptorType = LIBUSB_DT_DEVICE;
	desc->bDeviceClass = LIBUSB_CLASS_PER_INTERFACE;
	desc->idVendor = g_fake.vid;
	desc->idProduct = g_fake.pid;
	desc->iSerialNumber = g_fake.serial[0] ? 1 : 0;
	desc->bNumConfigurations = 1;
	return LIBUSB_SUCCESS;
}

int LIBUSB_CALL libusb_get_active_config_descriptor(
	libusb_device *dev, struct libusb_config_descriptor **config) {
	if (dev != &g_device || !config)
		return LIBUSB_ERROR_INVALID_PARAM;

	struct libusb_config_descriptor *copy = malloc(sizeof(*copy));
	if (!copy)
		return LIBUSB_ERROR_NO_MEM;
	*copy = g_config_template;
	*config = copy;
	return LIBUSB_SUCCESS;
}

void LIBUSB_CALL libusb_free_config_descriptor(
	struct libusb_config_descriptor *config) {
	free(config);
}

int LIBUSB_CALL libusb_open(
	libusb_device *dev, libusb_device_handle **dev_handle) {
	if (dev != &g_device || !dev_handle)
		return LIBUSB_ERROR_INVALID_PARAM;
	*dev_handle = &g_handle;
	return LIBUSB_SUCCESS;
}

void LIBUSB_CALL libusb_close(libusb_device_handle *dev_handle) {
	if (dev_handle != &g_handle)
		fake_fail("libusb_close received an unknown handle");
}

int LIBUSB_CALL libusb_get_string_descriptor_ascii(
	libusb_device_handle *dev_handle, uint8_t desc_index,
	unsigned char *data, int length) {
	if (dev_handle != &g_handle || !data || length <= 0)
		return LIBUSB_ERROR_INVALID_PARAM;
	if (desc_index == 0 || !g_fake.serial[0])
		return LIBUSB_ERROR_NOT_FOUND;

	size_t n = strlen(g_fake.serial);
	if (n >= (size_t)length)
		n = (size_t)length - 1;
	memcpy(data, g_fake.serial, n);
	data[n] = '\0';
	return (int)n;
}

int LIBUSB_CALL libusb_kernel_driver_active(
	libusb_device_handle *dev_handle, int interface_number) {
	if (dev_handle != &g_handle || interface_number != FAKE_HID_INTERFACE)
		return LIBUSB_ERROR_NOT_FOUND;
	return 0;
}

int LIBUSB_CALL libusb_detach_kernel_driver(
	libusb_device_handle *dev_handle, int interface_number) {
	if (dev_handle != &g_handle || interface_number != FAKE_HID_INTERFACE)
		return LIBUSB_ERROR_NOT_FOUND;
	return LIBUSB_SUCCESS;
}

int LIBUSB_CALL libusb_attach_kernel_driver(
	libusb_device_handle *dev_handle, int interface_number) {
	if (dev_handle != &g_handle || interface_number != FAKE_HID_INTERFACE)
		return LIBUSB_ERROR_NOT_FOUND;
	return LIBUSB_SUCCESS;
}

int LIBUSB_CALL libusb_claim_interface(
	libusb_device_handle *dev_handle, int interface_number) {
	if (dev_handle != &g_handle || interface_number != FAKE_HID_INTERFACE)
		return LIBUSB_ERROR_NOT_FOUND;
	return LIBUSB_SUCCESS;
}

int LIBUSB_CALL libusb_release_interface(
	libusb_device_handle *dev_handle, int interface_number) {
	if (dev_handle != &g_handle || interface_number != FAKE_HID_INTERFACE)
		return LIBUSB_ERROR_NOT_FOUND;
	return LIBUSB_SUCCESS;
}

libusb_device * LIBUSB_CALL libusb_get_device(
	libusb_device_handle *dev_handle) {
	return dev_handle == &g_handle ? &g_device : NULL;
}

uint8_t LIBUSB_CALL libusb_get_bus_number(libusb_device *dev) {
	return dev == &g_device ? 1u : 0u;
}

uint8_t LIBUSB_CALL libusb_get_device_address(libusb_device *dev) {
	return dev == &g_device ? 5u : 0u;
}

int LIBUSB_CALL libusb_interrupt_transfer(
	libusb_device_handle *dev_handle, unsigned char endpoint,
	unsigned char *data, int length, int *actual_length,
	unsigned int timeout) {
	(void)timeout;
	if (dev_handle != &g_handle)
		fake_fail("interrupt transfer used an unknown device handle");
	if (!data || length != MCP2221_PACKET_SIZE)
		fake_fail("interrupt transfer must use one 64-byte MCP2221 report");
	if (g_fake.transfer_count == 0)
		fake_fail("unexpected interrupt transfer; script queue is empty");

	fake_transfer_t *expected = &g_fake.transfers[g_fake.transfer_head];
	if (endpoint != expected->endpoint) {
		fprintf(stderr,
			"fake_libusb: endpoint mismatch: expected 0x%02X, got 0x%02X\n",
			expected->endpoint, endpoint);
		abort();
	}

	if (expected->direction == FAKE_TRANSFER_OUT) {
		if (memcmp(data, expected->report, MCP2221_PACKET_SIZE) != 0) {
			size_t mismatch = 0;
			while (mismatch < MCP2221_PACKET_SIZE &&
			       data[mismatch] == expected->report[mismatch])
				mismatch++;
			fprintf(stderr,
				"fake_libusb: OUT report mismatch at byte %zu: expected 0x%02X, got 0x%02X\n",
				mismatch,
				mismatch < MCP2221_PACKET_SIZE ? expected->report[mismatch] : 0,
				mismatch < MCP2221_PACKET_SIZE ? data[mismatch] : 0);
			abort();
		}
	} else {
		size_t copy_len = (size_t)expected->actual_length;
		if (copy_len > MCP2221_PACKET_SIZE)
			copy_len = MCP2221_PACKET_SIZE;
		memcpy(data, expected->report, copy_len);
	}

	if (actual_length)
		*actual_length = expected->actual_length;
	int result = expected->libusb_result;

	g_fake.transfer_head =
		(g_fake.transfer_head + 1) % FAKE_LIBUSB_MAX_TRANSFERS;
	g_fake.transfer_count--;
	return result;
}
