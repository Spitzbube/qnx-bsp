/*
 *  Copyright (c) Texas Instruments Incorporated 2018-2021
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/siginfo.h>
#include <sys/neutrino.h>
#include <hw/inout.h>
#include <sys/types.h>
#include <sys/neutrino.h>
#include <sys/trace.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <string.h>

#define GPIOPID               0x000
#define GPIO_PCR              0x004
#define GPIO_BINTEN           0x008

#define GPIO_DIR01            0x010
#define GPIO_OUT_DATA01       0x014
#define GPIO_SET_DATA01       0x018
#define GPIO_CLR_DATA01       0x01C
#define GPIO_IN_DATA01        0x020
#define GPIO_SET_RIS_TRIG01   0x024
#define GPIO_CLR_RIS_TRIG01   0x028
#define GPIO_SET_FAL_TRIG01   0x02C
#define GPIO_CLR_FAL_TRIG01   0x030
#define GPIO_INTSTAT01        0x034

#define GPIO_DIR23            0x038
#define GPIO_OUT_DATA23       0x03C
#define GPIO_SET_DATA23       0x040
#define GPIO_CLR_DATA23       0x044
#define GPIO_IN_DATA23        0x048
#define GPIO_SET_RIS_TRIG23   0x04c
#define GPIO_CLR_RIS_TRIG23   0x050
#define GPIO_SET_FAL_TRIG23   0x054
#define GPIO_CLR_FAL_TRIG23   0x058
#define GPIO_INTSTAT23        0x05C

#define GPIO_DIR45            0x060
#define GPIO_OUT_DATA45       0x064
#define GPIO_SET_DATA45       0x068
#define GPIO_CLR_DATA45       0x06C
#define GPIO_IN_DATA45        0x070
#define GPIO_SET_RIS_TRIG45   0x074
#define GPIO_CLR_RIS_TRIG45   0x078
#define GPIO_SET_FAL_TRIG45   0x07C
#define GPIO_CLR_FAL_TRIG45   0x080
#define GPIO_INTSTAT45        0x084

#define GPIO_DIR67            0x088
#define GPIO_OUT_DATA67       0x08C
#define GPIO_SET_DATA67       0x090
#define GPIO_CLR_DATA67       0x094
#define GPIO_IN_DATA67        0x098
#define GPIO_SET_RIS_TRIG67   0x09C
#define GPIO_CLR_RIS_TRIG67   0x0A0
#define GPIO_SET_FAL_TRIG67   0x0A4
#define GPIO_CLR_FAL_TRIG67   0x0A8
#define GPIO_INTSTAT67        0x0AC

#define GPIO_DIR8            0x0B0
#define GPIO_OUT_DATA8       0x0B4
#define GPIO_SET_DATA8       0x0B8
#define GPIO_CLR_DATA8       0x0BC
#define GPIO_IN_DATA8        0x0C0
#define GPIO_SET_RIS_TRIG8   0x0C4
#define GPIO_CLR_RIS_TRIG8   0x0C8
#define GPIO_SET_FAL_TRIG8   0x0CC
#define GPIO_CLR_FAL_TRIG8   0x0D0
#define GPIO_INTSTAT8        0x0D4

/* Base address and size, for the 8 GPIO modules */
#define GPIO0_BASE 0x00600000
#define GPIO1_BASE 0x00601000
#define GPIO2_BASE 0x00610000
#define GPIO3_BASE 0x00611000
#define GPIO4_BASE 0x00620000
#define GPIO5_BASE 0x00621000
#define GPIO6_BASE 0x00630000
#define GPIO7_BASE 0x00631000 // TRM says 0x68000000, but reading version reg seems like 0x00631000 is right

/* GPIO MUX output port to GIC IRQ value
 * as defined in TRM */
#define GPIOMUX_INTRTR0_OUTP_8 392

/* Read / Write register macros */
#define INREG32(x) in32(x)
#define OUTREG32(x, y) out32(x, y)

/* Forward Declarations */
void dumpGpioRegs(int base_address);
void parseCmdLine(int argc, char *argv[]);

