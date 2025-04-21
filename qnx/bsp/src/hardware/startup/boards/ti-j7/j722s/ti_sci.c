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


/*
 * TI J722S
 */
#include <startup.h>
#include "ti_sci.h"

#define Q_DATA_OFFSET(proxy, queue, reg)    \
            ((0x10000 * (proxy)) + (0x80 * (queue)) + ((reg) * 4))
#define Q_STATE_OFFSET(queue)           ((queue) * 0x4)
#define Q_STATE_ENTRY_COUNT_MASK        (0xFFF000)

#define SPROXY_THREAD_OFFSET(tid)       (0x1000 * (tid))
#define SPROXY_THREAD_DATA_OFFSET(tid, reg) \
            (unsigned)(SPROXY_THREAD_OFFSET(tid) + ((reg) * 0x4) + 0x4)

#define SPROXY_THREAD_STATUS_OFFSET(tid) (unsigned)(SPROXY_THREAD_OFFSET(tid))

#define SPROXY_THREAD_STATUS_COUNT_MASK (0xFF)

#define SPROXY_THREAD_CTRL_OFFSET(tid)  (0x1000 + SPROXY_THREAD_OFFSET(tid))
#define SPROXY_THREAD_CTRL_DIR_MASK     (0x1 << 31)

// hard code for now
#define SPROXY_QUEUE_COUNT              190
#define SPROXY_MAX_MESSAGE_SIZE         60

// TODO Make sure this is correct
#define SPROXY_RX_CHANNEL               12
#define SPROXY_TX_CHANNEL               13

#define SPROXY_TARGET_DATA(sci, chan)   ((sci)->target_data_addr + SPROXY_THREAD_DATA_OFFSET((chan), (sci)->tid))

#define MAX_RETRIES                     500000

static ti_sci_t ti_sci = {
    .scfg_addr        = 0x4A400000, // scfg
    .rt_addr          = 0x4A600000, // rt
    .target_data_addr = 0x4D000000, // target-data
    .max_message_size = SPROXY_MAX_MESSAGE_SIZE,
    .txchan           = SPROXY_TX_CHANNEL,
    .rxchan           = SPROXY_RX_CHANNEL,
    .host_id          = 12,
    .tid              = 0       // hard code to thread 0
};

static int seq = 0;

/*
 * memory copy for packed message structure to make sure there is no alignment violation
 */
static void sci_memcpy(void *dst, const void *src, size_t nbytes)
{
    while (nbytes) {
        *(unsigned char *)dst = *(const unsigned char *)src;
        dst = (char *)dst + 1;
        src = (const char *)src + 1;
        --nbytes;
    }
}

static int
ti_sci_send_message(const void *const data, const int tlen)
{
    const ti_sci_t    *const sci = &ti_sci;
    uint8_t     buf[SPROXY_MAX_MESSAGE_SIZE];
    uint32_t    *dport;
    const uint32_t    *dword;
    int         cnt;

    if (tlen > sci->max_message_size) {
        kprintf("message too long(%d), maximum message size %d\n", tlen, sci->max_message_size);
        return (-1);
    }

    memset(buf, 0, (unsigned)sci->max_message_size);
    memcpy(buf, data, (unsigned)tlen);
    dport = (uint32_t *)SPROXY_TARGET_DATA(sci, sci->txchan);
    dword = (uint32_t *)buf;
    for (cnt = 0; cnt < sci->max_message_size; cnt += 4) {
        *dport++ = *dword++;
    }

    return (0);
}

static inline void
ti_sci_setup_message(void *const buf, const uint8_t sequence, const uint8_t host_id, const uint16_t msg_type, const uint32_t msg_flags)
{
    struct ti_sci_msg_hdr   *hdr = buf;

    hdr->type  = msg_type;
    hdr->flags = msg_flags;
    hdr->host  = host_id;
    hdr->seq   = sequence;
}

