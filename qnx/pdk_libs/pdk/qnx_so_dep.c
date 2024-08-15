/*
 * These symbols need to be defined for QNX shared object build of
 * PDK to compile properly, though they are not needed
 * in any apps/libs that are actually being used.
 */
#include <stdint.h>

/* Including the following files to satisfy release packaging dependency */
#include <ti/csl/cslr_gtc.h>
#include <ti/csl/csl_crc.h>
#include <ti/csl/cslr_ringacc.h>
#include <ti/csl/csl_ringacc.h>

uint64_t TTBR3_BASE_ADDR, TTBR2_BASE_ADDR, TTBR1_BASE_ADDR;
