/*
 * Copyright (c) 2021, 2022, BlackBerry Limited.
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


/*
 * TI J7 SoC PCI INTPIN interrupt callouts
 *
 * Note: Due to errata, the legacy PCI INTA thru INTD may not operate correctly
 * depending on the EP using them. The HW module for the J7 disables their use
 * by default however users may enable them if they want by adding an [interrupts]
 * section to a HW config file. This file provides the demuxing for them should
 * that choice be made
 */

#include <aarch64/callout.ah>

/*
 * NOTE:
 *
 * For each of the 4 controllers, there are 4 registers for managing the the
 * legacy INTPINs within the PCIE_INTD register block as follows
 *
 *    PCIE_INTD_ENABLE_REG_SYS_0      - write 0b1 to enable the interrupt
 *    PCIE_INTD_ENABLE_CLR_REG_SYS_0  - write 0b1 to disable the interrupt
 *    PCIE_INTD_STATUS_REG_SYS_0      - read-only status bits
 *    PCIE_INTD_STATUS_CLR_REG_SYS_0  - write 0b1 to clear the status
 *
 * Writing 0b0 to any of the fields in these registers is a no-op
 *
 * These callouts are currently written to handle only the legacy PCI INTPIN
 * interrupts in bits [3..0] for INTA thru INTD respectively
 *
*/
#define ENABLE_REG_SYS_0        0x100
#define ENABLE_CLR_REG_SYS_0    0x300
#define STATUS_REG_SYS_0        0x500
#define STATUS_CLR_REG_SYS_0    0x700

/*
 * -----------------------------------------------------------------------
 * Patch callout code (for PCI INTPIN's)
 *
 * Patcher routine takes the following arguments:
 *
 *    x0 - syspage paddr
 *    x1 - vaddr of callout
 *    x2 - offset from start of syspage to start of callout routine
 *    x3 - offset from start of syspage to rw storage
 *    x4 - patch data
 *    x5 - callout_rtn
 * -----------------------------------------------------------------------
 */
.text
.align 4

interrupt_patch_intpin:

    sub     sp, sp, #16
    stp     x19, x30, [sp]

    add     x19, x0, x2        /* x19 = address of callout routine */

    /* Map using patch_data parameter */
    ldr     x1, [x4]
    mov     x0, #4096
    bl      callout_io_map

    CALLOUT_PATCH x19, w6, w7

    ldp     x19, x30, [sp]
    add     sp, sp, #16
    ret

/*
 * -----------------------------------------------------------------------
 * Identify interrupt source
 *
 * On entry
 * - syspage pointer in x20
 *
 * On exit
 * - interrupt id returned in x19
 * -----------------------------------------------------------------------
 */
CALLOUT_START(interrupt_id_j7_pci_intpin, 0, interrupt_patch_intpin)

    /* Get the base address (patched) */
    mov     x7, #0x0000
    movk    x7, #0x0000, lsl #16
    movk    x7, #0x0000, lsl #32
    movk    x7, #0x0000, lsl #48

    mov     x19, #-1

    /* Read status and enable registers clearing any disabled interrupts */
    ldr     w0, [x7, #ENABLE_REG_SYS_0]
    ldr     w1, [x7, #STATUS_REG_SYS_0]
    ands    w2, w0, w1
    beq     done

    /* INTA thru INTD are in bits 3 thru 0 respectively */
    ands    w2, w2, #0xF
    beq     done
    /*
     * Scan for first set bit
     * This will prioritize as INTA -> INTD
     */
    clz     w19, w2
    mov     w2, #(31 - 0)
    sub     w19, w2, w19
    mov     w3, #0x1
    lsl     w1, w3, w19

    /* Mask/disable the interrupt */
    str     w1, [x7, #ENABLE_CLR_REG_SYS_0]

    /* Clear status */
    str     w1, [x7, #STATUS_CLR_REG_SYS_0]
done:

CALLOUT_END(interrupt_id_j7_pci_intpin)

/*
 * -----------------------------------------------------------------------
 * Acknowledge specified interrupt
 *
 * On entry
 * - x19 - contains interrupt id
 * - x20 - contains syspage pointer (INTR_GENFLAG_LOAD_SYSPAGE was used)
 * - x22 - contains the interrupt mask count (INTR_GENFLAG_LOAD_INTRMASK was used)
 * -----------------------------------------------------------------------
 */
CALLOUT_START(interrupt_eoi_j7_pci_intpin, 0, interrupt_patch_intpin)
    /*
     * Get the base address (patched)
     */
    mov     x7, #0x0000
    movk    x7, #0x0000, lsl #16
    movk    x7, #0x0000, lsl #32
    movk    x7, #0x0000, lsl #48

    /* Only re-enable interrupt if mask count is zero */
    cbnz    x22, 0f

    /* Unmask/enable the interrupt */
    mov     w3, #1
    lsl     w0, w3, w19
    str     w0, [x7, #ENABLE_REG_SYS_0]
0:

CALLOUT_END(interrupt_eoi_j7_pci_intpin)

/*
 * -----------------------------------------------------------------------
 * Mask specified interrupt
 *
 * On entry:
 * - x0 - _syspage_ptr
 * - x1 - vector number (relative to controller)
 *
 * Returns:
 * - x0 - error status
 * -----------------------------------------------------------------------
 */
CALLOUT_START(interrupt_mask_j7_pci_intpin, 0, interrupt_patch_intpin)

    /* Get the base address (patched) */
    mov     x7, #0x0000
    movk    x7, #0x0000, lsl #16
    movk    x7, #0x0000, lsl #32
    movk    x7, #0x0000, lsl #48

    /* Mask/disable the interrupt */
    mov     w0, #0x1
    lsl     w0, w0, w1
    str     w0, [x7, #ENABLE_CLR_REG_SYS_0]

    mov     x0, #0

    ret

CALLOUT_END(interrupt_mask_j7_pci_intpin)

/*
 * -----------------------------------------------------------------------
 * Unmask specified interrupt
 *
 * On entry:
 * - x0 - _syspage_ptr
 * - x1 - vector number (relative to controller)
 *
 * Returns:
 * - x0 - error status
 * -----------------------------------------------------------------------
 */
CALLOUT_START(interrupt_unmask_j7_pci_intpin, 0, interrupt_patch_intpin)

    /* Get the base address (patched) */
    mov     x7, #0x0000
    movk    x7, #0x0000, lsl #16
    movk    x7, #0x0000, lsl #32
    movk    x7, #0x0000, lsl #48

    /* Unmask/enable the interrupt */
    mov     w0, #1
    lsl     w0, w0, w1
    str     w0, [x7, #ENABLE_REG_SYS_0]

    mov     x0, #0

    ret

CALLOUT_END(interrupt_unmask_j7_pci_intpin)

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/startup/boards/ti-j7/callout_interrupt_j7_intpin.S $ $Rev: 984398 $";
.previous
#endif
#endif
