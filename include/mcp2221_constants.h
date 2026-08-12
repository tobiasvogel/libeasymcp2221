#ifndef MCP2221_CONSTANTS_H
#define MCP2221_CONSTANTS_H

#define MCP2221_DEV_DEFAULT_VID 	0x04D8
#define MCP2221_DEV_DEFAULT_PID 	0x00DD

#define MCP2221_PACKET_SIZE 		64
#define MCP2221_DIR_OUTPUT  		0
#define MCP2221_DIR_INPUT   		1

#define MCP2221_I2C_ADDR_7BIT_MAX       127

// Commands
#define MCP2221_CMD_POLL_STATUS_SET_PARAMETERS    0x10
#define MCP2221_CMD_SET_GPIO_OUTPUT_VALUES        0x50
#define MCP2221_CMD_GET_GPIO_VALUES               0x51
#define MCP2221_CMD_SET_SRAM_SETTINGS             0x60
#define MCP2221_CMD_GET_SRAM_SETTINGS             0x61
#define MCP2221_CMD_I2C_READ_DATA_GET_I2C_DATA    0x40
#define MCP2221_CMD_I2C_WRITE_DATA                0x90
#define MCP2221_CMD_I2C_READ_DATA                 0x91
#define MCP2221_CMD_I2C_WRITE_DATA_REPEATED_START 0x92
#define MCP2221_CMD_I2C_READ_DATA_REPEATED_START  0x93
#define MCP2221_CMD_I2C_WRITE_DATA_NO_STOP        0x94
#define MCP2221_CMD_READ_FLASH_DATA               0xB0
#define MCP2221_CMD_WRITE_FLASH_DATA              0xB1
#define MCP2221_CMD_SEND_FLASH_ACCESS_PASSWORD    0xB2
#define MCP2221_CMD_RESET_CHIP                    0x70

#define MCP2221_RESPONSE_RESULT_OK 		0
#define MCP2221_RESPONSE_ECHO_BYTE   	0
#define MCP2221_RESPONSE_STATUS_BYTE 	1

// Flash data sections accepted by mcp2221_flash_read().
#define MCP2221_FLASH_DATA_CHIP_SETTINGS          0x00
#define MCP2221_FLASH_DATA_GP_SETTINGS            0x01
#define MCP2221_FLASH_DATA_USB_MANUFACTURER       0x02
#define MCP2221_FLASH_DATA_USB_PRODUCT            0x03
#define MCP2221_FLASH_DATA_USB_SERIALNUM          0x04
#define MCP2221_FLASH_DATA_CHIP_SERIALNUM         0x05

#define MCP2221_USB_CURRENT_MAX_MA           500u

// GPIO constants
#define MCP2221_GPIO_GP0	0
#define MCP2221_GPIO_GP1	1
#define MCP2221_GPIO_GP2	2
#define MCP2221_GPIO_GP3	3

// GPIO function values used by mcp2221_sram_config().
#define MCP2221_GPIO_FUNC_GPIO      0x00u
#define MCP2221_GPIO_FUNC_DEDICATED 0x01u
#define MCP2221_GPIO_FUNC_ALT_0     0x02u
#define MCP2221_GPIO_FUNC_ALT_1     0x03u
#define MCP2221_GPIO_FUNC_ALT_2     0x04u
#define MCP2221_GPIO_FUNC_ADC       MCP2221_GPIO_FUNC_ALT_0
#define MCP2221_GPIO_FUNC_DAC       MCP2221_GPIO_FUNC_ALT_1

// ADC reference values used by mcp2221_sram_config().
#define MCP2221_ADC_VRM_OFF         (0x00u << 1)
#define MCP2221_ADC_VRM_1024        (0x01u << 1)
#define MCP2221_ADC_VRM_2048        (0x02u << 1)
#define MCP2221_ADC_VRM_4096        (0x03u << 1)
#define MCP2221_ADC_REF_VRM         1
#define MCP2221_ADC_REF_VDD         0

// DAC reference values used by mcp2221_sram_config().
#define MCP2221_DAC_VRM_OFF         (0x00u << 1)
#define MCP2221_DAC_VRM_1024        (0x01u << 1)
#define MCP2221_DAC_VRM_2048        (0x02u << 1)
#define MCP2221_DAC_VRM_4096        (0x03u << 1)
#define MCP2221_DAC_REF_VRM         1
#define MCP2221_DAC_REF_VDD         0

// Clock output values used by mcp2221_sram_config().
#define MCP2221_CLK_DUTY_0          (0x00u << 3)
#define MCP2221_CLK_DUTY_25         (0x01u << 3)
#define MCP2221_CLK_DUTY_50         (0x02u << 3)
#define MCP2221_CLK_DUTY_75         (0x03u << 3)
#define MCP2221_CLK_DIV_1           0x01u
#define MCP2221_CLK_DIV_2           0x02u
#define MCP2221_CLK_DIV_3           0x03u
#define MCP2221_CLK_DIV_4           0x04u
#define MCP2221_CLK_DIV_5           0x05u
#define MCP2221_CLK_DIV_6           0x06u
#define MCP2221_CLK_DIV_7           0x07u
#define MCP2221_CLK_FREQ_375kHz     MCP2221_CLK_DIV_7
#define MCP2221_CLK_FREQ_750kHz     MCP2221_CLK_DIV_6
#define MCP2221_CLK_FREQ_1_5MHz     MCP2221_CLK_DIV_5
#define MCP2221_CLK_FREQ_3MHz       MCP2221_CLK_DIV_4
#define MCP2221_CLK_FREQ_6MHz       MCP2221_CLK_DIV_3
#define MCP2221_CLK_FREQ_12MHz      MCP2221_CLK_DIV_2
#define MCP2221_CLK_FREQ_24MHz      MCP2221_CLK_DIV_1

#define MCP2221_MIN_VDD_VOLTS           3.0
#define MCP2221_MAX_VDD_VOLTS           5.5

#endif // MCP2221_CONSTANTS_H
