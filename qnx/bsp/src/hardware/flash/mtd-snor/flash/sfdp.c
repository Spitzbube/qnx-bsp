/*
 * Copyright (c) 2024, BlackBerry Limited.
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

#include <math.h>
#include <stdbool.h>
#include "f3s_snor.h"

#define SIZE2POW2(x)           ((uint16_t)log2(x))
#define UCHAR_MAX_IDX          (UCHAR_MAX - 1)

#define SFDP_SIGNATURE          0x50444653U
#define SFDP_JESD216_MAJOR      1u

#define SFDP_PHDR_ID(p)         (((p)->pidm << 8) | (p)->pidl)
#define SFDP_PHDR_PTP(p)        (((p)->ptp[2] << 16) | ((p)->ptp[1] << 8) | ((p)->ptp[0] << 0))

#define SFDP_BFPT_ID            0xFF00u /* Basic Flash Parameter Table ID */
#define SFDP_SMPT_ID            0xFF81u /* Sector Map Parameter Table ID */
#define SFDP_4BAIT_ID           0xFF84u /* 4-byte Address Instruction Table ID */

#define SFDP_4BAIT_DWORD_MAX    2
#define SFDP_SMPT_DWORD_MIN     4

#define SFDP_PID_MSB            0xFFu
#define SFDP_PID_LSB_BASIC      0x00u
#define SFDP_PID_LSB_SECMAP     0x81u
#define SFDP_PID_LSB_4BAI       0x84u

#define BFPT_DWORD_MAX_JESD216  9
#define BFPT_DWORD_MAX_JESD216B 16

#define SFDP_LEN                256                     // default SFDP size in byte
#define SFDP_HDRSZ              (sizeof(uint32_t) * 2)  // header size in byte
#define SFDP_TBLSZ              20                      // table size, in double word

/* SFDP DWORDS are indexed from 1 but dwords arrays are indexed from 0 */
#define SFDP_DWORD(i)           ((i) - 1)

/** Configuration Detection Command Descriptor, 1st DWORD **/
#define SFDP_SMPT_CONFIG_DETCMD_DESC_LEN (2)       /* Configuration Detection Command Descriptor has 2 DWORDS */
/* Read data mask */
#define SFDP_SMPT_CMD_RD_DATA_SHFT       24U
#define SFDP_SMPT_CMD_RD_DATA_MSK        (0xFFU << SFDP_SMPT_CMD_RD_DATA_SHFT)
#define SFDP_SMPT_CMD_RD_DATA(cmd)       ((uint8_t)(((cmd) & SFDP_SMPT_CMD_RD_DATA_MSK) >> SFDP_SMPT_CMD_RD_DATA_SHFT))

/* Configuration detection command address length */
#define SFDP_SMPT_CMD_ADDR_LEN_SHFT      22U
#define SFDP_SMPT_CMD_ADDR_LEN_MSK       (0x3U << SFDP_SMPT_CMD_ADDR_LEN_SHFT)
#define SFDP_SMPT_CMD_ADDR_LEN_0         (0x0U << SFDP_SMPT_CMD_ADDR_LEN_SHFT)
#define SFDP_SMPT_CMD_ADDR_LEN_3         (0x1U << SFDP_SMPT_CMD_ADDR_LEN_SHFT)
#define SFDP_SMPT_CMD_ADDR_LEN_4         (0x2U << SFDP_SMPT_CMD_ADDR_LEN_SHFT)
#define SFDP_SMPT_CMD_ADDR_LEN_USE_CURR  (0x3U << SFDP_SMPT_CMD_ADDR_LEN_SHFT)

/* Configuration detection command read latency, in clock cycles */
#define SFDP_SMPT_CMD_RD_DC_SHFT         16U
#define SFDP_SMPT_CMD_RD_DC_MSK          (0xFU << SFDP_SMPT_CMD_RD_DC_SHFT)
#define SFDP_SMPT_CMD_RD_DC(cmd)         ((uint8_t)((((cmd) & SFDP_SMPT_CMD_RD_DC_MSK) >> SFDP_SMPT_CMD_RD_DC_SHFT)))
#define SFDP_SMPT_CMD_RD_DC_IS_VAR       0xFU

/* Detection command instruction */
#define SFDP_SMPT_CMD_OP_SHFT            8U
#define SFDP_SMPT_CMD_OP_MSK             (0xFFU << SFDP_SMPT_CMD_OP_SHFT)
#define SFDP_SMPT_CMD_OP(cmd)            ((uint8_t)((((cmd) & SFDP_SMPT_CMD_OP_MSK) >> SFDP_SMPT_CMD_OP_SHFT)))

/* Descriptor Type */
#define SFDP_SMPT_DESC_TYPE_MAP          (0x1U << 1) // 0b: Command descriptor; 1b: Map descriptor
#define SFDP_SMPT_DESC_END               (0x1U << 0) // 0b: Another descriptor of the same type follows this descriptor; 1b: This is the last descriptor of this type.

/** Configuration Map Descriptor Header DWORD **/
/* Region count */
#define SFDP_SMPT_REGION_CNT_SHFT       16U
#define SFDP_SMPT_REGION_CNT_MSK        (0xFFU << SFDP_SMPT_REGION_CNT_SHFT)
#define SFDP_SMPT_REGION_CNT(hdr)       ((uint8_t)(((((hdr) & SFDP_SMPT_REGION_CNT_MSK) >> SFDP_SMPT_REGION_CNT_SHFT) + 1)))

/* Configuration ID */
#define SFDP_SMPT_CONFIG_ID_SHFT        8U
#define SFDP_SMPT_CONFIG_ID_MSK         (0xFFU << SFDP_SMPT_CONFIG_ID_SHFT)
#define SFDP_SMPT_CONFIG_ID(hdr)        ((uint8_t)((((hdr) & SFDP_SMPT_CONFIG_ID_MSK) >> SFDP_SMPT_CONFIG_ID_SHFT)))

