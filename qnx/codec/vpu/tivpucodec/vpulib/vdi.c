//-----------------------------------------------------------------------------
// COPYRIGHT (C) 2020   CHIPS&MEDIA INC. ALL RIGHTS RESERVED
// COPYRIGHT (C) 2022   Texas Instruments Incorporated - http://www.ti.com/
//
// This file is distributed under BSD 3 clause and LGPL2.1 (dual license)
// SPDX License Identifier: BSD-3-Clause
// SPDX License Identifier: LGPL-2.1-only
//
// The entire notice above must be reproduced on all authorized copies.
//
// Description  :
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/neutrino.h>
#include <unistd.h>

#include "vdi.h"
#include "vdi_osal.h"
#include "mm.h"
#include "wave5_regdefine.h"
#include "vpuapi.h"
#include "vpu_device.h"
#include "ti/shmemallocator/SharedMemoryAllocatorUsr.h"
#include <pthread.h>
#include <semaphore.h>


#ifdef SUPPORT_SW_UART_ON_NONOS
extern void SwUartHandler(void *context);
#endif
#if defined(linux) || defined(__linux) || defined(ANDROID)
#else
#if (REQUIRED_VPU_MEMORY_SIZE > VPUDRV_INIT_VIDEO_MEMORY_SIZE_IN_BYTE)
#error "Warnning : VPU memory will be overflow"
#endif
#endif

// JB: Codec Memory Carveouts
//
#if defined (SOC_J722S)
#define CODEC0_VPU_SIZE                    0x10000
#define CODEC0_VPU_BASE                    0x30210000
#elif defined (SOC_J784S4) || defined(SOC_J721S2)
#define CODEC0_VPU_SIZE                    0x10000
#define CODEC0_VPU_BASE                    0x04210000
#if defined (SOC_J784S4)
#define CODEC1_VPU_SIZE                    0x10000
#define CODEC1_VPU_BASE                    0x04220000
#endif //defined (SOC_J784S4)
#else
#error "Unsupporterd SoC"
#endif // defined (SOC_J722S)

#define VDI_SRAM_BASE_ADDR                  0x70020000

/* NOTE: any change to the VPU DRAM base or size requires a memory carveout change in the BSP for startup */
#define VPU_DRAM_SIZE                       0x20000000 //  512MB
#define VPU_DRAM_LOWMEM_SIZE                0x0CC00000 // ~204MB

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
#define VPU_CORE_BASE_OFFSET                0x10000
#endif

#define VDI_SYSTEM_ENDIAN                   VDI_LITTLE_ENDIAN
#define VDI_128BIT_BUS_SYSTEM_ENDIAN        VDI_128BIT_LITTLE_ENDIAN

typedef struct vpu_buffer_t vpudrv_buffer_t;

typedef struct vpu_buffer_pool_t
{
    vpudrv_buffer_t vdb;
    int inuse;
} vpu_buffer_pool_t;

/* QNX Interrupt handling specific -- start */
#define ISR_PULSE                          _PULSE_CODE_MINAVAIL
#define ISR_THREAD_CLOSE                    _PULSE_CODE_MINAVAIL+1

typedef struct vpu_hwi_info_s {
    uint32_t evt_id;
    int chid;
    pthread_t tid;
    struct sigevent isr_event;
    uint32_t core_intr_num;
    uint32_t intr_priority;
    uintptr_t arg;
    int      isr_thread_running;
} vpu_hwi_info_t;

static vpu_hwi_info_t  s_hwi_info[MAX_VPU_CORE_NUM];
/* QNX Interrupt handling specific -- end */

typedef struct  {
    unsigned long coreIdx;
    int vpu_fd;
    vpu_instance_pool_t *pvip;
    int task_num;
    int clock_state;
    vpudrv_buffer_t vdb_video_memory;
    vpudrv_buffer_t vdb_register;
    vpu_buffer_t vpu_common_memory;
    vpu_buffer_pool_t vpu_buffer_pool[MAX_VPU_BUFFER_POOL];
    int vpu_buffer_pool_count;
    int product_code;
    shm_buf shmem_buf;
    int op_mode;
    sem_t *intr_sem;
    vpu_hwi_info_t *hwi;
    int int_sts_reg;
    int int_reason_reg;
} vdi_info_t;

static vdi_info_t s_vdi_info[MAX_VPU_CORE_NUM];

static uintptr_t g_vpu_dram_size = VPU_DRAM_SIZE; // Use VPU_DRAM_SIZE by default
static uint32_t g_axi_ext_addr = WAVE5_PROC_AXI_EXT_HIGHMEM_ADDR; // Use HIGHMEM as the default base


#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
static vpu_instance_pool_t s_vip[MAX_VPU_CORE_NUM]; // it can be used for a buffer space to save context for app process. to support running VPU in multiple process. this space should be a shared buffer.
#else
static vpu_instance_pool_t s_vip;   // it can be used for a buffer space to save context for app process. to support running VPU in multiple process. this space should be a shared buffer.
#endif
static int swap_endian(unsigned long coreIdx, unsigned char *data, int len, int endian);

int vdi_lock(unsigned long coreIdx)
{
    /* need to implement */
    return 0;
}

void vdi_unlock(unsigned long coreIdx)
{
    /* need to implement */
}

int vdi_disp_lock(unsigned long coreIdx)
{
    /* need to implement */

    return 0;
}

void vdi_disp_unlock(unsigned long coreIdx)
{

    /* need to implement */
}


int vmem_lock_init(unsigned long coreIdx)
{
    /* need to implement */
    return 0;
}

int vmem_lock(unsigned long coreIdx)
{

    /* need to implement */

    return 0;
}

void vmem_unlock(unsigned long coreIdx)
{
    /* need to implement */
}

int vmem_lock_deinit(unsigned long coreIdx)
{
    /* need to implement */
    return 0;
}

int vdi_probe(unsigned long coreIdx)
{
    int ret;

    ret = vdi_init(coreIdx);
    vdi_release(coreIdx);

    return ret;
}

inline uint32_t vdi_get_axi_ext_addr(void)
{
    return g_axi_ext_addr;
}

/* Unblocks the wait_interrupt call */
void vdi_handle_interrupt(uintptr_t arg)
{
    vdi_info_t *vdi = (vdi_info_t *) arg;

    if(vdi) {
        sem_post(vdi->intr_sem);
        codec_slogdbg("[VDI] signalling for coreIdx %ld \n", vdi->coreIdx);
    } else {
        codec_slogerr("[VDI] invalid vdi passed. Spurious interrupt!\n");
    }
}

