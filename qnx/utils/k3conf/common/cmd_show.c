/*
 * K3CONF Command Show
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
#include <sec_proxy.h>
#include <tisci.h>
#include <socinfo.h>
#include <help.h>
#include <autoadjust_table.h>
#include <k3conf.h>

#ifdef QNX_OS
static char table[TABLE_MAX_ROW][TABLE_MAX_COL][TABLE_MAX_ELT_LEN];
#endif

static int show_hosts_info(void)
{
	struct ti_sci_host_info *hosts = soc_info.sci_info.host_info;
#ifdef QNX_OS
	/* using global array */
#else
	char table[TABLE_MAX_ROW][TABLE_MAX_COL][TABLE_MAX_ELT_LEN];
#endif
	uint32_t row = 0;

	autoadjust_table_init(table);
	strncpy(table[row][0], "Host ID", TABLE_MAX_ELT_LEN);
	strncpy(table[row][1], "Host Name", TABLE_MAX_ELT_LEN);
	strncpy(table[row][2], "Security Status", TABLE_MAX_ELT_LEN);
	strncpy(table[row][3], "Description", TABLE_MAX_ELT_LEN);

	for (row = 0; row < soc_info.sci_info.num_hosts; row++) {
		snprintf(table[row + 1][0], TABLE_MAX_ELT_LEN, "%d",
			 hosts[row].host_id);
		strncpy(table[row + 1][1], hosts[row].host_name,
			TABLE_MAX_ELT_LEN);
		strncpy(table[row + 1][2], hosts[row].security_status,
			TABLE_MAX_ELT_LEN);
		strncpy(table[row + 1][3], hosts[row].description,
			TABLE_MAX_ELT_LEN);
	}

	return autoadjust_table_print(table, row + 1, 4);
}

static int show_sp_threads_info(void)
{
#ifdef QNX_OS
	/* using global array */
#else
	char table[TABLE_MAX_ROW][TABLE_MAX_COL][TABLE_MAX_ELT_LEN];
#endif
	struct ti_sci_sec_proxy_info *sp;
	uint32_t row = 0, i;

	autoadjust_table_init(table);
	strncpy(table[row][0],
		"Secure Proxy thread allocation for main_sec_proxy0",
		TABLE_MAX_ELT_LEN);
	row++;
	strncpy(table[row][0], "Thread ID", TABLE_MAX_ELT_LEN);
	strncpy(table[row][1], "Direction", TABLE_MAX_ELT_LEN);
	strncpy(table[row][2], "No. of msgs", TABLE_MAX_ELT_LEN);
	strncpy(table[row][3], "Host", TABLE_MAX_ELT_LEN);
	strncpy(table[row][4], "Host Function", TABLE_MAX_ELT_LEN);
	row++;

	sp = soc_info.sci_info.sp_info[MAIN_SEC_PROXY];
	for (i = 0; i < soc_info.sci_info.num_sp_threads[MAIN_SEC_PROXY];
	     row++, i++) {
		snprintf(table[row][0], TABLE_MAX_ELT_LEN, "%5d", sp[i].sp_id);
		strncpy(table[row][1], sp[i].sp_dir, TABLE_MAX_ELT_LEN);
		snprintf(table[row][2], TABLE_MAX_ELT_LEN, "%7d", sp[i].num_msgs);
		strncpy(table[row][3], sp[i].host, TABLE_MAX_ELT_LEN);
		strncpy(table[row][4], sp[i].host_function, TABLE_MAX_ELT_LEN);
	}

	autoadjust_table_generic_fprint(stdout, table, row, 5,
					TABLE_HAS_SUBTITLE | TABLE_HAS_TITLE);
	row = 0;
	autoadjust_table_init(table);
	strncpy(table[row][0],
		"Secure Proxy thread allocation for mcu_sec_proxy0",
		TABLE_MAX_ELT_LEN);
	row++;
	strncpy(table[row][0], "Thread ID", TABLE_MAX_ELT_LEN);
	strncpy(table[row][1], "Direction", TABLE_MAX_ELT_LEN);
	strncpy(table[row][2], "No. of msgs", TABLE_MAX_ELT_LEN);
	strncpy(table[row][3], "Host", TABLE_MAX_ELT_LEN);
	strncpy(table[row][4], "Host Function", TABLE_MAX_ELT_LEN);
	row++;

	sp = soc_info.sci_info.sp_info[MCU_SEC_PROXY];
	for (i = 0; i < soc_info.sci_info.num_sp_threads[MCU_SEC_PROXY];
	     row++, i++) {
		snprintf(table[row][0], TABLE_MAX_ELT_LEN, "%5d", sp[i].sp_id);
		strncpy(table[row][1], sp[i].sp_dir, TABLE_MAX_ELT_LEN);
		snprintf(table[row][2], TABLE_MAX_ELT_LEN, "%7d", sp[i].num_msgs);
		strncpy(table[row][3], sp[i].host, TABLE_MAX_ELT_LEN);
		strncpy(table[row][4], sp[i].host_function, TABLE_MAX_ELT_LEN);
	}

	return autoadjust_table_generic_fprint(stdout, table, row, 5,
					TABLE_HAS_SUBTITLE | TABLE_HAS_TITLE);
}

