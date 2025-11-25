#ifndef CONSTANTS_H
#define CONSTANTS_H

#define DEV_DEFAULT_VID = 0x04D8
#define DEV_DEFAULT_PID = 0x00DD

#define PACKET_SIZE = 64
#define DIR_OUTPUT  = 0
#define DIR_INPUT   = 1

// Commands
#define CMD_POLL_STATUS_SET_PARAMETERS    = 0x10
#define CMD_SET_GPIO_OUTPUT_VALUES        = 0x50
#define CMD_GET_GPIO_VALUES               = 0x51
#define CMD_SET_SRAM_SETTINGS             = 0x60
#define CMD_GET_SRAM_SETTINGS             = 0x61
#define CMD_I2C_READ_DATA_GET_I2C_DATA    = 0x40
#define CMD_I2C_WRITE_DATA                = 0x90
#define CMD_I2C_READ_DATA                 = 0x91
#define CMD_I2C_WRITE_DATA_REPEATED_START = 0x92
#define CMD_I2C_READ_DATA_REPEATED_START  = 0x93
#define CMD_I2C_WRITE_DATA_NO_STOP        = 0x94
#define CMD_READ_FLASH_DATA               = 0xB0
#define CMD_WRITE_FLASH_DATA              = 0xB1
#define CMD_SEND_FLASH_ACCESS_PASSWORD    = 0xB2
#define CMD_RESET_CHIP                    = 0x70

#define RESPONSE_RESULT_OK = 0
#define RESPONSE_ECHO_BYTE   = 0
#define RESPONSE_STATUS_BYTE = 1

//# Flash data constants
#define FLASH_DATA_CHIP_SETTINGS          = 0x00
#define FLASH_DATA_GP_SETTINGS            = 0x01
#define FLASH_DATA_USB_MANUFACTURER       = 0x02
#define FLASH_DATA_USB_PRODUCT            = 0x03
#define FLASH_DATA_USB_SERIALNUM          = 0x04
#define FLASH_DATA_CHIP_SERIALNUM         = 0x05

// Bytes in Flash Chip Settings register (0-based)
// Write and read are same order but different offsets
#define FLASH_OFFSET_WRITE = 2
#define FLASH_OFFSET_READ  = 4

#define FLASH_CHIP_SETTINGS_CDCSEC  =  (2 - 2)
#define FLASH_CHIP_SETTINGS_CLOCK   =  (3 - 2)
#define FLASH_CHIP_SETTINGS_DAC     =  (4 - 2)
#define FLASH_CHIP_SETTINGS_INT_ADC =  (5 - 2)
#define FLASH_CHIP_SETTINGS_LVID    =  (6 - 2)
#define FLASH_CHIP_SETTINGS_HVID    =  (7 - 2)
#define FLASH_CHIP_SETTINGS_LPID    =  (8 - 2)
#define FLASH_CHIP_SETTINGS_HPID    =  (9 - 2)
#define FLASH_CHIP_SETTINGS_USBPWR  = (10 - 2)
#define FLASH_CHIP_SETTINGS_USBMA   = (11 - 2)
#define FLASH_CHIP_SETTINGS_PWD1    = (12 - 2)
#define FLASH_CHIP_SETTINGS_PWD2    = (13 - 2)
#define FLASH_CHIP_SETTINGS_PWD3    = (14 - 2)
#define FLASH_CHIP_SETTINGS_PWD4    = (15 - 2)
#define FLASH_CHIP_SETTINGS_PWD5    = (16 - 2)
#define FLASH_CHIP_SETTINGS_PWD6    = (17 - 2)
#define FLASH_CHIP_SETTINGS_PWD7    = (18 - 2)
#define FLASH_CHIP_SETTINGS_PWD8    = (19 - 2)
// Bytes in Flash GP Settings register (0-based)
// Write and read are same order but different offsets
#define FLASH_GP_SETTINGS_GP0       =  (2 - 2)
#define FLASH_GP_SETTINGS_GP1       =  (3 - 2)
#define FLASH_GP_SETTINGS_GP2       =  (4 - 2)
#define FLASH_GP_SETTINGS_GP3       =  (5 - 2)

