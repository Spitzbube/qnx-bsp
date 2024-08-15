/*
 * Help Library for K3CONF
 *
 * Copyright (C) 2019 Texas Instruments Incorporated - https://www.ti.com/
 *	Lokesh Vutla <lokeshvutla@ti.com>
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

#include <help.h>
#include <stdio.h>

#define HELP_CLK_SET_PARENT_URL1 "http://downloads.ti.com/tisci/esd/latest/2_tisci_msgs/pm/clocks.html#power-management-clock-frequency-configuration-example-with-mux-programming"
#define HELP_CLK_SET_PARENT_URL2 "http://downloads.ti.com/tisci/esd/latest/2_tisci_msgs/pm/clocks.html#tisci-msg-set-clock-parent"

void help(help_category cat)
{
	if (cat >= HELP_CATEGORY_MAX) {
		fprintf(stderr, "help called with incorrect category!!! (%d)\n",
			cat);
		return;
	}

	if ((cat == HELP_ALL) || (cat == HELP_USAGE)) {
		printf("\nNAME\n");
		printf("\tk3conf - TI K3 Configuration Diagnostic Tool\n");

		printf("\nSYNOPSIS\n");
		printf("\tk3conf [--host <id>] [--version] [--help] [--cpuinfo] <command> [<args>]\n");
		if (cat == HELP_USAGE)
			printf("\n\tSee 'k3conf --help' for more information.\n\n");
	}

	if (cat == HELP_ALL) {
		printf("\nDESCRIPTION\n");
		printf("\tk3conf is standalone application designed to provide"
			" a quick'n easy way to \n\tdiagnose/debug/audit"
			" TI K3 architecture based processors configuration"
			" at\n\truntime, with no particular kernel dependency.\n");
		printf("\tk3conf is designed to be as much platform-agnostic"
			" as possible, being able to \n\trun on any Linux platform"
			" and easily ported to other OS.\n");
		printf("\tEven if k3conf today focuses mainly on TISCI"
			" related functionality, it is \n\tintended to be extended"
			" to any other area.\n");

		printf("\nOPTIONS\n");
		printf("\t--host <id>\n");
		printf("\t    Use this host id for communicating with sysfw\n\n");

		printf("\t--help\n");
		printf("\t    Print k3conf help.\n");

		printf("\n\t--version\n");
		printf("\t    Print k3conf version.\n");

		printf("\n\t--cpuinfo\n");
		printf("\t    Print the host processor information.\n");
	}

	if (cat != HELP_USAGE)
		printf("\nCOMMANDS\n");

	if ((cat == HELP_ALL) || (cat == HELP_SHOW) ||
	    (cat == HELP_SHOW_HOST)) {
		printf("\n\tk3conf show hosts\n");
		printf("\t	Prints all the available TISCI hosts\n");
	}
	if ((cat == HELP_ALL) || (cat == HELP_SHOW) ||
	    (cat == HELP_SHOW_SEC_PROXY)) {
		printf("\n\tk3conf show threads\n");
		printf("\t	Prints all the available TISCI secure proxy threads\n");
	}
	if ((cat == HELP_ALL) || (cat == HELP_SHOW) ||
	    (cat == HELP_SHOW_DEVICE)) {
		printf("\n\tk3conf show device\n");
		printf("\t	Prints all the available TISCI devices\n");
		printf("\n\tk3conf show device <dev_id>\n");
		printf("\t	Prints the corresponding device id information\n");
	}
	if ((cat == HELP_ALL) || (cat == HELP_SHOW) ||
	    (cat == HELP_SHOW_CLOCK)) {
		printf("\n\tk3conf show clock\n");
		printf("\t	Prints all the available TISCI clocks\n");
		printf("\n\tk3conf show clock <dev_id>\n");
		printf("\t	Prints the clocks for corresponding device id\n");
	}
	if ((cat == HELP_ALL) || (cat == HELP_SHOW) ||
	    (cat == HELP_SHOW_PROCESSOR)) {
		printf("\n\tk3conf show processor\n");
		printf("\t	Prints all the available TISCI processors\n");
	}
	if ((cat == HELP_ALL) || (cat == HELP_SHOW) ||
	    (cat == HELP_SHOW_RM)) {
		printf("\n\tk3conf show rm\n");
		printf("\t	Prints resources managed by System firmware\n");
		printf("\n\tk3conf show rm <dev_id>\n");
		printf("\t	Prints resources managed by System firmware for corresponding device\n");
	}
	if ((cat == HELP_ALL) || (cat == HELP_DUMP) ||
	    (cat == HELP_DUMP_DEVICE)) {
		printf("\n\tk3conf dump device\n");
		printf("\t	Prints device status of all the TISCI devices\n");
		printf("\n\tk3conf dump device <dev_id>\n");
		printf("\t	Prints the corresponding device id status\n");
	}
	if ((cat == HELP_ALL) || (cat == HELP_DUMP) ||
	    (cat == HELP_DUMP_CLOCK)) {
		printf("\n\tk3conf dump clock\n");
		printf("\t	Prints clock status all the available TISCI clocks\n");
		printf("\n\tk3conf dump clock <dev_id>\n");
		printf("\t	Prints the available clock status for corresponding device id\n");
	}
	if ((cat == HELP_ALL) || (cat == HELP_DUMP) ||
	    (cat == HELP_DUMP_CLOCK_PARENT)) {
		printf("\n\tk3conf dump parent_clock <dev_id> <clk_id>\n");
		printf("\t	Prints the clock parent of provided clock\n");
	}
	if ((cat == HELP_ALL) || (cat == HELP_DUMP) ||
	    (cat == HELP_DUMP_PROCESSOR)) {
		printf("\n\tk3conf dump processor\n");
		printf("\t	Prints status of all the available TISCI processors\n");
		printf("\n\tk3conf dump processor <proc_id>\n");
		printf("\t	Prints status of the given TISCI processors\n");
	}
	if ((cat == HELP_ALL) || (cat == HELP_DUMP) ||
	    (cat == HELP_DUMP_RM)) {
		printf("\n\tk3conf dump rm [OPTIONS]\n");
		printf("\t	Prints resource allocation for all utypes / hosts\n");
		printf("\n\tk3conf dump rm [OPTIONS] <type>\n");
		printf("\t	Prints resource allocation for corresponding type\n");
		printf("\n\tk3conf dump rm [OPTIONS] <type> <subtype>\n");
		printf("\t	Prints resource allocation for corresponding device/type\n");
		printf("\t	\n");
		printf("\t	[OPTIONS]\n");
		printf("\t	-h <host_id>\n");
		printf("\t		Filter only for corresponding host_id\n");
	}
	if ((cat == HELP_ALL) || (cat == HELP_ENABLE) ||
	    (cat == HELP_ENABLE_DEVICE)) {
		printf("\n\tk3conf enable device <dev_id>\n");
		printf("\t	Enables the TISCI device and prints the status\n");
	}
	if ((cat == HELP_ALL) || (cat == HELP_ENABLE) ||
	    (cat == HELP_ENABLE_CLOCK)) {
		printf("\n\tk3conf enable clock <dev_id> <clk_id>\n");
		printf("\t	Enables the TISCI clock and prints the status\n");
	}
	if ((cat == HELP_ALL) || (cat == HELP_DISABLE) ||
	    (cat == HELP_DISABLE_DEVICE)) {
		printf("\n\tk3conf disable device <dev_id>\n");
		printf("\t	Disables the TISCI device and prints the status\n");
	}
	if ((cat == HELP_ALL) || (cat == HELP_DISABLE) ||
	    (cat == HELP_DISABLE_CLOCK)) {
		printf("\n\tk3conf disable clock <dev_id> <clk_id>\n");
		printf("\t	Disables the TISCI clock and prints the status\n");
	}
	if ((cat == HELP_ALL) || (cat == HELP_SET) ||
	    (cat == HELP_SET_CLOCK)) {
		printf("\n\tk3conf set clock <dev_id> <clk_id> <freq>\n");
		printf("\t	Sets the clock frequency and prints the status\n");
	}
	if ((cat == HELP_ALL) || (cat == HELP_SET) ||
	    (cat == HELP_SET_CLOCK_PARENT)) {
		printf("\n\tk3conf set parent_clock <dev_id> <clk_id> <parent_clk_id>\n");
		printf("\t	Sets the parent clock for a clock mux and prints the mux status\n");
		printf("\t	Refer to the following documentation for preconditions:\n");
		printf("\t	%s\n", HELP_CLK_SET_PARENT_URL1);
		printf("\t	%s\n", HELP_CLK_SET_PARENT_URL2);
	}
	if ((cat == HELP_ALL) || (cat == HELP_READ)) {
		printf("\n\tk3conf read <addr> [<size>]\n");
		printf("\t	No.of bits to be read is given in the size argument\n");
		printf("\t	Expected input size is 8,16,32,64\n");
		printf("\t	Prints the value at the specified io memory\n");
	}
	if ((cat == HELP_ALL) || (cat == HELP_WRITE)) {
		printf("\n\tk3conf write <addr> <val> [<size>]\n");
		printf("\t	No.of bits to be written is given in the size argument\n");
		printf("\t	Expected input size is 8,16,32,64\n");
		printf("\t	Writes the value at the specified io memory\n");
	}
}
