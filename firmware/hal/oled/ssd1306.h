/*
 * SSD1309.h
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

#ifndef SSD1306_H_
#define SSD1306_H_

#include <stdint.h> //for uint16_t type

#define SSD1306_WIDTH		128
#define SSD1306_HEIGHT		32

void SSD1306_init();
void SSD1306_clear();
void SSD1306_display(uint8_t *Image);
void SSD1306_display_half(uint8_t *Image, int half);

#endif //SSD1306_H_
