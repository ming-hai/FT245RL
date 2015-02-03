/**
 * @file main.c
 *
 */
/* Copyright (C) 2015 by Arjan van Vught <pm @ http://forum.bitwizard.nl/>
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
#include <stdint.h>
#include <avr/io.h>

/*
 * Uno:328p			FT245RL
 *
 *  8:PB0	<--->	D0
 *  9:PB1	<--->	D1
 *  2:PD2	<--->	D2
 *  3:PD3	<--->	D3
 *  4:PD4	<--->	D4
 *  5:PD5	<--->	D5
 *  6:PD6	<--->	D6
 *  7:PD7	<--->	D7
 *
 * 12:PB4	---->	WR
 * 13:PB5	---->	RD#
 *
 * 10:PB2	<----	TXE#
 * 11:PB3	<----	RXF#
 */

static void data_io_ddr_output(void)
{
	uint8_t value = DDRD;
	value |= _BV(PORTD2) | _BV(PORTD3) | _BV(PORTD4) | _BV(PORTD5) | _BV(PORTD6) | _BV(PORTD7);
	DDRD = value;								// output D2, D3, D4, D5, D6, D7
	value = DDRB;
	value |= _BV(PORTB0) | _BV(PORTB1);
	DDRB = value;								// output D0, D1
}

static void data_io_ddr_input(void)
{
	uint8_t value = DDRD;
	value &= ~_BV(PORTD2) & ~_BV(PORTD3) & ~_BV(PORTD4) & ~_BV(PORTD5) & ~_BV(PORTD6) & ~_BV(PORTD7);
	DDRD = value;								// input D2, D3, D4, D5, D6, D7
	value = DDRB;
	value &= ~_BV(PORTB0) & ~_BV(PORTB1);
	DDRB = value;								// input D0, D1
}

void FT245RL_init(void)
{
	uint8_t value = DDRB;
	value &= ~_BV(PORTB2) & ~_BV(PORTB3);		// TXE#, RFX# input
	value |= _BV(PORTB4) | _BV(PORTB5);			// WR, RD# output
	DDRB = value;
	// RD#	high
	PORTB |= _BV(PORTB5);
	// WR	low
	PORTB &= ~_BV(PORTB4);
}

void FT245RL_write_data(uint8_t data)
{
	data_io_ddr_output();
	// Raise WR to start the write.
	PORTB |= _BV(PORTB4);
	asm volatile("nop"::);
	// Put the data on the bus.
	PORTB = _BV(PORTB4) | _BV(PORTB5) | (data & 0b00000011);// Keep WR high, and RD# high
	PORTD = data & ~0b00000011;
	asm volatile("nop"::);
	// Drop WR to tell the FT245 to read the data.
	PORTB &= ~_BV(PORTB4);
}

uint8_t FT245RL_read_data()
{
	data_io_ddr_input();
	PORTB &= ~_BV(PORTB5);
	// Wait for the FT245 to respond with data.
	asm volatile("nop"::);
	// Read the data from the data port.
	uint8_t data = (PINB & 0b00000011) | (PIND & ~0b00000011);
	// Bring RD# back up so the FT245 can let go of the data.
	PORTB |= _BV(PORTB5);
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
	return (!(PINB & _BV(PB3)));
}

int main(int argc, char *argv[])
{
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

	return 0;
}
