/*
 *  Copyright (c) 2012-21, Texas Instruments Incorporated
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  *  Neither the name of Texas Instruments Incorporated nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *  Contact information for paper mail:
 *  Texas Instruments
 *  Post Office Box 655303
 *  Dallas, Texas 75265
 *  Contact information:
 *  http://www-k.ext.ti.com/sc/technical-support/product-information-centers.htm?
 *  DCMP=TIHomeTracking&HQS=Other+OT+home_d_contact
 *  ============================================================================
 *
 */
/**
 * \defgroup QNX_SHMEM_ALLOCATOR Shared Memory Allocator Driver
 * 
 * The Shared Memory Allocator driver provides functions for allocating memory
 * to be shared between different processes.
 */

/**
 * \ingroup QNX_SHMEM_ALLOCATOR
 * \defgroup QNX_SHMEM_TOP_LEVEL Shared Memory Allocator Driver User Header
 * @{
 */

/**
 * \file SharedMemoryAllocatorUsr.h
 *
 * \brief Shared Memory Allocator Driver User Header
 */

#include "ti/shmemallocator/SharedMemoryAllocator.h"

/**
 * \brief Allocate shared memory block with given size. 
 * 
 * \param size **[in]**  Size in bytes to allocate.
 * \param buf  **[out]** Pointer to shared memory buffer structure.
 * 
 * \return Status. Returns 0 for pass, and -1 or -errno for error.
*/
int SHM_alloc(int size, shm_buf *buf);

/**
 * \brief Allocate n-bytes aligned shared memory block with given size.
 * The alignment will be with respect to the physical memory address.
 * 
 * \param size      **[in]**  Size in bytes to allocate.
 * \param alignment **[in]**  Alignment for physical address to be **alignment**-bytes aligned.
 * \param buf       **[out]** Pointer to shared memory buffer structure.
 * 
 * \return Status. Returns 0 for pass, and -1 or -errno for error.
*/
int SHM_alloc_aligned(int size, uint alignment, shm_buf *buf);

/**
 * \brief Allocate shared memory block with given size and block ID.
 * 
 * \param size    **[in]**  Size in bytes to allocate.
 * \param blockID **[in]**  Block ID in which to allocated.
 * \param buf     **[out]** Pointer to shared memory buffer structure.
 * 
 * \return Status. Returns 0 for pass, and -1 or -errno for error.
*/
int SHM_alloc_fromBlock(int size, int blockID, shm_buf *buf);

/**
 * \brief Allocate n-bytes aligned shared memory block with given size and block ID.
 * 
 * \param size      **[in]**  Size in bytes to allocate.
 * \param alignment **[in]**  Alignment for physical address to be **alignment**-bytes aligned.
 * \param blockID   **[in]**  Block ID in which to allocated.
 * \param buf       **[out]** Pointer to shared memory buffer structure.
 * 
 * \return Status. Returns 0 for pass, and -1 or -errno for error.
*/
int SHM_alloc_aligned_fromBlock(int size, uint alignment, int blockID, shm_buf *buf);

/**
 * \brief Allocate n-bytes aligned shared memory block with given size, block
 * ID, and specified flags.
 * 
 * \param size      **[in]**  Size in bytes to allocate.
 * \param alignment **[in]**  Alignment for physical address to be **alignment**-bytes aligned.
 * \param blockID   **[in]**  Block ID in which to allocated.
 * \param buf       **[out]** Pointer to shared memory buffer structure.
 * \param prot      **[in]**  Protection bits for virtually mapped addresses. 
 *                  For more information, check mmap() documentation in the "see also" section.
 * \param flags     **[in]**  Flags for virtually mapped addresses. For more 
 *                  information, check mmap() documentation in the "see also" section.
 * 
 * \return Status. Returns 0 for pass, and -1 or -errno for error.
 * 
 * \see https://www.qnx.com/developers/docs/7.1/#com.qnx.doc.neutrino.lib_ref/topic/m/mmap.html
*/
int SHM_alloc_aligned_fromBlock_withFlags(int size, uint alignment, int blockID,
                                                    shm_buf *buf, int prot, int flags);

/**
 * \brief Retrieves shared memory struct data for specified shared memory block.
 * 
 * \param blockID **[in]**  Block ID for info to be retrieved.
 * \param buf     **[out]** Pointer to shared memory buffer structure.
 * 
 * \return Status. Returns 0 for pass, and -1 or -errno for error.
*/
int SHM_get_blkInfo(int blockID, shm_buf *buf);

/**
 * \brief Frees shared memory block.
 * 
 * \param buf **[out]** Pointer to shared memory buffer structure.
 * 
 * \return Status. Returns 0 for pass, and -1 or -errno for error.
*/
int SHM_release(shm_buf *buf);

/**
 * @}
*/