/* Thread level ISR handler */
static void *isr_thread (void *arg)
{
    vdi_info_t *vdi = (vdi_info_t *)arg;
    vpu_hwi_info_t *hwi = NULL;
    int             rcvid;
    struct _pulse   pulse;

    if (vdi == NULL) {
        codec_slogerr("[VDI] Interrupt thread recieved NULL arg. No processing done\n");
        return NULL;
    }

    hwi = vdi->hwi;

    if (hwi == NULL) {
        codec_slogerr("[VDI] Interrupt thread. vdi points to invalid hwi. No processing done\n");
        return NULL;
    }

    while (hwi->isr_thread_running) {
        rcvid = MsgReceivePulse(hwi->chid, &pulse, sizeof(struct _pulse), NULL);
        if (rcvid != -1) {
            switch (pulse.code) {
                case ISR_PULSE:
                    /* Call the callback function */
                    vdi_handle_interrupt((uintptr_t)arg);
                    /* Re-enable interrupt here if needed */
                    break;
                case ISR_THREAD_CLOSE:
                    /* disable the interrupt and exit the ISR thread */
                    InterruptMask(hwi->core_intr_num, hwi->evt_id);
                    hwi->isr_thread_running = FALSE;
                    codec_sloginfo("[VDI] Exiting the ISR thread for interrupt number %d\n", hwi->core_intr_num);
                    break;
                default:
                    codec_slogerr("[VDI] Unknown pulse %d received", pulse.code);
                    break;
            }
        }
        else {
            codec_slogerr("[VDI] MsgReceivePulse failed\n");
            delay(20); // TODO:: This delay is from OSAL code. Do we really need this?
        }
    }
    return NULL;
}

void vpu_sem_destroy(sem_t *sem)
{
    sem_destroy(sem);
    free(sem);
}

void vdi_set_polling(int core_idx)
{
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE) {
        codec_slogerr("[VDI] invalid core_idx passed\n");
        return;
    }

    vdi = &s_vdi_info[core_idx];

    if (vdi == NULL) {
        codec_slogerr("[VDI] invalid vpu device info passed\n");
        return;
    }

    vdi->op_mode = VPU_OPMODE_POLL;
}

int vdi_interrupt_init(unsigned long core_idx)
{
    vdi_info_t *vdi;
    int ret = 0;
    vpu_hwi_info_t *hwi = NULL;
    pthread_attr_t thread_attr;
    struct sched_param  param;
    char threadName[128];

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];

    if (vdi == NULL) {
        codec_slogerr("[VDI] invalid vpu device info passed\n");
        return -1;
    }

    if (vdi->op_mode == VPU_OPMODE_POLL) {
        codec_slogerr("[VDI] %s called in polling mode operation\n", __func__);
        return -1;
    }

    hwi = vdi->hwi = &s_hwi_info[core_idx];

    hwi->core_intr_num = osal_get_interrupt_num(core_idx);

    if (hwi->core_intr_num == -1) {
        codec_slogerr("[VDI] invalid interrupt number passed\n");
        return -1;
    }

    /* init the semaphore */
    vdi->intr_sem =  (sem_t*)malloc(sizeof(sem_t));

    if(sem_init(vdi->intr_sem, 0, 0) < 0) {
        codec_slogerr("[VDI] failed to create the vdi interrupt semaphore\n");
        free(vdi->intr_sem);
        vdi->intr_sem = NULL;
        return -1;
    }

    hwi->chid = ChannelCreate(0);

    if(hwi->chid == -1) {
        codec_slogerr("[VDI] Failed to create chid for %ld\n", core_idx);
        vpu_sem_destroy(vdi->intr_sem);
        vdi->intr_sem = NULL;
        return -1;
    }

    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
    param.sched_priority = 21;
    pthread_attr_setschedparam(&thread_attr, &param);

    hwi->isr_thread_running = TRUE;
    if (pthread_create(&hwi->tid, &thread_attr, (void *)isr_thread, (void *)vdi) != EOK) {
        codec_slogerr("[VDI] Interrupt thread create failed for core %ld\n", core_idx);

        ChannelDestroy(hwi->chid);
        vpu_sem_destroy(vdi->intr_sem);
        vdi->intr_sem = NULL;
        return -1;
    }

    sprintf(threadName, "VPUIntrThread_%d", hwi->core_intr_num);
    pthread_setname_np(hwi->tid, threadName);

    /* Store pointer to the hwi structure */
    //hwi->isrFxn = hwi_fxn;
    hwi->intr_priority = param.sched_priority;
    hwi->arg = (uintptr_t)vdi;

    /* Init the pulse for interrupt event */
    hwi->isr_event.sigev_notify = SIGEV_PULSE;
    hwi->isr_event.sigev_code = _PULSE_CODE_MINAVAIL;
    hwi->isr_event.sigev_coid = ConnectAttach(0, 0, hwi->chid, _NTO_SIDE_CHANNEL, 0);
    hwi->isr_event.sigev_priority = hwi->intr_priority;     /* service interrupts at a higher priority then client requests */
    hwi->isr_event.sigev_value.sival_int = hwi->core_intr_num;


    hwi->evt_id = InterruptAttachEvent(hwi->core_intr_num, &hwi->isr_event,  0 /*_NTO_INTR_FLAGS_NO_UNMASK*/);

    if(hwi->evt_id == -1) {
        codec_slogerr("[VDI] InterruptAttachEvent failed for core %ld\n", core_idx);
        ret = -1;
    }

    if (ret) {
        pthread_cancel(hwi->tid);
        ChannelDestroy(hwi->chid);
        vpu_sem_destroy(vdi->intr_sem);
        vdi->intr_sem = NULL;
    }

    codec_sloginfo("[VDI] Successfully registered interrupt for core%ld\n", core_idx);
    return ret;
}