/** Region DWORD **/
/* Region size */
#define SFDP_SMPT_REGION_SZ_SHFT        8U
#define SFDP_SMPT_REGION_SZ_MSK         (0xFFFFFFU << SFDP_SMPT_REGION_SZ_SHFT)
#define SFDP_SMPT_REGION_SZ(region)     ((uint64_t)(((((region) & SFDP_SMPT_REGION_SZ_MSK) >> SFDP_SMPT_REGION_SZ_SHFT) + 1) * 256U))

/* Erase Type */
#define SFDP_SMPT_REGION_ETYPE_MSK      (0xFU)
#define SFDP_SMPT_REGION_ETYPE(region)  ((uint8_t)(((region) & SFDP_SMPT_REGION_ETYPE_MSK)))

/* SFDP SMPT region structure */
typedef struct _sfdp_smpt_region_t {
    uint64_t        offset;   // offset in the chip erase address where region starts
    uint64_t        size;     // size of the region in bytes
} sfdp_smpt_region_t;

typedef struct _sfdp_phdr_t {
    uint8_t     pidl;   // Parameter ID LSB
    uint8_t     minor;  // Minor revision
    uint8_t     major;  // Major revision
    uint8_t     pl;     // Parameter length in double word
    uint8_t     ptp[3]; // Parameter table pointer, byte address
    uint8_t     pidm;   // Parameter ID MSB
} sfdp_phdr_t;

typedef struct _sfdp_hdr_t {
    uint32_t    sfdp;
    uint8_t     minor;  // Minor revision
    uint8_t     major;  // Major revision
    uint8_t     nph;    // Number of parameter headers
    uint8_t     ap;     // Access Protocol
    sfdp_phdr_t bfpthdr;// Basic parameter table
} sfdp_hdr_t;


#define SFDP_PTP_OFFSET(p)  ((uint32_t)((p)->ptp[0]) | ((uint32_t)((p)->ptp[1]) << 8) | ((uint32_t)((p)->ptp[2] << 16)))

static int snor_read_sfdp(snor_chip_t* const chip, int off, uint8_t* buf, int len);
static int sfdp_parse_bfpt(snor_chip_t* const chip, sfdp_phdr_t* const phdr);
static int sfdp_parse_smpt(f3s_dbase_t *const dbase, snor_chip_t *const chip, const sfdp_phdr_t *const phdr);
static int sfdp_parse_4bai(snor_chip_t* const chip, sfdp_phdr_t* const phdr);

/**
 *  @brief             Ident callout for serial NOR flash which supports SFDP.
 *  @param dbase       F3S data base handle.
 *  @param access      F3S access handle.
 *  @param flags       Ident flags.
 *  @param cs          Chip select
 *
 *  @return            EOK --success otherwise fail.
 */
int32_t f3s_sfdp_ident(f3s_dbase_t *dbase, f3s_access_t *access, uint32_t flags, uint32_t cs)
{
    snor_ctrl_t *ctrl;
    snor_chip_t *chip;
    int         status;
    size_t      sz;
    sfdp_hdr_t  hdr;
    sfdp_phdr_t *phdr;
    sfdp_phdr_t *bfpthdr;

    if (access == NULL) return (ENODEV);

    ctrl = (snor_ctrl_t *)access->socket.memory;
    chip = &ctrl->chip[cs];

    status = snor_read_sfdp(chip, 0, (uint8_t *)&hdr, sizeof(hdr));
    if (status != EOK) return (status);
    if ((hdr.sfdp != SFDP_SIGNATURE) || (hdr.major != SFDP_JESD216_MAJOR)) {
        snor_slogf(_SLOG_ERROR, ctrl->verbosity, _SLOG_ERROR,
            "(devf  t%d::%s) SFDP signature incorrect: %08x : %02x",
                pthread_self(), __func__, hdr.sfdp, hdr.major);
        return (ENODEV);
    }

    if (hdr.nph == 0) {
        snor_slogf(_SLOG_ERROR, ctrl->verbosity, _SLOG_ERROR,
            "(devf  t%d::%s) No parameter table available", pthread_self(), __func__);
        return (ENODEV);
    }

    /*
     * Verify that the first and only mandatory parameter header is a
     * Basic Flash Parameter Table header as specified in JESD216.
     */
    bfpthdr = &hdr.bfpthdr;
    if ((SFDP_PHDR_ID(bfpthdr) != SFDP_BFPT_ID) ||
            (bfpthdr->major != SFDP_JESD216_MAJOR)) {
        snor_slogf(_SLOG_ERROR, ctrl->verbosity, _SLOG_ERROR,
            "(devf  t%d::%s) Invalid parameter header", pthread_self(), __func__);
        return (ENODEV);
    }

    sz = (size_t)hdr.nph * sizeof(sfdp_phdr_t);
    phdr = malloc(sz);
    if (phdr == NULL) {
        return (ENOMEM);
    }

    status = snor_read_sfdp(chip, sizeof(hdr), (uint8_t *)phdr, (int)sz);
    if (status != EOK) return (status);

    /* Basic parameter table */
    sfdp_phdr_t *phdrtmp;
    for (uint8_t i = 0; i < hdr.nph; i++) {
        phdrtmp = &phdr[i];
        if ((SFDP_PHDR_ID(phdrtmp) == SFDP_BFPT_ID) && (phdrtmp->major == SFDP_JESD216_MAJOR) &&
            ((phdrtmp->minor > bfpthdr->minor) ||
            ((phdrtmp->minor == bfpthdr->minor) && (phdrtmp->pl> bfpthdr->pl)))) {

            bfpthdr = phdrtmp;
        }
    }

    if (sfdp_parse_bfpt(chip, bfpthdr) != EOK) return (ENODEV);

    /* Parse optional parameter tables. */
    for (uint8_t i = 0; i < hdr.nph; i++) {
        phdrtmp = &phdr[i];
        switch (SFDP_PHDR_ID(phdrtmp)) {
            case SFDP_4BAIT_ID:
                status = sfdp_parse_4bai(chip, phdrtmp);
                break;
            case SFDP_SMPT_ID:
                status = sfdp_parse_smpt(dbase, chip, phdrtmp);
                break;
            default:
                break;
        }
    }

    free(phdr);

    if ((status == EOK) && (dbase != NULL)) {
        dbase->name = "SFDP";
    }

    return (status);
}

