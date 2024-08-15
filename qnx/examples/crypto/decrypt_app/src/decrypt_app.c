/*
 *
 * Copyright (c) 2018-2021 Texas Instruments Incorporated
 *
 * All rights reserved not granted herein.
 *
 * Limited License.
 *
 * Texas Instruments Incorporated grants a world-wide, royalty-free, non-exclusive
 * license under copyrights and patents it now or hereafter owns or controls to make,
 * have made, use, import, offer to sell and sell ("Utilize") this software subject to the
 * terms herein.  With respect to the foregoing patent license, such license is granted
 * solely to the extent that any such patent is necessary to Utilize the software alone.
 * The patent license shall not apply to any combinations which include this software,
 * other than combinations with devices manufactured by or for TI ("TI Devices").
 * No hardware patent is licensed hereunder.
 *
 * Redistributions must preserve existing copyright notices and reproduce this license
 * (including the above copyright notice and the disclaimer and (if applicable) source
 * code license limitations below) in the documentation and/or other materials provided
 * with the distribution
 *
 * Redistribution and use in binary form, without modification, are permitted provided
 * that the following conditions are met:
 *
 * *       No reverse engineering, decompilation, or disassembly of this software is
 * permitted with respect to any software provided in binary form.
 *
 * *       any redistribution and use are licensed by TI for use only with TI Devices.
 *
 * *       Nothing shall obligate TI to provide you with source code for the software
 * licensed and provided to you in object code.
 *
 * If software source code is provided to you, modification and redistribution of the
 * source code are permitted provided that the following conditions are met:
 *
 * *       any redistribution and use of the source code, including any resulting derivative
 * works, are licensed by TI for use only with TI Devices.
 *
 * *       any redistribution and use of any object code compiled from the source code
 * and any resulting derivative works, are licensed by TI for use only with TI Devices.
 *
 * Neither the name of Texas Instruments Incorporated nor the names of its suppliers
 *
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * DISCLAIMER.
 *
 * THIS SOFTWARE IS PROVIDED BY TI AND TI'S LICENSORS "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TI AND TI'S LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
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
#include <stdint.h>

#include "ti/drv/sciclient/sciclient.h"
#include "ti/csl/csl_types.h"
#if defined (SOC_J721E)
#include "ti/drv/sciclient/soc/V1/sciclient_fmwMsgParams.h"
#elif defined (SOC_J7200)
#include "ti/drv/sciclient/soc/V2/sciclient_fmwMsgParams.h"
#elif defined (SOC_J721S2)
#include "ti/drv/sciclient/soc/V4/sciclient_fmwMsgParams.h"
#elif defined (SOC_AM62X)
#include "ti/drv/sciclient/soc/V5/sciclient_fmwMsgParams.h"
#elif defined (SOC_J784S4)
#include "ti/drv/sciclient/soc/V6/sciclient_fmwMsgParams.h"
#elif defined (SOC_AM62A)
#include "ti/drv/sciclient/soc/V7/sciclient_fmwMsgParams.h"
#elif defined (SOC_J722S)
#include "ti/drv/sciclient/soc/V9/sciclient_fmwMsgParams.h"
#else
#error "unsupported SOC"
#endif


/* Global variable declarations */
uint32_t verbose = 0;
char     filename[256];

/* Forward function declarations */
void *file_read(size_t *file_size);
uint64_t virtToPhy(const void *virtAddr, uint32_t size);



uint8_t *FindSeq(uint8_t *x509_cert_ptr, uint32_t x509_cert_size, uint8_t *seq_oid, uint8_t seq_len)
{
    uint8_t *x509_cert_end = x509_cert_ptr + x509_cert_size - seq_len;

    /* searching for the following byte seq in the cert */
    /* seq_id(0x30) seq_len(< 0x80) 0x06 0x09 0x2B...   */
    while (x509_cert_ptr < x509_cert_end)
    {
        if ((*x509_cert_ptr == seq_oid[0]) &&
            (*(x509_cert_ptr + 2) == seq_oid[2]) &&
            (*(x509_cert_ptr - 2) == 0x30))
        {
            if ((memcmp((const void *)x509_cert_ptr, (const void *)seq_oid, seq_len)) == 0)
            {
                /* return start boot_seq */
                return (x509_cert_ptr - 2);
            }
        }
        x509_cert_ptr++;
    }

    return NULL;
}