// Bytes in Get SRAM Settings response (starting at 0)
#define SRAM_CHIP_SETTINGS_CDCSEC   =  (4 - 4)
#define SRAM_CHIP_SETTINGS_CLOCK    =  (5 - 4)
#define SRAM_CHIP_SETTINGS_DAC      =  (6 - 4)
#define SRAM_CHIP_SETTINGS_INT_ADC  =  (7 - 4)
#define SRAM_CHIP_SETTINGS_LVID     =  (8 - 4)
#define SRAM_CHIP_SETTINGS_HVID     =  (9 - 4)
#define SRAM_CHIP_SETTINGS_LPID     = (10 - 4)
#define SRAM_CHIP_SETTINGS_HPID     = (11 - 4)
#define SRAM_CHIP_SETTINGS_USBPWR   = (12 - 4)
#define SRAM_CHIP_SETTINGS_USBMA    = (13 - 4)
#define SRAM_CHIP_SETTINGS_PWD1     = (14 - 4)
#define SRAM_CHIP_SETTINGS_PWD2     = (15 - 4)
#define SRAM_CHIP_SETTINGS_PWD3     = (16 - 4)
#define SRAM_CHIP_SETTINGS_PWD4     = (17 - 4)
#define SRAM_CHIP_SETTINGS_PWD5     = (18 - 4)
#define SRAM_CHIP_SETTINGS_PWD6     = (19 - 4)
#define SRAM_CHIP_SETTINGS_PWD7     = (20 - 4)
#define SRAM_CHIP_SETTINGS_PWD8     = (21 - 4)
#define SRAM_GP_SETTINGS_GP0        = (22 - 4)
#define SRAM_GP_SETTINGS_GP1        = (23 - 4)
#define SRAM_GP_SETTINGS_GP2        = (24 - 4)
#define SRAM_GP_SETTINGS_GP3        = (25 - 4)

// CHIP SETTINGS0 bits
#define CDCSEC_CDCSNEN              = (1 << 7)  # USB CDC Serial Number Enable bit
#define CDCSEC_LEDURXINST           = (1 << 6)  # LED UART RX Inactive State bit
#define CDCSEC_LEDUTXINST           = (1 << 5)  # LED UART TX Inactive State bit
#define CDCSEC_LEDI2CINST           = (1 << 4)  # LED I2C Inactive State bit
#define CDCSEC_SSPNDINST            = (1 << 3)  # SSPND Inactive State bit
#define CDCSEC_USBCFGINST           = (1 << 2)  # USBCFG Inactive State bit
#define CDCSEC_CHIPPROT_RESERVED    = 0b11    # Chip protection
#define CDCSEC_CHIPPROT_LOCKED      = 0b10
#define CDCSEC_CHIPPROT_PROTECTED   = 0b01
#define CDCSEC_CHIPPROT_UNPROTECTED = 0b00


// GPIO constants
#define GPIO_GP0 = 0
#define GPIO_GP1 = 1
#define GPIO_GP2 = 2
#define GPIO_GP3 = 3

#define ALTER_GPIO_CONF    = 1 << 7 /* bit 7: alters the current GP designation */
#define PRESERVE_GPIO_CONF = (0 << 7)
#define GPIO_OUT_VAL_1  = (1 << 4)
#define GPIO_OUT_VAL_0  = (0 << 4)
#define GPIO_DIR_IN     = (1 << 3)
#define GPIO_DIR_OUT    = (0 << 3)
#define GPIO_FUNC_GPIO  = 0b000
#define GPIO_FUNC_DEDICATED = 0b001
#define GPIO_FUNC_ALT_0  = 0b010
#define GPIO_FUNC_ALT_1  = 0b011
#define GPIO_FUNC_ALT_2  = 0b100
#define GPIO_FUNC_ADC = GPIO_FUNC_ALT_0
#define GPIO_FUNC_DAC = GPIO_FUNC_ALT_1


