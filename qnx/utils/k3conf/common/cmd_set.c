/*
 * K3CONF Command Set
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <tisci.h>
#include <socinfo.h>
#include <help.h>
#include <k3conf.h>

#define HELP_SET_PARENT_URL1 "http://downloads.ti.com/tisci/esd/latest/2_tisci_msgs/pm/clocks.html#power-management-clock-frequency-configuration-example-with-mux-programming"
#define HELP_SET_PARENT_URL2 "http://downloads.ti.com/tisci/esd/latest/2_tisci_msgs/pm/clocks.html#tisci-msg-set-clock-parent"

static int set_clock(int argc, char *argv[])
{
	uint32_t dev_id, clk_id, ret;
	uint64_t freq;

	if (argc < 3)
		return -1;

	ret = sscanf(argv[0], "%u", &dev_id);
	if (ret != 1)
		return -1;

	ret = sscanf(argv[1], "%u", &clk_id);
	if (ret != 1)
		return -1;

	ret = sscanf(argv[2], "%lu", &freq);
	if (ret != 1)
		return -1;

	ret = ti_sci_cmd_set_clk_freq(dev_id, clk_id, freq);
	if (ret)
		return ret;

	return dump_clocks_info(argc, argv);
}

static int set_clock_parent(int argc, char *argv[])
{
	uint32_t dev_id, clk_id, parent_clk_id;
	int ret;

	if (argc < 3)
		return -1;

	ret = sscanf(argv[0], "%u", &dev_id);
	if (ret != 1)
		return -1;

	ret = sscanf(argv[1], "%u", &clk_id);
	if (ret != 1)
		return -1;

	ret = sscanf(argv[2], "%u", &parent_clk_id);
	if (ret != 1)
		return -1;

	ret = ti_sci_cmd_set_clk_parent(dev_id, clk_id, parent_clk_id);
	if (ret) {
		fprintf(stderr, "Request to set parent failed: %d\n",ret);
		fprintf(stderr, "Clock state is probably wrong!\n");
		fprintf(stderr, "Clock state of clk_id %d: %s\n",
			clk_id, ti_sci_cmd_get_clk_state(dev_id, clk_id));
		fprintf(stderr, "Clock state of parent_clk_id %d: %s\n",
			parent_clk_id,
			ti_sci_cmd_get_clk_state(dev_id, parent_clk_id));
		fprintf(stderr, "\nRefer to:\n\t%s\n\t%s\n",
			HELP_SET_PARENT_URL1, HELP_SET_PARENT_URL2);

		return ret;
	}

	return dump_clock_parent_info(argc - 1, argv);
}

int process_set_command(int argc, char *argv[])
{
	int ret;

	if (argc < 1) {
		help(HELP_SET);
		return -1;
	}

	if (!strncmp(argv[0], "clock", 5)) {
		argc--;
		argv++;
		ret = set_clock(argc, argv);
		if (ret) {
			fprintf(stderr, "Invalid clock arguments\n");
			help(HELP_SET_CLOCK);
		}
	} else if (!strncmp(argv[0], "parent_clock", 5)) {
		argc--;
		argv++;
		ret = set_clock_parent(argc, argv);
		if (ret) {
			if (ret == -1) {
				fprintf(stderr, "Invalid parent_clock arguments\n");
				help(HELP_SET_CLOCK_PARENT);
			}
		}
	} else if (!strcmp(argv[0], "--help")) {
		help(HELP_SET);
		return 0;
	} else {
		fprintf(stderr, "Invalid argument %s\n", argv[1]);
		help(HELP_SET);
		return -1;
	}
	return ret;
}