/**
 *  @brief             Read SFDP data block.
 *  @param chip        Chip handle.
 *  @param off         Offset into SFDP data block.
 *  @param buf         SFDP buffer.
 *  @param len         Read length.
 *
 *  @return            EOK --success otherwise fail.
 */
static int snor_read_sfdp(snor_chip_t* const chip, int off, uint8_t* buf, int len)
{
    snor_ctrl_t* const ctrl = chip->ctrl;
    snor_cmd_t  cmd;
    snor_op_t   rdsfdp = { .opcode = SNOR_CMD_RDSFDP, .dcycle = 8, .adrlen = (chip->cfg.bus_proto == SNOR_BUSPROTO_8_8_8_DTR) ? 4 : 3 };
    int         rlen;

    if (ctrl->funcs.read != NULL) {
        do {
            SNOR_SET_CMD(cmd, &rdsfdp, &chip->cfg, (uint32_t)off);
            rlen = ctrl->funcs.read(ctrl, &cmd, buf, len);
            if (rlen <= 0) return (EIO);
            off += rlen;
            buf += rlen;
            len -= rlen;
        } while (len > 0);

        return (EOK);
    }


    return (ENODEV);
}

/*
 * JESD216D.01
 * Parse basic parameter table
 *  DWORD #1 : Uniform 4KB Sectors, Write Buffer Size, Volatile Status Register,
 *             Fast Read Support (1-1-2) (1-2-2) (1-4-4)(1-1-4), Number of Address Bytes, DTR Support
 *              bit 0~1   : Block/Sector Erase Sizes
 *              bit 2     : Write Granularity
 *              bit 3     : Volatile Status Register Block Protect bits
 *              bit 4     : Write Enable Instruction Select for Writing to Volatile Status Register
 *              bit 8~15  : 4 Kilobyte Erase Instruction
 *              bit 16    : Supports (1-1-2) Fast Read
 *              bit 17~18 : Address Bytes
 *                          00 : 3-Byte only addressing
 *                          01 : 3- or 4-Byte addressing
 *                          10 : 4-Byte only addressing
 *              bit 19    : Supports Double Transfer Rate (DTR) Clocking
 *              bit 20    : Supports (1-2-2) Fast Read
 *              bit 21    : Supports (1-4-4) Fast Read
 *              bit 22    : Supports (1-1-4) Fast Read
 *  DWORD #2 : Memory Density
 *              For densities 2 gigabits or less, b31 = 0, b0~30: size in bits
 *              For densities over 2 gigabits, b31 = 1, b0~30: 2^N bits
 *  DWORD #3 : Fast Read (1-4-4) (1-1-4): Wait States, Mode Bit Clocks, Instruction
 *              bit 0~4   : (1-4-4) Fast Read Number of Wait states (dummy clocks)
 *              bit 5~7   : (1-4-4) Fast Read Number of Mode Clocks
 *              bit 8~15  : (1-4-4) Fast Read Instruction
 *              bit 16~20 : (1-1-4) Fast Read Number of Wait states (dummy clocks)
 *              bit 21~23 : (1-1-4) Fast Read Number of Mode Clocks
 *              bit 24~31 : (1-1-4) Fast Read Instruction
 *
 *  DOWRD #4 : Fast Read (1-1-2) (1-2-2): Wait States, Mode Bit Clocks, Instruction
 *              bit 0~4   : (1-1-2) Fast Read Number of Wait states (dummy clocks)
 *              bit 5~7   : (1-1-2) Fast Read Number of Mode Clocks
 *              bit 8~15  : (1-1-2) Fast Read Instruction
 *              bit 16~20 : (1-2-2) Fast Read Number of Wait states (dummy clocks)
 *              bit 21~23 : (1-2-2) Fast Read Number of Mode Clocks
 *              bit 24~31 : (1-2-2) Fast Read Instruction
 *  DWORD #5 : Fast Read (2-2-2) (4-4-4) Support
 *              bit 0     : Supports (2-2-2) Fast Read
 *              bit 4     : Supports (4-4-4) Fast Read
 *  DWORD #6 : Fast Read (2-2-2): Wait States, Mode Bit Clocks, Instruction
 *              bit 16~20 : (2-2-2) Fast Read Number of Wait states (dummy clocks)
 *              bit 21~23 : (2-2-2) Fast Read Number of Mode Clocks
 *              bit 24~31 : (2-2-2) Fast Read Instruction
 *  DWORD #7 : Fast Read (4-4-4): Wait States, Mode Bit Clocks, Instruction
 *              bit 16~20 : (4-4-4) Fast Read Number of Wait states (dummy clocks)
 *              bit 21~23 : (4-4-4) Fast Read Number of Mode Clocks
 *              bit 24~31 : (4-4-4) Fast Read Instruction
 *  DWORD #8 : Erase Type 1 & 2 Size and Instruction
 *              bit 0~7   : Erase Type 1 Size, erase type size = 2^N bytes
 *              bit 8~15  : Erase Type 1 Instruction
 *              bit 16~23 : Erase Type 2 Size, erase type size = 2^N bytes
 *              bit 24~31 : Erase Type 2 Instruction
 *  DWORD #9 : Erase Type 3 & 4 Size and Instruction
 *              bit 0~7   : Erase Type 4 Size, erase type size = 2^N bytes
 *              bit 8~15  : Erase Type 4 Instruction
 *              bit 16~23 : Erase Type 3 Size, erase type size = 2^N bytes
 *              bit 24~31 : Erase Type 3 Instruction
 *  DWORD #10: Erase Type (1:4) Typical Erase Times and Multiplier Used To Derive Max Erase Times
 *              bit 0~3   : Multiplier from typical erase time to maximum erase time
 *                          Formula: Erase Type n (or Chip) erase maximum time = 2 * (count + 1) * Erase Type n (or Chip) erase typical time
 *              bit 4~10  : Erase Type 1 Erase, Typical time
 *                          10~9    units (00b: 1 ms, 01b: 16 ms, 10b: 128 ms, 11b: 1 s)
 *                          8~4     count
 *                          Formula: typical time = (count + 1)*units
 *              bit 11~17 : Erase Type 2 Erase, Typical time
 *              bit 18~24 : Erase Type 3 Erase, Typical time
 *              bit 25~31 : Erase Type 4 Erase, Typical time
 *  DWORD #11: Chip Erase Typical Time, Byte Program and Page Program Typical Times, Page Size
 *              bit 0~3   : Multiplier from typical time to max time for Page or byte program
 *                          Formula: maximum time = 2 * (count + 1)*typical time
 *              bit 4~7   : Page Size
 *              bit 8~13  : Page Program Typical time
 *                          13 units (0: 8 μs, 1: 64 μs)
 *                          12~8 count
 *                          Formula: typical page program time = (count + 1)*units
 *              bit 14~18 : Byte Program Typical time, first byte
 *              bit 19~23 : Byte Program Typical time, additional byte
 *              bit 24~30 : Chip Erase, Typical time
 *  DWORD #12: Erase/Program Suspend/Resume Support, Intervals, Latency, Keep Out Area Size
 *              bit 0~3   : Prohibited Operations During Program Suspend
 *              bit 4~7   : Prohibited Operations During Erase Suspend
 *              bit 9~12  : Program Resume to Suspend Interval
 *                          Formula: program resume to suspend interval = (count + 1)*64 μs
 *              bit 13~19 : Suspend in-progress program max latency
 *                          19~18 units (00b: 128ns, 01b: 1μs, 10b: 8μs, 11b: 64μs)
 *                          17~13 count
 *                          Formula: suspend in-progress program max latency = (count+1)*units
 *              bit 20~23 : Erase Resume to Suspend Interval
 *                          Formula: erase resume to suspend interval = (count + 1)*64 μs
 *              bit 24~30 : Suspend in-progress erase max latency
 *                          30:29 units (00b: 128ns, 01b: 1μs, 10b: 8μs, 11b: 64μs)
 *                          28:24 count
 *                          Formula: erase max latency = (count + 1)*units
 *              bit 31    : Suspend / Resume supported, 0: supported, 1: not supported
 *  DWORD #13: Program/Erase Suspend/Resume Instructions
 *              bit 0~7   : Program Resume Instruction
 *              bit 8~15  : Program Suspend Instruction
 *              bit 16~23 : Resume Instruction
 *              bit 24~31 : Suspend Instruction
 *              bit 8~15  : Program Suspend Instruction
 *  DWORD #14: Deep Powerdown and Status Register Polling Device Busy
 *              bit 2     : Use of legacy polling is supported by reading the Status Register
 *                          with 05h instruction and checking write in progress bit[0] (0=ready; 1=busy).
 *              bit 3     : Bit 7 of the Flag Status Register may be polled any time a Program,
 *                          Erase, Suspend/Resume command is issued, or after a Reset command
 *                          while the device is busy. The read instruction is 70h.
 *                          Flag Status Register bit definitions: bit[7]: Program or erase controller status (0=busy; 1=ready)
 *  DWORD #15: Hold and WP Disable Function, Quad Enable Requirements, 4-4-4 Mode Enable/Disable Sequences, 0-4-4 Entry/Exit Methods and Support
 *  DWORD #16: 32-bit Address Entry/Exit Methods and Support, Soft Reset and Rescue Sequences, Volatile and Nonvolatile Status Register Support
 *  DWORD #17: Fast Read (1-8-8) (1-1-8): Wait States, Mode Bit Clocks, Instruction
 *              bit 0~4   : (1-8-8) Fast Read Number of Wait states (dummy clocks)
 *              bit 5~7   : (1-8-8) Fast Read Number of Mode Clocks
 *              bit 8~15  : (1-8-8) Fast Read Instruction
 *              bit 16~20 : (1-1-8) Fast Read Number of Wait states (dummy clocks)
 *              bit 21~23 : (1-1-8) Fast Read Number of Mode Clocks
 *              bit 24~31 : (1-1-8) Fast Read Instruction
 *  DWORD #18: Octal commands, Byte order, Data strobe, JEDEC SPI Protocol Reset
 *              bit 18~22 : Variable Output Driver Strength
 *              bit 23    : JEDEC SPI Protocol Reset (In-Band Reset)
 *              bit 24~25 : Data Strobe Waveforms in STR Mode
 *              bit 26    : Data Strobe support for QPI STR mode (4S-4S-4S)
 *              bit 27    : Data Strobe support for QPI DTR mode (4S-4D-4D)
 *              bit 29~30 : Octal DTR (8D-8D-8D) Command and Command Extension
 *              bit 31    : Byte Order in 8D-8D-8D mode
 *  DWORD #19: Octal Enable Requirements, 8-8-8 Mode Enable/Disable Sequences, 0-8-8 Entry/Exit Methods and Support
 *              bit 0~3   : 8s-8s-8s mode disable sequences
 *              bit 4~8   : 8s-8s-8s mode enable sequences
 *              bit 20~22 : Octal Enable Requirements:
 *                          000b: Device does not have an Octal Enable bit.
 *                          001b: Octal Enable is bit 3 of status register 2. It is set via Write status register 2
 *                          instruction 31h with one data byte where bit 3 is one.
 *                          It is cleared via Write status register 2 instruction 3Eh with one data byte where bit 3
 *                          is zero. The status register 2 is read using instruction 65h with address byte 02h and one dummy byte.
 *  DWORD #20: Maximum operating speeds
 *              bit 0~3   : Maximum operation speed of device in 4S-4S-4S mode when not utilizing Data Strobe
 *              bit 4~7   : Maximum operation speed of device in 4S-4S-4S mode when utilizing Data Strobe
 *              bit 8~11  : Maximum operation speed of device in 4S-4D-4D mode when not utilizing Data Strobe
 *              bit 12~15 : Maximum operation speed of device in 4S-4D-4D mode when utilizing Data Strobe
 *              bit 16~19 : Maximum operation speed of device in 8S-8S-8S mode when not utilizing Data Strobe
 *              bit 20~23 : Maximum operation speed of device in 8S-8S-8S mode when utilizing Data Strobe
 *              bit 24~27 : Maximum operation speed of device in 8D-8D-8D mode when not utilizing Data Strobe
 *              bit 28~31 : Maximum operation speed of device in 8D-8D-8D mode when utilizing Data Strobe
 *                          speed table :   1100b: 400 MHz
 *                                          1011b: 333 MHz
 *                                          1010b: 266 MHz
 *                                          1001b: 250 MHz
 *                                          1000b: 200 MHz
 *                                          0111b: 166 MHz
 *                                          0110b: 133 MHz
 *                                          0101b: 100 MHz
 *                                          0100b: 80 MHz
 *                                          0011b: 66 MHz
 *                                          0010b: 50 MHz
 *                                          0001b: 33 MHz
 */