/* Command line arguments */
char cmdBuf[128];
uint32_t gpio_module = 0;
uint32_t gpio_num = 0;
uint32_t gpio_bank = 0;
uint32_t runCmd = 0;
uint32_t attachToInterrupt = 0;
uint32_t dumpGpio = 0;
uint32_t gpioHigh = 0;
uint32_t gpioLow = 0;
uint32_t gpioInput = 0;
uint32_t gpioOutput = 0;
uint32_t set_rise = 0;
uint32_t detect_rise_val = 0;
uint32_t set_fall = 0;
uint32_t detect_fall_val = 0;
uint32_t interrupt = 0;
uint32_t mux_val = 0;

/* Event from ISR */
struct sigevent event;

/* Counter for interrupt handler */
uint32_t irqCount = 0;

/* GPIO Memory mapping variables */
/* GPIO Module size is 256 Bytes */
uint32_t GPIO_SIZE = 0x100;
uintptr_t BASE_ADDR = 0;

/* GPIOMUX_INTRTR0_MUXCNTL_n */
uint64_t GPIO_MUX_BASE_PHY_ADDR = 0x00a00024;
uint32_t GPIO_MUX_BASE_SIZE = 2048;
uintptr_t GPIO_MUX_BASE_ADDR = 0;

/* GIC_ICFGRn Interrupt Configuration Register */
uint64_t GIC_ICFG_BASE_PHY_ADDR = 0x01800c00;
uint32_t GIC_ICFG_SIZE = 768;
uintptr_t GIC_ICFG_BASE_ADDR = 0;

/*
 * Print usage information
 */
void printUsage(void)
{
	printf("Type 'use gpio', for usage information.\n");
	exit(0);
}

/*
 * Main loop
 */