int vdi_interrupt_cleanup(unsigned long coreIdx)
{
    vdi_info_t *vdi = &s_vdi_info[coreIdx];
    vpu_hwi_info_t *hwi = NULL;

    if (vdi == NULL) {
        codec_slogerr("[VDI] invalid vpu device info passed\n");
        return -1;
    }

    hwi = vdi->hwi;

    if (hwi == NULL) {
        codec_slogerr("[VDI] invalid interrupt info referenced\n");
        return -1;
    }

    // Send a pulse to exit the ISR thread
    MsgSendPulse(hwi->isr_event.sigev_coid, -1, ISR_THREAD_CLOSE, 0);

    pthread_join(hwi->tid, NULL);

    InterruptDetach(hwi->core_intr_num);
    vpu_sem_destroy(vdi->intr_sem);
    vdi->intr_sem = NULL;

    return 0;
}

int vdi_init(unsigned long coreIdx)
{
    int ret;
    vdi_info_t *vdi;
    int i;
    Uint32 product_code;
    void *p;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return 0;

    vdi = &s_vdi_info[coreIdx];

    if (vdi->vpu_fd != -1 && vdi->vpu_fd != 0x00)
    {
        vdi->task_num++;
        return 0;
    }

    vdi->vpu_fd = 1;
    osal_memset(vdi->vpu_buffer_pool, 0x00, sizeof(vpu_buffer_pool_t)*MAX_VPU_BUFFER_POOL);

    if (!vdi_get_instance_pool(coreIdx))
    {
        codec_slogerr("[VDI] failed to create shared info for saving context\n");
        goto ERR_VDI_INIT;
    }

    /* get the region that shared memory allocator is managing for the codec.
     * If low mem, then adjust the DRAM size accordingly. */
    {
        shm_buf block_info = {0};

        int status = SHM_get_blkInfo(BLOCK_IDX_2, &block_info);
        if (status != 0) {
            codec_slogwarn("[VDI] SHM_get_blkInfo failed, retaining high mem   \
                            dram size");
        }
        if (block_info.phy_addr < (uint32_t)~(0)) { /* The region mapped is below 4GB */
            g_axi_ext_addr = WAVE5_PROC_AXI_EXT_LOWMEM_ADDR;
            g_vpu_dram_size = VPU_DRAM_LOWMEM_SIZE;
        }
    }

    ret = SHM_alloc_aligned_fromBlock_withFlags(g_vpu_dram_size,
                0x1000, BLOCK_IDX_2, &vdi->shmem_buf, PROT_NOCACHE, 0);
    /* Use the first block to allocate memory instead.
     * NOTE:: This means that we are using the VISION APPS allocated memory instead
     * This should still be fine since the VISION APPS has memory that it can spare
     * ATM.
     */
    if(ret != 0) {
        ret = SHM_alloc_aligned_fromBlock_withFlags(g_vpu_dram_size,
                0x1000, BLOCK_IDX_1, &vdi->shmem_buf, PROT_NOCACHE, 0);
        if(ret != 0) {
            codec_slogerr("%s core[%ld] - Shared memory allocation failed for size 0x%lx\n",
                    __FUNCTION__, coreIdx, g_vpu_dram_size);
            goto ERR_VDI_INIT;
        }
    }

    vdi->vdb_video_memory.phys_addr = vdi->shmem_buf.phy_addr;
    vdi->vdb_video_memory.size      = g_vpu_dram_size;

    codec_sloginfo("%s core[%ld] - phys_addr = 0x%lx size = 0x%x",
                     __FUNCTION__, coreIdx, vdi->vdb_video_memory.phys_addr, vdi->vdb_video_memory.size);

#if 0
    if (REQUIRED_VPU_MEMORY_SIZE > vdi->vdb_video_memory.size)
    {
        codec_slogerr("[VDI] Warning : required VPU memory is greater than available video memory\n");
    }
#endif

    if (vdi_allocate_common_memory(coreIdx) < 0)
    {
        codec_slogerr("[VDI] failed to get vpu common buffer from driver\n");
        goto ERR_VDI_INIT;
    }

    if (!vdi->pvip->instance_pool_inited)
        osal_memset(&vdi->pvip->vmem, 0x00, sizeof(video_mm_t));

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    ret = vmem_init(&vdi->pvip->vmem, vdi->vdb_video_memory.phys_addr + (vdi->pvip->vpu_common_buffer.size*MAX_VPU_CORE_NUM),
            vdi->vdb_video_memory.size - (vdi->pvip->vpu_common_buffer.size*MAX_VPU_CORE_NUM));
#else
    ret = vmem_init(&vdi->pvip->vmem, vdi->vdb_video_memory.phys_addr + vdi->pvip->vpu_common_buffer.size,
            vdi->vdb_video_memory.size - vdi->pvip->vpu_common_buffer.size);
#endif
    vmem_lock_init(coreIdx);

    if (ret < 0)
    {
        codec_slogerr("[VDI] failed to init vpu memory management logic\n");
        goto ERR_VDI_INIT;
    }

    vdi->vdb_register.phys_addr = CODEC0_VPU_BASE;
    vdi->vdb_register.size = CODEC0_VPU_SIZE * MAX_NUM_VPU_CORE;

    p = osal_map_memory((Uint64)vdi->vdb_register.phys_addr, vdi->vdb_register.size);
    if (p == NULL)
    {
        codec_slogerr("[VDI] failure to map the physical memory for the vpu\n");
        goto ERR_VDI_INIT;
    }
    vdi->vdb_register.virt_addr = (uintptr_t)p;
    codec_slogdbg("%s core[%ld] - vdi->vdb_register.phys_addr=0x%lx, vdi->vdb_register.virt_addr=0x%lx, vdi->vdb_register.size0x%x", __FUNCTION__, coreIdx,
                    vdi->vdb_register.phys_addr, vdi->vdb_register.virt_addr, vdi->vdb_register.size);

    vpu_enable_device(coreIdx);
    vdi_set_clock_gate(coreIdx, TRUE);
    vdi->product_code = vdi_read_register(coreIdx, VPU_PRODUCT_CODE_REGISTER);
    product_code = vdi->product_code;
    if (PRODUCT_CODE_W_SERIES(product_code))
    {
        vdi->int_sts_reg = W5_VPU_VPU_INT_STS;
        vdi->int_reason_reg = W5_VPU_VINT_REASON;

        if (vdi_read_register(coreIdx, W5_VCPU_CUR_PC) == 0) // if BIT processor is not running.
        {
            for (i=0; i<64; i++)
                vdi_write_register(coreIdx, (i*4) + 0x100, 0x0);
        }
    }
    else
    {
        codec_slogerr("[VDI] Non Wave5 IPs not supported\n");
    }
    vdi_set_clock_gate(coreIdx, FALSE);

    vdi->coreIdx = coreIdx;
    vdi->task_num++;

    codec_sloginfo("[VDI] successful driver init \n");

    return 0;

ERR_VDI_INIT:

    vdi_release(coreIdx);
    return -1;
}

