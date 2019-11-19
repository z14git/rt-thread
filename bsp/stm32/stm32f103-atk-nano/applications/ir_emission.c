/**
 * @file ir_emission.c
 * @author z14git
 * @brief 红外转发
 * @version 0.1
 * @date 2019-11-15
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

#define EMISSION_PIN GET_PIN(A, 1)

static struct fro_module ir_emission;
static char buf[12];

static int ir_emission_init(void)
{
    // rt_pin_mode(EMISSION_PIN, PIN_MODE_OUTPUT);
    drv_infrared_init();
    return 0;
}

static void ir_emission_run(struct rt_work *work, void *param)
{
    uint32_t *ptr_work_status;
    ptr_work_status = work->work_data;

    struct infrared_decoder_data infrared_data;

    /* 选择 NEC 解码器 */
    ir_select_decoder("nec");

    for (;;)
    {
        infrared_data.data.nec.addr = 0x00;   /* aka 用户码 */
        infrared_data.data.nec.key = 0x12;    /* aka command */
        infrared_data.data.nec.repeat = 0x01; /* 重复次数，底层驱动最多发8次 */
        infrared_write("nec", &infrared_data);
        rt_thread_mdelay(1500);
        if (*ptr_work_status == 0)
            break;
    }
}

static void ir_emission_deinit(void)
{
    drv_infrared_deinit();
    // rt_pin_mode(EMISSION_PIN, PIN_MODE_INPUT);
}

static const struct fro_module_ops ir_emission_ops =
    {
        ir_emission_init,
        ir_emission_deinit,
        ir_emission_run,
        RT_NULL,
        RT_NULL,
};

static int ir_emission_module_init(void)
{
    ir_emission.ops = &ir_emission_ops;
    ir_emission.type = M_INFRARED_TRANS;
    ir_emission.name = "红外转发";

    fro_module_register(&ir_emission);
    return 0;
}
INIT_APP_EXPORT(ir_emission_module_init);
