/**
 * @file mcp2221_constants.h
 * @brief Public constants used by the libeasymcp2221 v2 API.
 *
 * This header contains device defaults, protocol constants, flash section
 * identifiers, GPIO selectors, SRAM configuration values, clock settings, and
 * analog limits used by the public API.
 */

#ifndef MCP2221_CONSTANTS_H
#define MCP2221_CONSTANTS_H

/** @brief Default Microchip USB vendor ID for MCP2221 devices. */
#define MCP2221_DEV_DEFAULT_VID 	0x04D8

/** @brief Default USB product ID for MCP2221/MCP2221A devices. */
#define MCP2221_DEV_DEFAULT_PID 	0x00DD

/** @brief MCP2221 HID report size in bytes. */
#define MCP2221_PACKET_SIZE 		64

/** @brief Public direction value selecting GPIO output mode. */
#define MCP2221_DIR_OUTPUT  		0

/** @brief Public direction value selecting GPIO input mode. */
#define MCP2221_DIR_INPUT   		1

/** @brief Largest valid 7-bit I2C address. */
#define MCP2221_I2C_ADDR_7BIT_MAX       127

/** @name MCP2221 command identifiers
 * Raw protocol command bytes used by low-level communication helpers.
 * @{
 */
#define MCP2221_CMD_POLL_STATUS_SET_PARAMETERS    0x10 /**< Poll status / set I2C parameters. */
#define MCP2221_CMD_SET_GPIO_OUTPUT_VALUES        0x50 /**< Set GPIO output values. */
#define MCP2221_CMD_GET_GPIO_VALUES               0x51 /**< Read GPIO values. */
#define MCP2221_CMD_SET_SRAM_SETTINGS             0x60 /**< Set runtime SRAM settings. */
#define MCP2221_CMD_GET_SRAM_SETTINGS             0x61 /**< Read runtime SRAM settings. */
#define MCP2221_CMD_I2C_READ_DATA_GET_I2C_DATA    0x40 /**< Retrieve buffered I2C read data. */
#define MCP2221_CMD_I2C_WRITE_DATA                0x90 /**< I2C write command. */
#define MCP2221_CMD_I2C_READ_DATA                 0x91 /**< I2C read command. */
#define MCP2221_CMD_I2C_WRITE_DATA_REPEATED_START 0x92 /**< I2C write using repeated-start sequencing. */
#define MCP2221_CMD_I2C_READ_DATA_REPEATED_START  0x93 /**< I2C read using repeated-start sequencing. */
#define MCP2221_CMD_I2C_WRITE_DATA_NO_STOP        0x94 /**< I2C write without issuing a stop condition. */
#define MCP2221_CMD_READ_FLASH_DATA               0xB0 /**< Read persistent flash data. */
#define MCP2221_CMD_WRITE_FLASH_DATA              0xB1 /**< Write persistent flash data. */
#define MCP2221_CMD_SEND_FLASH_ACCESS_PASSWORD    0xB2 /**< Send the flash access password. */
#define MCP2221_CMD_RESET_CHIP                    0x70 /**< Reset the MCP2221. */
/** @} */

/** @name Common response fields
 * @{
 */
#define MCP2221_RESPONSE_RESULT_OK 		0 /**< Successful device response status. */
#define MCP2221_RESPONSE_ECHO_BYTE   	0 /**< Response byte containing the echoed command. */
#define MCP2221_RESPONSE_STATUS_BYTE 	1 /**< Response byte containing command status. */
/** @} */

/** @name Flash section identifiers
 * Values accepted by mcp2221_flash_read() and, where appropriate,
 * mcp2221_flash_write().
 * @{
 */
#define MCP2221_FLASH_DATA_CHIP_SETTINGS          0x00 /**< Persistent chip settings. */
#define MCP2221_FLASH_DATA_GP_SETTINGS            0x01 /**< Persistent GP pin settings. */
#define MCP2221_FLASH_DATA_USB_MANUFACTURER       0x02 /**< USB manufacturer string descriptor. */
#define MCP2221_FLASH_DATA_USB_PRODUCT            0x03 /**< USB product string descriptor. */
#define MCP2221_FLASH_DATA_USB_SERIALNUM          0x04 /**< USB serial-number string descriptor. */
#define MCP2221_FLASH_DATA_CHIP_SERIALNUM         0x05 /**< Factory/chip serial-number data. */
/** @} */

/** @brief Maximum requested USB bus current accepted by the public API, in mA. */
#define MCP2221_USB_CURRENT_MAX_MA           500u

/** @name GP pin selectors
 * @{
 */
#define MCP2221_GPIO_GP0	0 /**< GP0 pin index. */
#define MCP2221_GPIO_GP1	1 /**< GP1 pin index. */
#define MCP2221_GPIO_GP2	2 /**< GP2 pin index. */
#define MCP2221_GPIO_GP3	3 /**< GP3 pin index. */
/** @} */

/** @name GPIO function selectors
 * Values used by mcp2221_sram_config(). Not every alternate function is valid
 * on every GP pin; see mcp2221_sram_gp_config_t::function.
 * @{
 */
