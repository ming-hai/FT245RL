/**
 * @file main.c
 *
 */
/* Copyright (C) 2015 by Arjan van Vught <pm @ http://www.raspberrypi.org/forum/>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// C library for Raspberry Pi (RPi)
// http://www.airspayce.com/mikem/bcm2835/
// After installing bcm2835, you can build this
// with something like:
// gcc -o main main.c -l bcm2835
// sudo ./main
//
// Or you can test it before installing with:
// gcc -o main -I ../../src ../../src/bcm2835.c main.c
// sudo ./main

#include <stdio.h>
#include <stdint.h>
#include "bcm2835.h"

#define GPFSEL0	*(bcm2835_gpio + 0)
#define GPFSEL1	*(bcm2835_gpio + 1)
#define GPFSEL2	*(bcm2835_gpio + 2)
#define GPSET0	*(bcm2835_gpio + 7)
#define GPCLR0	*(bcm2835_gpio + 10)
#define GPLEV0	*(bcm2835_gpio + 13)

// TODO Place the jumper on the left side for 3V3
/*
 * Raspberry Pi
 * P5:Broadcom			FT245RL
 *
 *  3:GPIO02	<--->	D0
 *  5:GPIO03	<--->	D1
 *  7:GPIO04	<--->	D2
 * 26:GPIO07	<--->	D3
 * 24:GPIO08	<--->	D4
 * 21:GPIO09	<--->	D5
 * 19:GPIO10	<--->	D6
 * 23:GPIO11	<--->	D7
 *
 * 15:GPIO22	---->	WR
 * 16:GPIO23	---->	RD#
 *
 * 18:GPIO24	<----	TXE#
 * 22:GPIO25	<----	RXF#
 */

#define WR	22
#define _RD	23

/**
 *
 */
static void data_gpio_fsel_output()
{
	// Data output
	uint32_t value = GPFSEL0;
	value &= ~(7 << 6);
	value |= BCM2835_GPIO_FSEL_OUTP << 6;
	value &= ~(7 << 9);
	value |= BCM2835_GPIO_FSEL_OUTP << 9;
	value &= ~(7 << 12);
	value |= BCM2835_GPIO_FSEL_OUTP << 12;
	value &= ~(7 << 21);
	value |= BCM2835_GPIO_FSEL_OUTP << 21;
	value &= ~(7 << 24);
	value |= BCM2835_GPIO_FSEL_OUTP << 24;
	value &= ~(7 << 27);
	value |= BCM2835_GPIO_FSEL_OUTP << 27;
	GPFSEL0 = value;
	value = GPFSEL1;
	value &= ~(7 << 0);
	value |= BCM2835_GPIO_FSEL_OUTP << 0;
	value &= ~(7 << 3);
	value |= BCM2835_GPIO_FSEL_OUTP << 3;
	GPFSEL1 = value;
}

/**
 *
 */
static void data_gpio_fsel_input()
{
	// Data input
	uint32_t value = GPFSEL0;
	value &= ~(7 << 6);
	value |= BCM2835_GPIO_FSEL_INPT << 6;
	value &= ~(7 << 9);
	value |= BCM2835_GPIO_FSEL_INPT << 9;
	value &= ~(7 << 12);
	value |= BCM2835_GPIO_FSEL_INPT << 12;
	value &= ~(7 << 21);
	value |= BCM2835_GPIO_FSEL_INPT << 21;
	value &= ~(7 << 24);
	value |= BCM2835_GPIO_FSEL_INPT << 24;
	value &= ~(7 << 27);
	value |= BCM2835_GPIO_FSEL_INPT << 27;
	GPFSEL0 = value;
	value = GPFSEL1;
	value &= ~(7 << 0);
	value |= BCM2835_GPIO_FSEL_INPT << 0;
	value &= ~(7 << 3);
	value |= BCM2835_GPIO_FSEL_INPT << 3;
	GPFSEL1 = value;
}

/**
 *
 */
void FT245RL_init(void)
{
	// RD#, WR output
	uint32_t value = GPFSEL2;
	value &= ~(7 << 6);
	value |= BCM2835_GPIO_FSEL_OUTP << 6;
	value &= ~(7 << 9);
	value |= BCM2835_GPIO_FSEL_OUTP << 9;
	// TXE#, RXF# input
	value &= ~(7 << 12);
	value |= BCM2835_GPIO_FSEL_INPT << 12;
	value &= ~(7 << 15);
	value |= BCM2835_GPIO_FSEL_INPT << 15;
	GPFSEL2 = value;
	// RD#	high
	bcm2835_gpio_set(_RD);
	// WR	low
	bcm2835_gpio_clr(WR);
}

/**
 *
 * @param data
 */
void FT245RL_write_data(uint8_t data)
{
	data_gpio_fsel_output();
	// Raise WR to start the write.
	bcm2835_gpio_set(WR);
	asm volatile("nop"::);
	// Put the data on the bus.
	uint32_t out_gpio = ((data & ~0b00000111) << 4)	| ((data & 0b00000111) << 2);
	GPSET0 = out_gpio;
	GPCLR0 = out_gpio ^ 0b111110011100;
	asm volatile("nop"::);
	// Drop WR to tell the FT245 to read the data.
	bcm2835_gpio_clr(WR);
}

/**
 *
 * @return
 */
uint8_t FT245RL_read_data()
{
	data_gpio_fsel_input();
	bcm2835_gpio_clr(_RD);
	// Wait for the FT245 to respond with data.
	asm volatile("nop"::);
	// Read the data from the data port.
	uint32_t in_gpio = (GPLEV0 & 0b111110011100) >> 2;
	uint8_t data = (uint8_t) ((in_gpio >> 2) & 0xF8) | (uint8_t) (in_gpio & 0x0F);
	// Bring RD# back up so the FT245 can let go of the data.
	bcm2835_gpio_set(_RD);
	// Wait to prevent false 'no data' readings.
	asm volatile("nop"::);
	return data;
}

/**
 * Read RXF#
 *
 * @return
 */
uint8_t FT245RL_data_available(void)
{
	return (!(GPLEV0 & (1 << 25)));
}

/**
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[])
{
	bcm2835_init();

	FT245RL_init();

	while (1)
	{
		if (FT245RL_data_available())
		{
			uint8_t data = FT245RL_read_data();
			if (data == '\r')
				FT245RL_write_data('\n');
			FT245RL_write_data(data);
		}
	}

	bcm2835_close();

	return 0;
}