int vdi_set_bit_firmware_to_pm(unsigned long coreIdx, const unsigned short *code)
{
    return 0;
}

int vdi_release(unsigned long coreIdx)
{
    int i;
    vpudrv_buffer_t vdb = {0, };
    vdi_info_t *vdi = &s_vdi_info[coreIdx];

    if (!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return 0;

    if (vdi_lock(coreIdx) < 0)
    {
        codec_slogerr("[VDI] failed to handle lock function\n");
        return -1;
    }

    if (vdi->task_num > 1) // means that the opened instance remains
    {
        vdi->task_num--;
        vdi_unlock(coreIdx);
        return 0;
    }

    // unregister the interrupt, exit the isr thread and destroy the semaphore
    if(vdi->op_mode == VPU_OPMODE_INTERRUPT)
        vdi_interrupt_cleanup(coreIdx);

    vmem_lock_deinit(coreIdx);
    vmem_exit(&vdi->pvip->vmem);

    SHM_release(&vdi->shmem_buf);

    osal_memset(&vdi->vdb_register, 0x00, sizeof(vpudrv_buffer_t));

    // get common memory information to free virtual address
    vdb.size = 0;
    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_common_memory.phys_addr >= vdi->vpu_buffer_pool[i].vdb.phys_addr &&
                vdi->vpu_common_memory.phys_addr < (vdi->vpu_buffer_pool[i].vdb.phys_addr + vdi->vpu_buffer_pool[i].vdb.size))
        {
            vdi->vpu_buffer_pool[i].inuse = 0;
            vdi->vpu_buffer_pool_count--;
            vdb = vdi->vpu_buffer_pool[i].vdb;
            break;
        }
    }

    if (vdb.size > 0)
        osal_memset(&vdi->vpu_common_memory, 0x00, sizeof(vpu_buffer_t));

    vpu_disable_device(coreIdx);

    vdi->task_num--;
    vdi->vpu_fd = -1;

    vdi_unlock(coreIdx);

    osal_memset(vdi, 0x00, sizeof(vdi_info_t));

    return 0;
}

int vdi_get_common_memory(unsigned long coreIdx, vpu_buffer_t *vb)
{
    vdi_info_t *vdi;

    codec_slogtrace("%s", __FUNCTION__);
    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;

    osal_memcpy(vb, &vdi->vpu_common_memory, sizeof(vpu_buffer_t));

    codec_sloginfo("[VDI] vdi_get_common_memory physaddr=0x%lx, virtaddr=0x%lx, size=0x%x(%d)",
        vdi->vpu_common_memory.phys_addr, vdi->vpu_common_memory.virt_addr, (int)vdi->vpu_common_memory.size, (int)vdi->vpu_common_memory.size);

    return 0;
}

int vdi_allocate_common_memory(unsigned long core_idx)
{
    vdi_info_t *vdi = &s_vdi_info[core_idx];
    vpudrv_buffer_t vdb;
    int i;
    void *p;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd==0x00)
        return -1;

    if (vdi->pvip->vpu_common_buffer.size == 0)
    {
        vdb.size = SIZE_COMMON*MAX_VPU_CORE_NUM;
        vdb.phys_addr = vdi->vdb_video_memory.phys_addr; // set at the beginning of base address
        p = osal_map_memory((Uint64)vdb.phys_addr, vdb.size);
        if (p == NULL)
        {
           codec_slogerr("[VDI] failure to map the physical memory for the vpu\n");
           return -1;
        }
        vdb.virt_addr = (unsigned long)p;
        vdb.base = vdi->vdb_video_memory.base;
        codec_slogdbg("%s core[%ld] vdb.phys_addr=0x%lx, vdb.virt_addr=0x%lx, vdb.size=0x%x(%d)", __FUNCTION__, core_idx, vdb.phys_addr, vdb.virt_addr, vdb.size, vdb.size);

        // convert os driver buffer type to vpu buffer type
#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
        vdi->pvip->vpu_common_buffer.size = SIZE_COMMON;
        vdi->pvip->vpu_common_buffer.phys_addr = (PhysicalAddress)(vdb.phys_addr + (core_idx*SIZE_COMMON));
        vdi->pvip->vpu_common_buffer.base = (unsigned long)(vdb.base + (core_idx*SIZE_COMMON));
        vdi->pvip->vpu_common_buffer.virt_addr = (unsigned long)(vdb.virt_addr + (core_idx*SIZE_COMMON));
#else
        vdi->pvip->vpu_common_buffer.size = SIZE_COMMON;
        vdi->pvip->vpu_common_buffer.phys_addr = (PhysicalAddress)(vdb.phys_addr);
        vdi->pvip->vpu_common_buffer.base = (unsigned long)(vdb.base);
        vdi->pvip->vpu_common_buffer.virt_addr = (unsigned long)(vdb.virt_addr);
#endif

        osal_memcpy(&vdi->vpu_common_memory, &vdi->pvip->vpu_common_buffer, sizeof(vpudrv_buffer_t));

    }
    else
    {
        vdb.size = SIZE_COMMON*MAX_VPU_CORE_NUM;
        vdb.phys_addr = vdi->vdb_video_memory.phys_addr; // set at the beginning of base address
        vdb.base =  vdi->vdb_video_memory.base;
        p = osal_map_memory((Uint64)vdb.phys_addr, vdb.size);
        if (p == NULL)
        {
           codec_slogerr("[VDI] failure to map the physical memory for the vpu\n");
           return -1;
        }
        vdb.virt_addr = (unsigned long)p;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
        vdi->pvip->vpu_common_buffer.virt_addr = (unsigned long)(vdb.virt_addr + (core_idx*SIZE_COMMON));
#else
        vdi->pvip->vpu_common_buffer.virt_addr = vdb.virt_addr;
#endif
        osal_memcpy(&vdi->vpu_common_memory, &vdi->pvip->vpu_common_buffer, sizeof(vpudrv_buffer_t));

        codec_sloginfo("[VDI] vdi_allocate_common_memory core[%ld] physaddr=0x%lx, virtaddr=0x%lx\n",
                        core_idx, vdi->pvip->vpu_common_buffer.phys_addr, vdi->pvip->vpu_common_buffer.virt_addr);
    }

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].inuse == 0)
        {
            vdi->vpu_buffer_pool[i].vdb = vdb;
            vdi->vpu_buffer_pool_count++;
            vdi->vpu_buffer_pool[i].inuse = 1;
            break;
        }
    }

    codec_sloginfo("[VDI] vdi_allocate_common_memory - core_idx - %ld physaddr=0x%lx, virtaddr=0x%lx, size=0x%x(%d)", core_idx,
        vdi->vpu_common_memory.phys_addr, vdi->vpu_common_memory.virt_addr, (int)vdi->vpu_common_memory.size, (int)vdi->vpu_common_memory.size);

    return 0;
}

