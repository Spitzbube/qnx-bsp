
#include <stdint.h>
#include "arm/mb86h60.h"
#include "gpio.h"
#include "uart.h"

#define UART_BAUDRATE 115200

Uart_Module* uart_hw_block; //20408480 +0 /  / 234c05c4
unsigned int uartClockFrequency; //20408484 + 4
Struct_20611068* Data_2040848c; //2040848c
Struct_20611068* Data_20408490; //20408490
Uart_Module uart_h60[2] = //20408494
{
		{0, (void*)MB86H60_UART0_BASE},
		{0, (void*)MB86H60_UART1_BASE},
};
Uart_Module uart_h61[2] = //204084a4
{
		{0, (void*)0xcd000000},
		{0, (void*)0xce000000},
};


int uart_write_byte(Uart_Module* r0, unsigned char ch)
{
	Uart_Regs* regs;

	if (r0 == 0)
	{
		return 1;
	}

	regs = r0->regs;

	while ((regs->FREG_UART_FR & 0x28) != 0)
	{
		/* Txff + Busy */
	}

	regs->FREG_UART_DR = ch;

	return 0;
}


char uart_init(Uart_Init_Params* pParams, Uart_Module** ppModule)
{
	int i;
	uint8_t sp = 0;

	if (pParams->index > 2)
	{
		return 3;
	}

	if (uart_hw_block[pParams->index].inUse != 0)
	{
		sp = 4;
	}
	else
	{
		Uart_Regs* pRegs = uart_hw_block[pParams->index].regs;

		unsigned int r8 = uartClockFrequency / (UART_BAUDRATE * 16);
		unsigned int r0 = ((uartClockFrequency * 4) / UART_BAUDRATE);
		r0 = r0 - (r8 * 64);
		r0 = (5 + r0 * 10) / 10;

		pRegs->FREG_UART_IBRD = r8;
		pRegs->FREG_UART_FBRD = r0;
		pRegs->FREG_UART_LCRH = 0x70;
		pRegs->FREG_UART_CR = 0x301;

		if (pParams->txPin.bPin != 0xff)
		{
			gpio_open(&pParams->txPin, &Data_2040848c);
		}

		if (pParams->rxPin.bPin != 0xff)
		{
			gpio_open(&pParams->rxPin, &Data_20408490);
		}

		uart_hw_block[pParams->index].inUse = 1;

		*ppModule = &uart_hw_block[pParams->index];

		for (i = 0; i < 0xffff; i++)
		{
			/* wait */
		}

#if 0
		pRegs->UART_IMSC = (1 << 4); //RXIM
#endif

		for (i = 0; i < 0x10; i++)
		{
			/* wait */
			pRegs->FREG_UART_DR;
		}
	}

	return sp;
}


int uart_setup(void)
{
	unsigned char i;

	if (0 != sys_get_device_id())
	{
		uart_hw_block = uart_h60;
		uartClockFrequency = 81000000;
	}
	else
	{
		uart_hw_block = uart_h61;
		uartClockFrequency = 99000000;
	}

	for (i = 0; i < 2; i++)
	{
		uart_hw_block[i].inUse = 0;
		uart_hw_block[i].regs->FREG_UART_CR = 0;
	}

	return 0;
}

