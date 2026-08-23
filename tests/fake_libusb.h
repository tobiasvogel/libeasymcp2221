#ifndef LIBEASYMCP2221_TEST_FAKE_LIBUSB_H
#define LIBEASYMCP2221_TEST_FAKE_LIBUSB_H

#include <stddef.h>
#include <stdint.h>

#include "mcp2221_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FAKE_LIBUSB_MAX_TRANSFERS 64u
#define FAKE_LIBUSB_HID_EP_IN     0x83u
#define FAKE_LIBUSB_HID_EP_OUT    0x03u

/* Reset the scripted transport queue and remove the simulated device. */
void fake_libusb_reset(void);

/* Expose one MCP2221-like composite USB device to libusb enumeration. */
void fake_libusb_configure_device(uint16_t vid, uint16_t pid, const char *serial);

/*
 * Queue the next expected interrupt OUT transfer. The supplied command bytes
 * are zero-padded to MCP2221_PACKET_SIZE and the complete 64-byte report is
 * compared. The default result is a successful full transfer.
 */
void fake_libusb_expect_write(const uint8_t *bytes, size_t len);
void fake_libusb_expect_write_result(
	const uint8_t *bytes, size_t len, int libusb_result, int actual_length);

/* Queue the next interrupt IN transfer. */
void fake_libusb_queue_read(const uint8_t response[MCP2221_PACKET_SIZE]);
void fake_libusb_queue_read_result(
	const uint8_t *response, size_t response_len,
	int libusb_result, int actual_length);

/* True only when every scripted interrupt transfer was consumed. */
int fake_libusb_all_expectations_met(void);

#ifdef __cplusplus
}
#endif

#endif /* LIBEASYMCP2221_TEST_FAKE_LIBUSB_H */
