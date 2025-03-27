
#include <stdint.h>
#include "gpio.h"
#include "uart.h"


Uart_Module* consoleUart; //23492280 +0
char debug_string[500];


/* 2341af84 - complete */
void console_init(Uart_Module* pUart)
{
	consoleUart = pUart;
}


/* 2341af98 - complete */
void console_send_string(unsigned char* a)
{
	if (consoleUart == 0)
	{
		return;
	}

	while (1)
	{
		unsigned char ch = *a++;
		if (ch == 0) break;
		uart_write_byte(consoleUart, ch);
		if (ch == 0x0a)
		{
			uart_write_byte(consoleUart, 0x0d);
		}
	}
}


/* 2341aff0 / 234264f8 - complete */
void console_send_number(uint32_t number, int numDigits)
{
#if 0
	console_send_string("console_send_number (todo.c): TODO\r\n");
#endif

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


void console_write_byte(uint8_t a)
{
#if 0
	console_send_string("console_write_byte (todo.c): TODO\r\n");
#endif

	uart_write_byte(consoleUart, a);
}

