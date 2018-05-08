/*
Gameboy Advance serial I/O test
Copyright (C) 2012  John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* This is a small test program that recieves a byte at a time through
 * the Gameboy Advance serial port (SIO mode, which is basically SPI),
 * and displays the value numerically and as a "progress bar" on the
 * GBA framebuffer.
 * Depends on my gbasys library: https://nuclear.mutantstargoat.com/hg/gbasys
 *
 * The other side of the communication link for this test is an Atmel
 * AVR microcontroller reading a potentiometer and transmitting the
 * value using SPI.
 *
 * You can see this test running in this video:
 * http://www.youtube.com/watch?v=5VfVZL0CwAA
 */

#include "gbasys.h"

#define REG_SIOCNT		*((volatile unsigned short*)0x4000128)
#define REG_SIODATA8	*((volatile unsigned short*)0x400012a)
#define REG_RCNT		*((volatile unsigned short*)0x4000134)

/* REG_SIOCNT bits */
#define SIOCNT_CLK_INTERN		(1 << 0)
#define SIOCNT_CLK_2MHZ			(1 << 1)
#define SIOCNT_RECV_ENABLE		(1 << 2)
#define SIOCNT_SEND_ENABLE		(1 << 3)
#define SIOCNT_START			(1 << 7)
#define SIOCNT_XFER_LEN			(1 << 12)
#define SIOCNT_INT_ENABLE		(1 << 14)

/* only in 16bit multi player mode */
#define SIOCNT_BAUD_38400		1
#define SIOCNT_BAUD_57600		2
#define SIOCNT_BAUD_115200		3
#define SIOCNT_SI_STATUS		(1 << 2)
#define SIOCNT_SD_STATUS		(1 << 3)
#define SIOCNT_SLAVE1			(1 << 4)
#define SIOCNT_SLAVE2			(2 << 4)
#define SIOCNT_SLAVE3			(3 << 4)
#define SIOCNT_ERROR			(1 << 6)

/* REG_RCNT bits */
#define RCNT_GPIO_DATA_MASK		0x000f
#define RCNT_GPIO_DIR_MASK		0x00f0
#define RCNT_GPIO_INT_ENABLE	(1 << 8)
#define RCNT_MODE_GPIO			0x8000
#define RCNT_MODE_JOYBUS		0xc000

void init_sio(void);
void sio_read_async(void);
void com_intr(void);
void draw(void);

static int val;

int main(void)
{
	gba_init();
	set_video_mode(VMODE_LFB_240x160_16, 1);
	clear_buffer(back_buffer, RGB(24, 24, 24));

	set_font(&font_8x8);
	set_text_color(RGB(160, 255, 160), RGB(24, 24, 24));
	set_text_writebg(1);

	init_sio();

	draw_string("foobar", 0, 0, back_buffer);

	sio_read_async();

	for(;;) {
		draw();
	}
	return 0;
}

void init_sio(void)
{
	REG_RCNT = 0;	/* serial mode */


	mask(INTR_COMM);
	interrupt(INTR_COMM, com_intr);
	unmask(INTR_COMM);
	set_int();
}

void sio_read_async(void)
{
	/* clear the data register */
	REG_SIODATA8 = 0;

	/* XXX p.134 says to set d03 of SIOCNT ... */
	/* IE=1, external clock */
	REG_SIOCNT = SIOCNT_INT_ENABLE | SIOCNT_SEND_ENABLE;

	/* XXX then it says to set it 0 again... I don't get it */
	REG_SIOCNT &= ~SIOCNT_SEND_ENABLE;

	/* start transfer */
	REG_SIOCNT |= SIOCNT_START;
}

void com_intr(void)
{
	if(REG_SIOCNT & SIOCNT_START) {
		panic("interrupt with start bit == 1");
	}
	val = REG_SIODATA8 & 0xff;

	/* restart reading */
	sio_read_async();
}

void rect(int x, int y, int w, int h, unsigned short col)
{
	int i, j;
	unsigned short *ptr = (unsigned short*)back_buffer->pixels + y * 240 + x;

	for(i=0; i<h; i++) {
		for(j=0; j<w; j++) {
			ptr[j] = col;
		}
		ptr += 240;
	}
}

void draw(void)
{
	char buf[128];

	sprintf(buf, "value: %d   ", val);
	draw_string(buf, 0, 0, back_buffer);

	rect(18, 78, 204, 24, 0xffff);
	rect(20, 80, 200, 20, 0);
	rect(20, 80, 200 * val / 256, 20, RGB(255, 0, 0));

	flip();
}
