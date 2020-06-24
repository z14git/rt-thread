/**
 * @file zl_motor.c
 * @author z14git
 * @brief 
 * @version 0.1
 * @date 2020-06-23
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "zl_motor.h"
#include <string.h>

static rt_err_t zl_motor_control(rt_device_t dev, int cmd, void *args)
{
        return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops motor_ops = { .control = zl_motor_control };
#endif //RT_USING_DEVICE_OPS

int zl_motor_register(struct zl_motor_device *device, const char *name,
                      const struct zl_motor_ops *ops, const void *user_data)
{
        int ret;
        RT_ASSERT(device != RT_NULL);
        memset(device, 0, sizeof(struct zl_motor_device));

        device->parent.type = RT_Device_Class_Miscellaneous;
#ifdef RT_USING_DEVICE_OPS
        device->parent.ops = &motor_ops;
#else
        device->parent.init = RT_NULL;
        device->parent.open = RT_NULL;
        device->parent.close = RT_NULL;
        device->parent.read = RT_NULL;
        device->parent.write = RT_NULL;
        device->parent.control = zl_motor_control;
#endif //RT_USING_DEVICE_OPS
        device->parent.user_data = (void *)user_data;
        device->ops = ops;

        ret = rt_device_register(&device->parent, name, RT_DEVICE_FLAG_RDWR);
        return ret;
}