vpu_instance_pool_t *vdi_get_instance_pool(unsigned long coreIdx)
{
    vdi_info_t *vdi;

    if (coreIdx >= MAX_VPU_CORE_NUM)
        return NULL;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd ==0x00 )
        return NULL;

    if (!vdi->pvip)
    {
#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
        vdi->pvip = &s_vip[coreIdx];
#else
        vdi->pvip = &s_vip;
#endif
        osal_memset(vdi->pvip, 0, sizeof(vpu_instance_pool_t));
    }

    return (vpu_instance_pool_t *)vdi->pvip;
}

int vdi_open_instance(unsigned long coreIdx, unsigned long instIdx)
{
    vdi_info_t *vdi = NULL;

    if (coreIdx >= MAX_VPU_CORE_NUM)
        return -1;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd ==-1 || vdi->vpu_fd == 0x00)
        return -1;

    vdi->pvip->vpu_instance_num++;

    return 0;
}

int vdi_close_instance(unsigned long coreIdx, unsigned long instIdx)
{
    vdi_info_t *vdi = NULL;

    if (coreIdx >= MAX_VPU_CORE_NUM)
        return -1;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd ==-1 || vdi->vpu_fd == 0x00)
        return -1;

    vdi->pvip->vpu_instance_num--;

    return 0;
}

int vdi_get_instance_num(unsigned long coreIdx)
{
    vdi_info_t *vdi = NULL;

    if (coreIdx >= MAX_VPU_CORE_NUM)
        return -1;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd ==-1 || vdi->vpu_fd == 0x00)
        return -1;

    return vdi->pvip->vpu_instance_num;
}

int vdi_hw_reset(unsigned long coreIdx) // DEVICE_ADDR_SW_RESET
{
    vdi_info_t *vdi = NULL;

    if (coreIdx >= MAX_VPU_CORE_NUM)
        return -1;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || !vdi || vdi->vpu_fd ==-1 || vdi->vpu_fd == 0x00)
        return -1;

    // to do any action for hw reset

    return 0;
}

void vdi_write_register(unsigned long coreIdx, unsigned int addr, unsigned int data)
{
    vdi_info_t *vdi = NULL;
    unsigned int *reg_addr;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    reg_addr = (unsigned int *)(addr + vdi->vdb_register.virt_addr + (coreIdx*VPU_CORE_BASE_OFFSET));
#else
    reg_addr = (unsigned int *)(addr + vdi->vdb_register.virt_addr);
#endif
    *(volatile unsigned int *)reg_addr = data;
}

unsigned int vdi_read_register(unsigned long coreIdx, unsigned int addr)
{
    vdi_info_t *vdi = NULL;
    unsigned int *reg_addr;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return (unsigned int)-1;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return (unsigned int)-1;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    reg_addr = (unsigned int *)(addr + vdi->vdb_register.virt_addr + (coreIdx*VPU_CORE_BASE_OFFSET));
#else
    reg_addr = (unsigned int *)(addr + vdi->vdb_register.virt_addr);
#endif
    return *(volatile unsigned int *)reg_addr;
}

#define FIO_TIMEOUT         10000
unsigned int vdi_fio_read_register(unsigned long coreIdx, unsigned int addr)
{
    unsigned int ctrl;
    unsigned int count = 0;
    unsigned int data  = 0xffffffff;

    ctrl  = (addr&0xffff);
    ctrl |= (0<<16);    /* read operation */
    vdi_write_register(coreIdx, W5_VPU_FIO_CTRL_ADDR, ctrl);
    count = FIO_TIMEOUT;
    while (count--) {
        ctrl = vdi_read_register(coreIdx, W5_VPU_FIO_CTRL_ADDR);
        if (ctrl & 0x80000000) {
            data = vdi_read_register(coreIdx, W5_VPU_FIO_DATA);
            break;
        }
    }

    return data;
}

void vdi_fio_write_register(unsigned long coreIdx, unsigned int addr, unsigned int data)
{
    unsigned int ctrl;
    unsigned int count = 0;

    vdi_write_register(coreIdx, W5_VPU_FIO_DATA, data);
    ctrl  = (addr&0xffff);
    ctrl |= (1<<16);    /* write operation */
    vdi_write_register(coreIdx, W5_VPU_FIO_CTRL_ADDR, ctrl);

    count = FIO_TIMEOUT;
    while (count--) {
        ctrl = vdi_read_register(coreIdx, W5_VPU_FIO_CTRL_ADDR);
        if (ctrl & 0x80000000) {
            break;
        }
    }
}

int vdi_clear_memory(unsigned long coreIdx, PhysicalAddress addr, int len, int endian)
{
    vdi_info_t *vdi;
    vpudrv_buffer_t vdb;
    unsigned long offset;
    int i;
    Uint8*  zero;

#ifndef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    coreIdx = 0;
#else
    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;
#endif

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].inuse == 1)
        {
            vdb = vdi->vpu_buffer_pool[i].vdb;
            if (addr >= vdb.phys_addr && addr < (vdb.phys_addr + vdb.size))
                break;
        }
    }

    if (!vdb.size) {
        codec_slogerr("address 0x%lx is not a mapped address!!!\n", addr);
        return -1;
    }

    offset = (unsigned long)(addr - vdb.phys_addr);

    zero = (Uint8*)osal_malloc(len);
    if (!zero) {
        codec_slogerr("failed to allocate mem for copying zeros, size: %d\n", len);
        return -1;
    }
    osal_memset((void*)zero, 0x00, len);

    osal_memcpy((void *)((unsigned long)vdb.virt_addr+offset), zero, len);

    osal_free(zero);

    return len;
}

