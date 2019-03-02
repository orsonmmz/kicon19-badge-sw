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

START_ENUM(CMD_TYPE)
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
END_ENUM(CMD_TYPE)

START_ENUM(CMD_RESP)
DEF_ENUM(CMD_RESP_NULL,         0)  /* no action, command is incomplete */
DEF_ENUM(CMD_RESP_RESET,        1)  /* command buffer reset */
DEF_ENUM(CMD_RESP_OK,           2)  /* command executed succesfully */
DEF_ENUM(CMD_RESP_INVALID_CMD,  3)  /* invalid command */
DEF_ENUM(CMD_RESP_OVERFLOW,     4)  /* command buffer overflown */
DEF_ENUM(CMD_RESP_CRC_ERR,      5)  /* incorrect CRC */
END_ENUM(CMD_RESP)
