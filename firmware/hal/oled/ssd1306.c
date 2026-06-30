/*
 * SSD1309.c
 *
 *  Created on: Oct 4, 2024
 *
 *  The code is based on file OLED_0in91.c version 2.0 from 2020-08-17 ("0.91inch OLED Module Drive function") by the Waveshare team
 *
 *  Adapted by Mario from demo code found at: https://www.waveshare.com/wiki/0.91inch_OLED_Module
 *
 *  This file is part of the Siluria / Captain Nemotron / Loopstyler V3s Firmware Development Framework.
 *  It can be used within the terms of MIT license.
 *
 *  Find more information at:
 *  http://phonicbloom.com/diy/
 *  http://gechologic.com/gechologists/
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "ssd1306.h"
#include "hal/i2c.h"

void SSD1306_init()
{
	i2c_init();

	i2cset(I2C_OLED, 0X00, 0xAE);	//SSD1306_DISPLAYOFF

    i2cset(I2C_OLED, 0X00, 0x40);	//PCD8544_SETYADDR ---set low column address
    i2cset(I2C_OLED, 0X00, 0xB0);	//SSD1306_SETPAGE ---set high column address

	#ifdef SSD1306_ROTATE
    i2cset(I2C_OLED, 0X00, 0xC0);	//SSD1306_COMSCANINC
	#else
    i2cset(I2C_OLED, 0X00, 0xC8);	//SSD1306_COMSCANDEC
	#endif

    i2cset(I2C_OLED, 0X00, 0x81);	//SSD1306_SETCONTRAST
    i2cset(I2C_OLED, 0X00, 0xff);

    i2cset(I2C_OLED, 0X00, 0xa1);

    i2cset(I2C_OLED, 0X00, 0xa6);	//SSD1306_NORMALDISPLAY

    i2cset(I2C_OLED, 0X00, 0xa8);	//SSD1306_SETMULTIPLEX
    i2cset(I2C_OLED, 0X00, 0x1f);

    i2cset(I2C_OLED, 0X00, 0xd3);	//SSD1306_SETDISPLAYOFFSET
    i2cset(I2C_OLED, 0X00, 0x00);

    i2cset(I2C_OLED, 0X00, 0xd5);	//SSD1306_SETDISPLAYCLOCKDIV
    i2cset(I2C_OLED, 0X00, 0xf0);

    i2cset(I2C_OLED, 0X00, 0xd9);	//SSD1306_SETPRECHARGE
    i2cset(I2C_OLED, 0X00, 0x22);

    i2cset(I2C_OLED, 0X00, 0xda);	//SSD1306_SETCOMPINS
    i2cset(I2C_OLED, 0X00, 0x02);

    i2cset(I2C_OLED, 0X00, 0xdb);	//SSD1306_SETVCOMDETECT
    i2cset(I2C_OLED, 0X00, 0x49);

    i2cset(I2C_OLED, 0X00, 0x8d);	//SSD1306_CHARGEPUMP
    i2cset(I2C_OLED, 0X00, 0x14);

    usleep(200000);

    i2cset(I2C_OLED, 0X00, 0xaf);	//SSD1306_DISPLAYON	---turn on the OLED display
}

void SSD1306_clear()
{
	//Clear screen

	uint8_t Column,Page;

	for(Page = 0; Page < SSD1306_HEIGHT/8; Page++)
	{
		i2cset(I2C_OLED, 0x00, 0xb0 + Page);    //Set page address
		i2cset(I2C_OLED, 0x00, 0x00);           //Set display position - column low address
		i2cset(I2C_OLED, 0x00, 0x10);           //Set display position - column high address

		/*
		for(Column = 0; Column < SSD1306_WIDTH; Column++)
		{
			i2cset(I2C_OLED, 0x40, 0x00); //write 1 byte
		}
		*/

		/*
		for(Column = 0; Column < SSD1306_WIDTH / 2; Column++)
		{
			//fill with pattern (test)
			i2cset_w(I2C_OLED, 0x40, 0x00, 0xff); //write 2 bytes
		}
		*/

		//i2cfill(I2C_OLED, 0x40, 0x55, SSD1306_WIDTH); //fill with pattern (test)
		i2cfill(I2C_OLED, 0x40, 0x00, SSD1306_WIDTH); //fill with black
	}
}

void SSD1306_display(uint8_t *Image)
{
	uint8_t /*Column,*/Page;
	//uint16_t temp;

	for(Page = 0; Page < SSD1306_HEIGHT/8; Page++)
	{
		//printf("i2cset(I2C_OLED, 0x00, 0xb0 + Page=%d);    //Set page address\n", Page);
		if(i2cset(I2C_OLED, 0x00, 0xb0 + Page))    //Set page address
		{
			return;
		}
		//printf("i2cset(I2C_OLED, 0x00, 0x00);           //Set display position - column low address\n");
		if(i2cset(I2C_OLED, 0x00, 0x00))           //Set display position - column low address
		{
			return;
		}
		//printf("i2cset(I2C_OLED, 0x00, 0x10);           //Set display position - column high address\n");
		if(i2cset(I2C_OLED, 0x00, 0x10))           //Set display position - column high address
		{
			return;
		}

		/*
		for(Column = 0; Column < SSD1306_WIDTH; Column++)
		{
            temp = Image[(3-Page) + Column*4];
			i2cset(I2C_OLED, 0x40, temp); //write data
		}
		*/

		//i2cwrite(I2C_OLED, 0x40, &Image[(3-Page)], SSD1306_WIDTH);
		//printf("i2cwrite_columns(I2C_OLED, 0x40, &Image[(3-Page)], SSD1306_WIDTH, 4);\n");
		if(i2cwrite_columns(I2C_OLED, 0x40, &Image[(3-Page)], SSD1306_WIDTH, 4))
		{
			return;
		}
	}
}

void SSD1306_display_half(uint8_t *Image, int half)
{
	uint8_t Page;

	for(Page = 0; Page < SSD1306_HEIGHT/16; Page++)
	{
		i2cset(I2C_OLED, 0x00, 0xb0 + Page + 2 * half);    //Set page address
		i2cset(I2C_OLED, 0x00, 0x00);           //Set display position - column low address
		i2cset(I2C_OLED, 0x00, 0x10);           //Set display position - column high address

		i2cwrite_columns(I2C_OLED, 0x40, &Image[(3-Page-2*half)], SSD1306_WIDTH, 4);
	}
}
