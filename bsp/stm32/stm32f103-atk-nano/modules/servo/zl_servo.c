/**
 * @file zl_servo.c
 * @author z14git
 * @brief 
 * @version 0.1
 * @date 2020-06-18
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "zl_servo.h"
#include <string.h>

static rt_err_t zl_servo_init(rt_device_t dev)
{
        return RT_EOK;
}

static rt_err_t zl_servo_open(rt_device_t dev, rt_uint16_t oflag)
{
        return RT_EOK;
}

static rt_err_t zl_servo_close(rt_device_t dev)
{
        return RT_EOK;
}
static rt_size_t zl_servo_read(rt_device_t dev, rt_off_t pos, void *buffer,
                               rt_size_t size)
{
        return 0;
}
static rt_size_t zl_servo_write(rt_device_t dev, rt_off_t pos,
                                const void *buffer, rt_size_t size)
{
        return 0;
}

static rt_err_t zl_servo_control(rt_device_t dev, int cmd, void *args)
{
        return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops servo_ops = {
        zl_servo_init, zl_servo_open,  zl_servo_close,
        zl_servo_read, zl_servo_write, zl_servo_control
};
#endif //RT_USING_DEVICE_OPS

int zl_servo_register(struct zl_servo_device *device, const char *name,
                      const struct zl_servo_ops *ops, const void *user_data)
{
        int ret;
        RT_ASSERT(device != RT_NULL);

        memset(device, 0, sizeof(struct zl_servo_device));

        device->parent.type = RT_Device_Class_Miscellaneous;
#ifdef RT_USING_DEVICE_OPS
        device->parent.ops = &servo_ops;
#else
        device->parent.init = zl_servo_init;
        device->parent.open = zl_servo_open;
        device->parent.close = zl_servo_close;
        device->parent.read = zl_servo_read;
        device->parent.write = zl_servo_write;
        device->parent.control = zl_servo_control;
#endif //RT_USING_DEVICE_OPS
        device->parent.user_data = (void *)user_data;
        device->ops = ops;

        ret = rt_device_register(&device->parent, name, RT_DEVICE_FLAG_RDWR);
        return ret;
}
