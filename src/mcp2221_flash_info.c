#include "mcp2221_internal.h"
#include "mcp2221_flash_info.h"

#include <string.h>

#include "constants.h"
#include "mcp2221_flash.h"
#include "mcp2221_sram.h"

mcp2221_error_code_t mcp2221_flash_read_info(mcp2221_t *dev, MCP2221_FlashInfo *info) {
	if (!dev || !info)
		return MCP2221_ERR_INVALID;

	memset(info, 0, sizeof(*info));

	if (mcp2221_flash_read(dev, MCP2221_FLASH_DATA_CHIP_SETTINGS, info->chip_settings) != MCP2221_ERR_OK)
		return MCP2221_ERR_FLASH_READ;
	if (mcp2221_flash_read(dev, MCP2221_FLASH_DATA_GP_SETTINGS, info->gp_settings) != MCP2221_ERR_OK)
		return MCP2221_ERR_FLASH_READ;
	if (mcp2221_flash_read(dev, MCP2221_FLASH_DATA_USB_MANUFACTURER, info->usb_manufacturer) != MCP2221_ERR_OK)
		return MCP2221_ERR_FLASH_READ;
	if (mcp2221_flash_read(dev, MCP2221_FLASH_DATA_USB_PRODUCT, info->usb_product) != MCP2221_ERR_OK)
		return MCP2221_ERR_FLASH_READ;
	if (mcp2221_flash_read(dev, MCP2221_FLASH_DATA_USB_SERIALNUM, info->usb_serial) != MCP2221_ERR_OK)
		return MCP2221_ERR_FLASH_READ;
	if (mcp2221_flash_read(dev, MCP2221_FLASH_DATA_CHIP_SERIALNUM, info->usb_factory_serial) != MCP2221_ERR_OK)
		return MCP2221_ERR_FLASH_READ;

	mcp2221_internal_parse_wchar_structure(info->usb_manufacturer, info->usb_manufacturer_str, sizeof(info->usb_manufacturer_str));
	mcp2221_internal_parse_wchar_structure(info->usb_product, info->usb_product_str, sizeof(info->usb_product_str));
	mcp2221_internal_parse_wchar_structure(info->usb_serial, info->usb_serial_str, sizeof(info->usb_serial_str));
	mcp2221_internal_parse_wchar_structure(info->usb_factory_serial, info->usb_factory_serial_str, sizeof(info->usb_factory_serial_str));

	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_flash_save_config(mcp2221_t *dev) {
	if (!dev)
		return MCP2221_ERR_INVALID;

	// Read flash sections
	uint8_t chip[60], gp[60];
	if (mcp2221_flash_read(dev, MCP2221_FLASH_DATA_CHIP_SETTINGS, chip) != MCP2221_ERR_OK)
		return MCP2221_ERR_FLASH_READ;
	if (mcp2221_flash_read(dev, MCP2221_FLASH_DATA_GP_SETTINGS, gp) != MCP2221_ERR_OK)
		return MCP2221_ERR_FLASH_READ;

	// Read current SRAM
	uint8_t cmd = MCP2221_CMD_GET_SRAM_SETTINGS;
	uint8_t sram[64];
	mcp2221_error_code_t err = mcp2221_send_cmd(dev, &cmd, 1, sram);
	if (err != MCP2221_ERR_OK)
		return err;

	// GPIO status: prefer cached state (includes GPIO_write changes)
	uint8_t gp_cached[4];
	if (mcp2221_internal_ensure_gpio_status(dev) == MCP2221_ERR_OK && mcp2221_internal_gpio_status_get(dev, gp_cached) == MCP2221_ERR_OK) {
		gp[MCP2221_FLASH_GP_SETTINGS_GP0] = gp_cached[0];
		gp[MCP2221_FLASH_GP_SETTINGS_GP1] = gp_cached[1];
		gp[MCP2221_FLASH_GP_SETTINGS_GP2] = gp_cached[2];
		gp[MCP2221_FLASH_GP_SETTINGS_GP3] = gp_cached[3];
	} else {
		gp[MCP2221_FLASH_GP_SETTINGS_GP0] = sram[22];
		gp[MCP2221_FLASH_GP_SETTINGS_GP1] = sram[23];
		gp[MCP2221_FLASH_GP_SETTINGS_GP2] = sram[24];
		gp[MCP2221_FLASH_GP_SETTINGS_GP3] = sram[25];
	}

	// Map SRAM -> Flash chip settings (see Python save_config)
	chip[MCP2221_FLASH_CHIP_SETTINGS_CDCSEC] = sram[MCP2221_SRAM_CHIP_SETTINGS_CDCSEC];
	chip[MCP2221_FLASH_CHIP_SETTINGS_CLOCK] = sram[MCP2221_SRAM_CHIP_SETTINGS_CLOCK];
	chip[MCP2221_FLASH_CHIP_SETTINGS_DAC] = sram[MCP2221_SRAM_CHIP_SETTINGS_DAC];
	chip[MCP2221_FLASH_CHIP_SETTINGS_INT_ADC] = sram[MCP2221_SRAM_CHIP_SETTINGS_INT_ADC];
	chip[MCP2221_FLASH_CHIP_SETTINGS_LVID] = sram[MCP2221_SRAM_CHIP_SETTINGS_LVID];
	chip[MCP2221_FLASH_CHIP_SETTINGS_HVID] = sram[MCP2221_SRAM_CHIP_SETTINGS_HVID];
	chip[MCP2221_FLASH_CHIP_SETTINGS_LPID] = sram[MCP2221_SRAM_CHIP_SETTINGS_LPID];
	chip[MCP2221_FLASH_CHIP_SETTINGS_HPID] = sram[MCP2221_SRAM_CHIP_SETTINGS_HPID];
	chip[MCP2221_FLASH_CHIP_SETTINGS_USBPWR] = sram[MCP2221_SRAM_CHIP_SETTINGS_USBPWR];
	chip[MCP2221_FLASH_CHIP_SETTINGS_USBMA] = sram[MCP2221_SRAM_CHIP_SETTINGS_USBMA];
	chip[MCP2221_FLASH_CHIP_SETTINGS_PWD1] = sram[MCP2221_SRAM_CHIP_SETTINGS_PWD1];
	chip[MCP2221_FLASH_CHIP_SETTINGS_PWD2] = sram[MCP2221_SRAM_CHIP_SETTINGS_PWD2];
	chip[MCP2221_FLASH_CHIP_SETTINGS_PWD3] = sram[MCP2221_SRAM_CHIP_SETTINGS_PWD3];
	chip[MCP2221_FLASH_CHIP_SETTINGS_PWD4] = sram[MCP2221_SRAM_CHIP_SETTINGS_PWD4];
	chip[MCP2221_FLASH_CHIP_SETTINGS_PWD5] = sram[MCP2221_SRAM_CHIP_SETTINGS_PWD5];
	chip[MCP2221_FLASH_CHIP_SETTINGS_PWD6] = sram[MCP2221_SRAM_CHIP_SETTINGS_PWD6];
	chip[MCP2221_FLASH_CHIP_SETTINGS_PWD7] = sram[MCP2221_SRAM_CHIP_SETTINGS_PWD7];
	chip[MCP2221_FLASH_CHIP_SETTINGS_PWD8] = sram[MCP2221_SRAM_CHIP_SETTINGS_PWD8];

	// Write back
	err = mcp2221_flash_write(dev, MCP2221_FLASH_DATA_CHIP_SETTINGS, chip);
	if (err != MCP2221_ERR_OK)
		return err;
	err = mcp2221_flash_write(dev, MCP2221_FLASH_DATA_GP_SETTINGS, gp);
	if (err != MCP2221_ERR_OK)
		return err;

	return MCP2221_ERR_OK;
}
