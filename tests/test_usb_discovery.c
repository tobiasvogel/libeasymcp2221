#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <libusb.h>

#include "mcp2221_constants.h"
#include "mcp2221_internal_constants.h"

static int discovery_mode;
static int mock_open_count;

enum {
	DISCOVERY_VALID_PAIR = 1,
	DISCOVERY_SPLIT_PAIR,
	DISCOVERY_CDC_BULK_BEFORE_HID,
	DISCOVERY_NO_PAIR
};

static libusb_device *const fake_device = (libusb_device *)(uintptr_t)1;
static libusb_device_handle *const fake_handle =
	(libusb_device_handle *)(uintptr_t)2;
static libusb_device *const stale_fake_device =
	(libusb_device *)(uintptr_t)3;
static libusb_device_handle *const stale_fake_handle =
	(libusb_device_handle *)(uintptr_t)4;

static struct libusb_endpoint_descriptor valid_endpoints[2];
static struct libusb_endpoint_descriptor in_only_endpoint[1];
static struct libusb_endpoint_descriptor out_only_endpoint[1];
static struct libusb_endpoint_descriptor cdc_bulk_endpoints[2];
static struct libusb_endpoint_descriptor hid_endpoints[2];
static struct libusb_interface_descriptor altsettings[2];
static struct libusb_interface interface_desc;
static struct libusb_interface composite_interfaces[2];
static struct libusb_config_descriptor config_desc;

static void prepare_descriptors(void) {
	memset(valid_endpoints, 0, sizeof(valid_endpoints));
	memset(in_only_endpoint, 0, sizeof(in_only_endpoint));
	memset(out_only_endpoint, 0, sizeof(out_only_endpoint));
	memset(cdc_bulk_endpoints, 0, sizeof(cdc_bulk_endpoints));
	memset(hid_endpoints, 0, sizeof(hid_endpoints));
	memset(altsettings, 0, sizeof(altsettings));
	memset(&interface_desc, 0, sizeof(interface_desc));
	memset(composite_interfaces, 0, sizeof(composite_interfaces));
	memset(&config_desc, 0, sizeof(config_desc));

	valid_endpoints[0].bEndpointAddress = 0x83;
	valid_endpoints[0].bmAttributes = LIBUSB_TRANSFER_TYPE_INTERRUPT;
	valid_endpoints[1].bEndpointAddress = 0x04;
	valid_endpoints[1].bmAttributes = LIBUSB_TRANSFER_TYPE_INTERRUPT;

	in_only_endpoint[0] = valid_endpoints[0];
	out_only_endpoint[0] = valid_endpoints[1];

	/* Stock MCP2221A layout: CDC data precedes the HID command interface. */
	cdc_bulk_endpoints[0].bEndpointAddress = 0x02;
	cdc_bulk_endpoints[0].bmAttributes = LIBUSB_TRANSFER_TYPE_BULK;
	cdc_bulk_endpoints[1].bEndpointAddress = 0x82;
	cdc_bulk_endpoints[1].bmAttributes = LIBUSB_TRANSFER_TYPE_BULK;
	hid_endpoints[0].bEndpointAddress = 0x83;
	hid_endpoints[0].bmAttributes = LIBUSB_TRANSFER_TYPE_INTERRUPT;
	hid_endpoints[1].bEndpointAddress = 0x03;
	hid_endpoints[1].bmAttributes = LIBUSB_TRANSFER_TYPE_INTERRUPT;

	if (discovery_mode == DISCOVERY_VALID_PAIR) {
		altsettings[0].bInterfaceNumber = 2;
		altsettings[0].bNumEndpoints = 2;
		altsettings[0].endpoint = valid_endpoints;
		interface_desc.num_altsetting = 1;
	} else if (discovery_mode == DISCOVERY_SPLIT_PAIR) {
		altsettings[0].bInterfaceNumber = 2;
		altsettings[0].bNumEndpoints = 1;
		altsettings[0].endpoint = in_only_endpoint;
		altsettings[1].bInterfaceNumber = 3;
		altsettings[1].bNumEndpoints = 1;
		altsettings[1].endpoint = out_only_endpoint;
		interface_desc.num_altsetting = 2;
	} else if (discovery_mode == DISCOVERY_CDC_BULK_BEFORE_HID) {
		altsettings[0].bInterfaceNumber = 1;
		altsettings[0].bInterfaceClass = LIBUSB_CLASS_DATA;
		altsettings[0].bNumEndpoints = 2;
		altsettings[0].endpoint = cdc_bulk_endpoints;
		altsettings[1].bInterfaceNumber = 2;
		altsettings[1].bInterfaceClass = LIBUSB_CLASS_HID;
		altsettings[1].bNumEndpoints = 2;
		altsettings[1].endpoint = hid_endpoints;
		composite_interfaces[0].num_altsetting = 1;
		composite_interfaces[0].altsetting = &altsettings[0];
		composite_interfaces[1].num_altsetting = 1;
		composite_interfaces[1].altsetting = &altsettings[1];
		config_desc.bNumInterfaces = 2;
		config_desc.interface = composite_interfaces;
		return;
	} else {
		altsettings[0].bInterfaceNumber = 2;
		altsettings[0].bNumEndpoints = 0;
		altsettings[0].endpoint = NULL;
		interface_desc.num_altsetting = 1;
	}

	interface_desc.altsetting = altsettings;
	config_desc.bNumInterfaces = 1;
	config_desc.interface = &interface_desc;
}