static inline void sfdp_fill_rdop(snor_chip_t* const chip, const enum snor_busproto_idx proto,
                const uint64_t cap, const uint32_t* const wptr, const int flgw, const int flgb, const int opw, const int opo)
{
    if ((wptr[flgw] & (1 << (flgb)))) {
        chip->hcaps |= cap;
        chip->rdops[proto].opcode = (uint8_t)((wptr[opw] >> (opo + 8)) & 0xff);
        chip->rdops[proto].dcycle = (uint8_t)(((wptr[opw] >> opo) & 0x1F) + ((wptr[opw] >> (opo + 5)) & 0x07));
    }
}

/**
 *  @brief             Fill erase information.
 *  @param chip        Chip handle.
 *  @param wptr        Pointer to erase parameter words.
 *  @param idx         Erase type index.
 *  @param oc          Erase info offset.
 *  @param os          Erase info shift.
 *  @param to          Timeout info offset.
 *  @param ts          Timeout info shift.
 *  @param tblen       Table length.
 *
 *  @return            EOK --success otherwise fail.
 */
static inline void sfdp_fill_einfo(snor_chip_t* chip, const uint32_t* const wptr,
                const int idx, const int oc, const int os, const uint8_t to, const uint8_t ts, const uint8_t tblen)
{
    const snor_ctrl_t* const ctrl = chip->ctrl;

    chip->blkers[idx].blksz_pow2 = (uint8_t)((wptr[oc] >> (os)) & 0xFF);
    chip->blkers[idx].opcode     = (uint8_t)((wptr[oc] >> (os + 8)) & 0xFF);

    snor_slogf(_SLOG_INFO, ctrl->verbosity, _SLOG_INFO, "%s: blkers %d: blksz_pow2 %2d, opcode 0x%02X",
                __func__, idx, chip->blkers[idx].blksz_pow2, chip->blkers[idx].opcode);

    if (tblen > to) {
        chip->blkers[idx].met = (uint16_t)((wptr[to]) & 0x0F);

        switch ((wptr[to] >> (5 + ts)) & 3) {
            case 0:
                chip->blkers[idx].tet = 1;
                break;
            case 1:
                chip->blkers[idx].tet = 16;
                break;
            case 2:
                chip->blkers[idx].tet = 128;
                break;
            case 3:
                chip->blkers[idx].tet = 1000;
                break;
            default:
                break;
        }
        chip->blkers[idx].tet *= (wptr[to] >> (ts)) & 0x1F;
    }
}