int vdi_write_memory(unsigned long coreIdx, PhysicalAddress addr, unsigned char *data, int len, int endian)
{
    vdi_info_t *vdi;
    vpudrv_buffer_t vdb = {0};
    unsigned long offset;
    int i;

    codec_slogtrace("%s", __FUNCTION__);
#ifndef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    coreIdx = 0;
#else
    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;
#endif
    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].inuse == 1)
        {
            vdb = vdi->vpu_buffer_pool[i].vdb;
            if (addr >= vdb.phys_addr && addr < (vdb.phys_addr + vdb.size))
                break;
        }
    }

    if (!vdb.size) { //lint !e644
        codec_slogerr("address 0x%lx is not a mapped address!!!\n", addr);
        return -1;
    }

    offset =  (unsigned long)(addr -vdb.phys_addr);

    swap_endian(coreIdx, data, len, endian);
    osal_memcpy((void *)((unsigned long)vdb.virt_addr+offset), data, len);

    return len;

}

int vdi_read_memory(unsigned long coreIdx, PhysicalAddress addr, unsigned char *data, int len, int endian)
{
    vdi_info_t *vdi = NULL;
    vpudrv_buffer_t vdb = {0};
    unsigned long offset;
    int i;

    codec_slogtrace("%s", __FUNCTION__);
#ifndef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    coreIdx = 0;
#else
    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;
#endif
    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].inuse == 1)
        {
            vdb = vdi->vpu_buffer_pool[i].vdb;
            if (addr >= vdb.phys_addr && addr < (vdb.phys_addr + vdb.size))
                break;
        }
    }

    if (!vdb.size) //lint !e644
        return -1;

    offset =  (unsigned long)(addr -vdb.phys_addr);

    osal_memcpy(data, (const void *)((unsigned long)vdb.virt_addr+offset), len);
    swap_endian(coreIdx, data, len,  endian);

    return len;
}

int vdi_allocate_dma_memory(unsigned long coreIdx, vpu_buffer_t *vb, int memTypes, int instIndex)
{
    vdi_info_t *vdi = NULL;
    int i;
    unsigned long offset;
    vpudrv_buffer_t vdb = {0};
    void *p;

    //codec_slogtrace("%s", __FUNCTION__);
#ifndef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    coreIdx = 0;
#else
    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;
#endif
    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    vdb.size = vb->size;
    vmem_lock(coreIdx);
    vdb.phys_addr = (PhysicalAddress)vmem_alloc(&vdi->pvip->vmem, vdb.size, 0);
    if (vdb.size < (1024*1024)) {
        codec_sloginfo("[VDI] allocated mem of size %u K of %u M\n", (vdb.size)/(1024), (vdi->pvip->vmem.free_page_count *16)/1024);

    } else {
        codec_sloginfo("[VDI] allocated mem of size %u M of %u M\n", (vdb.size)/(1024*1024), (vdi->pvip->vmem.free_page_count *16)/1024);
    }

    vmem_unlock(coreIdx);

    if ((PhysicalAddress)vdb.phys_addr == (PhysicalAddress)-1) {
        codec_slogerr("[VDI] core[%ld], Not enough memory available. Requested size %d M\n", coreIdx, (vdb.size)/(1024*1024));
        return -1; // not enough memory
    }

    offset = (unsigned long)(vdb.phys_addr - vdi->vdb_video_memory.phys_addr);
    vdb.base = (unsigned long )vdi->vdb_video_memory.base + offset;
    p = osal_map_memory((Uint64)vdb.phys_addr, vdb.size);
    if (p == NULL)
    {
        codec_slogerr("[VDI] core[%ld] failure to map the physical memory for the vpu\n", coreIdx);
        return -1;
    }
    vdb.virt_addr = (uintptr_t)p;

    vb->phys_addr = (unsigned long)vdb.phys_addr;
    vb->base = (unsigned long)vdb.base;
    vb->virt_addr = (unsigned long)vdb.virt_addr;
    codec_slogdbg("%s core[%ld] - vb->phys_addr=0x%lx, vb->virt_addr=0x%lx, vdb.size=0x%x(%d)", __FUNCTION__, coreIdx, vb->phys_addr, vb->virt_addr, vdb.size, vdb.size);

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].inuse == 0)
        {
            vdi->vpu_buffer_pool[i].vdb = vdb;
            vdi->vpu_buffer_pool_count++;
            vdi->vpu_buffer_pool[i].inuse = 1;
            break;
        }
    }

    if (MAX_VPU_BUFFER_POOL == i) {
        codec_slogerr("[VDI] core[%ld] failure in vdi_allocate_dma_memory: vpu_buffer_pool_count=%d MAX_VPU_BUFFER_POOL=%d\n",
                       coreIdx, vdi->vpu_buffer_pool_count, MAX_VPU_BUFFER_POOL);
        return -1;
    }

    return 0;
}

int vdi_attach_dma_memory(unsigned long coreIdx, vpu_buffer_t *vb)
{
    vdi_info_t *vdi;
    int i;
    unsigned long offset;
    vpudrv_buffer_t vdb = {0};

#ifndef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    coreIdx = 0;
#else
    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;
#endif
    vdi = &s_vdi_info[coreIdx];

    if(!vb || !vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    vdb.size = vb->size;
    vdb.phys_addr = vb->phys_addr;
    offset = (unsigned long)(vdb.phys_addr - vdi->vdb_video_memory.phys_addr);
    vdb.base = (unsigned long )vdi->vdb_video_memory.base + offset;
    vdb.virt_addr = vb->virt_addr;

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].vdb.phys_addr == vb->phys_addr)
        {
            vdi->vpu_buffer_pool[i].vdb = vdb;
            vdi->vpu_buffer_pool[i].inuse = 1;
            break;
        }
        else
        {
            if (vdi->vpu_buffer_pool[i].inuse == 0)
            {
                vdi->vpu_buffer_pool[i].vdb = vdb;
                vdi->vpu_buffer_pool_count++;
                vdi->vpu_buffer_pool[i].inuse = 1;
                break;
            }
        }
    }

    return 0;
}

