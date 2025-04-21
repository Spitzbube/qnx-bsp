/*
 * Copyright 2021, 2023, BlackBerry Limited.
 * Copyright 2021, Texas Instruments Incorporated.
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

#include <startup.h>
#include "aarch64/gic.h"

#define	GICD_PADDR          (0x01800000)
#define	GICR_PADDR          (0x01880000)
#define	GITS_PADDR          (0x01820000)

void init_intrinfo(void)
{
	static struct startup_intrinfo gic_v3_intr_lpi =
	{
		.vector_base     = 8192,
		.num_vectors     = 0, //patched based # supported by the GIC & nlpi parm
		.cascade_vector  = _NTO_INTR_SPARE,
		.cpu_intr_base   = 0,
		.cpu_intr_stride = 0,
		.flags           = 0,
		.id              = { .genflags = 0, .size = 0, .rtn = &interrupt_id_gic_v3_lpi },
		.eoi             = { .genflags = 0, .size = 0, .rtn = &interrupt_eoi_gic_v3_lpi_sr },
		.mask            = &interrupt_mask_gic_v3_lpi_direct,
		.unmask          = &interrupt_unmask_gic_v3_lpi_direct,
		.config          = NULL,
		.patch_data      = NULL,	/* set up at runtime */
	};
	static struct startup_intrinfo gic_v3_intr_lpi_msi =
	{
		.vector_base     = 8192,	// and patched based on number requested
		.num_vectors     = 0, //patched based # supported by the GIC & nlpi parm
		.cascade_vector  = _NTO_INTR_SPARE,
		.cpu_intr_base   = 0,
		.cpu_intr_stride = 0,
		.flags           = INTR_FLAG_MSI,
		.id              = { .genflags = 0, .size = 0, .rtn = &interrupt_id_gic_v3_lpi },
		.eoi             = { .genflags = 0, .size = 0, .rtn = &interrupt_eoi_gic_v3_lpi_sr },
		.mask            = &interrupt_mask_gic_v3_lpi_direct,
		.unmask          = &interrupt_unmask_gic_v3_lpi_direct,
		.config          = NULL,
		.patch_data      = NULL,	/* set up at runtime */
	};

	/* Initialise GIC */

	gic_v3_set_paddr(GICD_PADDR, GICR_PADDR, GITS_PADDR);

	/* Take the first 256 LPIs for PCI MSIs */
	const unsigned num_lpis_as_msis = 256;
	const unsigned num_lpis = gic_v3_num_lpis();
	gic_v3_intr_lpi_msi.num_vectors = num_lpis_as_msis;
	gic_v3_intr_lpi.vector_base = gic_v3_intr_lpi_msi.vector_base + gic_v3_intr_lpi_msi.num_vectors;
	gic_v3_intr_lpi.num_vectors = (num_lpis - num_lpis_as_msis);

	/* Note: entries must be added in .vector_base ascending order */
	if (gic_v3_intr_lpi_msi.num_vectors > 0)
	{
		const int r = gic_v3_lpi_add_entry(&gic_v3_intr_lpi_msi);
		kprintf("gic_v3_lpi_add_entry for vectors %u -> %u, %s\n",
				gic_v3_intr_lpi_msi.vector_base, gic_v3_intr_lpi_msi.vector_base + gic_v3_intr_lpi_msi.num_vectors - 1,
				(r == 0) ? "Ok" : "Failed");
	}
	if (gic_v3_intr_lpi.num_vectors > 0)
	{
		const int r = gic_v3_lpi_add_entry(&gic_v3_intr_lpi);
		kprintf("gic_v3_lpi_add_entry for vectors %u -> %u, %s\n",
				gic_v3_intr_lpi.vector_base, gic_v3_intr_lpi.vector_base + gic_v3_intr_lpi.num_vectors - 1,
				(r == 0) ? "Ok" : "Failed");
	}

	gic_v3_its_set_dt_page_size(0, 65536);
	gic_v3_initialize();
}
#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/startup/boards/ti-j7/j722s/init_intrinfo.c $ $Rev: 994584 $")
#endif