#define MCP2221_GPIO_FUNC_GPIO      0x00u /**< General-purpose GPIO function. */
#define MCP2221_GPIO_FUNC_DEDICATED 0x01u /**< Pin-specific dedicated function. */
#define MCP2221_GPIO_FUNC_ALT_0     0x02u /**< Pin-specific alternate function 0. */
#define MCP2221_GPIO_FUNC_ALT_1     0x03u /**< Pin-specific alternate function 1. */
#define MCP2221_GPIO_FUNC_ALT_2     0x04u /**< Pin-specific alternate function 2. */
#define MCP2221_GPIO_FUNC_ADC       MCP2221_GPIO_FUNC_ALT_0 /**< Alias for the ADC alternate function. */
#define MCP2221_GPIO_FUNC_DAC       MCP2221_GPIO_FUNC_ALT_1 /**< Alias for the DAC alternate function. */
/** @} */

/** @name ADC SRAM reference configuration
 * Values used by mcp2221_sram_adc_config_t.
 * @{
 */
#define MCP2221_ADC_VRM_OFF         (0x00u << 1) /**< Internal ADC voltage-reference module disabled. */
#define MCP2221_ADC_VRM_1024        (0x01u << 1) /**< Internal ADC reference set to 1.024 V. */
#define MCP2221_ADC_VRM_2048        (0x02u << 1) /**< Internal ADC reference set to 2.048 V. */
#define MCP2221_ADC_VRM_4096        (0x03u << 1) /**< Internal ADC reference set to 4.096 V. */
#define MCP2221_ADC_REF_VRM         1 /**< Select the internal ADC voltage-reference module. */
#define MCP2221_ADC_REF_VDD         0 /**< Select VDD as the ADC reference. */
/** @} */

/** @name DAC SRAM reference configuration
 * Values used by mcp2221_sram_dac_ref_config_t.
 * @{
 */
#define MCP2221_DAC_VRM_OFF         (0x00u << 1) /**< Internal DAC voltage-reference module disabled. */
#define MCP2221_DAC_VRM_1024        (0x01u << 1) /**< Internal DAC reference set to 1.024 V. */
#define MCP2221_DAC_VRM_2048        (0x02u << 1) /**< Internal DAC reference set to 2.048 V. */
#define MCP2221_DAC_VRM_4096        (0x03u << 1) /**< Internal DAC reference set to 4.096 V. */
#define MCP2221_DAC_REF_VRM         1 /**< Select the internal DAC voltage-reference module. */
#define MCP2221_DAC_REF_VDD         0 /**< Select VDD as the DAC reference. */
/** @} */

/** @name Clock-output configuration
 * Values used by mcp2221_sram_clock_config_t.
 * @{
 */
#define MCP2221_CLK_DUTY_0          (0x00u << 3) /**< 0 percent duty-cycle selector. */
#define MCP2221_CLK_DUTY_25         (0x01u << 3) /**< 25 percent duty-cycle selector. */
#define MCP2221_CLK_DUTY_50         (0x02u << 3) /**< 50 percent duty-cycle selector. */
#define MCP2221_CLK_DUTY_75         (0x03u << 3) /**< 75 percent duty-cycle selector. */
#define MCP2221_CLK_DIV_1           0x01u /**< Clock divider 1; 24 MHz output. */
#define MCP2221_CLK_DIV_2           0x02u /**< Clock divider 2; 12 MHz output. */
#define MCP2221_CLK_DIV_3           0x03u /**< Clock divider 3; 6 MHz output. */
#define MCP2221_CLK_DIV_4           0x04u /**< Clock divider 4; 3 MHz output. */
#define MCP2221_CLK_DIV_5           0x05u /**< Clock divider 5; 1.5 MHz output. */
#define MCP2221_CLK_DIV_6           0x06u /**< Clock divider 6; 750 kHz output. */
#define MCP2221_CLK_DIV_7           0x07u /**< Clock divider 7; 375 kHz output. */
#define MCP2221_CLK_FREQ_375kHz     MCP2221_CLK_DIV_7 /**< Alias selecting 375 kHz. */
#define MCP2221_CLK_FREQ_750kHz     MCP2221_CLK_DIV_6 /**< Alias selecting 750 kHz. */
#define MCP2221_CLK_FREQ_1_5MHz     MCP2221_CLK_DIV_5 /**< Alias selecting 1.5 MHz. */
#define MCP2221_CLK_FREQ_3MHz       MCP2221_CLK_DIV_4 /**< Alias selecting 3 MHz. */
#define MCP2221_CLK_FREQ_6MHz       MCP2221_CLK_DIV_3 /**< Alias selecting 6 MHz. */
#define MCP2221_CLK_FREQ_12MHz      MCP2221_CLK_DIV_2 /**< Alias selecting 12 MHz. */
#define MCP2221_CLK_FREQ_24MHz      MCP2221_CLK_DIV_1 /**< Alias selecting 24 MHz. */
/** @} */

/** @brief Minimum VDD value accepted by mcp2221_analog_set_vdd(), in volts. */
#define MCP2221_MIN_VDD_VOLTS           3.0

/** @brief Maximum VDD value accepted by mcp2221_analog_set_vdd(), in volts. */
#define MCP2221_MAX_VDD_VOLTS           5.5

#endif // MCP2221_CONSTANTS_H