int vdi_dettach_dma_memory(unsigned long coreIdx, vpu_buffer_t *vb)
{
    vdi_info_t *vdi;
    int i;

#ifndef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    coreIdx = 0;
#else
    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;
#endif
    vdi = &s_vdi_info[coreIdx];

    if(!vb || !vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    if (vb->size == 0)
        return -1;

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].vdb.phys_addr == vb->phys_addr)
        {
            vdi->vpu_buffer_pool[i].inuse = 0;
            vdi->vpu_buffer_pool_count--;
            break;
        }
    }

    return 0;
}

void vdi_free_dma_memory(unsigned long coreIdx, vpu_buffer_t *vb, int memTypes, int instIndex)
{
    vdi_info_t *vdi;
    int i;
    vpudrv_buffer_t vdb = {0};

    codec_slogtrace("%s", __FUNCTION__);
#ifndef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    coreIdx = 0;
#else
    if (coreIdx >= MAX_NUM_VPU_CORE)
        return;
#endif
    vdi = &s_vdi_info[coreIdx];

    if(!vb || !vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return;

    if (vb->size == 0)
        return;

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].vdb.phys_addr == vb->phys_addr)
        {
            vdi->vpu_buffer_pool[i].inuse = 0;
            vdi->vpu_buffer_pool_count--;
            vdb = vdi->vpu_buffer_pool[i].vdb;
            break;
        }
    }

    if (!vdb.size) //lint !e644
    {
        codec_slogerr("[VDI] invalid buffer to free address = 0x%x\n", (int)vdb.virt_addr);
        return ;
    }

    if (osal_munmap_memory((void *)(vdb.virt_addr), vdb.size)) {
        codec_slogerr("[VDI] error unmapping memory at 0x%lx of size 0x%x\n", vdb.virt_addr, vdb.size);
    }
    codec_slogdbg("[VDI] successfully unmapped memory at 0x%lx of size 0x%x\n", vdb.virt_addr, vdb.size);

    vmem_lock(coreIdx);
    vmem_free(&vdi->pvip->vmem, (unsigned long)vdb.phys_addr, 0);
    vmem_unlock(coreIdx);
    osal_memset(vb, 0, sizeof(vpu_buffer_t));
}


int vdi_get_sram_memory(unsigned long coreIdx, vpu_buffer_t *vb)
{
    vdi_info_t *vdi = &s_vdi_info[coreIdx];
    Uint32      sram_size=0;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;

    if(!vb || !vdi)
        return -1;

    switch (vdi->product_code) {
    case WAVE511_CODE:
    /* 10bit profile : 8Kx8K -> 129024, 4Kx2K -> 64512
     */
        sram_size = 0x1F800; break;
    case WAVE517_CODE:
    /* 10bit profile : 8Kx8K -> 272384, 4Kx2K -> 104448
     */
        sram_size = 0x42800; break;
    case WAVE537_CODE:
    /* 10bit profile : 8Kx8K -> 272384, 4Kx2K -> 104448
     */
        sram_size = 0x42800; break;
    case WAVE521_CODE:
    /* 10bit profile : 8Kx8K -> 126976, 4Kx2K -> 63488
     */
        sram_size = 0x1F000; break;
    case WAVE521E1_CODE:
    /* 10bit profile : 8Kx8K -> 126976, 4Kx2K -> 63488
     */
        sram_size = 0x1F000; break;
    case WAVE521C_CODE:
    /* 10bit profile : 8Kx8K -> 129024, 4Kx2K -> 64512
     * NOTE: Decoder > Encoder
     */
        sram_size = 0x1F800; break;
    case WAVE521C_DUAL_CODE:
    /* 10bit profile : 8Kx8K -> 129024, 4Kx2K -> 64512
     * NOTE: Decoder > Encoder
     */
        sram_size = 0x1F800; break;
    default:
        codec_slogerr("[VDI] check product_code(%x)\n", vdi->product_code);
        break;
    }


    // if we can know the sram address directly in vdi layer, we use it first for sdram address
    vb->phys_addr = VDI_SRAM_BASE_ADDR+(coreIdx*sram_size);
    vb->size      = sram_size;

    return 0;
}

int vdi_set_clock_gate(unsigned long coreIdx, int enable)
{
    vdi_info_t *vdi = NULL;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    if (PRODUCT_CODE_W_SERIES(vdi->product_code)) {
        return 0;
    }
    vdi->clock_state = enable;

    return 0;
}

int vdi_get_clock_gate(unsigned long coreIdx)
{
    vdi_info_t *vdi = NULL;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    return vdi->clock_state;
}

int vdi_wait_bus_busy(unsigned long coreIdx, int timeout, unsigned int gdi_busy_flag)
{
    vdi_info_t *vdi = &s_vdi_info[coreIdx];
    Uint32 gdi_status_check_value = 0x3f;
    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    if (PRODUCT_CODE_W_SERIES(vdi->product_code)) {
        gdi_status_check_value = 0x3f;
        if (vdi->product_code == WAVE521C_CODE || vdi->product_code == WAVE521_CODE || vdi->product_code == WAVE521E1_CODE) {
            gdi_status_check_value = 0x00ff1f3f;
        }
    }
    //VDI must implement timeout action in this function for multi-vpu core scheduling efficiency.
    //the setting small value as timeout gives a chance to wait the other vpu core.

    while(1)
    {
#ifdef SUPPORT_SW_UART_ON_NONOS
        SwUartHandler(NULL);
#endif
        if (PRODUCT_CODE_W_SERIES(vdi->product_code)) {
            if (vdi_fio_read_register(coreIdx, gdi_busy_flag) == gdi_status_check_value) break;
        }
        else {
            if (vdi_read_register(coreIdx, gdi_busy_flag) == 0x77) break;
        }
        //osal_msleep(1);   // 1ms sec
        //if (count++ > timeout)
        //  return -1;
    }

    return 0;
}

int vdi_wait_vpu_busy(unsigned long coreIdx, int timeout, unsigned int addr_bit_busy_flag)
{
    vdi_info_t *vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    //VDI must implement timeout action in this function for multi-vpu core scheduling efficiency.
    //the setting small value as timeout gives a chance to wait the other vpu core.

    while(1)
    {
#ifdef SUPPORT_SW_UART_ON_NONOS
        SwUartHandler(NULL);
#endif
        if (vdi_read_register(coreIdx, addr_bit_busy_flag) == 0)
            break;

        //osal_msleep(1);   // 1ms sec
        //if (count++ > timeout)
        //  return -1;
    }

    return 0;
}