int main(int argc, char **argv)
{
	uint32_t id;
	uint64_t base_address = 0;
	uint32_t current_value = 0;

	/* Parse command line arguments */
	parseCmdLine(argc, argv);

	if (gpioHigh && gpioLow)
	{
		printf("GPIO cannot be high and low at same time\n");
		printUsage();
		exit(1);
	}
	if ((gpioOutput && gpioInput))
	{
		printf("GPIO cannot be an input and an output\n");
		printUsage();
		exit(1);
	}

	// 2 Banks of 16 pins per GPIO instance
	gpio_bank = gpio_num / 16;

	// GPIO 0,2,4,6 have 8 banks per instance.
	if ((gpio_module == 0) || (gpio_module == 2) || (gpio_module == 4)
			|| (gpio_module == 6))
	{
		mux_val = 256 + (gpio_module / 2 * 8) + gpio_num / 16;
	}
	else // GPIO 1,3,5,7 only have 3 banks
	{
		mux_val = 288 + (gpio_module / 2 * 3) + gpio_num / 16;
	}
	printf("GPIO%d_%d, module/%d inst/%d bank/%d mux_val/%d\n", gpio_module,
			gpio_num, gpio_module, gpio_num, gpio_bank, mux_val);

	/* Set base address of gpio module*/
	switch (gpio_module)
	{
	case 0:
		base_address = GPIO0_BASE;
		break;
	case 1:
		base_address = GPIO1_BASE;
		break;
	case 2:
		base_address = GPIO2_BASE;
		break;
	case 3:
		base_address = GPIO3_BASE;
		break;
	case 4:
		base_address = GPIO4_BASE;
		break;
	case 5:
		base_address = GPIO5_BASE;
		break;
	case 6:
		base_address = GPIO6_BASE;
		break;
	case 7:
		base_address = GPIO7_BASE;
		break;
	default:
		printUsage();
		exit(1);
	}

	/* Get I/O privelege */
	if(ThreadCtl(_NTO_TCTL_IO, 0) == -1)
	{
            perror("ThreadCtl() failed: ");
	    exit(-1);
	}

	SIGEV_INTR_INIT(&event);

	/* Virtual Address for base address of GPIO Modules */
	BASE_ADDR = mmap_device_io(GPIO_SIZE, base_address);
	if (BASE_ADDR == 0)
	{
		printf("mmap_device_io failed for GPIO module\n");
		exit(-1);
	}

	/* Virtual Address for base address of GPIO Mux */
	GPIO_MUX_BASE_ADDR = mmap_device_io(GPIO_MUX_BASE_SIZE,
			GPIO_MUX_BASE_PHY_ADDR);
	if (GPIO_MUX_BASE_ADDR == 0)
	{
		printf("mmap_device_io failed for GPIO Mux\n");
		exit(-1);
	}

	/* Virtual Address GIC Interrupt configuration register */
	GIC_ICFG_BASE_ADDR = mmap_device_io(GIC_ICFG_SIZE, GIC_ICFG_BASE_PHY_ADDR);
	if (GIC_ICFG_BASE_ADDR == 0)
	{
		printf("mmap_device_io failed for GIC IFCG Register\n");
		exit(-1);
	}

	/* Optionally dump GPIO settings */
	if (dumpGpio)
	{
		printf("Dumping Module/%d to see gpio%d_%d\n", gpio_module, gpio_module,
				gpio_num);

		dumpGpioRegs(base_address);

		exit(0);
	}

	if (attachToInterrupt)
	{
		//
		// Main domain GPIO Module 0,2,4,6 have 8 banks each,
		// interrupts are mapped as follows:
		//
		//   GPIO0 Bank0:7  GPIOMUX_INTRTR0 256:263
		//   GPIO2 Bank0:7  GPIOMUX_INTRTR0 264:271
		//   GPIO4 Bank0:7  GPIOMUX_INTRTR0 272:279
		//   GPIO6 Bank0:7  GPIOMUX_INTRTR0 280:287
		//
		// When an individual gpio pin changes values, it will trigger the bank to raise an interrupt
		// The GPIO bank will trigger the GPIOMux_INTRR0, with the above input values.
		//
		// For each bank input, the output instance must be programmed which is the value used for the GIC
		//
		// GPIOMUX_INTRTR0_MUXCNTL_n, where 'n' is the INTRTR0_OUT 8:63 ... IRQ 392:447, where
		// 392 is defined as GPIOMUX_INTRTR0_OUTP_8, in the TRM.
		//
		// The instance of the Mux register 'n' defines the output port, hence the IRQ.
		//
		// See TRM for further information
		//
		int mux_offset = (interrupt - GPIOMUX_INTRTR0_OUTP_8) * 4;

		// Disable Mux
		current_value = in32(GPIO_MUX_BASE_ADDR + mux_offset);
		printf("setting 0x%08lx to 0x%08x\n",
				GPIO_MUX_BASE_PHY_ADDR + mux_offset,
				current_value & ~(1 << 16));
		out32(GPIO_MUX_BASE_ADDR + mux_offset, current_value & ~(1 << 16));

		// Map input bank identifier to output interrupt
		printf("setting 0x%08lx to 0x%08x\n",
				GPIO_MUX_BASE_PHY_ADDR + mux_offset, mux_val);
		out32(GPIO_MUX_BASE_ADDR + mux_offset, mux_val);

		// Set GIC to be edge triggered
		// NOTE: Setting of GIC at should be done using
		// gic_v3_set_intr_trig_mode(), funnction defined in the QNX BSP.
		// This code is included here, only to hi-lite that the GIC needs
		// to be configured, and to allow the interrupt portion of this
		// demo applicaiton to function.
		uint32_t gic_offset = (interrupt / 16) * 4;
		current_value = in32(GIC_ICFG_BASE_ADDR + gic_offset);
		printf("setting 0x%08lx to 0x%08x\n",
				GIC_ICFG_BASE_PHY_ADDR + gic_offset,
				current_value | (0x2 << (interrupt % 16) * 2));
		out32(GIC_ICFG_BASE_ADDR + gic_offset,
				current_value | (0x2 << (interrupt % 16) * 2));

		// Enable Mux
		current_value = in32(GPIO_MUX_BASE_ADDR + mux_offset);
		printf("setting 0x%08lx to 0x%08x\n",
				GPIO_MUX_BASE_PHY_ADDR + mux_offset, current_value | (1 << 16));
		out32(GPIO_MUX_BASE_ADDR + mux_offset, current_value | (1 << 16));

		printf("Attaching to gpio%d_%d gpio_bank/%d interrupt/%d mux_val/%d\n",
				gpio_module, gpio_num, gpio_bank, interrupt, mux_val);
	}

	/* Set GPIO as input */
	if (gpioInput)
	{
		printf("Setting to input\n");

		/* Set GPIO_DIR01 to 1 to input */
		current_value = INREG32(
				BASE_ADDR + GPIO_DIR01 + ((gpio_bank/2) * 0x28));
		printf("setting 0x%lx to 0x%x\n",
				base_address + GPIO_DIR01 + ((gpio_bank / 2) * 0x28),
				current_value | (1 << (gpio_num)));
		OUTREG32((BASE_ADDR + GPIO_DIR01 + ((gpio_bank/2) * 0x28)),
				current_value | (1 << (gpio_num)));

		// Dump
		dumpGpioRegs(base_address);
	}

	/* Set GPIO as output */
	if (gpioOutput)
	{
		printf("Setting to output\n");

		/* Set GPIO_DIR01 to 1 to output */
		current_value = INREG32(
				BASE_ADDR + GPIO_DIR01 + ((gpio_bank/2) * 0x28));
		printf("setting 0x%lx to 0x%x\n",
				base_address + GPIO_DIR01 + ((gpio_bank / 2) * 0x28),
				current_value & ~(1 << (gpio_num)));
		OUTREG32((BASE_ADDR + GPIO_DIR01 + ((gpio_bank/2) * 0x28)),
				current_value & ~(1 << (gpio_num)));

		// Dump
		dumpGpioRegs(base_address);
	}

	/* Set GPIO high or Low ... as we're setting the value, also set to direction to output */
	if ((gpioHigh) || (gpioLow))
	{

		/* Set GPIO_SETDATAOUT to 1 */
		if (gpioHigh)
		{
			current_value = INREG32(
					BASE_ADDR + GPIO_SET_DATA01 + ((gpio_bank/2) * 0x28));
			printf("setting 0x%lx to 0x%x\n",
					base_address + GPIO_SET_DATA01 + ((gpio_bank / 2) * 0x28),
					(current_value | (1 << gpio_num)));
			OUTREG32(BASE_ADDR + GPIO_SET_DATA01 + ((gpio_bank/2) * 0x28),
					(current_value | (1 << gpio_num)));
			printf("GPIO should now be high\n");
		}

		/* Set GPIO_CLEARDATAOUT to 1 (ie set output to 0) */
		else
		{
			current_value = INREG32(
					BASE_ADDR + GPIO_CLR_DATA01 + ((gpio_bank/2) * 0x28));
			printf("setting 0x%lx to 0x%x\n",
					base_address + GPIO_CLR_DATA01 + ((gpio_bank / 2) * 0x28),
					current_value & ~(1 << (gpio_num)));
			OUTREG32(BASE_ADDR + GPIO_CLR_DATA01 + ((gpio_bank/2) * 0x28),
					current_value | (1 << (gpio_num)));
			printf("GPIO should now be low\n");
		}
		// Dump
		dumpGpioRegs(base_address);
		exit(0);
	}

	/* Set or Clear Rise Detection */
	if (set_rise)
	{
		printf("Setting/Clearing to Rise Detect to %d\n", detect_rise_val);

		if (detect_rise_val == 1)
		{
			/* Set GPIO_SET_RIS_TRIG01 to 1 to input */
			current_value = INREG32(
					BASE_ADDR + GPIO_SET_RIS_TRIG01 + ((gpio_bank/2) * 0x28));
			printf("setting 0x%lx to 0x%x\n",
					base_address + GPIO_SET_RIS_TRIG01
							+ ((gpio_bank / 2) * 0x28), (1 << (gpio_num)));
			OUTREG32((BASE_ADDR + GPIO_SET_RIS_TRIG01 + ((gpio_bank/2) * 0x28)),
					(1 << (gpio_num)));
		}
		else
		{
			/* Set GPIO_CLR_RIS_TRIG01 to 1 to input */
			current_value = INREG32(
					BASE_ADDR + GPIO_CLR_RIS_TRIG01 + ((gpio_bank/2) * 0x28));
			printf("setting 0x%lx to 0x%x\n",
					base_address + GPIO_CLR_RIS_TRIG01
							+ ((gpio_bank / 2) * 0x28), (1 << (gpio_num)));
			OUTREG32((BASE_ADDR + GPIO_CLR_RIS_TRIG01 + ((gpio_bank/2) * 0x28)),
					(1 << (gpio_num)));
		}
		// Dump
		dumpGpioRegs(base_address);

	}

	/* Set or Clear Fall Detection */
	if (set_fall)
	{
		printf("Setting/Clearing to Fall Detect to %d\n", detect_fall_val);

		if (detect_fall_val == 1)
		{
			/* Set GPIO_SET_FAL_TRIG01 to 1 to input */
			current_value = INREG32(
					BASE_ADDR + GPIO_SET_FAL_TRIG01 + ((gpio_bank/2) * 0x28));
			printf("setting 0x%lx to 0x%x\n",
					base_address + GPIO_SET_FAL_TRIG01
							+ ((gpio_bank / 2) * 0x28), (1 << (gpio_num)));
			OUTREG32((BASE_ADDR + GPIO_SET_FAL_TRIG01 + ((gpio_bank/2) * 0x28)),
					(1 << (gpio_num)));
		}
		else
		{
			/* Set GPIO_CLR_FAL_TRIG01 to 1 to input */
			current_value = INREG32(
					BASE_ADDR + GPIO_CLR_FAL_TRIG01 + ((gpio_bank/2) * 0x28));
		        printf("setting 0x%lx to 0x%x\n",
					base_address + GPIO_CLR_FAL_TRIG01
							+ ((gpio_bank / 2) * 0x28), (1 << (gpio_num)));
			OUTREG32((BASE_ADDR + GPIO_CLR_FAL_TRIG01 + ((gpio_bank/2) * 0x28)),
					(1 << (gpio_num)));
		}

		// Dump
		dumpGpioRegs(base_address);
	}

	if (attachToInterrupt)
	{
		/* Set GPIO_BINTEN to 1 to input for the appropriate bank */
		current_value = INREG32(BASE_ADDR + GPIO_BINTEN);
		printf("setting 0x%lx to 0x%x\n", base_address + GPIO_BINTEN,
				current_value | (1 << (gpio_bank)));
		OUTREG32(BASE_ADDR + GPIO_BINTEN, current_value | (1 << (gpio_bank)));
	}

	/*
	 * Attach interrupt handler (thread level)
	 */
	if (attachToInterrupt)
	{
		id = InterruptAttachEvent(interrupt, &event, _NTO_INTR_FLAGS_TRK_MSK);
		for (;;)
		{
			InterruptWait(0, NULL);

			/* Run the optional command line argument, or add a
			 * functional call here, running at thread priority */
			if (runCmd)
			{
				system(cmdBuf);
			}

			/* Dump number of times invoked if desired */
			printf("Thread Level Interrupt handler Called %d times.\n",
					++irqCount);

			/* Unmask interrupt */
			InterruptUnmask(interrupt, id);

			/* Clear the interrupt status */
			printf("setting 0x%lx to 0x%x\n",
					base_address + GPIO_INTSTAT01 + ((gpio_bank / 2) * 0x28),
					(1 << (gpio_num)));
			OUTREG32(BASE_ADDR + GPIO_INTSTAT01 + ((gpio_bank/2) * 0x28),
					(1 << (gpio_num)));
		}
	}

	return EXIT_SUCCESS;
}