uint32_t GetMsgLen(uint8_t *x509_cert_ptr, uint32_t x509_cert_size)
{
    uint8_t *boot_seq_ptr;
    uint32_t msg_len = 0, boot_seq_len;
    uint8_t *msg_len_ptr = (uint8_t *)&msg_len;
    /* oid encoding of boot_seq extension - 1.3.6.1.4.1.294.1.1 */
    uint8_t boot_seq_oid[] = {0x06, 0x09, 0x2B, 0x06, 0x01, 0x04, 0x01, 0x82, 0x26, 0x01, 0x01};

    boot_seq_ptr = FindSeq(x509_cert_ptr, x509_cert_size, boot_seq_oid, sizeof(boot_seq_oid));

    /* length of seq is stored in the byte after the 0x30 seq_id */
    /* length of seq is stored as offset of the last byte of seq */
    /* from current offset. Jump to the end of the boot seq as   */
    /* the length of the message  is the last field of this seq  */
    boot_seq_len = *(++boot_seq_ptr);
    boot_seq_ptr = boot_seq_ptr + boot_seq_len;

    /* The last integer in this sequence is the msg length    */
    /* integers are tagged 0x20, so search backwards for 0x20 */
    /* The msg size can be encoded in 1, 2, 3 or 4 bytes      */
    /* 0x02 0x01 0x##                                         */
    /* 0x02 0x02 0x## 0x##                                    */
    /* 0x02 0x03 0x## 0x## 0x##                               */
    /* 0x02 0x04 0x## 0x## 0x## 0x##                          */
    if ( (*(boot_seq_ptr - 5) == 0x02) &&
         (*(boot_seq_ptr - 4) == 0x04) )
    {
        /* msg length encoded in 4 bytes */
        *msg_len_ptr = *boot_seq_ptr;
        *(msg_len_ptr + 1) = *(boot_seq_ptr - 1);
        *(msg_len_ptr + 2) = *(boot_seq_ptr - 2);
        *(msg_len_ptr + 3) = *(boot_seq_ptr - 3);
    }
    else if ( (*(boot_seq_ptr - 4) == 0x02) &&
         (*(boot_seq_ptr - 3) == 0x03) )
    {
        /* msg length encoded in 3 bytes */
        *msg_len_ptr = *boot_seq_ptr;
        *(msg_len_ptr + 1) = *(boot_seq_ptr - 1);
        *(msg_len_ptr + 2) = *(boot_seq_ptr - 2);
    }
    else if ( (*(boot_seq_ptr - 3) == 0x02) &&
         (*(boot_seq_ptr - 2) == 0x02) )
    {
        /* msg length encoded in 2 bytes */
        *msg_len_ptr = *boot_seq_ptr;
        *(msg_len_ptr + 1) = *(boot_seq_ptr - 1);
    }
    else if ( (*(boot_seq_ptr - 2) == 0x02) &&
         (*(boot_seq_ptr - 1) == 0x01) )
    {
        /* msg length encoded in 1 byte */
        *msg_len_ptr = *boot_seq_ptr;
    }

    printf("image length = %d bytes\r\n", msg_len);

    return msg_len;
}

uint32_t GetCertLen(uint8_t *x509_cert_ptr)
{
    uint32_t cert_len = 0;
    uint8_t *cert_len_ptr = (uint8_t *)&cert_len;

    printf("Searching for X509 certificate ...");
    if ( *x509_cert_ptr != 0x30)
    {
        printf("not found\r\n");
        return 0;
    }

    cert_len = *(x509_cert_ptr + 1);

    /* If you need more than 2 bytes to store the cert length  */
    /* it means that the cert length is greater than 64 Kbytes */
    /* and we do not support it                                */
    if ((cert_len > 0x80) &&
        (cert_len != 0x82))
    {
        printf("size invalid\r\n");
        return 0;
    }

    if ( cert_len == 0x82)
    {
        *cert_len_ptr = *(x509_cert_ptr + 3);
        *(cert_len_ptr + 1) = *(x509_cert_ptr + 2);

        /* add current offset from start of x509 cert */
        cert_len += 3;
    }
    else
    {
        /* add current offset from start of x509 cert  */
        /* if cert len was obtained from 2nd byte i.e. */
        /* cert size is 127 bytes or less              */
        cert_len += 1;
    }

    /* cert_len now contains the offset of the last byte */
    /* of the cert from the ccert_start. To get the size */
    /* of certificate, add 1                             */
    printf("size = %d bytes\r\n", cert_len + 1);


    return cert_len + 1;
}


