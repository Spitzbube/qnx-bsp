/*
 * K3CONF main entry file
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

#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sec_proxy.h>
#include <tisci.h>
#include <version.h>
#include <socinfo.h>
#include <help.h>
#include <string.h>
#include <autoadjust_table.h>
#include <k3conf.h>
#ifdef QNX_OS
#include <sys/mman.h>
#include <sys/neutrino.h>
#endif

#ifdef DEBUG
#define dprintf(format, ...)	 printf(format, ## __VA_ARGS__)
#else
#define dprintf(format, ...)
#endif

#ifdef QNX_OS
static char table[TABLE_MAX_ROW][TABLE_MAX_COL][TABLE_MAX_ELT_LEN];
#endif

void k3conf_print_version(FILE *stream)
{
#ifdef QNX_OS
	/* using global array */
#else
	char table[TABLE_MAX_ROW][TABLE_MAX_COL][TABLE_MAX_ELT_LEN];
#endif
	struct ti_sci_version_info ver = soc_info.sci_info.version;
	uint32_t row = 0;

	if (stream == NULL) {
		fprintf(stderr, "%s(): stream == NULL!!!\n", __func__);
		return;
	}

	autoadjust_table_init(table);
	strncpy(table[row][0], "VERSION INFO", TABLE_MAX_ELT_LEN);
	row++;
	strncpy(table[row][0], "K3CONF", TABLE_MAX_ELT_LEN);
#ifdef QNX_OS
	snprintf(table[row][1], TABLE_MAX_ELT_LEN, "QNX (version %s built %s)", k3conf_version, builddate);
#else
	snprintf(table[row][1], TABLE_MAX_ELT_LEN, "(version %s built %s)", k3conf_version, builddate);
#endif
	row++;
	strncpy(table[row][0], "SoC", TABLE_MAX_ELT_LEN);
	strncpy(table[row][1], soc_info.soc_full_name, TABLE_MAX_ELT_LEN);
	row++;

	if (soc_info.ti_sci_enabled) {
		strncpy(table[row][0], "SYSFW", TABLE_MAX_ELT_LEN);
		snprintf(table[row][1], TABLE_MAX_ELT_LEN,
			 "ABI: %d.%d (firmware version 0x%04x '%.*s)')",
			 ver.abi_major, ver.abi_minor, ver.firmware_version,
			 (int)sizeof(ver.firmware_description),
			 ver.firmware_description);
		row++;
	}

	autoadjust_table_generic_fprint(stream, table, row, 2, TABLE_HAS_TITLE);

	return;
}

int main(int argc, char *argv[])
{
	uint32_t host_id;
	int ret = 0;

	/* Scan user arguments for options */
	argc--;
	argv++;

	if (argc == 0) {
		help(HELP_USAGE);
		ret = -1;
		goto main_exit;
	}

	host_id = INVALID_HOST_ID;
	if (!strcmp(argv[0], "--host")) {
		argc--; argv++;
		ret = sscanf(argv[0], "%u", &host_id);
		if (ret != 1) {
			fprintf(stderr, "Invalid host id %s\n", argv[0]);
			return -1;
		}
		dprintf("%s: host_id from user = %d\n", __func__,
			soc_info.host_id);
		argc--; argv++;
	}

	if (soc_init(host_id))
		goto main_exit;

	if (!strcmp(argv[0], "--help")) {
		k3conf_print_version(stdout);
		help(HELP_ALL);
		goto main_exit;
	} else if (!strcmp(argv[0], "--version")) {
		k3conf_print_version(stdout);
		goto main_exit;
	} else if (!strcmp(argv[0], "--cpuinfo")) {
		k3conf_print_version(stdout);
		dump_cpu_info();
		goto main_exit;
	} else if (!strcmp(argv[0], "show")) {
		argc--;
		argv++;
		k3conf_print_version(stdout);
		return process_show_command(argc, argv);
	} else if (!strcmp(argv[0], "dump")) {
		argc--;
		argv++;
		k3conf_print_version(stdout);
		return process_dump_command(argc, argv);
	} else if (!strcmp(argv[0], "enable")) {
		argc--;
		argv++;
		k3conf_print_version(stdout);
		return process_enable_command(argc, argv);
	} else if (!strcmp(argv[0], "disable")) {
		argc--;
		argv++;
		k3conf_print_version(stdout);
		return process_disable_command(argc, argv);
	} else if (!strcmp(argv[0], "set")) {
		argc--;
		argv++;
		k3conf_print_version(stdout);
		return process_set_command(argc, argv);
	} else if (!strcmp(argv[0], "read")) {
		argc--;
		argv++;
		k3conf_print_version(stdout);
		return process_read_command(argc, argv);
	} else if (!strcmp(argv[0], "write")) {
		argc--;
		argv++;
		k3conf_print_version(stdout);
		return process_write_command(argc, argv);
	} else {
		fprintf(stderr, "Invalid argument %s", argv[0]);
		help(HELP_USAGE);
		ret = -1;
		goto main_exit;
	}

main_exit:
	return ret;
}