/**
 *  @brief             Parse basic flash parameter table.
 *  @param chip        Chip handle.
 *  @param phdr        Parameter header pointer.
 *
 *  @return            EOK --success otherwise fail.
 */
static int sfdp_parse_bfpt(snor_chip_t* const chip, sfdp_phdr_t* const phdr)
{
    uint32_t    dwords[SFDP_TBLSZ];
    int         status;
    uint8_t     tblen = phdr->pl;

    if (tblen < BFPT_DWORD_MAX_JESD216) return (EINVAL);
    if (tblen > SFDP_TBLSZ) {
        tblen = SFDP_TBLSZ;
    }

    status = snor_read_sfdp(chip, SFDP_PHDR_PTP(phdr), (uint8_t *)&dwords[0], (int)tblen * 4);
    if (status != EOK) return (status);

    static const uint32_t   adrm[4] = { 0, SNOR_DCAPS_ADDR_3B_4B, SNOR_DCAPS_ADDR_4B, 0 };
    chip->dcaps |= adrm[(dwords[SFDP_DWORD(1)] >> 17) & 3];

    /* read OPs */
    sfdp_fill_rdop(chip, SNOR_BPI_1_1_2, SNOR_HCAPS_RD_1_1_2, dwords, 0, 16, 3,  0);
    sfdp_fill_rdop(chip, SNOR_BPI_1_2_2, SNOR_HCAPS_RD_1_2_2, dwords, 0, 20, 3, 16);
    sfdp_fill_rdop(chip, SNOR_BPI_1_4_4, SNOR_HCAPS_RD_1_4_4, dwords, 0, 21, 2,  0);
    sfdp_fill_rdop(chip, SNOR_BPI_1_1_4, SNOR_HCAPS_RD_1_1_4, dwords, 0, 22, 2, 16);
    sfdp_fill_rdop(chip, SNOR_BPI_2_2_2, SNOR_HCAPS_RD_2_2_2, dwords, 4,  0, 5, 16);
    sfdp_fill_rdop(chip, SNOR_BPI_4_4_4, SNOR_HCAPS_RD_4_4_4, dwords, 4,  4, 6, 16);

    /* DWORD #1: DTR */
    if (dwords[SFDP_DWORD(1)] & (1u << 19)) {
        chip->hcaps |= SNOR_HCAPS_DTR_CMD;
    }

    /* DWORD #2: Chip density */
    if (dwords[SFDP_DWORD(2)] & (1u << 31)) {
        chip->chipsz = (1u << ((dwords[SFDP_DWORD(2)] & 0x7FFFFFFF) - 3));
    } else {
        chip->chipsz = (dwords[SFDP_DWORD(2)] + 1) >> 3;
    }

    /* Fill erase types */
    sfdp_fill_einfo(chip, dwords, 0, 7,  0, 9,  4, tblen);
    sfdp_fill_einfo(chip, dwords, 1, 7, 16, 9, 11, tblen);
    sfdp_fill_einfo(chip, dwords, 2, 8,  0, 9, 18, tblen);
    sfdp_fill_einfo(chip, dwords, 3, 8, 16, 9, 25, tblen);

    if (tblen <= BFPT_DWORD_MAX_JESD216) return EOK;

    /* DWORD #11: program info */
    chip->pagesz = (uint16_t)(1u << ((dwords[SFDP_DWORD(11)] >> 4) & 0x0F));
    chip->pptt   = (uint16_t)(((dwords[SFDP_DWORD(11)] >> 8) & 0x1F) + 1);
    chip->pptt  *= (uint16_t)((dwords[SFDP_DWORD(11)] & (1 << 13)) ? 64 : 8);   /* in us */
    chip->ppmt   = (uint16_t)((dwords[SFDP_DWORD(11)] & 0x0F) + 1);  /* multiplier from typical time to max time */

    /* DWORD #12: Erase/Program Suspend/Resume Support. bit 31, 0: supported, 1: not supported */
    if ((dwords[SFDP_DWORD(12)] & (1 << 31)) == 0) {
        chip->dcaps |= SNOR_DCAPS_PSR | SNOR_DCAPS_ESR;
        /* DWORD #13: Program/Erase Suspend/Resume Instructions */
        chip->op_pr = (uint8_t)(dwords[SFDP_DWORD(13)] & 0xFF);
        chip->op_ps = (uint8_t)((dwords[SFDP_DWORD(13)] >> 8) & 0xFF);
        chip->op_er = (uint8_t)((dwords[SFDP_DWORD(13)] >> 16) & 0xFF);
        chip->op_es = (uint8_t)((dwords[SFDP_DWORD(13)] >> 24) & 0xFF);
    }

    /* DWORD #12: Erase/Program: Resume to Suspend Interval, to Suspend Latency */
    static const uint32_t sml[4] = { 128, 1000, 8 * 1000, 64 * 1000 };
    chip->e2sl  = sml[(dwords[SFDP_DWORD(12)] >> 29) & 3] * (((dwords[SFDP_DWORD(12)] >> 24) & 0x1F) + 1);
    chip->er2si = (((dwords[SFDP_DWORD(12)] >> 20) & 0x0F) + 1) * 64;
    chip->p2sl  = sml[(dwords[SFDP_DWORD(12)] >> 18) & 3] * (((dwords[SFDP_DWORD(12)] >> 13) & 0x1F) + 1);
    chip->pr2si = (((dwords[SFDP_DWORD(12)] >> 9) & 0x0F) + 1) * 64;

    /* DWORD #14: Status Register Polling */
    if (dwords[SFDP_DWORD(14)] & (1 << 2)) {
//        fprintf(stderr, "Legacy status polling\n");
    }
    if (dwords[SFDP_DWORD(14)] & (1 << 3)) {
        chip->dcaps |= SNOR_DCAPS_FSTATUS;
    }

    /* DWORD #15: quad mode related */
    chip->qer = (uint8_t)((dwords[SFDP_DWORD(15)] >> 20) & 7);
    chip->qes = (uint8_t)((dwords[SFDP_DWORD(15)] >> 4) & 0x1F);
    chip->qds = (uint8_t)((dwords[SFDP_DWORD(15)] >> 0) & 0x0F);

    /* DWORD #16: enter 4B address protocol etc */
    chip->e4ba = (uint8_t)((dwords[SFDP_DWORD(16)] >> 24) & 0xFF);
    chip->x4ba = (uint16_t)((dwords[SFDP_DWORD(16)] >> 14) & 0x3FF);
    chip->ssrs = (uint8_t)((dwords[SFDP_DWORD(16)] >> 8) & 0x3F);
    chip->asr1 = (uint8_t)((dwords[SFDP_DWORD(16)] >> 0) & 0x7F);

    if (tblen <= BFPT_DWORD_MAX_JESD216B) return (EOK);

    /* DWORD #17: 1-1-8 and 1-8-8 support */
    chip->rdops[SNOR_BPI_1_1_8].opcode = (uint8_t)((dwords[SFDP_DWORD(17)] >> 24) & 0xFF);
    if (chip->rdops[SNOR_BPI_1_1_8].opcode != 0) {
        chip->hcaps |= SNOR_HCAPS_RD_1_1_8;
        chip->rdops[SNOR_BPI_1_1_8].dcycle = (uint8_t)(((dwords[SFDP_DWORD(17)] >> 16) & 0x1F) + ((dwords[SFDP_DWORD(17)] >> 21) & 0x07));
    }

    chip->rdops[SNOR_BPI_1_8_8].opcode = (uint8_t)((dwords[SFDP_DWORD(17)] >> 8) & 0xFF);
    if (chip->rdops[SNOR_BPI_1_8_8].opcode != 0) {
        chip->hcaps |= SNOR_HCAPS_RD_1_8_8;
        chip->rdops[SNOR_BPI_1_8_8].dcycle = (uint8_t)(((dwords[SFDP_DWORD(17)] >> 0) & 0x1F) + ((dwords[SFDP_DWORD(17)] >> 5) & 0x07));
    }

    // Add DWORD18 support
    // DQS etc
    // Add DWORD19 support
    // Octal mode etc
    // Add DWORD20 support
    // quad/octal bus speed limit

    return (EOK);
}

