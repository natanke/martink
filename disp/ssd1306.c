/*******
 * I2C asynchronous driver for ssd1306 series 128x64 point displays
 *
 * License: GPLv3
 * Author: Martin K. Schröder
 * Email: info@fortmax.se
 * Website: http://oskit.se
 */

#ifdef __AVR__
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#else
#define PROGMEM
#define pgm_read_byte(a) (*a)
#endif

#include <stdlib.h>
#include <arch/soc.h>

#include "ssd1306.h"
#include "ssd1306_priv.h"

#include <string.h>

#include <stddef.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define ABS(x) ((x >= 0) ? x : -x)


#define SSD1306_SETCONTRAST 0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON 0xA5
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF

#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS 0xDA

#define SSD1306_SETVCOMDETECT 0xDB

#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9

#define SSD1306_SETMULTIPLEX 0xA8

#define SSD1306_SETLOWCOLUMN 0x00
#define SSD1306_SETHIGHCOLUMN 0x10
#define SSD1306_SETSTARTPAGE 0xb0
#define SSD1306_SETSTARTLINE 0x40

#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR   0x22

#define SSD1306_COMSCANINC 0xC0
#define SSD1306_COMSCANDEC 0xC8

#define SSD1306_SEGREMAP 0xA0

#define SSD1306_CHARGEPUMP 0x8D

#define SSD1306_EXTERNALVCC 0x1
#define SSD1306_SWITCHCAPVCC 0x2

// Scrolling #defines
#define SSD1306_ACTIVATE_SCROLL 0x2F
#define SSD1306_DEACTIVATE_SCROLL 0x2E
#define SSD1306_SET_VERTICAL_SCROLL_AREA 0xA3
#define SSD1306_RIGHT_HORIZONTAL_SCROLL 0x26
#define SSD1306_LEFT_HORIZONTAL_SCROLL 0x27
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL 0x2A

#define SCREEN_HEIGHT 64
#define SCREEN_WIDTH  128
#define FRAGMENT_WIDTH 128
#define FRAGMENT_HEIGHT 8

#define DISPLAY_ADDRESS 0x78

#define TEXT_W (21)
#define TEXT_H (8)
#define TEXTSZ (TEXT_W * TEXT_H)
#define TEXTPACKETSZ (8)
#define TEXTCHAR_W (5)
#define PACKETSZ (TEXTPACKETSZ * 6 + 1) // 8 chars * 6 bytes each + 1 command byte
#define debug(msg) {} //uart_printf("SSD1306: %s\n", msg);

void ssd1306_command(ssd1306_t *self, uint8_t cmd){
	uint8_t buf[3] = {DISPLAY_ADDRESS, 0, cmd};
	p_begin(self->port);
	p_write(self->port, buf, 3);
	p_sync(self->port);
	p_end(self->port);
	//twi0_start_transaction(TWI_OP_LIST(TWI_OP(DISPLAY_ADDRESS | I2C_WRITE, buf, 2))); 
	//twi0_wait(); 
	/*
	i2c_start_wait(DISPLAY_ADDRESS | I2C_WRITE);
	i2c_write(0x00);
	i2c_write(cmd); 
	i2c_stop();*/
}

void ssd1306_init(ssd1306_t *dev, struct packet_interface *port){
	dev->port = port;
	
	for(int c = 0; c < sizeof(cmd_init); c++){
		ssd1306_command(dev, pgm_read_byte(&cmd_init[c]));
		//twi0_start_transaction(TWI_OP_LIST(TWI_OP(DISPLAY_ADDRESS | I2C_WRITE, buf, 2))); 
		//twi0_wait(); 
		/*
		i2c_start_wait(DISPLAY_ADDRESS | I2C_WRITE);
			i2c_write(0x00);
			i2c_write(pgm_read_byte(&cmd_init[c])); 
		i2c_stop();*/
		_delay_us(10); 
		//ssd1306_command(pgm_read_byte(&cmd_init[c])); 
	}
}

void ssd1306_gotoChar(ssd1306_t *dev, uint8_t x, uint8_t y){
	uint8_t col = x * 6; 
	uint8_t row = y; 
	uint8_t home[] = {
		U8G_ESC_ADR(0),           // instruction mode 
		U8G_ESC_CS(1),             // enable chip 
		0x00 | (col & 0x0f),		// set lower 4 bit of the col adr to 0 
		0x10 | ((col >> 4) & 0x0f),		// set higher 4 bit of the col adr to 0 
		0xb0 | (row & 0x07),  	// page address
		
		0x021, col, 0x7f, // (7d for 125 col screen!) column addressing mode
		0x022, row, 0x07,		
	
		U8G_ESC_END,                // end of sequence 
	}; 
	for(int c = 0; c < sizeof(home); c++){
		ssd1306_command(dev, home[c]); 
	}
}

void ssd1306_clear(ssd1306_t *dev){
	//ssd1306_gotoChar(dev, 0, 0); 
	ssd1306_command(dev, SSD1306_COLUMNADDR);
  ssd1306_command(dev, 0);   // Column start address (0 = reset)
  ssd1306_command(dev, 127); // Column end address (127 = reset)

  ssd1306_command(dev, SSD1306_PAGEADDR);
  ssd1306_command(dev, 0); // Page start address (0 = reset)
  ssd1306_command(dev, (SSD1306_LCDHEIGHT == 64) ? 7 : 3); // Page end address
	
	/*
	i2c_start_wait(DISPLAY_ADDRESS | I2C_WRITE);
	i2c_write(0x40);
	for(int c = 0; c < (128 * 8); c++){
		i2c_write(0); 
	}
	i2c_stop();*/
}