static int
ti_sci_recv_message(void *const buf, const int len)
{
    ti_sci_t    *const sci = &ti_sci;
    uint32_t    rbuf[(SPROXY_MAX_MESSAGE_SIZE + sizeof(uint32_t) - 1) / sizeof(uint32_t)] = {};
    paddr_t     const base = sci->rt_addr;
    uint32_t    state;
    const uint32_t    *dport;
    uint32_t    *dword;
    int         cnt;
    int         loop_count = 0;

    while (1) {
        // use proxy 0
        state = in32(base + SPROXY_THREAD_STATUS_OFFSET(sci->rxchan));
        if (state & (1 << 31)) {
            kprintf("message error\n");
            return (-1);
        }
        state &= 0xff;
        if (state > 0) {
            break;
        }
        if (loop_count++ > MAX_RETRIES) {
            kprintf("ti_sci_recv_message, unable to receive message\n");
            return (-1);
        }
    }

    // we got a message
    dport = (uint32_t *)SPROXY_TARGET_DATA(sci, sci->rxchan);
    dword = (uint32_t *)rbuf;
    for (cnt = 0; cnt < sci->max_message_size; cnt += 4) {
        *dword++ = *dport++;
    }

    memcpy(buf, rbuf, (unsigned)len);

    return 0;
}

static int ti_sci_do_xfer(const void *const tbuf, const int tsize, void *const rbuf, const int rsize)
{
    if (ti_sci_send_message(tbuf, tsize) != 0) {
        return (-1);
    }

    if (ti_sci_recv_message(rbuf, rsize) != 0) {
        return (-1);
    }

    return (0);
}

int ti_sci_cmd_get_revision(struct ti_sci_msg_resp_version *const revinfo)
{
    const ti_sci_t    *const sci = &ti_sci;
    uint32_t    buf[(SPROXY_MAX_MESSAGE_SIZE + sizeof(uint32_t) - 1) / sizeof(uint32_t)];

    ti_sci_setup_message((void *)buf, (uint8_t)seq++, (uint8_t)sci->host_id, TI_SCI_MSG_VERSION, TI_SCI_FLAG_REQ_ACK_ON_PROCESSED);
    seq &= 7;

    if (ti_sci_do_xfer(buf, (int)sizeof(struct ti_sci_msg_hdr),
            revinfo, (int)sizeof(struct ti_sci_msg_resp_version)) != 0) {
        return (-1);
    }

    return (0);
}

/**
 * ti_sci_cmd_clk_get_freq() - Get current frequency
 * @handle: pointer to TI SCI handle
 * @dev_id: Device identifier this request is for
 * @clk_id: Clock identifier for the device for this request.
 *      Each device has it's own set of clock inputs. This indexes
 *      which clock input to modify.
 * @freq:   Currently frequency in Hz
 *
 * Return: 0 if all went well, else returns appropriate error value.
 */
int ti_sci_cmd_clk_get_freq(const uint32_t dev_id, const uint32_t clk_id, uint64_t *freq)
{
    const ti_sci_t    *const sci = &ti_sci;
    uint64_t    buf[(SPROXY_MAX_MESSAGE_SIZE + sizeof(uint64_t) - 1) / sizeof(uint64_t)];
    struct ti_sci_msg_req_get_clock_freq *req;
    const struct ti_sci_msg_resp_get_clock_freq *resp;

    ti_sci_setup_message((void *)buf, (uint8_t)seq++,
                    (uint8_t)sci->host_id, TI_SCI_MSG_GET_CLOCK_FREQ, TI_SCI_FLAG_REQ_ACK_ON_PROCESSED);
    req = (struct ti_sci_msg_req_get_clock_freq *)buf;
    req->dev_id = dev_id;
    if (clk_id < 255) {
        req->clk_id = (uint8_t)clk_id;
    } else {
        req->clk_id = 255;
        sci_memcpy(&req->clk_id_32, &clk_id, sizeof(uint32_t));
    }

    if (ti_sci_do_xfer(buf, sizeof(*req), buf, sizeof(*resp)) != 0) {
        return (-1);
    }

    resp = (struct ti_sci_msg_resp_get_clock_freq *)buf;
    if (resp->hdr.flags != TI_SCI_FLAG_RESP_GENERIC_ACK) {
        kprintf("%s: response NACK\n", __func__);
        return (-1);
    }

    *freq = resp->freq_hz;

    return (0);
}


/**
 * ti_sci_cmd_get_boot_status() - Command to get the processor boot status
 * @handle: Pointer to TI SCI handle
 * @proc_id:    Processor ID this request is for
 *
 * Return: 0 if all went well, else returns appropriate error value.
 */