#define ALTER_INT_CONF    = (1 << 7) /* Enable the modification of the interrupt detection conditions */
#define PRESERVE_INT_CONF = (0 << 7)
#define INT_POS_EDGE_ENABLE  = (0b11 << 3)
#define INT_POS_EDGE_DISABLE = (0b10 << 3)
#define INT_NEG_EDGE_ENABLE  = (0b11 << 1)
#define INT_NEG_EDGE_DISABLE = (0b10 << 1)
#define INT_FLAG_CLEAR    = 1
#define INT_FLAG_PRESERVE = 0

#define ALTER_ADC_REF    = (1 << 7) /* Enable loading of a new ADC reference */
#define PRESERVE_ADC_REF = (0 << 7)
#define ADC_VRM_OFF  = (0b00 << 1)
#define ADC_VRM_1024 = (0b01 << 1)
#define ADC_VRM_2048 = (0b10 << 1)
#define ADC_VRM_4096 = (0b11 << 1)
#define ADC_REF_VRM  = 1
#define ADC_REF_VDD  = 0

#define ALTER_DAC_REF    = (1 << 7) /* Enable loading of a new DAC reference */
#define PRESERVE_DAC_REF = (0 << 7)
#define DAC_VRM_OFF  = (0b00 << 1)
#define DAC_VRM_1024 = (0b01 << 1)
#define DAC_VRM_2048 = (0b10 << 1)
#define DAC_VRM_4096 = (0b11 << 1)
#define DAC_REF_VRM  = 1
#define DAC_REF_VDD  = 0

#define ALTER_DAC_VALUE    = (1 << 7) /* Enable loading of a new DAC value */
#define PRESERVE_DAC_VALUE = (0 << 7)

#define ALTER_CLK_OUTPUT    = (1 << 7) /* Enable loading of a new clock divider */
#define PRESERVE_CLK_OUTPUT = (0 << 7)
#define CLK_DUTY_0  = (0b00 << 3)
#define CLK_DUTY_25 = (0b01 << 3)
#define CLK_DUTY_50 = (0b10 << 3)
#define CLK_DUTY_75 = (0b11 << 3)
#define CLK_DIV_1 = 0b001
#define CLK_DIV_2 = 0b010
#define CLK_DIV_3 = 0b011
#define CLK_DIV_4 = 0b100
#define CLK_DIV_5 = 0b101
#define CLK_DIV_6 = 0b110
#define CLK_DIV_7 = 0b111
#define CLK_FREQ_375kHz = CLK_DIV_7
#define CLK_FREQ_750kHz = CLK_DIV_6
#define CLK_FREQ_1_5MHz = CLK_DIV_5
#define CLK_FREQ_3MHz   = CLK_DIV_4
#define CLK_FREQ_6MHz   = CLK_DIV_3
#define CLK_FREQ_12MHz  = CLK_DIV_2
#define CLK_FREQ_24MHz  = CLK_DIV_1

#define I2C_CMD_CANCEL_CURRENT_TRANSFER = 0x10
#define I2C_CMD_SET_BUS_SPEED = 0x20

#define RESET_CHIP_SURE           = 0xAB
#define RESET_CHIP_VERY_SURE      = 0xCD
#define RESET_CHIP_VERY_VERY_SURE = 0xEF


#define I2C_CHUNK_SIZE = 60

// For CMD_I2C_READ_DATA_GET_I2C_DATA, I2C READ, etc
// but not for CMD_POLL_STATUS_SET_PARAMETERS.
#define I2C_INTERNAL_STATUS_BYTE      = 2

// Internal status machine code
// from Microchip's SMBbus driver example
// meaning got by trial and error
#define I2C_ST_IDLE                   = 0x00
#define I2C_ST_START                  = 0x10  /* sending start condition */
#define I2C_ST_START_ACK              = 0x11
#define I2C_ST_START_TOUT             = 0x12
#define I2C_ST_REPSTART               = 0x15
#define I2C_ST_REPSTART_ACK           = 0x16
#define I2C_ST_REPSTART_TOUT          = 0x17