/**
 *  @brief             Return address length in bytes for the config detection command.
 *  @param desc        Configuration detection command descriptor 1st DWORD.
 *
 *  @return            The number of address bytes for an SMPT read.
 */
static uint8_t sfdp_smpt_addr_len(const uint32_t desc)
{
    switch (desc & SFDP_SMPT_CMD_ADDR_LEN_MSK) {
        case SFDP_SMPT_CMD_ADDR_LEN_0:
            return 0;
        case SFDP_SMPT_CMD_ADDR_LEN_3:
            return 3;
        case SFDP_SMPT_CMD_ADDR_LEN_4:
            return 4;
        case SFDP_SMPT_CMD_ADDR_LEN_USE_CURR:
        default:
            return 3;
    }
}

/**
 *  @brief             Return the config detection command read dummy cycles.
 *  @param chip        Chip handle.
 *  @param desc        Config detection command descriptor 1st DWORD.
 *
 *  @return            The number of SMPT read dummy cycles.
 */
static uint8_t sfdp_smpt_rd_dc(snor_chip_t *const chip, const uint32_t desc)
{
    const uint8_t rd_dc = SFDP_SMPT_CMD_RD_DC(desc);

    if (rd_dc == SFDP_SMPT_CMD_RD_DC_IS_VAR) {
        return chip->rdr_dc;
    }
    return rd_dc;
}