int vdi_wait_vcpu_bus_busy(unsigned long coreIdx, int timeout, unsigned int gdi_busy_flag)
{
    vdi_info_t *vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    //VDI must implement timeout action in this function for multi-vpu core scheduling efficiency.
    //the setting small value as timeout gives a chance to wait the other vpu core.
    while(1)
    {
#ifdef SUPPORT_SW_UART_ON_NONOS
        SwUartHandler(NULL);
#endif
        if (vdi_fio_read_register(coreIdx, gdi_busy_flag) == 0x00)
            break;
        //osal_msleep(1);   // 1ms sec
        //if (count++ > timeout)
        //  return -1;
    }

    return 0;
}


static int vdi_get_int_reason(unsigned long core_idx)
{
    vdi_info_t *vdi = &s_vdi_info[core_idx];
    int intr_reason = 0;
    int int_sts_reg;
    int int_reason_reg;

    //VDI must implement timeout action in this function for multi-vpu core scheduling efficiency.
    //the setting small value as timeout gives a chance to wait the other vpu core.
    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00) {
        codec_slogerr("[VDI] %s: invalid vdi instance\n", __func__);
        return -1;
    }

    int_reason_reg = vdi->int_reason_reg;
    int_sts_reg = vdi->int_sts_reg;

#ifdef SUPPORT_SW_UART_ON_NONOS
    SwUartHandler(NULL);
#endif
    if (vdi_read_register(core_idx, int_sts_reg)) {

        if ((intr_reason=vdi_read_register(core_idx, int_reason_reg)) > 0) {
            if (PRODUCT_CODE_W_SERIES(vdi->product_code)) {
                vdi_write_register(core_idx, W5_VPU_VINT_REASON_CLR, intr_reason);
                vdi_write_register(core_idx, W5_VPU_VINT_CLEAR, 0x1);
            }

        } else {
            codec_slogerr("[VDI] spurious interrupt???\n");
        }
    }

    return intr_reason;
}

int vdi_wait_interrupt(unsigned long core_idx, unsigned int instIdx, int timeout)
{
    vdi_info_t *vdi = &s_vdi_info[core_idx];
    int intr_reason = -1;
    vpu_hwi_info_t *hwi = NULL;
    //unsigned long cur_time;

    //VDI must implement timeout action in this function for multi-vpu core scheduling efficiency.
    //the setting small value as timeout gives a chance to wait the other vpu core.
    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00) {
        codec_slogerr("[VDI] %s: invalid vdi instance\n", __func__);
        return -1;
    }

    if (vdi->op_mode == VPU_OPMODE_INTERRUPT) {
        hwi = vdi->hwi;
        if (!hwi) {
            codec_slogerr("[VDI] %s: NULL hwi pointer\n", __func__);
            return -1;
        }

        /* NOTE::
         * Enable the interrupt. Ideally we want to process faster and leave the
         * interrupt autoenabled. Currently this causes a deluge of interrupts.
         * The reason for the flood of interrupts needs to be looked at and once
         * addressed. The interrupt can be re-armed right away
         */
        InterruptUnmask(hwi->core_intr_num, hwi->evt_id);

        // wait until we get a signal from the threaded interrupt handler
        sem_wait(vdi->intr_sem);

        /* Received an interrupt! Process it. */
        intr_reason = vdi_get_int_reason(core_idx);

    } else {
        //cur_time = jiffies;
        while(1) {
            int int_reason_reg = vdi->int_reason_reg;
            int int_sts_reg = vdi->int_sts_reg;
#ifdef SUPPORT_SW_UART_ON_NONOS
            SwUartHandler(NULL);
#endif
            if (vdi_read_register(core_idx, int_sts_reg))
            {
                if ((intr_reason=vdi_read_register(core_idx, int_reason_reg)))
                {
                    if (PRODUCT_CODE_W_SERIES(vdi->product_code))
                    {
                        vdi_write_register(core_idx, W5_VPU_VINT_REASON_CLR, intr_reason);
                        vdi_write_register(core_idx, W5_VPU_VINT_CLEAR, 0x1);
                    }
                    break;
                }
            }

            /*
               if(jiffies_to_msecs(jiffies - cur_time) > timeout) {
               return -1;
               }
             */
        }
    }

    return intr_reason;
}

int vdi_get_system_endian(unsigned long coreIdx)
{
    vdi_info_t *vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;

    if (PRODUCT_CODE_W_SERIES(vdi->product_code))
        return VDI_128BIT_BUS_SYSTEM_ENDIAN;
    else
        return VDI_SYSTEM_ENDIAN;
}

int vdi_convert_endian(unsigned long coreIdx, unsigned int endian)
{
    vdi_info_t *vdi;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[coreIdx];

    if (!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;

    if (PRODUCT_CODE_W_SERIES(vdi->product_code)) {
        switch (endian) {
            case VDI_LITTLE_ENDIAN:       endian = 0x00; break;
            case VDI_BIG_ENDIAN:          endian = 0x0f; break;
            case VDI_32BIT_LITTLE_ENDIAN: endian = 0x04; break;
            case VDI_32BIT_BIG_ENDIAN:    endian = 0x03; break;
        }
    }
    return (endian&0x0f);
}

int swap_endian(unsigned long coreIdx, unsigned char *data, int len, int endian)
{
    vdi_info_t* vdi = &s_vdi_info[coreIdx];
    int         changes;
    int         sys_endian;
    BOOL        byteChange, wordChange, dwordChange, lwordChange;

    if (PRODUCT_CODE_W_SERIES(vdi->product_code)) {
        sys_endian = VDI_128BIT_BUS_SYSTEM_ENDIAN;
    }
    else {
        codec_slogerr("Unknown product id : %08x\n", vdi->product_code);
        return -1;
    }

    endian     = vdi_convert_endian(coreIdx, endian);
    sys_endian = vdi_convert_endian(coreIdx, sys_endian);
    if (endian == sys_endian)
        return 0;

    changes     = endian ^ sys_endian;
    byteChange  = changes&0x01;
    wordChange  = ((changes&0x02) == 0x02);
    dwordChange = ((changes&0x04) == 0x04);
    lwordChange = ((changes&0x08) == 0x08);

    if (byteChange)  osal_byte_swap(data, len);
    if (wordChange)  osal_word_swap(data, len);
    if (dwordChange) osal_dword_swap(data, len);
    if (lwordChange) osal_lword_swap(data, len);

    return 1;
}