int ti_sci_cmd_proc_get_status(const uint8_t proc_id, uint64_t *bv, uint32_t *cfg_flags, uint32_t *ctrl_flags, uint32_t *sts_flags)
{
    const ti_sci_t    *const sci = &ti_sci;
    uint32_t    buf[(SPROXY_MAX_MESSAGE_SIZE + sizeof(uint32_t) - 1) / sizeof(uint32_t)];
    const struct ti_sci_msg_resp_get_status *resp;
    struct ti_sci_msg_req_get_status *req;

    ti_sci_setup_message((void *)buf, (uint8_t)seq++,
            (uint8_t)sci->host_id, TI_SCI_MSG_GET_STATUS, TI_SCI_FLAG_REQ_ACK_ON_PROCESSED);

    req = (struct ti_sci_msg_req_get_status *)buf;
    req->processor_id = proc_id;

    if (ti_sci_do_xfer(buf, sizeof(*req), buf, sizeof(*resp)) != 0) {
        return (-1);
    }

    resp = (struct ti_sci_msg_resp_get_status *)buf;
    if (resp->hdr.flags != TI_SCI_FLAG_RESP_GENERIC_ACK) {
        kprintf("%s: response NACK\n", __func__);
        return (-1);
    }

    if (bv != NULL) {
        *bv = UNALIGNED_RET32(&resp->bootvector_high);
        *bv <<= TI_SCI_ADDR_HIGH_SHIFT;
        *bv = UNALIGNED_RET32(&resp->bootvector_low);
    }

    if (cfg_flags != NULL) {
        *cfg_flags = UNALIGNED_RET32(&resp->config_flags);
    }

    if (ctrl_flags != NULL) {
        *ctrl_flags = UNALIGNED_RET32(&resp->control_flags);
    }

    if (sts_flags != NULL) {
        *sts_flags = UNALIGNED_RET32(&resp->status_flags);
    }

    return (0);
}

/**
 * ti_sci_cmd_clk_set_freq() - Set a frequency for clock
 * @handle: pointer to TI SCI handle
 * @dev_id: Device identifier this request is for
 * @clk_id: Clock identifier for the device for this request.
 *      Each device has it's own set of clock inputs. This indexes
 *      which clock input to modify.
 * @min_freq:   The minimum allowable frequency in Hz. This is the minimum
 *      allowable programmed frequency and does not account for clock
 *      tolerances and jitter.
 * @target_freq: The target clock frequency in Hz. A frequency will be
 *      processed as close to this target frequency as possible.
 * @max_freq:   The maximum allowable frequency in Hz. This is the maximum
 *      allowable programmed frequency and does not account for clock
 *      tolerances and jitter.
 *
 * Return: 0 if all went well, else returns appropriate error value.
 */
int ti_sci_cmd_clk_set_freq(const uint32_t dev_id, const uint32_t clk_id, const uint64_t min_freq, const uint64_t target_freq, const uint64_t max_freq)
{
    const ti_sci_t    *const sci = &ti_sci;
    uint32_t    buf[(SPROXY_MAX_MESSAGE_SIZE + sizeof(uint32_t) - 1) / sizeof(uint32_t)];
    struct ti_sci_msg_req_set_clock_freq *req;
    const struct ti_sci_msg_hdr *resp;

    ti_sci_setup_message((void *)buf, (uint8_t)seq++,
            (uint8_t)sci->host_id, TI_SCI_MSG_SET_CLOCK_FREQ, TI_SCI_FLAG_REQ_ACK_ON_PROCESSED);

    req = (struct ti_sci_msg_req_set_clock_freq *)buf;
    req->dev_id = dev_id;
    if (clk_id < 255) {
        req->clk_id = (uint8_t)clk_id;
    } else {
        req->clk_id = 255;
        sci_memcpy(&req->clk_id_32, &clk_id, sizeof(uint32_t));
    }
#if 0
    req->min_freq_hz = min_freq;
    req->target_freq_hz = target_freq;
    req->max_freq_hz = max_freq;
#else
    sci_memcpy(&req->min_freq_hz, &min_freq, sizeof(req->min_freq_hz));
    sci_memcpy(&req->target_freq_hz, &target_freq, sizeof(req->target_freq_hz));
    sci_memcpy(&req->max_freq_hz, &max_freq, sizeof(req->max_freq_hz));
#endif

    if (ti_sci_do_xfer(buf, sizeof(*req), buf, sizeof(*resp)) != 0) {
        return (-1);
    }

    resp = (struct ti_sci_msg_hdr *)buf;
    if (resp->flags != TI_SCI_FLAG_RESP_GENERIC_ACK) {
        kprintf("%s: response NACK\n", __func__);
        return (-1);
    }

    return (0);
}