void *file_read(size_t *file_size)
{
    FILE       *f;
    size_t      memBlkSize = 0;
    int32_t     status;
    void        *buf;

    f      = NULL;
    status = 0;

    if (!strlen(filename)) {
        printf("Filename is empty.\n");
        status = -1;
    }

    if (status == 0) {
        /* Open the file to read the data. */
        f = fopen(filename, "rb");
        if (!f) {
            printf("Failed to open file <%s>.\n", filename);
            status = -1;
        }
    }

    if (status == 0) {
        /* Get the file size. */
        fseek(f, 0, SEEK_END);
        memBlkSize = (size_t)ftell(f);
        fseek(f, 0, SEEK_SET);

        if (memBlkSize <= 0) {
            printf("Invalid file size: %lu bytes.\n",
                    memBlkSize);
            status = -1;
        }
        else {
            /* Allocate buffer from shared memory location, assign it to buf  */
            if ((buf = mmap64(0, memBlkSize, PROT_READ|PROT_WRITE|PROT_NOCACHE, MAP_ANON | MAP_PHYS | MAP_PRIVATE, NOFD, 0)) == (void *)MAP_FAILED) {
                perror("mmap");
                exit(1);
            }

            *file_size = memBlkSize;
        }

        if (status == 0) {
            size_t  bytesRead;

            bytesRead = fread(buf, 1, memBlkSize, f);
            if (bytesRead != memBlkSize) {
                printf("Could only read %lu bytes of %lu bytes.\n",
                        bytesRead, memBlkSize);
                status = -1;
            }
            if(verbose)
                printf("%s: filename:%s size/%ld bytesRead/%ld\n",__FUNCTION__,filename,memBlkSize,bytesRead);
        }
    }

    if (f) {
        fclose(f);
    }

    // Memory still needs to be freed
    if(status == 0)
        return buf;
    else
        return NULL;
}

/*
 * Print usage information
 */
void printUsage(void)
{
    printf("Usage:                                  \n");
    printf("                                        \n");
    printf("  decrypt_app -f<filename> -v           \n");
    printf("                                        \n");
    printf("  -f     : filename up to 256 characters\n");
    printf("  -v     : enables verbosity            \n");
    printf("                                        \n");

    exit(0);
}

/*
 * Parse Command line options
 */

void parseCmdLine (int argc, char *argv[])
{
    int c;

    printf("%s: entered\n",__FUNCTION__);

    while ((c = getopt (argc, argv, "vf:")) != -1)
    {
        switch (c)
        {
            case 'v':
                verbose = 1;
                break;
            case 'f':
                strcpy(filename, optarg);
                break;
            case '?':
                printUsage();
                break;
            default:
                printUsage();
        }
    }
}

uint64_t virtToPhy(const void *virtAddr,
                  uint32_t size)
{
    int      ret;
    off64_t  phyAddr = 0;
    size_t   contigLen;

    /* Get destination physical address */
    ret = mem_offset64(virtAddr, NOFD, size, &phyAddr, &contigLen);
    if (ret) {
        if (errno != EAGAIN) {
            printf("%s:Error from mem_offset - errno=%d\n", __func__, errno);
        }
        else if (phyAddr == 0) {
            printf("%s:Error from mem_offset - errno=%d and phyAddr is NULL \n", __func__, errno);
        }
    }
    if(verbose)
        printf("%s: virt/0x%016lx phyAddr/0x%016lx, contigLen/%ld\n",__FUNCTION__, *((uint64_t *) virtAddr), phyAddr, contigLen);

    return phyAddr;
}

