/**
 * @file drv_pca9685.c
 * @author z14git
 * @brief 
 * @version 0.1
 * @date 2020-01-18
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "drv_pca9685.h"
#include <board.h>

#ifndef ULOG_USING_SYSLOG
    #define LOG_TAG "pca9685"
    #define LOG_LVL LOG_LVL_ASSERT
    #include <ulog.h>
#else
    #include <syslog.h>
#endif /* ULOG_USING_SYSLOG */

// Registers/etc
#define DEFAULT_ADDRESS 0x40
#define MODE1           0x00
#define MODE2           0x01
#define SUBADR1         0x02
#define SUBADR2         0x03
#define SUBADR3         0x04
#define PRESCALE        0xFE
#define LED0_ON_L       0x06
#define LED0_ON_H       0x07
#define LED0_OFF_L      0x08
#define LED0_OFF_H      0x09
#define ALL_LED_ON_L    0xFA
#define ALL_LED_ON_H    0xFB
#define ALL_LED_OFF_L   0xFC
#define ALL_LED_OFF_H   0xFD

// Bits
#define RESTART 0x80
#define SLEEP   0x10
#define ALLCALL 0x01
#define INVRT   0x10
#define OUTDRV  0x04

static int write_byte(struct pca9685_device *dev, uint8_t reg, uint8_t data)
{
    struct rt_i2c_msg msgs;
    uint8_t           buf[2] = {reg, data};
    RT_ASSERT(dev);

    msgs.addr  = dev->i2c_addr;
    msgs.flags = RT_I2C_WR;
    msgs.buf   = buf;
    msgs.len   = sizeof(buf);

    if (rt_i2c_transfer(dev->bus, &msgs, 1) == 1) {
        return RT_EOK;
    } else {
        return -RT_ERROR;
    }
}

static int read_byte(struct pca9685_device *dev, uint8_t reg, uint8_t *buff)
{
    struct rt_i2c_msg msgs[2];
    RT_ASSERT(dev);

    msgs[0].addr  = dev->i2c_addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf   = &reg;
    msgs[0].len   = 1;

    msgs[1].addr  = dev->i2c_addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf   = buff;
    msgs[1].len   = 1;

    if (rt_i2c_transfer(dev->bus, msgs, 2) == 2) {
        return RT_EOK;
    } else {
        return -RT_ERROR;
    }
}

static int set_pwm_freq(struct pca9685_device *dev, uint32_t freq_hz)
{
    RT_ASSERT(dev);
    RT_ASSERT(freq_hz <= 1526 && freq_hz >= 24);
    float prescaleval = 25000000.0; // 25MHz
    prescaleval /= 4096.0;          // 12-bit
    prescaleval /= (float)freq_hz;
    prescaleval -= 1.0;

    uint8_t prescale = (uint8_t)(prescaleval + 0.5);
    uint8_t old_mode;
    read_byte(dev, MODE1, &old_mode);
    uint8_t new_mode = (old_mode & 0x7F) | 0x10; // sleep
    write_byte(dev, MODE1, new_mode);            // go to sleep
    write_byte(dev, PRESCALE, prescale);
    write_byte(dev, MODE1, old_mode);
    rt_hw_us_delay(500);
    return write_byte(dev, MODE1, old_mode | 0x80);
}

static int set_pwm(struct pca9685_device *dev,
                   uint8_t                channel,
                   uint16_t               on_time,
                   uint16_t               off_time)
{
    RT_ASSERT(dev);
    write_byte(dev, LED0_ON_L + 4 * channel, on_time & 0xFF);
    write_byte(dev, LED0_ON_H + 4 * channel, on_time >> 8);
    write_byte(dev, LED0_OFF_L + 4 * channel, off_time & 0xFF);
    return write_byte(dev, LED0_OFF_H + 4 * channel, off_time >> 8);
}

static int
set_all_pwm(struct pca9685_device *dev, uint8_t on_time, uint8_t off_time)
{
    RT_ASSERT(dev);
    write_byte(dev, ALL_LED_ON_L, on_time & 0xFF);
    write_byte(dev, ALL_LED_ON_H, on_time >> 8);
    write_byte(dev, ALL_LED_OFF_L, off_time & 0xFF);
    return write_byte(dev, ALL_LED_OFF_H, off_time >> 8);
}

static int pca9685_init(struct pca9685_device *dev)
{
    int ret = 0;
    RT_ASSERT(dev);
    set_all_pwm(dev, 0, 0);
    write_byte(dev, MODE2, OUTDRV);
    write_byte(dev, MODE1, ALLCALL);
    rt_hw_us_delay(500);
    uint8_t mode1;
    read_byte(dev, MODE1, &mode1);
    mode1 = mode1 & ~SLEEP; // wake up (reset sleep)
    ret   = write_byte(dev, MODE1, mode1);
    rt_hw_us_delay(500); // wait for oscillator
    return ret;
}

static const struct pca9685_ops _ops = {
    .init         = pca9685_init,
    .set_pwm_freq = set_pwm_freq,
    .set_pwm      = set_pwm,
    .set_all_pwm  = set_all_pwm,
};

struct pca9685_device *pca9685_device_create(const char *bus_name,
                                             uint8_t     i2c_addr)
{
    RT_ASSERT(bus_name);
    struct pca9685_device *   dev = RT_NULL;
    struct rt_i2c_bus_device *bus = RT_NULL;

    dev = rt_calloc(1, sizeof(struct pca9685_device));
    if (RT_NULL == dev) {
        LOG_E("no enough memory for creating pca9685 device");
        goto __exit;
    }

    bus = rt_i2c_bus_device_find(bus_name);
    if (RT_NULL == bus) {
        LOG_E("i2c bus \"%s\" not found", bus_name);
        goto __exit;
    }
    dev->bus = bus;

    if (i2c_addr == 0) {
        dev->i2c_addr = DEFAULT_ADDRESS;
    } else {
        dev->i2c_addr = i2c_addr;
    }

    dev->ops = &_ops;

    return dev;
__exit:
    if (dev != RT_NULL) {
        rt_free(dev);
    }
    return RT_NULL;
}
