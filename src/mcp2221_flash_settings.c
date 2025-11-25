#include "mcp2221_flash_settings.h"

#include "constants.h"
#include "mcp2221_flash.h"
#include "exceptions.h"

int mcp2221_flash_get_settings(MCP2221 *dev, MCP2221_FlashSettings *st) {
	int err;

	err = mcp2221_flash_read(dev, FLASH_DATA_CHIP_SETTINGS, st->chip_settings);
	if (err)
		return err;

	err = mcp2221_flash_read(dev, FLASH_DATA_GP_SETTINGS, st->gp_settings);
	if (err)
		return err;

	return MCP_ERR_OK;
}