int main (int argc, char **argv)
{
    int32_t retVal = -1;
    uint8_t x509Header[4];
    uint8_t unaligned_bytes;
    uint8_t pad_align;
    uint8_t *img_ptr = NULL;
    uint8_t *tmp_img_ptr = NULL;
    uint8_t *scratch_mem = NULL;
    uint8_t *scratch_mem_ptr = NULL;
    uint32_t cert_len = 0, img_len;
    uint64_t cert_load_addr = 0;
    uint32_t scratch_sz;
    size_t   file_size;

    /* Get I/O privelege */
    if(ThreadCtl (_NTO_TCTL_IO, 0) == -1)
    {
       printf("ThreadCtl failed\n");
       exit(-1);
    }

    /* Parse command line arguments */
    parseCmdLine(argc, argv);

    /* Open encrypted image from SD card */
    /* The file_read creates a memory location to store the encrypted image */
    /* and returns the virtual pointer in img_ptr  */
    if ((img_ptr = (uint8_t *) file_read((size_t *) &file_size)) == NULL) {
        printf("%s: Reading of file failed\n",__FUNCTION__);
        exit(0);
    }

    /* Retrieve physical address of encrypted image in memory */
    cert_load_addr = virtToPhy(img_ptr, file_size );
    tmp_img_ptr = img_ptr;

    /* Allocate scratch temp buffer from shared memory of same size as image size */
    if ((scratch_mem = (uint8_t *) mmap64(0, file_size + (1024*128), PROT_READ|PROT_WRITE|PROT_NOCACHE, MAP_ANON | MAP_PHYS | MAP_PRIVATE, NOFD, 0)) == (void *)MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    scratch_sz = file_size + (1024*128); // add 128K to scratch area, figure out required space

    if(verbose) {
        printf("%s: scratch_mem_ptr/0x%016lx phyAddr/0x%016lx size/%ld\n",
                __FUNCTION__, (uint64_t) scratch_mem, virtToPhy((void *) *((uint64_t *) scratch_mem), file_size + (1024*128)), file_size + (1024*128));
    }


    /* Copy encrypted image into shared memory location */

    /* Read first 4 bytes of image to */
    /* determine if it is a x509 img  */
    memcpy(x509Header, tmp_img_ptr, 4);
    cert_len = GetCertLen(x509Header);

    /* Check if cert size is within valid range */
    if ((cert_len > 0x100) &&
            (cert_len < 0x800))
    {
        unaligned_bytes = cert_len % 4u;
        pad_align = 4u - unaligned_bytes;

        /* adjust cert load addr so that the */
        /* msg is always word aligned        */
        scratch_mem_ptr = scratch_mem + pad_align;
        printf("Copying %d bytes\r\n", (cert_len + pad_align));

        memcpy(scratch_mem_ptr, tmp_img_ptr, cert_len + pad_align);

        /*cert_load_addr = (uint32_t)scratch_mem_ptr;*/
        cert_load_addr = virtToPhy((void *) scratch_mem_ptr, file_size );

        img_len = GetMsgLen(scratch_mem_ptr, cert_len);
        if ((scratch_mem_ptr + cert_len + img_len)  <
                (scratch_mem + scratch_sz))
        {
            struct tisci_msg_proc_auth_boot_req authReq;

            tmp_img_ptr = tmp_img_ptr + (cert_len - unaligned_bytes);
            scratch_mem_ptr += (cert_len - unaligned_bytes);

            img_len += unaligned_bytes;
            unaligned_bytes = img_len % 4;
            pad_align = 4 - unaligned_bytes;
            img_len += pad_align;

            memcpy(scratch_mem_ptr, tmp_img_ptr, img_len);

            /* TODO - name pointer correctly, or cast cert_load_addr below when setting hi/lo */
            uint32_t *tmp = (uint32_t *) &cert_load_addr;
            if(verbose)
                printf("lo/0x%08x hi/0x%08x\n",tmp[0], tmp[1]);


            /* Request DMSC to authenticate the image */
            authReq.hdr.type = TISCI_MSG_PROC_AUTH_BOOT;
            authReq.hdr.seq = 0;
            authReq.hdr.flags = TISCI_MSG_FLAG_AOP;
            authReq.certificate_address_hi = tmp[1];
            authReq.certificate_address_lo = tmp[0];
            printf("Cert @ 0x%x%x ...", authReq.certificate_address_hi, authReq.certificate_address_lo);

            retVal = Sciclient_procBootAuthAndStart(&authReq, SCICLIENT_SERVICE_WAIT_FOREVER);
            if (retVal == 0)
            {
                printf("Verify Passed...");
            }
            else
            {
                printf ("Failed to decrypt and authenticate image \n");
            }
        }
    }

    return 0;
}
