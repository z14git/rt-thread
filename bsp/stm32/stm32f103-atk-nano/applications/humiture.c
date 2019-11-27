/**
 * @file humiture.c
 * @author z14git
 * @brief 温湿度传感器
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

#define delay_us rt_hw_us_delay
#define delay_ms(m)                                                            \
    do {                                                                       \
        delay_us(m * 100);                                                     \
        delay_us(m * 100);                                                     \
        delay_us(m * 100);                                                     \
        delay_us(m * 100);                                                     \
        delay_us(m * 100);                                                     \
        delay_us(m * 100);                                                     \
        delay_us(m * 100);                                                     \
        delay_us(m * 100);                                                     \
        delay_us(m * 100);                                                     \
        delay_us(m * 100);                                                     \
    } while (0)

#define DATA_PIN  GET_PIN(A, 0)
#define SDA_H()   rt_pin_write(DATA_PIN, PIN_HIGH)
#define SDA_L()   rt_pin_write(DATA_PIN, PIN_LOW)
#define GET_SDA() rt_pin_read(DATA_PIN)

static struct fro_module humiture;
static uint8_t           sensor_data[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
static uint16_t          temperature, humidity;
static char              buf[36];

/**
 * @brief 读传感器发送的单个字节
 *
 * @return uint8_t 传感器发送的单个字节
 */
static uint8_t read_sensor_data(void)
{
    uint16_t cnt;
    uint8_t  i;
    uint8_t  buffer, tmp;
    buffer = 0;
    for (i = 0; i < 8; i++) {
        cnt = 0;
        while (!GET_SDA()) //检测上次低电平是否结束
        {
            if (++cnt >= 3000) {
                break;
            }
        }
        //延时Min=26us Max50us 跳过数据"0" 的高电平
        delay_us(30); //延时30us

        //判断传感器发送数据位
        tmp = 0;
        if (GET_SDA()) {
            tmp = 1;
        }
        cnt = 0;
        while (GET_SDA()) //等待高电平 结束
        {
            if (++cnt >= 2000) {
                break;
            }
        }
        buffer <<= 1;
        buffer |= tmp;
    }
    return buffer;
}

/**
 * @brief 读传感器
 *
 * @return int32_t 0：读取成功； -1：读取失败
 */
static int32_t read_sensor(void)
{
    uint16_t overtime_cnt;
    uint8_t  i;
    //主机拉低(Min=800US Max=20Ms)
    SDA_L();
    delay_ms(2); //延时2Ms

    //释放总线 延时(Min=30us Max=50us)
    SDA_H();
    delay_us(30); //延时30us
    //主机设为输入 判断传感器响应信号
    SDA_H();

    //判断从机是否有低电平响应信号 如不响应则跳出，响应则向下运行
    if (GET_SDA() == 0) {
        overtime_cnt = 0;
        //判断从机是否发出 80us 的低电平响应信号是否结束
        while ((!GET_SDA())) {
            if (++overtime_cnt > 3000) //防止进入死循环
            {
                return -1;
            }
        }
        overtime_cnt = 0;
        //判断从机是否发出 80us 的高电平，如发出则进入数据接收状态
        while ((GET_SDA())) {
            if (++overtime_cnt > 3000) //防止进入死循环
            {
                return -1;
            }
        }
        // 数据接收 传感器共发送40位数据
        // 即5个字节 高位先送  5个字节分别为湿度高位 湿度低位 温度高位 温度低位
        // 校验和 校验和为：湿度高位+湿度低位+温度高位+温度低位
        for (i = 0; i < 5; i++) {
            sensor_data[i] = read_sensor_data();
        }
    } else {
        return -1; // 未收到传感器响应
    }
    return 0;
}

static int humiture_init(void)
{
    rt_pin_mode(DATA_PIN, PIN_MODE_OUTPUT_OD);
    rt_pin_write(DATA_PIN, PIN_HIGH);
    return 0;
}

static void humiture_run(struct rt_work *work, void *param)
{
    uint8_t   sum;
    uint32_t *ptr_work_status;
    ptr_work_status = work->work_data;
    rt_thread_mdelay(2000);
    if (*ptr_work_status == 0)
        return;
    for (;;) {
        rt_enter_critical();
        if (read_sensor() == 0) {
            sum = sensor_data[0] + sensor_data[1] + sensor_data[2] +
                  sensor_data[3];
            if (sum == sensor_data[4]) {
                humidity    = ((uint16_t)sensor_data[0] << 8) + sensor_data[1];
                temperature = ((uint16_t)sensor_data[2] << 8) + sensor_data[3];
            }
        }
        rt_exit_critical();
        if (*ptr_work_status == 0)
            break;
        rt_thread_mdelay(2000);
    }
}

static void humiture_deinit(void)
{
    temperature = 0;
    humidity    = 0;
    rt_pin_mode(DATA_PIN, PIN_MODE_INPUT);
}

static int humiture_read(void *cmd, void *data)
{
    rt_snprintf(buf,
                36,
                "温度:%d.%d℃湿度:%d.%dRH",
                temperature / 10,
                temperature % 10,
                humidity / 10,
                humidity % 10);

    *(char **)data = buf;
    return 0;
}

static const struct fro_module_ops humiture_ops = {
    humiture_init,
    humiture_deinit,
    humiture_run,
    RT_NULL,
    humiture_read,
};

static int humiture_module_init(void)
{
    humiture.ops  = &humiture_ops;
    humiture.type = M_HUMITURE;
    humiture.name = "温湿度传感器";

    temperature = 0;
    humidity    = 0;

    fro_module_register(&humiture);
    return 0;
}
INIT_COMPONENT_EXPORT(humiture_module_init);
