/*
 * $QNXLicenseC:
 * Copyright 2008, QNX Software Systems. 
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"). You 
 * may not reproduce, modify or distribute this software except in 
 * compliance with the License. You may obtain a copy of the License 
 * at: http://www.apache.org/licenses/LICENSE-2.0 
 * 
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" basis, 
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied.
 *
 * This file may contain contributions from others, either as 
 * contributors under the License or as licensors under other terms.  
 * Please review this entire file for other proprietary rights or license 
 * notices, as well as the QNX Development Suite License Guide at 
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */



#include "startup.h"
#include "arm/mb86h60.h"

#include <stdint.h>
#include "gpio.h"
#include "uart.h"


static Uart_Module* hUart0 = 0;
Struct_20611068* main_hUsbGpio = 0;


void mb86h60_board_init(void)
{
	gpio_init();
	uart_setup();

	Uart_Init_Params uartParams;

	uartParams.index = 0;

	uartParams.rxPin.bPin = 32;
	uartParams.rxPin.dwOutFunction = 0xff;
	uartParams.rxPin.dwInFunction = 21; //GPIO_IN_UART0_DATA

	uartParams.txPin.bPin = 33;
	uartParams.txPin.dwOutFunction = 23; //GPIO_OUT_UART0_DATA
	uartParams.txPin.dwInFunction = 0xff;

	uart_init(&uartParams, &hUart0);

	GPIO_Params sp_0x10;

	sp_0x10.dwOutFunction = 0;
	sp_0x10.dwInFunction = 0xff;
	sp_0x10.bPin = 0x53;

	gpio_open(&sp_0x10, &main_hUsbGpio);

	gpio_set(main_hUsbGpio, 0);

}

void console_send_string(unsigned char* a)
{
	if (hUart0 == 0)
	{
		return;
	}

	while (1)
	{
		unsigned char ch = *a++;
		if (ch == 0) break;
		uart_write_byte(hUart0, ch);
		if (ch == 0x0a)
		{
			uart_write_byte(hUart0, 0x0d);
		}
	}
}


void console_send_number(uint32_t number, int numDigits)
{
	uint8_t buf[10]; //size???
	uint32_t i;

	for (i = 0; i < numDigits; i++)
	{
		uint8_t digit = number & 0x0f;
		if (digit > 9)
		{
			digit += '7';
		}
		else
		{
			digit += '0';
		}

		buf[numDigits - i - 1] = digit;
		number >>= 4;
	}

	buf[numDigits] = 0;
	console_send_string(buf);
}


/*
 * Syntax: base^shift.baud.clock
 * Where:
 *
 *	base  = physical address (0x90003400)
 *	shift = not used by driver code (typically set to 0)
 *	baud  = baud rate
 *	clock = clock frequency (typically 3686400, but may be different)
 */
static void
parse_line(unsigned channel, const char *line, unsigned *baud, unsigned *clk)
{
	/*
	 * Get device base address and register stride
	 */
	if (*line != '.' && *line != '\0') {
		//dbg_device[channel].base = strtoul(line, (char **)&line, 16);
		dbg_device[channel].base = MB86H60_UART0_BASE; //fix me hardcoded
		if (*line == '^')
			dbg_device[channel].shift = strtoul(line+1, (char **)&line, 0);
	}

	/*
	 * Get baud rate
	 */
	if (*line == '.')
		++line;
	if (*line != '.' && *line != '\0')
		*baud = strtoul(line, (char **)&line, 0);

	/*
	 * Get clock rate
	 */
	if (*line == '.')
		++line;
	if (*line != '.' && *line != '\0')
		*clk = strtoul(line, (char **)&line, 0);
}


/*
 * Initialise one of the serial ports
 * buad rates fixed to default- 115200
 * so options are disabled
 */
void
init_mb86h60_debug(unsigned channel, const char *init, const char *defaults)
{
	console_send_string("init_mb86h60_debug\n");

	unsigned	baud, clk, base;

	parse_line(channel, defaults, &baud, &clk);
	parse_line(channel, init, &baud, &clk);
	base = dbg_device[channel].base;
}

/*
 * Send a character
 */
void
put_mb86h60(int data)
{
	uart_write_byte(hUart0, data);
}

