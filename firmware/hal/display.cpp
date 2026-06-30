/*
 * Display.cpp
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

#include "hal/display.h"

#include "oled/GUI/GUI_Paint.h"

//#define DEBUG_DISPLAY
//#define TEST_DISPLAY

#ifdef USE_SSD1306

Display::Display(void)
{
	//printf("Display::Display()\n");

	//printf("Display::Display(): Imagesize=%d, ScrollingTextsize=%d\n", MainImagesize, ScrollingTextsize);

	if((MainImage = (uint8_t*)malloc(MainImagesize)) == NULL)
	{
		printf("Display::Display()(): malloc(Imagesize) failed...\r\n");
		return;
	}

	if((ScrollingText = (uint8_t*)malloc(ScrollingTextsize)) == NULL)
	{
		printf("Display::Display()(): malloc(ScrollingText) failed...\r\n");
		return;
	}

	SSD1306_init();
	SSD1306_clear();

	init_driver_thread();
	usleep(100000);

	//oled_mode = OLED_MODE_NOISE;
	oled_mode = OLED_MODE_LOGO;

	//Paint_SetRotate(ROTATE_180); changes coords system

	show_menu("VARANISKO", "", DISPLAY_TIMEOUT_NONE);
	scroll_message("Because MP3 players ain't rocket science...");

	//printf("Display::Display(): constructor done\n");
}

Display::~Display(void)
{
	//printf("Display::~Display()\n");

	SSD1306_clear();
	free(MainImage);
	free(ScrollingText);
}

void Display::init_driver_thread()
{
	pthread_attr_t attr;
	struct sched_param param;
	int res;

	res = pthread_attr_init (&attr);
	res = pthread_attr_getschedparam (&attr, &param);

	//printf("Display::init_thread(): initial param.sched_priority = %d, increasing\n", param.sched_priority);
	(param.sched_priority)+=THREAD_PRIORITY_OLED;
	//printf("Display::init_thread(): param.sched_priority set to %d\n", param.sched_priority);

	res = pthread_attr_setschedparam (&attr, &param);

	res = pthread_create(&OLED_thr, &attr, OLED_driver_wrapper, this);
	//res = pthread_create(&MIDI_thr, NULL, OLED_driver, NULL);
	//printf("Display::init_thread(): \"OLED_driver\" thread id = 0x%x, res = %d\n", OLED_thr, res);
}

void Display::OLED_driver()
{
	int refresh_loops = 0;

	oled_mode0 = oled_mode;

	Paint_NewImage(MainImage, SSD1306_HEIGHT, SSD1306_WIDTH, 90, BLACK);
	Paint_NewImage(ScrollingText, SSD1306_HEIGHT, SSD1306_WIDTH * OLED_SCROLL_BUFFER_WIDTH, 90, BLACK);

	while(1) //!exit_program || halt_program)
    {
		if(oled_mode!=oled_mode0)
		{
			#ifdef DEBUG_DISPLAY
			printf("Display::OLED_driver(): oled_mode!=oled_mode0 (%d->%d)\n", oled_mode0, oled_mode);
			#endif //DEBUG_DISPLAY

			oled_mode0 = oled_mode;

			if(oled_mode==OLED_MODE_OFF)
			{
				#ifdef DEBUG_DISPLAY
				printf("Display::OLED_driver(): oled_mode!=oled_mode0 && oled_mode==OLED_MODE_OFF, clearing the display\n");
				#endif //DEBUG_DISPLAY

				clear_main();
				//SSD1306_display(TestImage);
				SSD1306_clear();
				SSD1306_clear(); //for higher display reliability
			}
			else if(oled_mode==OLED_MODE_LOGO)
			{
				#ifdef DEBUG_DISPLAY
				printf("Display::OLED_driver(): oled_mode!=oled_mode0 && oled_mode==OLED_MODE_LOGO, displaying the logo\n");
				#endif //DEBUG_DISPLAY

				clear_main();
				frame(MainImage, MainImagesize);
				Paint_SelectImage(MainImage);

				#ifdef BOARD_VR291
				Paint_DrawString_EN(4, 2, "VARAN MP3", &Font16, WHITE, WHITE);
				Paint_DrawString_EN(5, 18, "Let's techno!", &Font12, WHITE, WHITE);
				#elif BOARD_SILURIA
				Paint_DrawString_EN(4, 2, "SILURIA", &Font16, WHITE, WHITE);
				Paint_DrawString_EN(5, 18, "by PhonicBloom", &Font12, WHITE, WHITE);
				#endif

				SSD1306_display(MainImage);
			}
			else if(oled_mode==OLED_MODE_SCROLLING)
			{
				SSD1306_display(MainImage);
				scroll_position = 0;
			}
		}
		else
		{
			if(oled_mode==OLED_MODE_NOISE)
			{
				#ifdef DEBUG_DISPLAY
				printf("Display::OLED_driver(): OLED_MODE_NOISE, loop %d\n", refresh_loops);
				#endif //DEBUG_DISPLAY

				// NOISE screensaver removed in Phase 0 (it depended on the
				// prototype's signals.c). Not used by the navigate-only UI.
				SSD1306_display(MainImage);

				//usleep(40000); //25Hz
				usleep(20000); //50Hz
				//usleep(10000); //100Hz
			}
			else if(oled_mode==OLED_MODE_SCROLLING)
			{
				#ifdef DEBUG_DISPLAY
				printf("Display::OLED_driver(): OLED_MODE_SCROLLING, loop %d\n", refresh_loops);
				#endif //DEBUG_DISPLAY

				if(scroll_position>scroll_delay)
				{
					copy_region(ScrollingText, MainImage, scroll_position-scroll_delay, scroll_position-scroll_delay+SSD1306_WIDTH-1, 0, 15, 0, 16); //y coordinates need to be aligned to 8 (1 byte = 8 pixels column)
					frame(MainImage, MainImagesize);
					SSD1306_display_half(MainImage, 1);
				}
				else if(scroll_position==0)
				{
					copy_region(ScrollingText, MainImage, 0, SSD1306_WIDTH-1, 0, 15, 0, 16); //y coordinates need to be aligned to 8 (1 byte = 8 pixels column)
					frame(MainImage, MainImagesize);
					SSD1306_display_half(MainImage, 1);
				}

				scroll_position++;

				if(scroll_position > scroll_limit + scroll_delay)
				{
					scroll_position = 0;
				}

				//usleep(40000); //25Hz
				usleep(20000); //50Hz
				//usleep(10000); //100Hz
			}
			else if(oled_mode==OLED_MODE_OFF)
			{
				#ifdef DEBUG_DISPLAY
				printf("Display::OLED_driver(): OLED_MODE_OFF, loop %d\n", refresh_loops);
				#endif //DEBUG_DISPLAY
				usleep(100000);
			}
			else
			{
				usleep(100000);
			}

			refresh_loops++;

			if(screen_timeout>0)
			{
				screen_timeout--;
				if(screen_timeout==0)
				{
					hide_menu();
				}
			}
		}
    }
}

#ifdef TEST_DISPLAY

void Display::test()
{
	printf("Display::test();\n");

	//for(int i=0;i<1000;i++) //~14 sec (14ms per update)
	//for(int i=0;i<72;i++)

	int loops = 0;

	Paint_NewImage(MainImage, SSD1306_HEIGHT, SSD1306_WIDTH, 90, BLACK);

	while(!exit_program)
	{
		/*
		for(int i=0;i<Imagesize/4;i++)
		{
			fill_with_random_value(TestImage+4*i);
		}

		for(int i=0;i<Imagesize;i++)
		{
			fill_with_random_value_s(&TestImage[i], 1);
		}
		*/

		for(int i=0;i<MainImagesize/2;i++)
		{
			fill_with_random_value_s(&MainImage[i*2], 2);
		}

		SSD1306_display(MainImage);

		usleep(40000); //25Hz

		loops++;

		if(loops%100==0)
		{
			printf("all LEDs on\n");
			for(int i=0;i<12;i++)
			{
				LEDs_x_set(i, 1);
			}

			memset(MainImage,0x00,MainImagesize);
			Paint_SelectImage(MainImage);
			Paint_DrawString_EN(0, 0, "ALL LEDs set to ON", &Font16, WHITE, WHITE);
			SSD1306_display(MainImage);
			sleep(1);
		}

		if(loops%100==50)
		{
			printf("all LEDs off\n");
			for(int i=0;i<12;i++)
			{
				LEDs_x_set(i, 0);
			}

			memset(MainImage,0x00,MainImagesize);
			Paint_SelectImage(MainImage);
			Paint_DrawString_EN(0, 0, "ALL LEDs set to OFF", &Font16, WHITE, WHITE);
			SSD1306_display(MainImage);
			sleep(1);
		}
	}

	SSD1306_clear();

	//memset(TestImage,0x0f,Imagesize);

	//memset(TestImage,0xaa,Imagesize/2);
	//memset(TestImage+Imagesize/2,0x55,Imagesize/2);

	memset(MainImage,0x00,MainImagesize);

	/*
	TestImage[0] = 0xff;
	TestImage[1] = 0xff;
	TestImage[2] = 0xff;
	TestImage[3] = 0xff;

	TestImage[Imagesize-1] = 0xff;
	TestImage[Imagesize-2] = 0xff;
	TestImage[Imagesize-3] = 0xff;
	TestImage[Imagesize-4] = 0xff;

	for(int i=2;i<126;i++)
	{
		TestImage[4*i] = 0x80;//0x81;
		TestImage[4*i+3] = 0x01;//0x81;
	}
	*/

	SSD1306_display(MainImage);

	printf("Paint_NewImage\r\n");

	Paint_NewImage(MainImage, SSD1306_HEIGHT, SSD1306_WIDTH, 90, BLACK);
	Paint_NewImage(ScrollingText, SSD1306_HEIGHT, SSD1306_WIDTH * OLED_SCROLL_BUFFER_WIDTH, 90, BLACK);

	Paint_SelectImage(MainImage);

	printf("Drawing:page 2\r\n");

	//Paint_DrawString_EN(10, 2, "SILURIA", &Font16, WHITE, WHITE);
	//Paint_DrawNum(10, 18, 123.456789, &Font12, 4, WHITE, WHITE);
	//Paint_DrawString_EN(10, 18, "by PhonicBloom", &Font12, WHITE, WHITE);

	Paint_DrawString_EN(0, 0, "SILURIA12345", &Font16, WHITE, WHITE);

	Paint_SelectImage(ScrollingText);
	//Paint_DrawString_EN(0, 18, "ABCDEFGH ijklmnop----qrst 12345678", &Font12, WHITE, WHITE);
	Paint_DrawString_EN(0, 0, "ABCDEFGH ijklmnop----qrst 1234567890 +++ --- *** /// %%% $$$ ^^^ &&& \"\"\" !!! 1 2 3 4 5 6 7 8 9 0 - - - + + + [end]", &Font12, WHITE, WHITE);
	Paint_DrawString_EN(0, 16, "IJKLMNOP abcdefgh----uvwx 9876543210 +++ --- *** /// %%% $$$ ^^^ &&& \"\"\" !!! 1 2 3 4 5 6 7 8 9 0 - - - + + + [end]", &Font12, WHITE, WHITE);

	Paint_SelectImage(MainImage);
	Paint_DrawString_EN(0, 18, "abc***", &Font12, WHITE, WHITE);

	SSD1306_display(MainImage);

	sleep(1);

	//SSD1306_display(ScrollingText);

	//test scroll
	for(int i=0;i<SSD1306_WIDTH*6;i++)
	{
		//copy_region(ScrollingText, TestImage, 0, SSD1306_WIDTH-1, 16, 31, 0, 16); //y coordinates need to be aligned to 8 (1 byte = 8 pixels column)
		//copy_region(ScrollingText, TestImage, 0, SSD1306_WIDTH-1, 16, 31, 0, 16); //y coordinates need to be aligned to 8 (1 byte = 8 pixels column)

		//copy_region(ScrollingText, TestImage, i, i+SSD1306_WIDTH-1, 0, 31, 0, 0); //y coordinates need to be aligned to 8 (1 byte = 8 pixels column)
		//SSD1306_display(TestImage);

		copy_region(ScrollingText, MainImage, i, i+SSD1306_WIDTH-1, 0, 15, 0, 16); //y coordinates need to be aligned to 8 (1 byte = 8 pixels column)
		SSD1306_display_half(MainImage, 1);

		usleep(20000);
	}
}

