#include "mcp2221_flash_info.h"

#include <string.h>

#include "constants.h"
#include "mcp2221_flash.h"
#include "mcp2221_sram.h"

// Internal GPIO cache helpers
extern mcp_err_t mcp2221__ensure_gpio_status(MCP2221 *dev);
extern mcp_err_t mcp2221__gpio_status_get(MCP2221 *dev, uint8_t out_gp[4]);

static void utf16le_to_utf8(const uint8_t *in, size_t in_len, char *out, size_t out_len) {
	// Simple UTF16LE -> UTF8 (BMP only). Best effort; stop at out_len-1.
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
	// Python: strlen = buf[2]-2, data starts at buf[4]
	size_t declared = (buf[2] >= 2) ? (size_t)(buf[2] - 2) : 0;
	if (declared > 56)
		declared = 56;
	utf16le_to_utf8(&buf[4], declared, out, out_len);
}

int mcp2221_flash_read_info(MCP2221 *dev, MCP2221_FlashInfo *info) {
	if (!dev || !info)
		return MCP_ERR_INVALID;

	memset(info, 0, sizeof(*info));

	if (mcp2221_flash_read(dev, FLASH_DATA_CHIP_SETTINGS, info->chip_settings) != MCP_ERR_OK)
		return MCP_ERR_FLASH_WRITE;
	if (mcp2221_flash_read(dev, FLASH_DATA_GP_SETTINGS, info->gp_settings) != MCP_ERR_OK)
		return MCP_ERR_FLASH_WRITE;
	if (mcp2221_flash_read(dev, FLASH_DATA_USB_MANUFACTURER, info->usb_manufacturer) != MCP_ERR_OK)
		return MCP_ERR_FLASH_WRITE;
	if (mcp2221_flash_read(dev, FLASH_DATA_USB_PRODUCT, info->usb_product) != MCP_ERR_OK)
		return MCP_ERR_FLASH_WRITE;
	if (mcp2221_flash_read(dev, FLASH_DATA_USB_SERIALNUM, info->usb_serial) != MCP_ERR_OK)
		return MCP_ERR_FLASH_WRITE;
	if (mcp2221_flash_read(dev, FLASH_DATA_CHIP_SERIALNUM, info->usb_factory_serial) != MCP_ERR_OK)
		return MCP_ERR_FLASH_WRITE;

	parse_wchar_structure(info->usb_manufacturer, info->usb_manufacturer_str, sizeof(info->usb_manufacturer_str));
	parse_wchar_structure(info->usb_product, info->usb_product_str, sizeof(info->usb_product_str));
	parse_wchar_structure(info->usb_serial, info->usb_serial_str, sizeof(info->usb_serial_str));
	parse_wchar_structure(info->usb_factory_serial, info->usb_factory_serial_str, sizeof(info->usb_factory_serial_str));

	return MCP_ERR_OK;
}

int mcp2221_flash_save_config(MCP2221 *dev) {
	if (!dev)
		return MCP_ERR_INVALID;

	// Read flash sections
	uint8_t chip[60], gp[60];
	if (mcp2221_flash_read(dev, FLASH_DATA_CHIP_SETTINGS, chip) != MCP_ERR_OK)
		return MCP_ERR_FLASH_WRITE;
	if (mcp2221_flash_read(dev, FLASH_DATA_GP_SETTINGS, gp) != MCP_ERR_OK)
		return MCP_ERR_FLASH_WRITE;

	// Read current SRAM
	uint8_t cmd = CMD_GET_SRAM_SETTINGS;
	uint8_t sram[64];
	mcp_err_t err = mcp2221_send_cmd(dev, &cmd, 1, sram);
	if (err != MCP_ERR_OK)
		return err;

	// GPIO status: prefer cached state (includes GPIO_write changes)
	uint8_t gp_cached[4];
	if (mcp2221__ensure_gpio_status(dev) == MCP_ERR_OK && mcp2221__gpio_status_get(dev, gp_cached) == MCP_ERR_OK) {
		gp[FLASH_GP_SETTINGS_GP0] = gp_cached[0];
		gp[FLASH_GP_SETTINGS_GP1] = gp_cached[1];
		gp[FLASH_GP_SETTINGS_GP2] = gp_cached[2];
		gp[FLASH_GP_SETTINGS_GP3] = gp_cached[3];
	} else {
		gp[FLASH_GP_SETTINGS_GP0] = sram[22];
		gp[FLASH_GP_SETTINGS_GP1] = sram[23];
		gp[FLASH_GP_SETTINGS_GP2] = sram[24];
		gp[FLASH_GP_SETTINGS_GP3] = sram[25];
	}

	// Map SRAM -> Flash chip settings (see Python save_config)
	chip[FLASH_CHIP_SETTINGS_CDCSEC] = sram[SRAM_CHIP_SETTINGS_CDCSEC];
	chip[FLASH_CHIP_SETTINGS_CLOCK] = sram[SRAM_CHIP_SETTINGS_CLOCK];
	chip[FLASH_CHIP_SETTINGS_DAC] = sram[SRAM_CHIP_SETTINGS_DAC];
	chip[FLASH_CHIP_SETTINGS_INT_ADC] = sram[SRAM_CHIP_SETTINGS_INT_ADC];
	chip[FLASH_CHIP_SETTINGS_LVID] = sram[SRAM_CHIP_SETTINGS_LVID];
	chip[FLASH_CHIP_SETTINGS_HVID] = sram[SRAM_CHIP_SETTINGS_HVID];
	chip[FLASH_CHIP_SETTINGS_LPID] = sram[SRAM_CHIP_SETTINGS_LPID];
	chip[FLASH_CHIP_SETTINGS_HPID] = sram[SRAM_CHIP_SETTINGS_HPID];
	chip[FLASH_CHIP_SETTINGS_USBPWR] = sram[SRAM_CHIP_SETTINGS_USBPWR];
	chip[FLASH_CHIP_SETTINGS_USBMA] = sram[SRAM_CHIP_SETTINGS_USBMA];
	chip[FLASH_CHIP_SETTINGS_PWD1] = sram[SRAM_CHIP_SETTINGS_PWD1];
	chip[FLASH_CHIP_SETTINGS_PWD2] = sram[SRAM_CHIP_SETTINGS_PWD2];
	chip[FLASH_CHIP_SETTINGS_PWD3] = sram[SRAM_CHIP_SETTINGS_PWD3];
	chip[FLASH_CHIP_SETTINGS_PWD4] = sram[SRAM_CHIP_SETTINGS_PWD4];
	chip[FLASH_CHIP_SETTINGS_PWD5] = sram[SRAM_CHIP_SETTINGS_PWD5];
	chip[FLASH_CHIP_SETTINGS_PWD6] = sram[SRAM_CHIP_SETTINGS_PWD6];
	chip[FLASH_CHIP_SETTINGS_PWD7] = sram[SRAM_CHIP_SETTINGS_PWD7];
	chip[FLASH_CHIP_SETTINGS_PWD8] = sram[SRAM_CHIP_SETTINGS_PWD8];

	// Write back
	err = mcp2221_flash_write(dev, FLASH_DATA_CHIP_SETTINGS, chip);
	if (err != MCP_ERR_OK)
		return err;
	err = mcp2221_flash_write(dev, FLASH_DATA_GP_SETTINGS, gp);
	if (err != MCP_ERR_OK)
		return err;

	return MCP_ERR_OK;
}