/**
 *  @brief             Find the current in use configuration map.
 *  @param chip        Chip handle.
 *  @param dwords      Pointer to the sector map parameter table.
 *  @param tblen       Sector map parameter table length.
 *
 *  @return            Pointer to the current configuration map descriptor header, NULL otherwise.
 */
static const uint32_t *sfdp_find_inuse_config_map(snor_chip_t *const chip, const uint32_t *const dwords, const uint8_t tblen)
{
    const snor_ctrl_t *const ctrl = chip->ctrl;
    const uint32_t    *config_map = NULL;
    uint8_t  idx;
    uint8_t  idx_max;
    int      status;
    uint8_t  alen;
    uint8_t  opcode;
    uint8_t  dc;
    uint8_t  data_msk;
    uint8_t  config_id = 0;
    uint8_t  data;
    uint32_t addr;

    /* Get chip read register dummy cycles */
    dc = chip->rdr_dc;

    /* The max SMPT tblen(uint8_t) is 255 so max dwords index would be 254.
     * Each config detection command descriptor has 2 dwords.
     * Therefore to search config detection command descriptors the max index would be 254 - 2 = 252.
     */
    idx_max = min(UCHAR_MAX_IDX - SFDP_SMPT_CONFIG_DETCMD_DESC_LEN, tblen);
    /* Search config detection command descriptors */
    for (idx = 0; idx <= idx_max; idx += SFDP_SMPT_CONFIG_DETCMD_DESC_LEN) {
        if (dwords[idx] & SFDP_SMPT_DESC_TYPE_MAP) {
            snor_slogf(_SLOG_DEBUG1, ctrl->verbosity, _SLOG_DEBUG1, "%s: dwords[%d] MAP desc", __func__, idx);
            break;
        } else {
            snor_slogf(_SLOG_DEBUG1, ctrl->verbosity, _SLOG_DEBUG1, "%s: dwords[%d] CMD desc", __func__, idx);
        }

        data_msk = SFDP_SMPT_CMD_RD_DATA(dwords[idx]);
        alen = sfdp_smpt_addr_len(dwords[idx]);
        chip->rdr_dc = sfdp_smpt_rd_dc(chip, dwords[idx]);
        opcode = SFDP_SMPT_CMD_OP(dwords[idx]);

        /* Sector map config detection command address: Config Detection Command Descriptor 2nd DWORD */
        addr = dwords[idx + 1];

        /* Get chip current configuration */
        status = snor_read_register(chip, opcode, addr, alen, &data, sizeof(data));
        if (status != EOK) {
            snor_slogf(_SLOG_ERROR, ctrl->verbosity, _SLOG_ERROR, "%s: snor_read_register faied", __func__);
            chip->rdr_dc = dc;
            return NULL;
        }

        snor_slogf(_SLOG_DEBUG1, ctrl->verbosity, _SLOG_DEBUG1, "%s: addr 0x%x, data 0x%x, mask 0x%x", __func__, addr, data, data_msk);

        /* Set configuration selector value.
         * Each detected configuration bit is shifted left into the configuration selector value
         * such that the last detected bit is in the least significant bit of the selector value.
         * to be config selector value
         */
        config_id = (!!(data & data_msk)) | (config_id << 1);
    }

    /* Search config map descriptor which always follow the config detection command descriptor.
     * Since each config map desc would be followed by at least one region dword,
     * to search config detection command descriptor the max index would be 254 - 1 = 253.
     */
    idx_max = min(UCHAR_MAX_IDX - 1, tblen);
    while (idx < idx_max) {
        /* Found the current selected config map */
        if (SFDP_SMPT_CONFIG_ID(dwords[idx]) == config_id) {
            config_map = dwords + idx;
            snor_slogf(_SLOG_DEBUG1, ctrl->verbosity, _SLOG_DEBUG1, "%s: Found matched config ID %d", __func__, config_id);
            break;
        }

        /* There is no config map descriptors and didn't find config selector */
        if (dwords[idx] & SFDP_SMPT_DESC_END) {
            snor_slogf(_SLOG_ERROR, ctrl->verbosity, _SLOG_ERROR, "%s: dwords[%d] is unknown", __func__, idx);
            break;
        }

        /* Skip current config map desc and its regions. Move to the next config map desc */
        idx += 1u + SFDP_SMPT_REGION_CNT(dwords[idx]);
    }

    chip->rdr_dc = dc;

    return config_map;
}

/* Convert erase type to eid which is the index in blkers array */
static uint8_t sfdp_smpt_etype_to_eid(const uint8_t etype)
{
    switch (etype) {
        case (1 << 3):
            return 3;
        case (1 << 2):
            return 2;
        case (1 << 1):
            return 1;
        case (1 << 0):
        default:
            return 0;
    }
}

/* Function to check if size is power of 2 */
static bool sfdp_smpt_ispow2(const uint64_t size)
{
    if (size == 0) return false;

    return (ceil(log2(size)) == floor(log2(size)));
}

/**
 *  @brief             Parse erase map regions to build flash dbase.
 *  @param chip        Chip handle.
 *  @param config_map  Pointer to the current config map descriptors
 *
 *  @return            EOK --success otherwise fail.
 */