static int show_clocks_info(int argc, char *argv[])
{
	struct ti_sci_clocks_info *c = soc_info.sci_info.clocks_info;
#ifdef QNX_OS
	/* using global array */
#else
	char table[TABLE_MAX_ROW][TABLE_MAX_COL][TABLE_MAX_ELT_LEN];
#endif
	uint32_t row = 0, dev_id;
	int found = 0, ret;

	autoadjust_table_init(table);
	strncpy(table[row][0], "Device ID", TABLE_MAX_ELT_LEN);
	strncpy(table[row][1], "Clock ID", TABLE_MAX_ELT_LEN);
	strncpy(table[row][2], "Clock Name", TABLE_MAX_ELT_LEN);
	strncpy(table[row][3], "Clock Function", TABLE_MAX_ELT_LEN);

	if (argc)
		goto print_single_device;

	for (row = 0; row < soc_info.sci_info.num_clocks; row++) {
		snprintf(table[row + 1][0], TABLE_MAX_ELT_LEN, "%5d",
			 c[row].dev_id);
		snprintf(table[row + 1][1], TABLE_MAX_ELT_LEN, "%5d",
			 c[row].clk_id);
		strncpy(table[row + 1][2], c[row].clk_name, TABLE_MAX_ELT_LEN);
		strncpy(table[row + 1][3], c[row].clk_function, TABLE_MAX_ELT_LEN);
	}

	return autoadjust_table_print(table, row + 1, 4);

print_single_device:
	ret = sscanf(argv[0], "%u", &dev_id);
	if (ret != 1)
		return -1;

	for (row = 0; row < soc_info.sci_info.num_clocks; row++) {
		if (dev_id == c[row].dev_id) {
			snprintf(table[found + 1][0], TABLE_MAX_ELT_LEN, "%5d",
				 c[row].dev_id);
			snprintf(table[found + 1][1], TABLE_MAX_ELT_LEN, "%5d",
				 c[row].clk_id);
			strncpy(table[found + 1][2], c[row].clk_name,
				TABLE_MAX_ELT_LEN);
			strncpy(table[found + 1][3], c[row].clk_function, TABLE_MAX_ELT_LEN);
			found++;
		}
	}

	if (!found)
		return -1;

	return autoadjust_table_print(table, found + 1, 4);
}

static int show_devices_info(int argc, char *argv[])
{
	struct ti_sci_devices_info *p = soc_info.sci_info.devices_info;
#ifdef QNX_OS
	/* using global array */
#else
	char table[TABLE_MAX_ROW][TABLE_MAX_COL][TABLE_MAX_ELT_LEN];
#endif
	uint32_t row = 0, dev_id;
	int found = 0, ret;

	autoadjust_table_init(table);
	strncpy(table[row][0], "Device ID", TABLE_MAX_ELT_LEN);
	strncpy(table[row][1], "Device Name", TABLE_MAX_ELT_LEN);

	if (argc)
		goto print_single_device;

	for (row = 0; row < soc_info.sci_info.num_devices; row++) {
		snprintf(table[row + 1][0], TABLE_MAX_ELT_LEN, "%5d",
			 p[row].dev_id);
		strncpy(table[row + 1][1], p[row].name, TABLE_MAX_ELT_LEN);
	}

	return autoadjust_table_print(table, row + 1, 2);

print_single_device:
	ret = sscanf(argv[0], "%u", &dev_id);
	if (ret != 1)
		return -1;

	for (row = 0; row < soc_info.sci_info.num_devices; row++) {
		if (dev_id == p[row].dev_id) {
			snprintf(table[1][0], TABLE_MAX_ELT_LEN, "%5d",
				 p[row].dev_id);
			strncpy(table[1][1], p[row].name,
				TABLE_MAX_ELT_LEN);
			found = 1;
			break;
		}
	}

	if (!found)
		return -1;

	return autoadjust_table_print(table, 2, 2);
}