#define I2C_ST_WRADDRL                = 0x20
#define I2C_ST_WRADDRL_WAITSEND       = 0x21
#define I2C_ST_WRADDRL_ACK            = 0x22
#define I2C_ST_WRADDRL_TOUT           = 0x23
#define I2C_ST_WRADDRL_NACK_STOP_PEND = 0x24
#define I2C_ST_WRADDRL_NACK_STOP      = 0x25  /* device did not ack */
#define I2C_ST_WRADDRH                = 0x30
#define I2C_ST_WRADDRH_WAITSEND       = 0x31
#define I2C_ST_WRADDRH_ACK            = 0x32
#define I2C_ST_WRADDRH_TOUT           = 0x33

#define I2C_ST_WRITEDATA              = 0x40  /* sending data chunk to slave */
#define I2C_ST_WRITEDATA_WAITSEND     = 0x41  /* happens sometimes, retry works ok */
#define I2C_ST_WRITEDATA_ACK          = 0x42
#define I2C_ST_WRITEDATA_WAIT         = 0x43  /* waiting for slave to ack after sending a byte */
#define I2C_ST_WRITEDATA_TOUT         = 0x44
#define I2C_ST_WRITEDATA_END_NOSTOP   = 0x45  /* last transfer finished, in non stop mode */

#define I2C_ST_READDATA               = 0x50  /* reading data from i2c slave */
#define I2C_ST_READDATA_RCEN          = 0x51
#define I2C_ST_READDATA_TOUT          = 0x52  /* read data timed out */
#define I2C_ST_READDATA_ACK           = 0x53
#define I2C_ST_READDATA_WAIT          = 0x54  /* data buffer is full, more data to come */
#define I2C_ST_READDATA_WAITGET       = 0x55  /* data buffer is full, no more data to come */

#define I2C_ST_STOP                   = 0x60
#define I2C_ST_STOP_WAIT              = 0x61
#define I2C_ST_STOP_TOUT              = 0x62  /* timeout in stop condition (bus busy) */

// Bytes in CMD_POLL_STATUS_SET_PARAMETERS response
#define I2C_POLL_RESP_NEWSPEED_STATUS =  3
#define I2C_POLL_RESP_STATUS          =  8
#define I2C_POLL_RESP_REQ_LEN_L       =  9
#define I2C_POLL_RESP_REQ_LEN_H       = 10
#define I2C_POLL_RESP_TX_LEN_L        = 11
#define I2C_POLL_RESP_TX_LEN_H        = 12
#define I2C_POLL_RESP_CLKDIV          = 14
#define I2C_POLL_RESP_UNDOCUMENTED_18 = 18
#define I2C_POLL_RESP_ACK             = 20
#define I2C_POLL_RESP_UNDOCUMENTED_21 = 21
#define I2C_POLL_RESP_SCL             = 22
#define I2C_POLL_RESP_SDA             = 23
#define I2C_POLL_RESP_INT_FLAG        = 24
#define I2C_POLL_RESP_READ_PEND       = 25

#define I2C_POLL_RESP_HARD_MAYOR      = 46
#define I2C_POLL_RESP_HARD_MINOR      = 47
#define I2C_POLL_RESP_FIRM_MAYOR      = 48
#define I2C_POLL_RESP_FIRM_MINOR      = 49

#define I2C_POLL_RESP_ADC_CH0_LSB     = 50
#define I2C_POLL_RESP_ADC_CH0_MSB     = 51
#define I2C_POLL_RESP_ADC_CH1_LSB     = 52
#define I2C_POLL_RESP_ADC_CH1_MSB     = 53
#define I2C_POLL_RESP_ADC_CH2_LSB     = 54
#define I2C_POLL_RESP_ADC_CH2_MSB     = 55

#endif // CONSTANTS_H