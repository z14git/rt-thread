/**
 * @file drv_at24cxx.c
 * @author z14git
 * @brief AT24C02驱动
 * @version 0.1
 * @date 2018-11-22
 *
 * @copyright Copyright (c) 2018
 *
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#ifndef ULOG_USING_SYSLOG
#define LOG_TAG "AT24CXX"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>
#else
#include <syslog.h>
#endif /* ULOG_USING_SYSLOG */

#define AT24CXX_ADDR (0xa0 >> 1)
#define AT24CXX_I2CBUS_NAME "i2c1" /* 与drv_i2c.c中的名称一致 */

static struct rt_i2c_bus_device *at24cxx_i2c_bus;

/**
 * @brief 往指定地址写入1字节数据
 *
 * @param wr_addr 数据的地址
 * @param data 需要写入的数据
 * @return rt_err_t 成功返加RT_EOK，错误返回-RT_ERROR
 */
rt_err_t at24cxx_write_byte(rt_uint8_t wr_addr, rt_uint8_t data)
{
    struct rt_i2c_msg msgs;
    rt_uint8_t buf[2] = {wr_addr, data};

    msgs.addr = AT24CXX_ADDR;
    msgs.flags = RT_I2C_WR;
    msgs.buf = buf;
    msgs.len = sizeof(buf);

    if (rt_i2c_transfer(at24cxx_i2c_bus, &msgs, 1) == 1)
    {
        rt_thread_mdelay(5); //该芯片每次写入数据后至少等5ms才能再次操作
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}

/**
 * @brief 从 rd_addr 地址读取1字节到*rd_buf中
 *
 * @param rd_addr
 * @param rd_buf
 * @return rt_err_t 成功返加RT_EOK，错误返回-RT_ERROR
 */
rt_err_t at24cxx_read_byte(rt_uint8_t rd_addr, rt_uint8_t *rd_buf)
{
    struct rt_i2c_msg msgs[2];
    // rt_uint8_t wr_buf[2] = {(rd_addr >> 8) & 0xff, rd_addr & 0xff};

    msgs[0].addr = AT24CXX_ADDR;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = &rd_addr;
    msgs[0].len = 1;

    msgs[1].addr = AT24CXX_ADDR;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf = rd_buf;
    msgs[1].len = 1;

    if (rt_i2c_transfer(at24cxx_i2c_bus, msgs, 2) == 2)
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}

/**
 * @brief
 *
 * @param wr_addr
 * @param wr_buf
 * @param num
 * @return rt_err_t
 */
rt_err_t at24cxx_write_n_byte(rt_uint8_t wr_addr, rt_uint8_t *wr_buf, rt_uint8_t num)
{
    rt_err_t ret;
    while (num--)
    {
        ret = at24cxx_write_byte(wr_addr++, *wr_buf++);
        if (RT_EOK != ret)
        {
            return ret;
        }
    }
    return RT_EOK;
}

/**
 * @brief
 *
 * @param rd_addr
 * @param rd_buf
 * @param num
 * @return rt_err_t
 */
rt_err_t at24cxx_read_n_byte(rt_uint8_t rd_addr, rt_uint8_t *rd_buf, rt_uint8_t num)
{
    rt_err_t ret;
    while (num--)
    {
        ret = at24cxx_read_byte(rd_addr++, rd_buf++);
        if (RT_EOK != ret)
        {
            return ret;
        }
    }
    return RT_EOK;
}

int at24cxx_hw_init(void)
{
    at24cxx_i2c_bus = rt_i2c_bus_device_find(AT24CXX_I2CBUS_NAME);
    if (RT_NULL == at24cxx_i2c_bus)
    {
        LOG_D("can't find at24cxx device\r\n");
        return -RT_ERROR;
    }

    return RT_EOK;
}
INIT_APP_EXPORT(at24cxx_hw_init);

static int at24_write(int argc, char **argv)
{
    int32_t ret;
    char *p_end;
    uint8_t i;
    uint8_t addr;
    uint8_t buf[FINSH_ARG_MAX - 2];
    if (argc < 3)
    {
        /* 输入格式有误 */
        rt_kprintf("\xca\xe4\xc8\xeb\xb8\xf1\xca\xbd\xd3\xd0\xce\xf3\r\n");

        /* 请按以下格式输入： */
        rt_kprintf("\xc7\xeb\xb0\xb4\xd2\xd4\xcf\xc2\xb8\xf1"
               "\xca\xbd\xca\xe4\xc8\xeb:\r\n");

        /*   write [addr] [data1] [data2] ...\r\n */
        rt_kprintf("  write [addr] [data1] [data2] ...\\r\\n\r\n");

        /*   addr: 要写入的EEPROM地址 */
        rt_kprintf("  addr: \xd2\xaa\xd0\xb4\xc8\xeb\xb5\xc4"
               "EEPROM\xb5\xd8\xd6\xb7\r\n");

        /*   data: 要写入的数据，该命令支持一次写入5个数据 */
        rt_kprintf("  data: \xd2\xaa\xd0\xb4\xc8\xeb\xb5\xc4\xca\xfd\xbe"
               "\xdd\xa3\xac\xb8\xc3\xc3\xfc\xc1\xee\xd6\xa7"
               "\xb3\xd6\xd2\xbb\xb4\xce\xd0\xb4\xc8\xeb"
               "5\xb8\xf6\xca\xfd\xbe\xdd\r\n");

        /*   \r\n: 回车 */
        rt_kprintf("  \\r\\n: \xbb\xd8\xb3\xb5\r\n");
        return -1;
    }

    addr = strtol(argv[1], &p_end, 0);
    if (*p_end != '\0')
    {
        rt_kprintf("illegal addr format: '%s'\r\n", argv[1]);
        return -1;
    }

    for (i = 2; i < argc; i++)
    {
        buf[i - 2] = strtol(argv[i], &p_end, 0);
        if (*p_end != '\0')
        {
            rt_kprintf("illegal data format: '%s'\r\n", argv[i]);
            return -1;
        }
    }

    ret = at24cxx_write_n_byte(addr, buf, argc - 2);
    if (0 != ret)
    {
        rt_kprintf("write at24cxx failed\r\n");
        return ret;
    }
    /* 已从[%02Xh]地址开始写入： */
    rt_kprintf("\xd2\xd1\xb4\xd3[%02Xh]\xb5\xd8\xd6\xb7\xbf\xaa"
           "\xca\xbc\xd0\xb4\xc8\xeb\xa3\xba",
           addr);
    for (i = 2; i < argc; i++)
    {
        rt_kprintf(" 0x%02X", buf[i - 2]);
    }
    rt_kprintf("\r\n");
    return 0;
}
MSH_CMD_EXPORT(at24_write, write[addr][data1][data2]...);

#define READ_NUM 5

static int at24_read(int argc, char **argv)
{
    int32_t ret;
    char *p_end;
    uint8_t i;
    uint8_t addr;
    uint8_t buf[READ_NUM];
    if (argc != 2)
    {
        /* 请按以下格式输入： */
        rt_kprintf("\xc7\xeb\xb0\xb4\xd2\xd4\xcf\xc2\xb8\xf1"
               "\xca\xbd\xca\xe4\xc8\xeb:\r\n");

        rt_kprintf("  read [addr]\\r\\n\r\n");
        /*   addr: 要读取的EEPROM地址 */
        rt_kprintf("  addr: \xd2\xaa\xb6\xc1\xc8\xa1\xb5\xc4"
               "EEPROM\xb5\xd8\xd6\xb7\r\n");

        /*   \r\n: 回车 */
        rt_kprintf("  \\r\\n: \xbb\xd8\xb3\xb5\r\n");

        /* 该命令会从addr地址开始读出5个数据 */
        rt_kprintf("\xb8\xc3\xc3\xfc\xc1\xee\xbb\xe1\xb4\xd3"
               "addr\xb5\xd8\xd6\xb7\xbf\xaa\xca\xbc\xb6"
               "\xc1\xb3\xf6"
               "5\xb8\xf6\xca\xfd\xbe\xdd\r\n");
        return -1;
    }
    addr = strtol(argv[1], &p_end, 0);
    if (*p_end != '\0')
    {
        rt_kprintf("illegal addr format: '%s'\r\n", argv[1]);
        return -1;
    }
    ret = at24cxx_read_n_byte(addr, buf, READ_NUM);
    if (0 != ret)
    {
        rt_kprintf("read at24cxx failed\r\n");
        return ret;
    }
    /* 读取成功：*/
    rt_kprintf("\xb6\xc1\xc8\xa1\xb3\xc9\xb9\xa6\xa3\xba\r\n");

    rt_kprintf("+----+--- --- --- --- ---+\r\n|addr|");
    for (i = 0; i < READ_NUM; i++)
    {
        rt_kprintf("%02Xh", addr++);
        if (i < READ_NUM - 1)
        {
            rt_kprintf(" ");
        }
    }
    rt_kprintf("|\r\n");
    rt_kprintf("+----+--- --- --- --- ---+\r\n|data|");

    for (i = 0; i < READ_NUM; i++)
    {
        rt_kprintf("%02Xh", buf[i]);
        if (i < READ_NUM - 1)
        {
            rt_kprintf(" ");
        }
    }
    rt_kprintf("|\r\n");
    rt_kprintf("+----+--- --- --- --- ---+\r\n");

    return 0;
}
MSH_CMD_EXPORT(at24_read, read[addr]);