/**
 * ti_sci_set_clock_state() - Set clock state helper
 * @handle: pointer to TI SCI handle
 * @dev_id: Device identifier this request is for
 * @clk_id: Clock identifier for the device for this request.
 *      Each device has it's own set of clock inputs. This indexes
 *      which clock input to modify.
 * @flags:  Header flags as needed
 * @state:  State to request for the clock.
 *
 * Return: 0 if all went well, else returns appropriate error value.
 */
int ti_sci_set_clock_state(const uint32_t dev_id, const uint32_t clk_id, const uint32_t flags, const uint8_t state)
{
    const ti_sci_t    *const sci = &ti_sci;
    uint32_t    buf[(SPROXY_MAX_MESSAGE_SIZE + sizeof(uint32_t) - 1) / sizeof(uint32_t)];
    struct ti_sci_msg_req_set_clock_state *req;
    const struct ti_sci_msg_hdr *resp;

    ti_sci_setup_message((void *)buf, (uint8_t)seq++,
            (uint8_t)sci->host_id, TI_SCI_MSG_SET_CLOCK_STATE, flags | TI_SCI_FLAG_REQ_ACK_ON_PROCESSED);

    req = (struct ti_sci_msg_req_set_clock_state *)buf;
    req->dev_id = dev_id;
    if (clk_id < 255) {
        req->clk_id = (uint8_t)clk_id;
    } else {
        req->clk_id = 255;
        sci_memcpy(&req->clk_id_32, &clk_id, sizeof(uint32_t));
    }
    req->request_state = state;

    if (ti_sci_do_xfer(buf, sizeof(*req), buf, sizeof(*resp)) != 0) {
        return (-1);
    }

    resp = (struct ti_sci_msg_hdr *)buf;
    if (resp->flags != TI_SCI_FLAG_RESP_GENERIC_ACK) {
        kprintf("%s: response NACK\n", __func__);
        return (-1);
    }

    return (0);
}

int ti_sci_cmd_set_clk_parent(const uint32_t dev_id, const uint32_t clk_id, const uint32_t parent)
{
    const ti_sci_t    *const sci = &ti_sci;
    uint32_t    buf[(SPROXY_MAX_MESSAGE_SIZE + sizeof(uint32_t) - 1) / sizeof(uint32_t)];
    struct tisci_msg_set_clock_parent_req *req;
    const struct ti_sci_msg_hdr *resp;

    ti_sci_setup_message((void *)buf, (uint8_t)seq++,
            (uint8_t)sci->host_id, TI_SCI_MSG_SET_CLOCK_PARENT, TI_SCI_FLAG_REQ_ACK_ON_PROCESSED);

    req = (struct tisci_msg_set_clock_parent_req *)buf;
    req->dev_id = dev_id;
    if (clk_id < 255) {
        req->clk_id = (uint8_t)clk_id;
    } else {
        req->clk_id = 255;
        sci_memcpy(&req->clk_id_32, &clk_id, sizeof(uint32_t));
    }
    if (parent < 255) {
        req->parent = (uint8_t)parent;
    } else {
        req->parent = 255;
        sci_memcpy(&req->parent_id_32, &parent, sizeof(uint32_t));
    }

    if (ti_sci_do_xfer(buf, sizeof(*req), buf, sizeof(*resp)) != 0) {
        return (-1);
    }

    resp = (struct ti_sci_msg_hdr *)buf;
    if (resp->flags != TI_SCI_FLAG_RESP_GENERIC_ACK) {
        kprintf("%s: response NACK\n", __func__);
        return (-1);
    }

    return (0);
}