void Display::welcome()
{
	printf("Display::welcome();\n");

	//for(int i=0;i<1000;i++) //~14 sec (14ms per update)
	for(int i=0;i<72;i++) //1 sec
	{
		for(int i=0;i<MainImagesize/2;i++)
		{
			fill_with_random_value_s(&MainImage[i*2], 2);
		}

		SSD1306_display(MainImage);

		//usleep(40000); //25Hz
	}

	SSD1306_clear();
	memset(MainImage,0x00,MainImagesize);
	//SSD1306_display(TestImage);

	frame(MainImage, MainImagesize);

	Paint_NewImage(MainImage, SSD1306_HEIGHT, SSD1306_WIDTH, 90, BLACK);
	Paint_NewImage(ScrollingText, SSD1306_HEIGHT, SSD1306_WIDTH * OLED_SCROLL_BUFFER_WIDTH, 90, BLACK);

	Paint_SelectImage(MainImage);

	Paint_DrawString_EN(4, 2, "SILURIA", &Font16, WHITE, WHITE);
	Paint_DrawString_EN(5, 18, "by PhonicBloom", &Font12, WHITE, WHITE);

	SSD1306_display(MainImage);

	Paint_SelectImage(ScrollingText);
	//one display witdth is "01234567890123456" plus frames (18-19 chars)
	Paint_DrawString_EN(4,  2, "0123456789012345 We are creating innovative synthesizers. This is our latest creation Siluria. Brought to you by Phonicbloom", &Font12, WHITE, WHITE);
	//Paint_DrawString_EN(4, 18, "IJKLMNOP abcdefgh----uvwx 9876543210 +++ --- *** /// %%% $$$ ^^^ &&& \"\"\" !!! 1 2 3 4 5 6 7 8 9 0 - - - + + + [end]", &Font12, WHITE, WHITE);

	//Paint_SelectImage(TestImage);
	//Paint_DrawString_EN(0, 18, "abc***", &Font12, WHITE, WHITE);

	sleep(2);

	//SSD1306_display(ScrollingText);

	for(int i=0;i<SSD1306_WIDTH*6+1;i++)
	{
		//copy_region(ScrollingText, TestImage, 0, SSD1306_WIDTH-1, 16, 31, 0, 16); //y coordinates need to be aligned to 8 (1 byte = 8 pixels column)
		//copy_region(ScrollingText, TestImage, 0, SSD1306_WIDTH-1, 16, 31, 0, 16); //y coordinates need to be aligned to 8 (1 byte = 8 pixels column)

		//copy_region(ScrollingText, TestImage, i, i+SSD1306_WIDTH-1, 0, 31, 0, 0); //y coordinates need to be aligned to 8 (1 byte = 8 pixels column)
		//SSD1306_display(TestImage);

		copy_region(ScrollingText, MainImage, i, i+SSD1306_WIDTH-1, 0, 15, 0, 16); //y coordinates need to be aligned to 8 (1 byte = 8 pixels column)
		frame(MainImage, MainImagesize);
		SSD1306_display_half(MainImage, 1);

		usleep(20000);
	}

	frame(MainImage, MainImagesize);
	SSD1306_display(MainImage);
}