static int sfdp_build_dbase(f3s_dbase_t *const dbase, snor_chip_t *const chip, const uint32_t *const config_map)
{
    uint8_t            region_cnt = 0;
    uint8_t            idx;
    uint8_t            region_idx;
    uint8_t            etype = 0;
    uint64_t           offset = 0;
    sfdp_smpt_region_t *region = NULL;
    const snor_ctrl_t  *const ctrl = chip->ctrl;

    region_cnt = min((uint8_t)SNOR_MAX_GEONUM, SFDP_SMPT_REGION_CNT(*config_map));
    snor_slogf(_SLOG_INFO, ctrl->verbosity, _SLOG_INFO, "%s: There are %d regions", __func__, region_cnt);

    region = calloc(region_cnt, sizeof(sfdp_smpt_region_t));
    if (region == NULL) {
        snor_slogf(_SLOG_ERROR, ctrl->verbosity, _SLOG_ERROR, "%s: Failed to alloc mem for regions", __func__);
        return (ENOMEM);
    }

    /* Parse regions */
    for (idx = 0; idx < region_cnt; idx ++) {
        /* Region dword follows config map descriptor header */
        region_idx = idx + 1;

        region[idx].offset = offset;
        region[idx].size = SFDP_SMPT_REGION_SZ(config_map[region_idx]);
        etype = SFDP_SMPT_REGION_ETYPE(config_map[region_idx]);

        snor_slogf(_SLOG_INFO, ctrl->verbosity, _SLOG_INFO, "%s: Region %d: Offset 0x%08x, Size 0x%08x, Erase Type %d",
                                    __func__, idx, region[idx].offset, region[idx].size, sfdp_smpt_etype_to_eid(etype) + 1);

        /* Erase type index in chip->blkers array */
        chip->eid[idx] = sfdp_smpt_etype_to_eid(etype);

        /* Get unit num and pow2 */
        dbase->geo_vect[idx].unit_num  = (uint16_t)(region[idx].size / (1u << chip->blkers[chip->eid[idx]].blksz_pow2));
        dbase->geo_vect[idx].unit_pow2 = (uint16_t)chip->blkers[chip->eid[idx]].blksz_pow2;

        /* Region size is smaller than 1 << blksz_pow2 which was found via etype */
        if (dbase->geo_vect[idx].unit_num == 0) {
            if (!sfdp_smpt_ispow2(region[idx].size)) {
                /* Non-power of two sector size not supported */
                snor_slogf(_SLOG_ERROR, ctrl->verbosity, _SLOG_ERROR, "%s: Region %d: Size 0x%08x is not power of 2. Not supported", __func__, idx, region[idx].size);
                free (region);
                return (ENOTSUP);
            }
            dbase->geo_vect[idx].unit_pow2 = SIZE2POW2(region[idx].size);
            dbase->geo_vect[idx].unit_num = 1;
        }

        dbase->geo_num ++;
        snor_slogf(_SLOG_INFO, ctrl->verbosity, _SLOG_INFO, "%s: G_vect %d: unit_num %4d, unit_pow2 %2d, eid %d",
                                     __func__, idx, dbase->geo_vect[idx].unit_num, dbase->geo_vect[idx].unit_pow2, chip->eid[idx]);

        /* Move to next region */
        offset = region[idx].offset + region[idx].size;
    }

    free (region);

    return (EOK);
}

/**
 *  @brief             Parse sector map parameter table.
 *  @param chip        Chip handle.
 *  @param phdr        Parameter header pointer.
 *
 *  @return            EOK --success otherwise fail.
 */
static int sfdp_parse_smpt(f3s_dbase_t *const dbase, snor_chip_t *const chip, const sfdp_phdr_t *const phdr)
{
    const uint8_t     tblen = phdr->pl;
    uint32_t          *dwords;
    const uint32_t    *config_map;
    int               status;
    const snor_ctrl_t *const ctrl = chip->ctrl;

    if ((phdr->major != SFDP_JESD216_MAJOR) || (tblen < SFDP_SMPT_DWORD_MIN)) {
        return (EINVAL);
    }

    /* Allocate smpt dwords */
    dwords = calloc(tblen, sizeof(uint32_t));
    if (dwords == NULL) {
        snor_slogf(_SLOG_ERROR, ctrl->verbosity, _SLOG_ERROR, "%s: Could not allocate smpt memory", __func__);
        return (ENOMEM);
    }

    snor_slogf(_SLOG_INFO, ctrl->verbosity, _SLOG_INFO, "%s: SMPT length %d DWORDS", __func__, tblen);

    /* Read sector map parameter table */
    status = snor_read_sfdp(chip, SFDP_PHDR_PTP(phdr), (uint8_t *)dwords, (int)(tblen * sizeof(uint32_t)));
    if (status != EOK) {
        free (dwords);
        return (status);
    }

    /* Find the current in use configuration map */
    config_map = sfdp_find_inuse_config_map(chip, dwords, tblen);
    if (config_map == NULL) {
        snor_slogf(_SLOG_ERROR, ctrl->verbosity, _SLOG_ERROR, "%s: Failed to find the current config map", __func__);
        free (dwords);
        return (EIO);
    }

    /* Build dbase from erase regions info */
    status = sfdp_build_dbase(dbase, chip, config_map);

    free (dwords);
    return (status);
}


/**
 *  @brief             Parse 4-bytes address instruction parameter table.
 *  @param chip        Chip handle.
 *  @param phdr        Parameter header pointer.
 *
 *  @return            EOK --success otherwise fail.
 */
static int sfdp_parse_4bai(snor_chip_t* const chip, sfdp_phdr_t* const phdr)
{
    uint32_t    dwords[SFDP_4BAIT_DWORD_MAX];
    int         status;

    if ((phdr->major != SFDP_JESD216_MAJOR) || (phdr->pl < SFDP_4BAIT_DWORD_MAX)) return (EINVAL);

    status = snor_read_sfdp(chip, SFDP_PHDR_PTP(phdr), (uint8_t *)&dwords[0], (int)sizeof(dwords));
    if (status != EOK) return (status);

    chip->dcaps |= dwords[SFDP_DWORD(1)] & 0x01F0E1FF;      /* 4B read/pp capabilities */

    /* sector erase */
    for (int i = 0; i <= 3; i++) {
        if (dwords[SFDP_DWORD(1)] & (1 << (i + 9))) {
            chip->blkers[i].opcode_4b = (uint8_t)((dwords[SFDP_DWORD(2)] >> (i * 8)) & 0xFF);
        }
    }

    return (EOK);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
