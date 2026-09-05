#include "mcp2221_internal.h"
#include "mcp2221_flash_info.h"
#include "mcp2221_internal_usb.h"

#include <string.h>

#include "mcp2221_internal_constants.h"
#include "mcp2221_flash.h"

mcp2221_error_code_t mcp2221_flash_read_info(mcp2221_t *dev, mcp2221_flash_info_t *info) {
	if (!dev || !info)
		return MCP2221_ERR_INVALID;

	memset(info, 0, sizeof(*info));

	mcp2221_error_code_t err =
		mcp2221_flash_read(dev, MCP2221_FLASH_DATA_CHIP_SETTINGS, info->chip_settings);
	if (err != MCP2221_ERR_OK)
		return err;

	err = mcp2221_flash_read(dev, MCP2221_FLASH_DATA_GP_SETTINGS, info->gp_settings);
	if (err != MCP2221_ERR_OK)
		return err;

	uint8_t manufacturer_length = 0;
	uint8_t product_length = 0;
	uint8_t serial_length = 0;
	uint8_t factory_serial_length = 0;

	err = mcp2221_internal_flash_read(dev, MCP2221_FLASH_DATA_USB_MANUFACTURER, info->usb_manufacturer, &manufacturer_length);
	if (err != MCP2221_ERR_OK)
		return err;

	err = mcp2221_internal_flash_read(dev, MCP2221_FLASH_DATA_USB_PRODUCT, info->usb_product, &product_length);
	if (err != MCP2221_ERR_OK)
		return err;

	err = mcp2221_internal_flash_read(dev, MCP2221_FLASH_DATA_USB_SERIALNUM, info->usb_serial, &serial_length);
	if (err != MCP2221_ERR_OK)
		return err;

	err = mcp2221_internal_flash_read(dev, MCP2221_FLASH_DATA_CHIP_SERIALNUM, info->usb_factory_serial, &factory_serial_length);
	if (err != MCP2221_ERR_OK)
		return err;

	mcp2221_internal_parse_wchar_structure(info->usb_manufacturer, sizeof(info->usb_manufacturer), manufacturer_length,
									info->usb_manufacturer_str, sizeof(info->usb_manufacturer_str));
	mcp2221_internal_parse_wchar_structure(info->usb_product, sizeof(info->usb_product), product_length,
									info->usb_product_str, sizeof(info->usb_product_str));
	mcp2221_internal_parse_wchar_structure(info->usb_serial, sizeof(info->usb_serial), serial_length,
									info->usb_serial_str, sizeof(info->usb_serial_str));
	mcp2221_internal_parse_factory_serial(info->usb_factory_serial, sizeof(info->usb_factory_serial), factory_serial_length,
									  info->usb_factory_serial_str, sizeof(info->usb_factory_serial_str));

	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_flash_save_config(mcp2221_t *dev) {
	if (!dev)
		return MCP2221_ERR_INVALID;

	// Read flash sections
	uint8_t chip[60], gp[60];
	mcp2221_error_code_t err =
		mcp2221_flash_read(dev, MCP2221_FLASH_DATA_CHIP_SETTINGS, chip);
	if (err != MCP2221_ERR_OK)
		return err;

	err = mcp2221_flash_read(dev, MCP2221_FLASH_DATA_GP_SETTINGS, gp);
	if (err != MCP2221_ERR_OK)
		return err;

	// Read current SRAM
	uint8_t cmd = MCP2221_CMD_GET_SRAM_SETTINGS;
	uint8_t sram[64];
	err = mcp2221_internal_send_cmd_retry_safe(dev, &cmd, 1, sram);
	if (err != MCP2221_ERR_OK)
		return err;
	if (sram[MCP2221_SRAM_RESPONSE_CHIP_LENGTH_BYTE] !=
	        MCP2221_SRAM_CHIP_SETTINGS_LENGTH ||
	    sram[MCP2221_SRAM_RESPONSE_GP_LENGTH_BYTE] !=
	        MCP2221_SRAM_GP_SETTINGS_LENGTH)
		return MCP2221_ERR_PROTOCOL;

	const uint8_t *sram_settings =
		&sram[MCP2221_SRAM_RESPONSE_SETTINGS_OFFSET];

	// GPIO status: prefer cached state (includes GPIO_write changes)
	uint8_t gp_cached[4];
	if (mcp2221_internal_ensure_gpio_status(dev) == MCP2221_ERR_OK && mcp2221_internal_gpio_status_get(dev, gp_cached) == MCP2221_ERR_OK) {
		gp[MCP2221_FLASH_GP_SETTINGS_GP0] = gp_cached[0];
		gp[MCP2221_FLASH_GP_SETTINGS_GP1] = gp_cached[1];
		gp[MCP2221_FLASH_GP_SETTINGS_GP2] = gp_cached[2];
		gp[MCP2221_FLASH_GP_SETTINGS_GP3] = gp_cached[3];
	} else {
		gp[MCP2221_FLASH_GP_SETTINGS_GP0] = sram[MCP2221_SRAM_RESPONSE_GP0];
		gp[MCP2221_FLASH_GP_SETTINGS_GP1] = sram[MCP2221_SRAM_RESPONSE_GP1];
		gp[MCP2221_FLASH_GP_SETTINGS_GP2] = sram[MCP2221_SRAM_RESPONSE_GP2];
		gp[MCP2221_FLASH_GP_SETTINGS_GP3] = sram[MCP2221_SRAM_RESPONSE_GP3];
	}

	// Map SRAM -> Flash chip settings (see Python save_config)
	chip[MCP2221_FLASH_CHIP_SETTINGS_CDCSEC] = sram_settings[MCP2221_SRAM_CHIP_SETTINGS_CDCSEC];
	chip[MCP2221_FLASH_CHIP_SETTINGS_CLOCK] = sram_settings[MCP2221_SRAM_CHIP_SETTINGS_CLOCK];
	chip[MCP2221_FLASH_CHIP_SETTINGS_DAC] = sram_settings[MCP2221_SRAM_CHIP_SETTINGS_DAC];
	chip[MCP2221_FLASH_CHIP_SETTINGS_INT_ADC] = sram_settings[MCP2221_SRAM_CHIP_SETTINGS_INT_ADC];
	chip[MCP2221_FLASH_CHIP_SETTINGS_LVID] = sram_settings[MCP2221_SRAM_CHIP_SETTINGS_LVID];
	chip[MCP2221_FLASH_CHIP_SETTINGS_HVID] = sram_settings[MCP2221_SRAM_CHIP_SETTINGS_HVID];
	chip[MCP2221_FLASH_CHIP_SETTINGS_LPID] = sram_settings[MCP2221_SRAM_CHIP_SETTINGS_LPID];
	chip[MCP2221_FLASH_CHIP_SETTINGS_HPID] = sram_settings[MCP2221_SRAM_CHIP_SETTINGS_HPID];
	chip[MCP2221_FLASH_CHIP_SETTINGS_PWD1] = sram_settings[MCP2221_SRAM_CHIP_SETTINGS_PWD1];
	chip[MCP2221_FLASH_CHIP_SETTINGS_PWD2] = sram_settings[MCP2221_SRAM_CHIP_SETTINGS_PWD2];
	chip[MCP2221_FLASH_CHIP_SETTINGS_PWD3] = sram_settings[MCP2221_SRAM_CHIP_SETTINGS_PWD3];
	chip[MCP2221_FLASH_CHIP_SETTINGS_PWD4] = sram_settings[MCP2221_SRAM_CHIP_SETTINGS_PWD4];
	chip[MCP2221_FLASH_CHIP_SETTINGS_PWD5] = sram_settings[MCP2221_SRAM_CHIP_SETTINGS_PWD5];
	chip[MCP2221_FLASH_CHIP_SETTINGS_PWD6] = sram_settings[MCP2221_SRAM_CHIP_SETTINGS_PWD6];
	chip[MCP2221_FLASH_CHIP_SETTINGS_PWD7] = sram_settings[MCP2221_SRAM_CHIP_SETTINGS_PWD7];
	chip[MCP2221_FLASH_CHIP_SETTINGS_PWD8] = sram_settings[MCP2221_SRAM_CHIP_SETTINGS_PWD8];

	/*
	 * CDCSEC, USBPWRATTR and USBREQCRT contain enumeration-time settings. Keep
	 * the values copied/read above unless an explicit USB setter staged a
	 * change. Each apply helper preserves unrelated bits or fields.
	 */
	mcp2221_internal_usb_state_t *usb = mcp2221_internal_usb_get_state(dev);
	if (!usb)
		return MCP2221_ERR_INVALID;

	uint8_t usb_value;
	err = mcp2221_internal_usb_state_apply_cdc_serial(
		usb,
		chip[MCP2221_FLASH_CHIP_SETTINGS_CDCSEC],
		&usb_value);
	if (err != MCP2221_ERR_OK)
		return err;
	chip[MCP2221_FLASH_CHIP_SETTINGS_CDCSEC] = usb_value;

	err = mcp2221_internal_usb_state_apply_power_attr(
		usb,
		chip[MCP2221_FLASH_CHIP_SETTINGS_USBPWR],
		&usb_value);
	if (err != MCP2221_ERR_OK)
		return err;
	chip[MCP2221_FLASH_CHIP_SETTINGS_USBPWR] = usb_value;

	err = mcp2221_internal_usb_state_apply_requested_current(
		usb,
		chip[MCP2221_FLASH_CHIP_SETTINGS_USBMA],
		&usb_value);
	if (err != MCP2221_ERR_OK)
		return err;
	chip[MCP2221_FLASH_CHIP_SETTINGS_USBMA] = usb_value;

	// Write back
	err = mcp2221_flash_write(dev, MCP2221_FLASH_DATA_CHIP_SETTINGS, chip);
	if (err != MCP2221_ERR_OK)
		return err;
	err = mcp2221_flash_write(dev, MCP2221_FLASH_DATA_GP_SETTINGS, gp);
	if (err != MCP2221_ERR_OK)
		return err;

	// Clear staged USB settings only after the complete save succeeded.
	mcp2221_internal_usb_state_clear(usb);

	return MCP2221_ERR_OK;
}