#endif //TEST_DISPLAY

//data format: one byte is 8 pixels column

void Display::copy_region(uint8_t *src, uint8_t *dst, int src_x0, int src_x1, int src_y0, int src_y1, int dst_x, int dst_y)
{
	//printf("copy_region(): src_x0=%d, src_x1=%d, src_y0=%d, src_y1=%d, dst_x=%d, dst_y=%d)\n", src_x0, src_x1, src_y0, src_y1, dst_x, dst_y);

	for(int x=0;x<src_x1-src_x0+1; x++)
	{
		for(int y=0; y<(src_y1-src_y0+1)/8; y++)
		{
			//printf("copying %d/%d -> %d/%d (0x%02x)\n", src_x0+x, src_y0+y*8, dst_x+x, dst_y+y*8, src[4*(src_x0+x)+src_y0/8+y]);
			dst[4*(dst_x+x)+3-(dst_y/8+y)] = src[4*(src_x0+x)+3-(src_y0/8+y)];
		}
	}

}

void Display::frame(uint8_t *Image, int image_size)
{
	Image[0] |= 0xff;
	Image[1] |= 0xff;
	Image[2] |= 0xff;
	Image[3] |= 0xff;

	Image[image_size-1] |= 0xff;
	Image[image_size-2] |= 0xff;
	Image[image_size-3] |= 0xff;
	Image[image_size-4] |= 0xff;

	for(int i=1;i<127;i++)
	{
		Image[4*i] |= 0x80;//0x81;
		Image[4*i+3] |= 0x01;//0x81;
	}
}

