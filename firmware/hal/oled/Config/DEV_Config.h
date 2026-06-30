/*****************************************************************************
* | File        :   DEV_Config.h
* | Author      :   Waveshare team (types) — trimmed for Pocket Varan
* | Function    :   Type aliases and debug macro used by the Waveshare GUI code.
* | Info        :
* | License     :   MIT
*-----------------------------------------------------------------------------
* Original DEV_Config pulled in a Raspberry-Pi GPIO/SPI/I2C hardware layer that
* the Pocket Varan OLED path does not use (ssd1306.c talks to /dev/i2c directly
* via hal/i2c.h). This trimmed version keeps only what GUI_Paint / GUI_BMPfile
* actually need: the UBYTE/UWORD/UDOUBLE aliases and the Debug() macro.
******************************************************************************/
#ifndef _DEV_CONFIG_H_
#define _DEV_CONFIG_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "Debug.h"

#define USE_SPI 0
#define USE_IIC 1
#define IIC_CMD 0x00
#define IIC_RAM 0x40

// data types used throughout the Waveshare GUI code
#define UBYTE   uint8_t
#define UWORD   uint16_t
#define UDOUBLE uint32_t

#endif  // _DEV_CONFIG_H_