/**
 * ti_sci_cmd_get_clock_state() - Get clock state helper
 * @handle: pointer to TI SCI handle
 * @dev_id: Device identifier this request is for
 * @clk_id: Clock identifier for the device for this request.
 *      Each device has it's own set of clock inputs. This indexes
 *      which clock input to modify.
 * @programmed_state:   State requested for clock to move to
 * @current_state:  State that the clock is currently in
 *
 * Return: 0 if all went well, else returns appropriate error value.
 */
int ti_sci_cmd_get_clock_state(const uint32_t dev_id, const uint32_t clk_id, uint8_t *programmed_state, uint8_t *current_state)
{
    const ti_sci_t    *const sci = &ti_sci;
    uint32_t    buf[(SPROXY_MAX_MESSAGE_SIZE + sizeof(uint32_t) - 1) / sizeof(uint32_t)];
    struct ti_sci_msg_req_get_clock_state *req;
    const struct ti_sci_msg_resp_get_clock_state *resp;

    ti_sci_setup_message((void *)buf, (uint8_t)seq++,
            (uint8_t)sci->host_id, TI_SCI_MSG_GET_CLOCK_STATE, TI_SCI_FLAG_REQ_ACK_ON_PROCESSED);

    req = (struct ti_sci_msg_req_get_clock_state *)buf;
    if (clk_id < 255) {
        req->clk_id = (uint8_t)clk_id;
    } else {
        req->clk_id = 255;
        sci_memcpy(&req->clk_id_32, &clk_id, sizeof(uint32_t));
    }
    req->clk_id = (uint8_t)clk_id;

    if (ti_sci_do_xfer(buf, sizeof(*req), buf, sizeof(*resp)) != 0) {
        return (-1);
    }

    resp = (struct ti_sci_msg_resp_get_clock_state *)buf;
    if (resp->hdr.flags != TI_SCI_FLAG_RESP_GENERIC_ACK) {
        kprintf("%s: response NACK\n", __func__);
        return (-1);
    }

    if (programmed_state) {
        *programmed_state = resp->programmed_state;
    }
    if (current_state) {
        *current_state = resp->current_state;
    }

    return (0);
}

/**
 * ti_sci_set_device_state() - Set device state helper
 * @handle: pointer to TI SCI handle
 * @id:     Device identifier
 * @flags:  flags to setup for the device
 * @state:  State to move the device to
 *
 * Return: 0 if all went well, else returns appropriate error value.
 */
int ti_sci_set_device_state(const uint32_t id, const uint32_t flags, const uint8_t state)
{
    const ti_sci_t    *const sci = &ti_sci;
    uint32_t    buf[(SPROXY_MAX_MESSAGE_SIZE + sizeof(uint32_t) - 1) / sizeof(uint32_t)];
    struct ti_sci_msg_req_set_device_state *req;
    const struct ti_sci_msg_hdr *resp;

    ti_sci_setup_message((void *)buf, (uint8_t)seq++,
            (uint8_t)sci->host_id, TI_SCI_MSG_SET_DEVICE_STATE, flags | TI_SCI_FLAG_REQ_ACK_ON_PROCESSED);

    req = (struct ti_sci_msg_req_set_device_state *)buf;
    req->id = id;
    req->state = state;

    if (ti_sci_do_xfer(buf, sizeof(*req), buf, sizeof(*resp)) != 0) {
        return (-1);
    }

    resp = (struct ti_sci_msg_hdr *)buf;
    if (resp->flags != TI_SCI_FLAG_RESP_GENERIC_ACK) {
        kprintf("%s: response NACK\n", __func__);
        return (-1);
    }

    return (0);
}

/**
 * ti_sci_get_device_state() - Get device state helper
 * @handle: Handle to the device
 * @id:     Device Identifier
 * @clcnt:  Pointer to Context Loss Count
 * @resets: pointer to resets
 * @p_state:    pointer to p_state
 * @c_state:    pointer to c_state
 *
 * Return: 0 if all went fine, else return appropriate error.
 */