ssize_t libusb_get_device_list(
	libusb_context *ctx, libusb_device ***list) {
	static libusb_device *devices[2];
	(void)ctx;
	devices[0] = fake_device;
	devices[1] = NULL;
	*list = devices;
	return 1;
}

void libusb_free_device_list(libusb_device **list, int unref_devices) {
	(void)list;
	(void)unref_devices;
}

int libusb_get_device_descriptor(
	libusb_device *dev, struct libusb_device_descriptor *desc) {
	assert(dev == fake_device);
	memset(desc, 0, sizeof(*desc));
	desc->idVendor = MCP2221_DEV_DEFAULT_VID;
	desc->idProduct = MCP2221_DEV_DEFAULT_PID;
	return 0;
}

int libusb_get_active_config_descriptor(
	libusb_device *dev, struct libusb_config_descriptor **config) {
	assert(dev == fake_device);
	prepare_descriptors();
	*config = &config_desc;
	return 0;
}

void libusb_free_config_descriptor(struct libusb_config_descriptor *config) {
	assert(config == &config_desc);
}

int libusb_open(
	libusb_device *dev, libusb_device_handle **dev_handle) {
	assert(dev == fake_device);
	mock_open_count++;
	*dev_handle = fake_handle;
	return 0;
}

libusb_device *libusb_get_device(libusb_device_handle *dev_handle) {
	if (dev_handle == fake_handle)
		return fake_device;
	assert(dev_handle == stale_fake_handle);
	return stale_fake_device;
}

/*
 * Include the implementation so the static open_by_vid_pid() helper can be
 * exercised directly with the synthetic descriptors above.
 */
#include "../src/mcp2221.c"

static mcp2221_error_code_t discover(
	int *iface, uint8_t *ep_in, uint8_t *ep_out,
	libusb_device_handle **handle) {
	return open_by_vid_pid(
		MCP2221_DEV_DEFAULT_VID,
		MCP2221_DEV_DEFAULT_PID,
		0,
		NULL,
		0,
		iface,
		ep_in,
		ep_out,
		NULL,
		NULL,
		NULL,
		0,
		NULL,
		handle);
}

static void test_catalog_rejects_reused_bus_address_for_different_device(void) {
	mcp2221_t stale = {0};
	stale.handle = stale_fake_handle;
	stale.bus = 1;
	stale.addr = 5;

	memset(g_catalog, 0, sizeof(g_catalog));
	catalog_add(&stale, NULL);

	assert(catalog_find(1, 5, NULL, fake_handle) == NULL);
	assert(catalog_find(1, 5, NULL, stale_fake_handle) == &stale);

	memset(g_catalog, 0, sizeof(g_catalog));
}