/*
 * Parse command line options
 */
void parseCmdLine(int argc, char *argv[])
{
	int c;
	char *strPtr;

	if (argc < 3)
	{
		printf("Invalid arguments, see usage\n");
		exit(0);
	}

	while ((c = getopt(argc, argv, "m:n:a:c:dhlior:f:")) != -1)
	{
		switch (c)
		{
		case 'm':
			gpio_module = atol(optarg);
			break;
		case 'n':
			gpio_num = atol(optarg);
			break;
		case 'a':
			attachToInterrupt = 1;
			interrupt = atol(optarg);
			break;
		case 'd':
			dumpGpio = 1;
			break;
		case 'h':
			gpioHigh = 1;
			break;
		case 'l':
			gpioLow = 1;
			break;
		case 'c':
			runCmd = 1;
			strPtr = strtok(optarg, "'");
			strcpy(cmdBuf, strPtr);
			break;
		case 'i':
			gpioInput = 1;
			break;
		case 'o':
			gpioOutput = 1;
			break;
		case 'r':
			set_rise = 1;
			detect_rise_val = atol(optarg);
			break;
		case 'f':
			set_fall = 1;
			detect_fall_val = atol(optarg);
			break;
		case '?':
			printUsage();
			break;
		default:
			printf("Unknown option\n");
			printUsage();
		}
	}
}