/*
int16_t ssd1306_puts(ssd1306_t *dev, const char *str, uint8_t col){
	i2c_start_wait(DISPLAY_ADDRESS | I2C_WRITE);
	i2c_write(0x40);
	for(int c = 0; c < strlen(str); c++){
		uint16_t start = str[c] * 5; 
		for(int j = 0; j < 5; j++){
			uint8_t ch = pgm_read_byte(&font[start + j]); 
			if(!col) ch = ~ch; 
			ch &= ~0x80; 
			i2c_write(ch); 
		}
		i2c_write((col)?0:0x7f); // insert spacing
	}
	i2c_stop();
	return 1; 
}
*/
void ssd1306_set_region(ssd1306_t *dev, uint16_t x0, uint16_t y0, uint16_t w, uint16_t h){
	ssd1306_command(dev, SSD1306_SETLOWCOLUMN | (x0 & 0x0f)); 
	ssd1306_command(dev, SSD1306_SETHIGHCOLUMN | ((x0 >> 4) & 0x0f)); 
	ssd1306_command(dev, SSD1306_SETSTARTPAGE | ((y0 >> 3) & 0x07)); 
	
	ssd1306_command(dev, SSD1306_COLUMNADDR);
  ssd1306_command(dev, x0);   // Column start address (0 = reset)
  ssd1306_command(dev, x0 + w + 1); // Column end address (127 = reset)

  ssd1306_command(dev, SSD1306_PAGEADDR);
  ssd1306_command(dev, y0 >> 3); // Page start address (0 = reset)
  ssd1306_command(dev, ((y0 + h) >> 3) + 1); // Page end address
  
	/*
	uint8_t col = x0; 
	uint8_t row = (y0 >> 3); 
	uint8_t col_end = x0 + w; 
	if(col_end > 128) col_end = 128; 
	uint8_t row_end = (y0 + h) >> 3; 
	uint8_t home[] = {
		U8G_ESC_ADR(0),           // instruction mode 
		U8G_ESC_CS(1),             // enable chip 
		0x00 | (col & 0x0f),		// set lower 4 bit of the col adr to 0 
		0x10 | ((col >> 4) & 0x0f),		// set higher 4 bit of the col adr to 0 
		0xb0 | (row & 0x07),  	// page address
		
		0x021, col, col_end & 0x7f, // (7d for 125 col screen!) column addressing mode
		0x022, w, row_end & 0x07,		
		U8G_ESC_END,                // end of sequence 
	}; 
	for(int c = 0; c < sizeof(home); c++){
		ssd1306_command(home[c]); 
	}*/
}

void ssd1306_draw(ssd1306_t *dev, uint16_t x, uint16_t y, fb_image_t img){
	ssd1306_set_region(dev, x, y, img.w, img.h); 
	
	// writing needs to be done in columns of 8 pixels
	uint8_t byte = 0; 
	size_t idx = 0, total_size = (img.w * img.h) >> 3; 
	uint8_t buffer[8]; 
	
	for(int top = 0; top < img.h; top++){
		for(int left = 0; left < img.w; left++){
			for(int col = 0; (col < 8 && (left + col) < img.w); col++){
				for(int row = 0; (row < 8 && (top + row) < img.h); row++){
					if(img.data[left + col + ((top + row) * img.w) + 1] & _BV(7 - col)){
						byte |= _BV(7 - col); 
					} 
				}
				buffer[idx++] = byte;
				byte = 0; 
			}
			/*
			twi0_start_transaction(TWI_OP_LIST(
				TWI_OP(DISPLAY_ADDRESS | I2C_WRITE, buffer, idx)
			)); 
			twi0_wait(); */
			idx = 0; 
		}
	}
}

void ssd1306_fill(ssd1306_t *dev, fb_color_t col, size_t len){
	uint8_t color = 0; 
	uint8_t _buf[32] = {DISPLAY_ADDRESS};
	uint8_t *buf = &_buf[1]; 
	buf[0] = 0x40; 
	uint8_t bi = 1; 
	
	if(col.r || col.g || col.b || col.a) color = 1; else color = 0; 
	uint8_t byte = 0; 
	
	for(size_t c = 0; c < len; c++){
		if(bi == sizeof(_buf)){
			p_begin(dev->port);
			p_write(dev->port, _buf, bi);
			p_sync(dev->port);
			p_end(dev->port); 
			//twi0_start_transaction(TWI_OP_LIST(TWI_OP(DISPLAY_ADDRESS | I2C_WRITE, buf, bi))); 
			//twi0_wait(); 
			bi = 1; 
		} 
		
		byte |= color; 
		if((c & 0x07) == 0x07){
			buf[bi++] = (color)?0xf0:0; 
			/*i2c_start_wait(DISPLAY_ADDRESS | I2C_WRITE);
			i2c_write(0x40);
			i2c_write(byte);
			i2c_stop();*/
			byte = 0; 
		} else {
			byte <<= 1; 
		}
	}
	p_begin(dev->port);
	p_write(dev->port, _buf, bi);
	p_sync(dev->port);
	p_end(dev->port); 
	//twi0_start_transaction(TWI_OP_LIST(TWI_OP(DISPLAY_ADDRESS | I2C_WRITE, buf, bi))); 
	//twi0_wait(); 
}

int16_t ssd1306_printf(ssd1306_t *dev, uint8_t col, const char *fmt, ...){
	uint16_t n; 
	va_list vl; 
	va_start(vl, fmt);
	char buf[64]; 
	
	n = vsnprintf(buf, 64, fmt, vl);
	 
	va_end(vl);

	ssd1306_puts(dev, buf, col); 
	
	return n; 
}