static void test_catalog_preserves_long_utf8_serial(void) {
	enum {
		MCP2221_TEST_USB_SERIAL_UTF16_UNITS = 30,
	};

	mcp2221_t dev = {0};
	dev.handle = fake_handle;
	dev.bus = 1;
	dev.addr = 5;

	char serial[MCP2221_USB_SERIAL_UTF8_BUFFER_SIZE] = {0};
	size_t offset = 0;
	for (int i = 0; i < MCP2221_TEST_USB_SERIAL_UTF16_UNITS; i++) {
		/* U+0800 occupies one UTF-16 code unit and three UTF-8 bytes. */
		serial[offset++] = (char)0xE0;
		serial[offset++] = (char)0xA0;
		serial[offset++] = (char)0x80;
	}
	assert(offset + 1 <= sizeof(serial));
	serial[offset] = '\0';

	memset(g_catalog, 0, sizeof(g_catalog));
	catalog_add(&dev, serial);

	assert(strcmp(g_catalog[0].serial, serial) == 0);
	assert(catalog_find(1, 5, serial, fake_handle) == &dev);

	memset(g_catalog, 0, sizeof(g_catalog));
}

static void test_discovers_pair_from_same_altsetting(void) {
	int iface = -1;
	uint8_t ep_in = 0;
	uint8_t ep_out = 0;
	libusb_device_handle *handle = NULL;

	discovery_mode = DISCOVERY_VALID_PAIR;
	mock_open_count = 0;

	assert(discover(&iface, &ep_in, &ep_out, &handle) == MCP2221_ERR_OK);
	assert(handle == fake_handle);
	assert(iface == 2);
	assert(ep_in == 0x83);
	assert(ep_out == 0x04);
	assert(mock_open_count == 1);
}

static void test_rejects_endpoints_split_across_altsettings(void) {
	int iface = -1;
	uint8_t ep_in = 0;
	uint8_t ep_out = 0;
	libusb_device_handle *handle = NULL;

	discovery_mode = DISCOVERY_SPLIT_PAIR;
	mock_open_count = 0;

	assert(discover(&iface, &ep_in, &ep_out, &handle) ==
	       MCP2221_ERR_USB_ENUM);
	assert(handle == NULL);
	assert(mock_open_count == 0);
}

static void test_skips_cdc_bulk_pair_before_hid(void) {
	int iface = -1;
	uint8_t ep_in = 0;
	uint8_t ep_out = 0;
	libusb_device_handle *handle = NULL;

	discovery_mode = DISCOVERY_CDC_BULK_BEFORE_HID;
	mock_open_count = 0;

	assert(discover(&iface, &ep_in, &ep_out, &handle) == MCP2221_ERR_OK);
	assert(handle == fake_handle);
	assert(iface == 2);
	assert(ep_in == 0x83);
	assert(ep_out == 0x03);
	assert(mock_open_count == 1);
}

static void test_rejects_missing_endpoint_pair(void) {
	int iface = -1;
	uint8_t ep_in = 0;
	uint8_t ep_out = 0;
	libusb_device_handle *handle = NULL;

	discovery_mode = DISCOVERY_NO_PAIR;
	mock_open_count = 0;

	assert(discover(&iface, &ep_in, &ep_out, &handle) ==
	       MCP2221_ERR_USB_ENUM);
	assert(handle == NULL);
	assert(mock_open_count == 0);
}

int main(void) {
	test_catalog_rejects_reused_bus_address_for_different_device();
	test_catalog_preserves_long_utf8_serial();
	test_discovers_pair_from_same_altsetting();
	test_rejects_endpoints_split_across_altsettings();
	test_skips_cdc_bulk_pair_before_hid();
	test_rejects_missing_endpoint_pair();
	return 0;
}