/*
 * Dump the GPIO registers
 */
void dumpGpioRegs(int base_address)
{

	/* GPIO Interrupt Mux */
	/* Need to search Mux's for input port */
	int mux_offset = 0;

	for (int i = 0; i < 32; i++)
	{
		mux_offset = i * 4;
		if ((INREG32(GPIO_MUX_BASE_ADDR + mux_offset) & 0x1FF) == mux_val)
		{
			printf(
					" GPIOMUX_INTRTR0_MUXCNTL_%d               0x%08lx = 0x%08x\n",
					8 + gpio_bank, GPIO_MUX_BASE_PHY_ADDR + mux_offset,
					INREG32(GPIO_MUX_BASE_ADDR + mux_offset));
		}
	}

	/* Registers common to all Banks */
	printf(" GPIOPID               0x%08x = 0x%08x\n", base_address + GPIOPID,
			INREG32(BASE_ADDR + GPIOPID));
	printf(" GPIO_PCR              0x%08x = 0x%08x\n", base_address + GPIO_PCR,
			INREG32(BASE_ADDR + GPIO_PCR));
	printf(" GPIO_BINTEN           0x%08x = 0x%08x\n",
			base_address + GPIO_BINTEN, INREG32(BASE_ADDR + GPIO_BINTEN));

	/* Per bank registers */
	if ((gpio_bank == 0) || (gpio_bank == 1))
	{
		printf(" GPIO_DIR01            0x%08x = 0x%08x\n",
				base_address + GPIO_DIR01, INREG32(BASE_ADDR + GPIO_DIR01));
		printf(" GPIO_OUT_DATA01       0x%08x = 0x%08x\n",
				base_address + GPIO_OUT_DATA01,
				INREG32(BASE_ADDR + GPIO_OUT_DATA01));
		printf(" GPIO_SET_DATA01       0x%08x = 0x%08x\n",
				base_address + GPIO_SET_DATA01,
				INREG32(BASE_ADDR + GPIO_SET_DATA01));
		printf(" GPIO_CLR_DATA01       0x%08x = 0x%08x\n",
				base_address + GPIO_CLR_DATA01,
				INREG32(BASE_ADDR + GPIO_CLR_DATA01));
		printf(" GPIO_IN_DATA01        0x%08x = 0x%08x\n",
				base_address + GPIO_IN_DATA01,
				INREG32(BASE_ADDR + GPIO_IN_DATA01));
		printf(" GPIO_SET_RIS_TRIG01   0x%08x = 0x%08x\n",
				base_address + GPIO_SET_RIS_TRIG01,
				INREG32(BASE_ADDR + GPIO_SET_RIS_TRIG01));
		printf(" GPIO_CLR_RIS_TRIG01   0x%08x = 0x%08x\n",
				base_address + GPIO_CLR_RIS_TRIG01,
				INREG32(BASE_ADDR + GPIO_CLR_RIS_TRIG01));
		printf(" GPIO_SET_FAL_TRIG01   0x%08x = 0x%08x\n",
				base_address + GPIO_SET_FAL_TRIG01,
				INREG32(BASE_ADDR + GPIO_SET_FAL_TRIG01));
		printf(" GPIO_CLR_FAL_TRIG01   0x%08x = 0x%08x\n",
				base_address + GPIO_CLR_FAL_TRIG01,
				INREG32(BASE_ADDR + GPIO_CLR_FAL_TRIG01));
		printf(" GPIO_INTSTAT01        0x%08x = 0x%08x\n",
				base_address + GPIO_INTSTAT01,
				INREG32(BASE_ADDR + GPIO_INTSTAT01));
	}
	if ((gpio_bank == 2) || (gpio_bank == 3))
	{
		printf(" GPIO_DIR23            0x%08x = 0x%08x\n",
				base_address + GPIO_DIR23, INREG32(BASE_ADDR + GPIO_DIR23));
		printf(" GPIO_OUT_DATA23       0x%08x = 0x%08x\n",
				base_address + GPIO_OUT_DATA23,
				INREG32(BASE_ADDR + GPIO_OUT_DATA23));
		printf(" GPIO_SET_DATA23       0x%08x = 0x%08x\n",
				base_address + GPIO_SET_DATA23,
				INREG32(BASE_ADDR + GPIO_SET_DATA23));
		printf(" GPIO_CLR_DATA23       0x%08x = 0x%08x\n",
				base_address + GPIO_CLR_DATA23,
				INREG32(BASE_ADDR + GPIO_CLR_DATA23));
		printf(" GPIO_IN_DATA23        0x%08x = 0x%08x\n",
				base_address + GPIO_IN_DATA23,
				INREG32(BASE_ADDR + GPIO_IN_DATA23));
		printf(" GPIO_SET_RIS_TRIG23   0x%08x = 0x%08x\n",
				base_address + GPIO_SET_RIS_TRIG23,
				INREG32(BASE_ADDR + GPIO_SET_RIS_TRIG23));
		printf(" GPIO_CLR_RIS_TRIG23   0x%08x = 0x%08x\n",
				base_address + GPIO_CLR_RIS_TRIG23,
				INREG32(BASE_ADDR + GPIO_CLR_RIS_TRIG23));
		printf(" GPIO_SET_FAL_TRIG23   0x%08x = 0x%08x\n",
				base_address + GPIO_SET_FAL_TRIG23,
				INREG32(BASE_ADDR + GPIO_SET_FAL_TRIG23));
		printf(" GPIO_CLR_FAL_TRIG23   0x%08x = 0x%08x\n",
				base_address + GPIO_CLR_FAL_TRIG23,
				INREG32(BASE_ADDR + GPIO_CLR_FAL_TRIG23));
		printf(" GPIO_INTSTAT23        0x%08x = 0x%08x\n",
				base_address + GPIO_INTSTAT23,
				INREG32(BASE_ADDR + GPIO_INTSTAT23));
	}
	if ((gpio_bank == 4) || (gpio_bank == 5))
	{
		printf(" GPIO_DIR45            0x%08x = 0x%08x\n",
				base_address + GPIO_DIR45, INREG32(BASE_ADDR + GPIO_DIR45));
		printf(" GPIO_OUT_DATA45       0x%08x = 0x%08x\n",
				base_address + GPIO_OUT_DATA45,
				INREG32(BASE_ADDR + GPIO_OUT_DATA45));
		printf(" GPIO_SET_DATA45       0x%08x = 0x%08x\n",
				base_address + GPIO_SET_DATA45,
				INREG32(BASE_ADDR + GPIO_SET_DATA45));
		printf(" GPIO_CLR_DATA45       0x%08x = 0x%08x\n",
				base_address + GPIO_CLR_DATA45,
				INREG32(BASE_ADDR + GPIO_CLR_DATA45));
		printf(" GPIO_IN_DATA45        0x%08x = 0x%08x\n",
				base_address + GPIO_IN_DATA45,
				INREG32(BASE_ADDR + GPIO_IN_DATA45));
		printf(" GPIO_SET_RIS_TRIG45   0x%08x = 0x%08x\n",
				base_address + GPIO_SET_RIS_TRIG45,
				INREG32(BASE_ADDR + GPIO_SET_RIS_TRIG45));
		printf(" GPIO_CLR_RIS_TRIG45   0x%08x = 0x%08x\n",
				base_address + GPIO_CLR_RIS_TRIG45,
				INREG32(BASE_ADDR + GPIO_CLR_RIS_TRIG45));
		printf(" GPIO_SET_FAL_TRIG45   0x%08x = 0x%08x\n",
				base_address + GPIO_SET_FAL_TRIG45,
				INREG32(BASE_ADDR + GPIO_SET_FAL_TRIG45));
		printf(" GPIO_CLR_FAL_TRIG45   0x%08x = 0x%08x\n",
				base_address + GPIO_CLR_FAL_TRIG45,
				INREG32(BASE_ADDR + GPIO_CLR_FAL_TRIG45));
		printf(" GPIO_INTSTAT45        0x%08x = 0x%08x\n",
				base_address + GPIO_INTSTAT45,
				INREG32(BASE_ADDR + GPIO_INTSTAT45));
	}
	if ((gpio_bank == 6) || (gpio_bank == 7))
	{
		printf(" GPIO_DIR67            0x%08x = 0x%08x\n",
				base_address + GPIO_DIR67, INREG32(BASE_ADDR + GPIO_DIR67));
		printf(" GPIO_OUT_DATA67       0x%08x = 0x%08x\n",
				base_address + GPIO_OUT_DATA67,
				INREG32(BASE_ADDR + GPIO_OUT_DATA67));
		printf(" GPIO_SET_DATA67       0x%08x = 0x%08x\n",
				base_address + GPIO_SET_DATA67,
				INREG32(BASE_ADDR + GPIO_SET_DATA67));
		printf(" GPIO_CLR_DATA67       0x%08x = 0x%08x\n",
				base_address + GPIO_CLR_DATA67,
				INREG32(BASE_ADDR + GPIO_CLR_DATA67));
		printf(" GPIO_IN_DATA67        0x%08x = 0x%08x\n",
				base_address + GPIO_IN_DATA67,
				INREG32(BASE_ADDR + GPIO_IN_DATA67));
		printf(" GPIO_SET_RIS_TRIG67   0x%08x = 0x%08x\n",
				base_address + GPIO_SET_RIS_TRIG67,
				INREG32(BASE_ADDR + GPIO_SET_RIS_TRIG67));
		printf(" GPIO_CLR_RIS_TRIG67   0x%08x = 0x%08x\n",
				base_address + GPIO_CLR_RIS_TRIG67,
				INREG32(BASE_ADDR + GPIO_CLR_RIS_TRIG67));
		printf(" GPIO_SET_FAL_TRIG67   0x%08x = 0x%08x\n",
				base_address + GPIO_SET_FAL_TRIG67,
				INREG32(BASE_ADDR + GPIO_SET_FAL_TRIG67));
		printf(" GPIO_CLR_FAL_TRIG67   0x%08x = 0x%08x\n",
				base_address + GPIO_CLR_FAL_TRIG67,
				INREG32(BASE_ADDR + GPIO_CLR_FAL_TRIG67));
		printf(" GPIO_INTSTAT67        0x%08x = 0x%08x\n",
				base_address + GPIO_INTSTAT67,
				INREG32(BASE_ADDR + GPIO_INTSTAT67));
	}
	if (gpio_bank == 8)
	{
		printf(" GPIO_DIR8            0x%08x = 0x%08x\n",
				base_address + GPIO_DIR8, INREG32(BASE_ADDR + GPIO_DIR8));
		printf(" GPIO_OUT_DATA8       0x%08x = 0x%08x\n",
				base_address + GPIO_OUT_DATA8,
				INREG32(BASE_ADDR + GPIO_OUT_DATA8));
		printf(" GPIO_SET_DATA8       0x%08x = 0x%08x\n",
				base_address + GPIO_SET_DATA8,
				INREG32(BASE_ADDR + GPIO_SET_DATA8));
		printf(" GPIO_CLR_DATA8       0x%08x = 0x%08x\n",
				base_address + GPIO_CLR_DATA8,
				INREG32(BASE_ADDR + GPIO_CLR_DATA8));
		printf(" GPIO_IN_DATA8        0x%08x = 0x%08x\n",
				base_address + GPIO_IN_DATA8,
				INREG32(BASE_ADDR + GPIO_IN_DATA8));
		printf(" GPIO_SET_RIS_TRIG8   0x%08x = 0x%08x\n",
				base_address + GPIO_SET_RIS_TRIG8,
				INREG32(BASE_ADDR + GPIO_SET_RIS_TRIG8));
		printf(" GPIO_CLR_RIS_TRIG8   0x%08x = 0x%08x\n",
				base_address + GPIO_CLR_RIS_TRIG8,
				INREG32(BASE_ADDR + GPIO_CLR_RIS_TRIG8));
		printf(" GPIO_SET_FAL_TRIG8   0x%08x = 0x%08x\n",
				base_address + GPIO_SET_FAL_TRIG8,
				INREG32(BASE_ADDR + GPIO_SET_FAL_TRIG8));
		printf(" GPIO_CLR_FAL_TRIG8   0x%08x = 0x%08x\n",
				base_address + GPIO_CLR_FAL_TRIG8,
				INREG32(BASE_ADDR + GPIO_CLR_FAL_TRIG8));
		printf(" GPIO_INTSTAT8        0x%08x = 0x%08x\n",
				base_address + GPIO_INTSTAT8,
				INREG32(BASE_ADDR + GPIO_INTSTAT8));
	}
	printf("\n");
}