static int show_rm_info(int argc, char *argv[])
{
	struct ti_sci_devices_info *d = soc_info.sci_info.devices_info;
#ifdef QNX_OS
	/* using global array */
#else
	char table[TABLE_MAX_ROW][TABLE_MAX_COL][TABLE_MAX_ELT_LEN];
#endif
	struct ti_sci_rm_info *r = soc_info.sci_info.rm_info;
	uint32_t filter_type = 0, type, subtype, i, j, row;
	char *subtype_name;

	if (argc > 1)
		return -1;
	if (argc == 1)
		sscanf(argv[0], "%u", &filter_type);

	snprintf(table[0][0], TABLE_MAX_ELT_LEN,
			"Resources managed by System Firmware");
	snprintf(table[1][0], TABLE_MAX_ELT_LEN, "Unique Type");
	snprintf(table[1][1], TABLE_MAX_ELT_LEN, "dev_id");
	snprintf(table[1][2], TABLE_MAX_ELT_LEN, "Device name");
	snprintf(table[1][3], TABLE_MAX_ELT_LEN, "subtype_id");
	snprintf(table[1][4], TABLE_MAX_ELT_LEN, "Subtype name");

	row = 2;
	for (i = 0; i < soc_info.sci_info.num_res; i++) {

		type = r[i].utype >> 6;
		subtype = r[i].utype & 0x3F;
		subtype_name = r[i].subtype_name;

		if (filter_type && filter_type != type)
			continue;

		for (j = 0; j < soc_info.sci_info.num_devices; j++) {
			if (type == d[j].dev_id)
				break;
		}

		snprintf(table[row][0], TABLE_MAX_ELT_LEN,
			"0x%04x", r[i].utype);
		snprintf(table[row][1], TABLE_MAX_ELT_LEN,
			"%d", d[j].dev_id);
		snprintf(table[row][2], TABLE_MAX_ELT_LEN,
			"%s", d[j].name);
		snprintf(table[row][3], TABLE_MAX_ELT_LEN,
			"%d", subtype);
		snprintf(table[row][4], TABLE_MAX_ELT_LEN,
			"%s", subtype_name);
		row++;
	}

	if (row == 2) {
		fprintf(stderr, "Resources for type %d are not managed by SYSFW\n",
				filter_type);
		return -1;
	}

	return autoadjust_table_generic_fprint(stdout, table, row, 5,
			TABLE_HAS_SUBTITLE | TABLE_HAS_TITLE);
}

static int show_processors_info(void)
{
	struct ti_sci_processors_info *p = soc_info.sci_info.processors_info;
#ifdef QNX_OS
	/* using global array */
#else
	char table[TABLE_MAX_ROW][TABLE_MAX_COL][TABLE_MAX_ELT_LEN];
#endif
	uint32_t row = 0;

	autoadjust_table_init(table);
	strncpy(table[row][0], "Device ID", TABLE_MAX_ELT_LEN);
	strncpy(table[row][1], "Processor ID", TABLE_MAX_ELT_LEN);
	strncpy(table[row][2], "Processor Name", TABLE_MAX_ELT_LEN);

	for (row = 0; row < soc_info.sci_info.num_processors; row++) {
		snprintf(table[row + 1][0], TABLE_MAX_ELT_LEN, "%5d",
			 p[row].dev_id);
		snprintf(table[row + 1][1], TABLE_MAX_ELT_LEN, "%7d",
			 p[row].processor_id);
		strncpy(table[row + 1][2], p[row].name, TABLE_MAX_ELT_LEN);
	}

	return autoadjust_table_print(table, row + 1, 3);
}

int process_show_command(int argc, char *argv[])
{
	int ret;

	if (argc < 1) {
		help(HELP_SHOW);
		return -1;
	}

	if (!strncmp(argv[0], "host", 4)) {
		ret = show_hosts_info();
		if (ret)
			help(HELP_SHOW_HOST);
	} else if (!strncmp(argv[0], "thread", 6)) {
		ret = show_sp_threads_info();
		if (ret)
			help(HELP_SHOW_SEC_PROXY);
	} else if (!strncmp(argv[0], "device", 6)) {
		argc--;
		argv++;
		ret = show_devices_info(argc, argv);
		if (ret) {
			fprintf(stderr, "Invalid device arguments\n");
			help(HELP_SHOW_DEVICE);
		}
	} else if (!strncmp(argv[0], "clock", 5)) {
		argc--;
		argv++;
		ret = show_clocks_info(argc, argv);
		if (ret) {
			fprintf(stderr, "Invalid clock arguments\n");
			help(HELP_SHOW_CLOCK);
		}
	} else if(!strncmp(argv[0], "processor", 9)) {
		ret = show_processors_info();
		if (ret)
			help(HELP_SHOW_PROCESSOR);
	} else if(!strncmp(argv[0], "rm", 2)) {
		argc--;
		argv++;
		ret = show_rm_info(argc, argv);
		if (ret) {
			fprintf(stderr, "Invalid device_id arguments\n");
			help(HELP_SHOW_RM);
		}
	} else if (!strcmp(argv[0], "--help")) {
		help(HELP_SHOW);
		return 0;
	} else {
		fprintf(stderr, "Invalid argument %s\n", argv[1]);
		help(HELP_SHOW);
		return -1;
	}
	return ret;
}
