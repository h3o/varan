/*
 * Display.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: 14 Oct 2024
 *      Author: mario
 *
 *  Part of the Pocket Varan firmware. SPDX-License-Identifier: MIT
 *
 *  Find more information at:
 *  http://phonicbloom.com/diy/
 *  http://gechologic.com/gechologists/
 *  http://loopstyler.com/
 *
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <string.h>
#include <stdlib.h>
#include <stdint.h> //for uint16_t type
#include <pthread.h>

#include "oled/ssd1306.h"

// Priority bump for the OLED refresh thread (was defined in the prototype's
// peripherals.h). 0 = inherit; raise if the display lags under load.
#ifndef THREAD_PRIORITY_OLED
#define THREAD_PRIORITY_OLED 0
#endif

#ifdef __cplusplus


#define OLED_SCROLL_BUFFER_WIDTH	8 //in standard screen sizes

#define OLED_MODE_OFF			0
#define OLED_MODE_LOGO			1
#define OLED_MODE_NOISE			2
#define OLED_MODE_NORMAL		3
#define OLED_MODE_SCROLLING		4

#define DISPLAY_TIMEOUT_NONE	0
#define DISPLAY_TIMEOUT_SUCCESS	2000 //in ms, resolution is 0.1s
#define DISPLAY_TIMEOUT_INVALID	4000

#define SCROLL_DELAY_DEFAULT	100

class Display {

public:

	// constructor and destructor
	Display(void);
	~Display(void);

	void test();
	void welcome();

	inline void set_mode(int mode) { oled_mode = mode; };

	void show_menu(char *text1, char *text2, int timeout);
	void hide_menu();

	void scroll_message(char *text);

protected:


private:

	pthread_t OLED_thr;

	void init_driver_thread();
	void OLED_driver();

	static void* OLED_driver_wrapper(void* object)
	{
		reinterpret_cast<Display*>(object)->OLED_driver();
	    return NULL;
	}

	int oled_mode = OLED_MODE_OFF;
	int oled_mode0;

	int screen_timeout = 0;

	uint8_t *MainImage;
	uint16_t MainImagesize = ((SSD1306_WIDTH%8==0)? (SSD1306_WIDTH/8): (SSD1306_WIDTH/8+1)) * SSD1306_HEIGHT;

	uint8_t *ScrollingText;
	uint16_t ScrollingTextsize = MainImagesize * (OLED_SCROLL_BUFFER_WIDTH+1);

	int scroll_position, scroll_limit, scroll_delay;

	void copy_region(uint8_t *src, uint8_t *dst, int src_x0, int src_x1, int src_y0, int src_y1, int dst_x, int dst_y);

	inline void clear_main() { memset(MainImage,0x00,MainImagesize); }
	inline void clear_scroll() { memset(ScrollingText,0x00,ScrollingTextsize); }

	void frame(uint8_t *Image, int image_size);

};

#else

//this needs to be accessible with c-linkage

#endif

#endif /* DISPLAY_H_ */