void Display::show_menu(char *text1, char *text2, int timeout)
{
	oled_mode = OLED_MODE_NORMAL;

	clear_main();

	Paint_SelectImage(MainImage);

	Paint_DrawString_EN(4, 2, text1, &Font16, WHITE, WHITE);
	Paint_DrawString_EN(4, 18, text2, &Font12, WHITE, WHITE);

	frame(MainImage, MainImagesize);
	SSD1306_display(MainImage);
	SSD1306_display(MainImage); //for higher display reliability

	if(timeout)
	{
		screen_timeout = timeout / 100;
	}
}

void Display::hide_menu()
{
	oled_mode = OLED_MODE_OFF;
	//oled_mode = OLED_MODE_NOISE; //test display reliability
}

void Display::scroll_message(char *text)
{
	clear_scroll();

	Paint_SelectImage(ScrollingText);
	Paint_DrawString_EN(5, 2, text, &Font12, WHITE, WHITE);

	scroll_delay = SCROLL_DELAY_DEFAULT;

	int text_length = strlen(text);

	if(text_length<=16) //initially 17 chars fit without scrolling
	{
		scroll_limit = 0;
	}
	else //at 145 chars the length should be SSD1306_WIDTH * OLED_SCROLL_BUFFER_WIDTH (128 * 8)
	{
		//scroll_limit = (((text_length+17) * SSD1306_WIDTH * OLED_SCROLL_BUFFER_WIDTH) / 162);
		scroll_limit = (((text_length+2) * SSD1306_WIDTH * OLED_SCROLL_BUFFER_WIDTH) / 147);
		#ifdef DEBUG_DISPLAY
		printf("Display::scroll_message(): text length = %d chars, scroll limit = %d\n", text_length, scroll_limit);
		#endif

		if(scroll_limit > SSD1306_WIDTH * OLED_SCROLL_BUFFER_WIDTH)
		{
			scroll_limit = SSD1306_WIDTH * OLED_SCROLL_BUFFER_WIDTH;
		}
	}

	oled_mode = OLED_MODE_SCROLLING;
}

#endif //USE_SSD1306