int ti_sci_get_device_state(const uint32_t id,  uint32_t *clcnt,  uint32_t *resets, uint8_t *p_state,  uint8_t *c_state)
{
    const ti_sci_t    *const sci = &ti_sci;
    uint32_t    buf[(SPROXY_MAX_MESSAGE_SIZE + sizeof(uint32_t) - 1) / sizeof(uint32_t)];
    struct ti_sci_msg_req_get_device_state *req;
    const struct ti_sci_msg_resp_get_device_state *resp;

    ti_sci_setup_message((void *)buf, (uint8_t)seq++,
            (uint8_t)sci->host_id, TI_SCI_MSG_GET_DEVICE_STATE, TI_SCI_FLAG_REQ_ACK_ON_PROCESSED);

    req = (struct ti_sci_msg_req_get_device_state *)buf;
    req->id = id;

    if (ti_sci_do_xfer(buf, sizeof(*req), buf, sizeof(*resp)) != 0) {
        return (-1);
    }

    resp = (struct ti_sci_msg_resp_get_device_state *)buf;
    if (resp->hdr.flags != TI_SCI_FLAG_RESP_GENERIC_ACK) {
        kprintf("%s: response NACK\n", __func__);
        return (-1);
    }

    if (clcnt) {
        *clcnt = resp->context_loss_count;
    }
    if (resets) {
        *resets = resp->resets;
    }
    if (p_state) {
        *p_state = resp->programmed_state;
    }
    if (c_state) {
        *c_state = resp->current_state;
    }

    return (0);
}

/**
 * ti_sci_set_device_resets() - command to set resets for device managed
 *                  by TISCI
 * @handle: Pointer to TISCI handle as retrieved by *ti_sci_get_handle
 * @id:     Device Identifier
 * @reset_state: Device specific reset bit field
 *
 * Return: 0 if all went fine, else return appropriate error.
 */
int ti_sci_set_device_resets(const uint32_t id, const uint32_t reset_state)
{
    const ti_sci_t    *const sci = &ti_sci;
    uint32_t    buf[(SPROXY_MAX_MESSAGE_SIZE + sizeof(uint32_t) - 1) / sizeof(uint32_t)];
    struct ti_sci_msg_req_set_device_resets *req;
    const struct ti_sci_msg_hdr *resp;

    ti_sci_setup_message((void *)buf, (uint8_t)seq++,
        (uint8_t)sci->host_id, TI_SCI_MSG_SET_DEVICE_RESETS, TI_SCI_FLAG_REQ_ACK_ON_PROCESSED);

    req = (struct ti_sci_msg_req_set_device_resets *)buf;
    req->id = id;
    req->resets = reset_state;

    if (ti_sci_do_xfer(buf, sizeof(*req), buf, sizeof(*resp)) != 0) {
        return (-1);
    }

    resp = (struct ti_sci_msg_hdr *)buf;
    if (resp->flags != TI_SCI_FLAG_RESP_GENERIC_ACK) {
        kprintf("%s: response NACK\n", __func__);
        return (-1);
    }

    return (0);
}

int ti_sci_sys_reset(void)
{
    const ti_sci_t    *const sci = &ti_sci;
    uint8_t     buf[SPROXY_MAX_MESSAGE_SIZE];
    const struct ti_sci_msg_hdr   *rhdr;

    ti_sci_setup_message((void *)buf, (uint8_t)seq++,
            (uint8_t)sci->host_id, TI_SCI_MSG_SYS_RESET, TI_SCI_FLAG_REQ_ACK_ON_PROCESSED);

    if (ti_sci_do_xfer(buf, (int)sizeof(struct ti_sci_msg_hdr),
                            buf, (int)sizeof(struct ti_sci_msg_hdr)) != 0) {
        return (-1);
    }

    rhdr = (struct ti_sci_msg_hdr *)buf;
    if (rhdr->flags != TI_SCI_FLAG_RESP_GENERIC_ACK) {
        kprintf("%s: response NACK\n", __func__);
        return (-1);
    }

    return (0);
}

/*
 *
 */
void *
ti_sci_init(void)
{
    struct ti_sci_msg_resp_version  revinfo;
    static uint32_t init_flag = 0;

    if (!init_flag)
    {
        if (debug_flag > 0) {
            if (ti_sci_cmd_get_revision(&revinfo) != -1) {
                kprintf("SYSFW ABI: %d.%d (firmware rev %d '%s'\n",
                    revinfo.abi_major, revinfo.abi_minor, revinfo.firmware_revision, revinfo.firmware_description);
            }
        }

        init_flag = 1;
    }

    return (void *)&ti_sci;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/startup/boards/ti-j7/j722s/ti_sci.c $ $Rev: 994584 $")
#endif
