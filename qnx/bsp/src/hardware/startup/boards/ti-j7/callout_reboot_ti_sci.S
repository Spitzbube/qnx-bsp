/*
 * Copyright (c) 2019, 2022, BlackBerry Limited.
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


/*
 * TI SCI system reset
 */

#include <aarch64/callout.ah>

/*
 * -----------------------------------------------------------------------
 * Routine to patch callout code
 *
 * On entry:
 *  x0 - physical address of syspage
 *  x1 - virtual  address of syspage
 *  x2 - offset from start of syspage to start of the callout routine
 *  x3 - offset from start of syspage to read/write data used by callout
 * -----------------------------------------------------------------------
 */

patch_reboot:
    sub     sp, sp, #16
    stp     x19, x30, [sp]

    add     x19, x0, x2             /* x19 = address of callout routine */

    /*
     * Map reset control registers
     */
    mov     x0, #0x1000
    adr     x1, Lpaddr
    bl      callout_io_map_indirect

    /*
     * Patch the callout routine
     */
    CALLOUT_PATCH   x19, w4, w5

    ldp     x19, x30, [sp]
    add     sp, sp, #16
    ret

    .align 3
Lpaddr: .quad 0x32C0C000    /* SCI data port, channel 12, thread 0 */


CALLOUT_START(reboot_ti_sci, 0, patch_reboot)
    /* Get the MU base address (patched) */
    mov     x5, #0xabcd
    movk    x5, #0xabcd, lsl #16
    movk    x5, #0xabcd, lsl #32
    movk    x5, #0xabcd, lsl #48

    msr     daifset, #0xf                       /* disable system interrupts */

    tbz     w1, #0, do_reboot                   /* w1 == abnormal reboot flag */
    ldurh   w2, [x0, #SYSPAGE_SYSTEM_PRIVATE]   /* offset to system_private */
    add     x0, x0, x2                          /* system_private address */
    ldr     x2, [x0, #SP_PRIVATE_FLAGS]
    tst     x2, #SYSTEM_PRIVATE_FLAG_ABNORMAL_REBOOT
    beq     0f                                  /* spin if ABNORMAL_REBOOT is NOT set */

do_reboot:
    mov     w0, #0x0005             /* TI_SCI_MSG_SYS_RESET, 0x0005 */
    movk    w0, #0x070c, lsl #16    /* Host ID: 0x0c, sequence number 0x07, doesn't matter */
    str     w0, [x5, #4]

    mov     w0, #0x00000002         /* TI_SCI_FLAG_REQ_ACK_ON_PROCESSED */
    str     w0, [x5, #8]

    mov     w0, #0                  /* any value should do */
    mov     x6, #12

fill_msg:
    str     w0, [x5, x6]            /* Fill the rest of the data port to send the message */
    add     x6, x6, #4
    cmp     x6, #60
    ble     fill_msg

0:
    b       0b
CALLOUT_END(reboot_ti_sci)

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/startup/boards/ti-j7/callout_reboot_ti_sci.S $ $Rev: 984398 $";
.previous
#endif
#endif
