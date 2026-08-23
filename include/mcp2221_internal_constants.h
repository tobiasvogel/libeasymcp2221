#ifndef MCP2221_INTERNAL_CONSTANTS_H
#define MCP2221_INTERNAL_CONSTANTS_H

/*
 * Internal MCP2221 protocol and packet-layout constants.
 *
 * This header is part of the libeasymcp2221 implementation and is not part of
 * the public API. It is intentionally excluded from installation by CMake.
 */

#include "mcp2221_constants.h"

#define MCP2221_DEFAULT_EP_IN   0x81
#define MCP2221_DEFAULT_EP_OUT  0x01
#define MCP2221_FLASH_OFFSET_WRITE 	2
#define MCP2221_FLASH_OFFSET_READ  	4
#define MCP2221_FLASH_RESPONSE_STRUCTURE_LENGTH_BYTE 2
#define MCP2221_USB_STRING_DESCRIPTOR_HEADER_SIZE     2u
#define MCP2221_FLASH_CHIP_SETTINGS_CDCSEC   (2 - 2)
#define MCP2221_FLASH_CHIP_SETTINGS_CLOCK    (3 - 2)
#define MCP2221_FLASH_CHIP_SETTINGS_DAC      (4 - 2)
#define MCP2221_FLASH_CHIP_SETTINGS_INT_ADC  (5 - 2)
#define MCP2221_FLASH_CHIP_SETTINGS_LVID     (6 - 2)
#define MCP2221_FLASH_CHIP_SETTINGS_HVID     (7 - 2)
#define MCP2221_FLASH_CHIP_SETTINGS_LPID     (8 - 2)
#define MCP2221_FLASH_CHIP_SETTINGS_HPID     (9 - 2)
#define MCP2221_FLASH_CHIP_SETTINGS_USBPWR  (10 - 2)
#define MCP2221_FLASH_CHIP_SETTINGS_USBMA   (11 - 2)
#define MCP2221_FLASH_CHIP_SETTINGS_PWD1    (12 - 2)
#define MCP2221_FLASH_CHIP_SETTINGS_PWD2    (13 - 2)
#define MCP2221_FLASH_CHIP_SETTINGS_PWD3    (14 - 2)
#define MCP2221_FLASH_CHIP_SETTINGS_PWD4    (15 - 2)
#define MCP2221_FLASH_CHIP_SETTINGS_PWD5    (16 - 2)
#define MCP2221_FLASH_CHIP_SETTINGS_PWD6    (17 - 2)
#define MCP2221_FLASH_CHIP_SETTINGS_PWD7    (18 - 2)
#define MCP2221_FLASH_CHIP_SETTINGS_PWD8    (19 - 2)
#define MCP2221_USB_PWR_SELF_POWERED        (1u << 6)
#define MCP2221_USB_PWR_REMOTE_WAKEUP       (1u << 5)
#define MCP2221_USB_CURRENT_UNIT_MA          2u
#define MCP2221_FLASH_GP_SETTINGS_GP0        (2 - 2)
#define MCP2221_FLASH_GP_SETTINGS_GP1        (3 - 2)
#define MCP2221_FLASH_GP_SETTINGS_GP2        (4 - 2)
#define MCP2221_FLASH_GP_SETTINGS_GP3        (5 - 2)
#define MCP2221_SRAM_RESPONSE_SETTINGS_OFFSET 4u
#define MCP2221_SRAM_CHIP_SETTINGS_CDCSEC    (4 - 4)
#define MCP2221_SRAM_CHIP_SETTINGS_CLOCK     (5 - 4)
#define MCP2221_SRAM_CHIP_SETTINGS_DAC       (6 - 4)
#define MCP2221_SRAM_CHIP_SETTINGS_INT_ADC   (7 - 4)
#define MCP2221_SRAM_CHIP_SETTINGS_LVID      (8 - 4)
#define MCP2221_SRAM_CHIP_SETTINGS_HVID      (9 - 4)
#define MCP2221_SRAM_CHIP_SETTINGS_LPID     (10 - 4)
#define MCP2221_SRAM_CHIP_SETTINGS_HPID     (11 - 4)
#define MCP2221_SRAM_CHIP_SETTINGS_USBPWR   (12 - 4)
#define MCP2221_SRAM_CHIP_SETTINGS_USBMA    (13 - 4)
#define MCP2221_SRAM_CHIP_SETTINGS_PWD1     (14 - 4)
#define MCP2221_SRAM_CHIP_SETTINGS_PWD2     (15 - 4)
#define MCP2221_SRAM_CHIP_SETTINGS_PWD3     (16 - 4)
#define MCP2221_SRAM_CHIP_SETTINGS_PWD4     (17 - 4)
#define MCP2221_SRAM_CHIP_SETTINGS_PWD5     (18 - 4)
#define MCP2221_SRAM_CHIP_SETTINGS_PWD6     (19 - 4)
#define MCP2221_SRAM_CHIP_SETTINGS_PWD7     (20 - 4)
#define MCP2221_SRAM_CHIP_SETTINGS_PWD8     (21 - 4)
#define MCP2221_SRAM_GP_SETTINGS_GP0        (22 - 4)
#define MCP2221_SRAM_GP_SETTINGS_GP1        (23 - 4)
#define MCP2221_SRAM_GP_SETTINGS_GP2        (24 - 4)
#define MCP2221_SRAM_GP_SETTINGS_GP3        (25 - 4)
#define MCP2221_SRAM_RESPONSE_DAC           6
#define MCP2221_SRAM_RESPONSE_INT_ADC       7
#define MCP2221_SRAM_RESPONSE_GP0          22
#define MCP2221_SRAM_RESPONSE_GP1          23
#define MCP2221_SRAM_RESPONSE_GP2          24
#define MCP2221_SRAM_RESPONSE_GP3          25
#define MCP2221_CDCSEC_CDCSNEN              (1 << 7)  // USB CDC Serial Number Enable bit
#define MCP2221_CDCSEC_LEDURXINST           (1 << 6)  // LED UART RX Inactive State bit
#define MCP2221_CDCSEC_LEDUTXINST           (1 << 5)  // LED UART TX Inactive State bit
#define MCP2221_CDCSEC_LEDI2CINST           (1 << 4)  // LED I2C Inactive State bit
#define MCP2221_CDCSEC_SSPNDINST            (1 << 3)  // SSPND Inactive State bit
#define MCP2221_CDCSEC_USBCFGINST           (1 << 2)  // USBCFG Inactive State bit
#define MCP2221_CDCSEC_CHIPPROT_RESERVED    0x03u    // Chip protection
#define MCP2221_CDCSEC_CHIPPROT_LOCKED      0x02u
#define MCP2221_CDCSEC_CHIPPROT_PROTECTED   0x01u
#define MCP2221_CDCSEC_CHIPPROT_UNPROTECTED 0x00u
#define MCP2221_ALTER_GPIO_CONF		(1 << 7) /* bit 7: alters the current GP designation */
#define MCP2221_PRESERVE_GPIO_CONF 	(0 << 7)
#define MCP2221_GPIO_OUT_VAL_1  	(1 << 4)
#define MCP2221_GPIO_OUT_VAL_0  	(0 << 4)
#define MCP2221_GPIO_DIR_IN     	(1 << 3)
#define MCP2221_GPIO_DIR_OUT    	(0 << 3)
#define MCP2221_ALTER_INT_CONF			(1 << 7) /* Enable the modification of the interrupt detection conditions */
#define MCP2221_PRESERVE_INT_CONF		(0 << 7)
#define MCP2221_INT_POS_EDGE_ENABLE		(0x03u << 3)
#define MCP2221_INT_POS_EDGE_DISABLE	(0x02u << 3)
#define MCP2221_INT_NEG_EDGE_ENABLE		(0x03u << 1)
#define MCP2221_INT_NEG_EDGE_DISABLE	(0x02u << 1)
#define MCP2221_INT_FLAG_CLEAR			1
#define MCP2221_INT_FLAG_PRESERVE		0
#define MCP2221_INT_EDGE_STATE_INACTIVE         0u
#define MCP2221_INT_EDGE_STATE_ACTIVE           1u
#define MCP2221_ALTER_ADC_REF			(1 << 7) /* Enable loading of a new ADC reference */
#define MCP2221_PRESERVE_ADC_REF		(0 << 7)
#define MCP2221_ADC_VRM_MASK			(0x03u << 1)
#define MCP2221_ADC_REF_MASK			0x01u
#define MCP2221_ADC_RAW_MAX                     1023u
#define MCP2221_ALTER_DAC_REF			(1 << 7) /* Enable loading of a new DAC reference */
#define MCP2221_PRESERVE_DAC_REF		(0 << 7)
#define MCP2221_DAC_VRM_MASK            (0x03u << 1)
#define MCP2221_DAC_REF_MASK            0x01u
#define MCP2221_ALTER_DAC_VALUE			(1 << 7) /* Enable loading of a new DAC value */
#define MCP2221_PRESERVE_DAC_VALUE		(0 << 7)
#define MCP2221_DAC_LEVEL_COUNT         32u
#define MCP2221_DAC_RAW_MAX             (MCP2221_DAC_LEVEL_COUNT - 1u)
#define MCP2221_ALTER_CLK_OUTPUT		(1 << 7) /* Enable loading of a new clock divider */
#define MCP2221_PRESERVE_CLK_OUTPUT		(0 << 7)
#define MCP2221_I2C_CMD_CANCEL_CURRENT_TRANSFER		0x10
#define MCP2221_I2C_CMD_SET_BUS_SPEED				0x20
#define MCP2221_I2C_CONFUSED_MARKER                  8u
#define MCP2221_I2C_RELEASE_ATTEMPTS                 3
#define MCP2221_I2C_RELEASE_DELAY_NS                 10000000L
#define MCP2221_I2C_BASE_CLOCK_HZ                    12000000.0
#define MCP2221_I2C_CLOCK_DIVIDER_OFFSET             2L
#define MCP2221_I2C_CLOCK_DIVIDER_MAX                255
#define MCP2221_I2C_NEWSPEED_ACCEPTED                0x20u
#define MCP2221_RESET_CHIP_SURE					0xAB
#define MCP2221_RESET_CHIP_VERY_SURE			0xCD
#define MCP2221_RESET_CHIP_VERY_VERY_SURE       0xEF
#define MCP2221_GPIO_GET_RESP_GP0_VALUE       2
#define MCP2221_GPIO_GET_RESP_GP1_VALUE       4
#define MCP2221_GPIO_GET_RESP_GP2_VALUE       6
#define MCP2221_GPIO_GET_RESP_GP3_VALUE       8
#define MCP2221_I2C_CHUNK_SIZE		          60
#define MCP2221_I2C_GET_DATA_ERROR_COUNT      127u
#define MCP2221_I2C_INTERNAL_STATUS_BYTE      2
#define MCP2221_I2C_ST_IDLE                   0x00
#define MCP2221_I2C_ST_START                  0x10  /* sending start condition */
#define MCP2221_I2C_ST_START_ACK              0x11
#define MCP2221_I2C_ST_START_TOUT             0x12
#define MCP2221_I2C_ST_REPSTART               0x15
#define MCP2221_I2C_ST_REPSTART_ACK           0x16
#define MCP2221_I2C_ST_REPSTART_TOUT          0x17
#define MCP2221_I2C_ST_WRADDRL                0x20
#define MCP2221_I2C_ST_WRADDRL_WAITSEND       0x21
#define MCP2221_I2C_ST_WRADDRL_ACK            0x22
#define MCP2221_I2C_ST_WRADDRL_TOUT           0x23
#define MCP2221_I2C_ST_WRADDRL_NACK_STOP_PEND 0x24
#define MCP2221_I2C_ST_WRADDRL_NACK_STOP      0x25  /* device did not ack */
#define MCP2221_I2C_ST_WRADDRH                0x30
#define MCP2221_I2C_ST_WRADDRH_WAITSEND       0x31
#define MCP2221_I2C_ST_WRADDRH_ACK            0x32
#define MCP2221_I2C_ST_WRADDRH_TOUT           0x33
#define MCP2221_I2C_ST_WRITEDATA              0x40  /* sending data chunk to slave */
#define MCP2221_I2C_ST_WRITEDATA_WAITSEND     0x41  /* happens sometimes, retry works ok */
#define MCP2221_I2C_ST_WRITEDATA_ACK          0x42
#define MCP2221_I2C_ST_WRITEDATA_WAIT         0x43  /* waiting for slave to ack after sending a byte */
#define MCP2221_I2C_ST_WRITEDATA_TOUT         0x44
#define MCP2221_I2C_ST_WRITEDATA_END_NOSTOP   0x45  /* last transfer finished, in non stop mode */
#define MCP2221_I2C_ST_READDATA               0x50  /* reading data from i2c slave */
#define MCP2221_I2C_ST_READDATA_RCEN          0x51
#define MCP2221_I2C_ST_READDATA_TOUT          0x52  /* read data timed out */
#define MCP2221_I2C_ST_READDATA_ACK           0x53
#define MCP2221_I2C_ST_READDATA_WAIT          0x54  /* data buffer is full, more data to come */
#define MCP2221_I2C_ST_READDATA_WAITGET       0x55  /* data buffer is full, no more data to come */
#define MCP2221_I2C_ST_STOP                   0x60
#define MCP2221_I2C_ST_STOP_WAIT              0x61
#define MCP2221_I2C_ST_STOP_TOUT              0x62  /* timeout in stop condition (bus busy) */
#define MCP2221_I2C_POLL_RESP_NEWSPEED_STATUS  3
#define MCP2221_I2C_POLL_RESP_STATUS           8
#define MCP2221_I2C_POLL_RESP_REQ_LEN_L        9
#define MCP2221_I2C_POLL_RESP_REQ_LEN_H       10
#define MCP2221_I2C_POLL_RESP_TX_LEN_L        11
#define MCP2221_I2C_POLL_RESP_TX_LEN_H        12
#define MCP2221_I2C_POLL_RESP_CLKDIV          14
#define MCP2221_I2C_POLL_RESP_UNDOCUMENTED_18 18
#define MCP2221_I2C_POLL_RESP_ACK             20
#define MCP2221_I2C_POLL_RESP_UNDOCUMENTED_21 21
#define MCP2221_I2C_POLL_RESP_SCL             22
#define MCP2221_I2C_POLL_RESP_SDA             23
#define MCP2221_I2C_POLL_RESP_INT_FLAG        24
#define MCP2221_I2C_POLL_RESP_READ_PEND       25
#define MCP2221_I2C_POLL_RESP_HARD_MAJOR      46
#define MCP2221_I2C_POLL_RESP_HARD_MINOR      47
#define MCP2221_I2C_POLL_RESP_FIRM_MAJOR      48
#define MCP2221_I2C_POLL_RESP_FIRM_MINOR      49
#define MCP2221_I2C_POLL_RESP_ADC_CH0_LSB     50
#define MCP2221_I2C_POLL_RESP_ADC_CH0_MSB     51
#define MCP2221_I2C_POLL_RESP_ADC_CH1_LSB     52
#define MCP2221_I2C_POLL_RESP_ADC_CH1_MSB     53
#define MCP2221_I2C_POLL_RESP_ADC_CH2_LSB     54
#define MCP2221_I2C_POLL_RESP_ADC_CH2_MSB     55

#endif // MCP2221_INTERNAL_CONSTANTS_H
