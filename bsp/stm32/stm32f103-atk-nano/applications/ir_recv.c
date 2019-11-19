/**
 * @file ir_recv.c
 * @author z14git
 * @brief 红外接收模块
 * @version 0.1
 * @date 2019-11-13
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "fro_module.h"
#include "infrared.h"
#include "drv_infrared.h"

#ifndef ULOG_USING_SYSLOG
#define LOG_TAG "GESTURE"
#define LOG_LVL LOG_LVL_INFO
#include <ulog.h>
#else
#include <syslog.h>
#endif /* ULOG_USING_SYSLOG */

#define RECV_PIN GET_PIN(A, 0)

static struct fro_module ir_recv;
static char buf[16];

static int ir_recv_init(void)
{
    rt_pin_mode(RECV_PIN, PIN_MODE_INPUT);
    drv_infrared_init();
    return 0;
}

static void ir_recv_run(struct rt_work *work, void *param)
{
    uint32_t *ptr_work_status;
    ptr_work_status = work->work_data;

    struct infrared_decoder_data infrared_data;

    /* 选择 NEC 解码器 */
    ir_select_decoder("nec");

    for (;;)
    {
        if (infrared_read("nec", &infrared_data) == RT_EOK)
        {
            LOG_I("RECEIVE OK: addr:0x%02X key:0x%02X repeat:%d",
                  infrared_data.data.nec.addr, infrared_data.data.nec.key, infrared_data.data.nec.repeat);
            rt_snprintf(buf, 16, "key:0x%X", infrared_data.data.nec.key);
            fro_module_info_str = buf;
        }
        else
        {
            fro_module_info_str = "";
        }
        rt_thread_mdelay(300);
        if (*ptr_work_status == 0)
            break;
    }
}

static void ir_recv_deinit(void)
{
    drv_infrared_deinit();
}

static const struct fro_module_ops ir_recv_ops =
    {
        ir_recv_init,
        ir_recv_deinit,
        ir_recv_run,
        RT_NULL,
        RT_NULL,
};

static int ir_recv_module_init(void)
{
    ir_recv.ops = &ir_recv_ops;
    ir_recv.type = M_INFRARED_RECV;
    ir_recv.name = "红外接收模块";

    fro_module_register(&ir_recv);
    return 0;
}
INIT_APP_EXPORT(ir_recv_module_init);
