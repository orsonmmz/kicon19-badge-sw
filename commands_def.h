#ifdef GENERATE_PYTHON
## auto generated from commands_def.h
  #define START_ENUM(x)
  #define END_ENUM(x)
  #define DEF_ENUM(x,y) x = y
#else
  #define START_ENUM(x) typedef enum {
  #define END_ENUM(x) } x;
  #define DEF_ENUM(x,y) x = y,
#endif

START_ENUM(cmd_type_t)
DEF_ENUM(CMD_TYPE_RESET,    0)
DEF_ENUM(CMD_TYPE_UART,     1)
DEF_ENUM(CMD_TYPE_I2C,      2)
DEF_ENUM(CMD_TYPE_SPI,      3)
DEF_ENUM(CMD_TYPE_PIO,      4)
DEF_ENUM(CMD_TYPE_LED,      5)
DEF_ENUM(CMD_TYPE_LCD,      6)
DEF_ENUM(CMD_TYPE_BTN,      7)
DEF_ENUM(CMD_TYPE_ADC,      8)
DEF_ENUM(CMD_TYPE_DAC,      9)
END_ENUM(cmd_type_t)

START_ENUM(cmd_resp_t)
DEF_ENUM(CMD_RESP_NULL,         0)  /* no action, command is incomplete */
DEF_ENUM(CMD_RESP_RESET,        1)  /* command buffer reset */
DEF_ENUM(CMD_RESP_OK,           2)  /* command executed succesfully */
DEF_ENUM(CMD_RESP_INVALID_CMD,  3)  /* invalid command */
DEF_ENUM(CMD_RESP_OVERFLOW,     4)  /* command buffer overflown */
DEF_ENUM(CMD_RESP_CRC_ERR,      5)  /* incorrect CRC */
DEF_ENUM(CMD_RESP_EXEC_ERR,     6)  /* valid command, execution error */
END_ENUM(cmd_resp_t)

START_ENUM(cmd_led_t)
DEF_ENUM(CMD_LED_SET,           0)  /* params: led number and state */
DEF_ENUM(CMD_LED_BLINK,         1)  /* params: led number and period */
END_ENUM(cmd_led_t)

START_ENUM(cmd_i2c_t)
DEF_ENUM(CMD_I2C_READ,          0)  /* params: device address, byte count */
DEF_ENUM(CMD_I2C_WRITE,         1)  /* params: device address, byte count, data */
END_ENUM(cmd_i2c_t)

START_ENUM(cmd_spi_t)
DEF_ENUM(CMD_SPI_CONFIG,        0)  /* params: clock speed (2 bytes; kHz), SPI mode */
DEF_ENUM(CMD_SPI_TRANSFER,      1)  /* params: byte count, data */
END_ENUM(cmd_spi_t)

START_ENUM(cmd_lcd_t)
DEF_ENUM(CMD_LCD_CLEAR,         0)
DEF_ENUM(CMD_LCD_REFRESH,       1)
DEF_ENUM(CMD_LCD_PIXEL,         2)  /* params: x, y, color */
DEF_ENUM(CMD_LCD_TEXT,          3)  /* params: row, col, byte count, text */
END_ENUM(cmd_lcd_t)
