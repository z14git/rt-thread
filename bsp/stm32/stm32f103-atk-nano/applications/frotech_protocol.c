/**
 * @file frotech_protocol.c
 * @author z14git
 * @brief
 * @version 0.1
 * @date 2018-11-22
 *
 * @copyright Copyright (c) 2018
 *
 */

#include "frotech_protocol.h"
#include "rtdevice.h"
#include "crc.h"
#include "bsp_at24cxx.h"

#define THREAD_PRIORITY 5
#define THREAD_STACK_SIZE 4096
#define THREAD_TIMESLICE 5

#define PENDING(NOW, SOON) ((long)(NOW - (SOON)) < 0)
#define ELAPSED(NOW, SOON) (!PENDING(NOW, SOON))

#define EEPROM_ADDR_OF_DEVICE_ADDR 3 /* 设备的Modbus地址保存在EEPROM中的地址 */

static struct rt_ringbuffer rb;
static uint8_t rx_ringbuf[50];

static rt_device_t uart_model;
static struct rt_semaphore  rx_sem;
static rt_thread_t tid = RT_NULL;

static uint8_t device_addr;

int fro_module_collect_data(uint8_t *              protocol_buf,
                           protocol_data_node_t **ptr_ptr_node);

int fro_module_event_process(uint8_t *protocol_buf);

/**
 * @brief 功能：将接收到的字节放入环形缓冲区。
 *        该函数只能从串口中断中调用！
 * @param data 串口中接收到的数据
 * @return int32_t 成功返回1，失败返回0
 */
static int32_t fro_put_data(uint8_t data)
{
    int32_t ret;
    ret = rt_ringbuffer_putchar_force(&rb, data);
    return ret;
}

/**
 * @brief 获取一段完整的指令
 *
 * @note 读取格式：
 * +------+--------------+---------------+--------------+----------+---------+---------+----------+
 * |  00  |      01      |       02      |      03      |    04    |    05   |    06   |    07    |
 * +------+--------------+---------------+--------------+----------+---------+---------+----------+
 * | addr | CMD_READ_REG | reg_addr_high | reg_addr_low | num_high | num_low | crc_low | crc_high |
 * +------+--------------+---------------+--------------+----------+---------+---------+----------+
 * addr:          节点地址
 * CMD_READ_REG： 0x03
 * reg_addr_high: 传感器起始地址高8位（实际为0）
 * reg_addr_low:  传感器起始地址低8位
 * num_high:      要读取的传感器数量的高8位（实际为0）
 * num_low:       要读取的传感器数量的低8位
 * crc_low:       CRC校验码的低8位
 * crc_high:      CRC校验码的高8位
 *
 * @note 写入格式：
 * +------------+---------------+---------------+--------------+-----------+-----------+-----------+
 * |     00     |       01      |       02      |      03      |     04    |     05    |     06    |
 * +------------+---------------+---------------+--------------+-----------+-----------+-----------+
 * |    addr    | CMD_WRITE_REG | reg_addr_high | reg_addr_low |  num_high |  num_low  |   count   |
 * +------------+---------------+---------------+--------------+-----------+-----------+-----------+
 * +------------+---------------+---------------+--------------+-----------+-----------+-----------+
 * |     07     |       08      |      ...      |    count+5   |  count+6  | count + 7 | count + 8 |
 * +------------+---------------+---------------+--------------+-----------+-----------+-----------+
 * | data1_high |   data1_low   |      ...      |  datan_high  | datan_low |  crc_low  |  crc_high |
 * +------------+---------------+---------------+--------------+-----------+-----------+-----------+
 * addr:          节点地址
 * CMD_WRITE_REG: 0x10
 * reg_addr_high: 传感器起始地址高8位（实际为0）
 * reg_addr_low:  传感器起始地址低8位
 * num_high:      要写入的传感器数量的高8位（实际为0）
 * num_low:       要写入的传感器数量的低8位
 * count:         要写入的字节数，1个传感器2个字节
 * data1_high:    要写入的第1个数据的高8位
 * data1_low:     要写入的第1个数据的低8位
 * datan_high:    要写入的第n个数据的高8位
 * datan_low:     要写入的第n个数据的低8位
 * crc_low:       CRC校验码的低8位
 * crc_high:      CRC校验码的高8位
 *
 * @设置节点参数格式：
 * +------------+---------------------+---------------+--------------+-----------+-----------+
 * |     00     |          01         |       02      |      03      |     04    |     05    |
 * +------------+---------------------+---------------+--------------+-----------+-----------+
 * |     xx     | CMD_SET_DEVICE_ADDR |   addr_high   |   addr_low   |   trans   |   status  |
 * +------------+---------------------+---------------+--------------+-----------+-----------+
 * xx:                  预留用，暂无用
 * CMD_SET_DEVICE_ADDR: 0x30
 * addr_high:           无用
 * addr_low:            要设置的节点地址
 * trans:               01：无线传输；00：串口传输
 * status:              01：初始化OK；其它：节点待机，等待初始化OK
 *
 * @读取节点参数格式：
 * +------------+----------------------+
 * |     00     |          01          |
 * +------------+----------------------+
 * |     xx     | CMD_READ_DEVICE_ADDR |
 * +------------+----------------------+
 * xx:                   预留用，暂无用
 * CMD_READ_DEVICE_ADDR: 0x31
 *
 * @param rb [in] 串口接收数据的环形缓冲区
 * @param protocol_buf [out] 储存指令的缓冲区,该缓冲区地址要保持不变！！！！！
 * @param len [out] 指令长度
 * @return int32_t -1: 获取失败； 0: 获取成功；
 */
static int32_t get_one_pack(struct rt_ringbuffer *rb, uint8_t *protocol_buf, uint8_t *len)
{
    uint8_t tmp_len, tmp_data;
    uint8_t ret;
    uint8_t i;
    uint8_t *protocol_type = &protocol_buf[1];
    uint32_t ms;
    static uint8_t protocol_cnt = 0;
    static uint8_t last_data = 0;
    static uint8_t protocol_flag = 0;
    static uint32_t overtime_ms;

    if ((RT_NULL == rb) || (RT_NULL == protocol_buf) || (RT_NULL == len))
    {
        return -1;
    }

    ms = HAL_GetTick();
    if (protocol_cnt)
    {
        if (ELAPSED(ms, overtime_ms))
        {
            protocol_cnt = 0;
        }
    }

    tmp_len = rt_ringbuffer_data_len(rb);
    if (0 == tmp_len)
    {
        return -1;
    }
    for (i = 0; i < tmp_len; i++)
    {
        ret = rt_ringbuffer_getchar(rb, &tmp_data);
        if (0 != ret)
        {
            if (0 == protocol_cnt++)
            {
                overtime_ms = ms + 500;
            }
            protocol_buf[protocol_cnt - 1] = tmp_data;
            if (protocol_flag)
            {
                switch (*protocol_type)
                {
                case CMD_READ_REG:
                    if (PROTOCOL_READ_LEN == protocol_cnt)
                    {
                        *len = protocol_cnt;
                        protocol_flag = 0;
                        protocol_cnt = 0;
                        return 0;
                    }
                    break;
                case CMD_WRITE_REG:
                    if (PROTOCOL_WRITE_BASE_LEN == protocol_cnt)
                    {
                        if (tmp_data + PROTOCOL_WRITE_BASE_LEN + sizeof(protocol_crc_type_t) > PROTOCOL_BUF_MAX_LEN)
                        {
                            protocol_flag = 0;
                            protocol_cnt = 0;
                            return -1;
                        }
                    }
                    else if (protocol_buf[PROTOCOL_WRITE_BASE_LEN - 1] + PROTOCOL_WRITE_BASE_LEN + sizeof(protocol_crc_type_t) == protocol_cnt)
                    {
                        *len = protocol_cnt;
                        protocol_flag = 0;
                        protocol_cnt = 0;
                        return 0;
                    }
                    break;
                case CMD_SET_DEVICE_ADDR:
                    if (PROTOCOL_SET_ADDR_LEN == protocol_cnt)
                    {
                        *len = protocol_cnt;
                        protocol_flag = 0;
                        protocol_cnt = 0;
                        return 0;
                    }
                    break;

                default:
                    break;
                }
            }
            else
            {
                if (2 == protocol_cnt)
                {
                    if ((CMD_READ_REG == tmp_data) || (CMD_WRITE_REG == tmp_data))
                    {
                        if (device_addr == last_data)
                        {
                            protocol_flag = 1;
                        }
                        else
                        {
                            protocol_cnt = 0;
                        }
                    }
                    else if ((CMD_READ_DEVICE_ADDR == tmp_data) &&
                            (0xff == last_data))
                    {
                        *len = protocol_cnt;
                        protocol_cnt = 0;
                        return 0;
                    }
                    else if ((CMD_SET_DEVICE_ADDR == tmp_data) &&
                            (0xff == last_data))
                    {
                        protocol_flag = 1;
                    }
                    else
                    {
                        // 不能识别的功能码，暂不处理
                        protocol_cnt = 0;
                    }
                }
            }
            last_data = tmp_data;
        }
    }
    return -1;
}

/**
 * @brief 验证协议包的准确性
 *
 * @note 读取节点参数，与设置节点参数的协议包不用校验
 *
 * @param protocol_buf
 * @param len 协议长度减去2，即不包括CRC检验值的协议长度
 * @return int32_t 正确返回: 0；错误返回: -1;
 */
static int32_t verify_pack(uint8_t *protocol_buf, uint8_t len)
{
    uint16_t crc16;
    if (0 == len)
    {
        return 0;
    }
    if (CMD_SET_DEVICE_ADDR == protocol_buf[1])
    {
        return 0;
    }
    crc16 = cal_crc(protocol_buf, len);
    if ((((crc16 >> 8) & 0xff) == protocol_buf[len + 1]) && ((crc16 & 0xff) == protocol_buf[len]))
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

static void fro_protocol_handle(void)
{
    static protocol_rx_buf_t protocol_buf;
    uint8_t ret_buf[PROTOCOL_BUF_MAX_LEN];
    protocol_ret_read_t *ret_read;
    uint8_t len;
    protocol_data_node_t *node = NULL;
    uint8_t i;
    uint16_t crc16;
    uint8_t *protocol_type = &protocol_buf.arry[1];
    ret_read = (protocol_ret_read_t *)&ret_buf;
    rt_uint16_t old_flag;

    if (0 == get_one_pack(&rb, protocol_buf.arry, &len))
    {
        // 接收了一份协议包
        if (0 == verify_pack(protocol_buf.arry, len - sizeof(protocol_crc_type_t)))
        {
            // 数据正确
            switch (*protocol_type)
            {
            case CMD_READ_REG:
                if (0 == fro_module_collect_data(protocol_buf.arry, &node))
                {
                    /**
                     * @brief 获取数据成功，准备返回数据
                     * 返回格式：
                     * +------+--------------+-----------+------------+-----------+-----+
                     * |  00  |      01      |     02    |     03     |     04    | ... |
                     * +------+--------------+-----------+------------+-----------+-----+
                     * | addr | CMD_READ_REG |   count   | data1_high | data1_low | ... |
                     * +------+--------------+-----------+------------+-----------+-----+
                     * +------+--------------+-----------+------------+-----------+
                     * | ...  |    count+1   |  count+2  |   count+3  |  count+4  |
                     * +------+--------------+-----------+------------+-----------+
                     * | ...  |  datan_high  | datan_low |   crc_low  |  crc_high |
                     * +------+--------------+-----------+------------+-----------+
                     *
                     */
                    ret_read->head = protocol_buf.read.head;
                    ret_read->count = protocol_buf.read.num_lo * 2;

                    for (i = 0; i < protocol_buf.read.num_lo; i++)
                    {
                        if (NULL == node)
                        {
                            break;
                        }
                        ret_buf[i * 2 + 3] = node->reg_data_hi;
                        ret_buf[i * 2 + 4] = node->reg_data_lo;
                        node = node->next;
                    }

                    crc16 = cal_crc(ret_buf, 3 + ret_read->count);
                    ret_buf[3 + ret_read->count] = crc16 & 0xff;
                    ret_buf[4 + ret_read->count] = (crc16 >> 8) & 0xff;

                    old_flag = uart_model->open_flag;
                    uart_model->open_flag = RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX;
                    rt_device_write(uart_model, 0, &ret_buf, 5 + ret_read->count);
                    uart_model->open_flag = old_flag;
                }
                else
                {
                    // todo: return error code
                }
                break;
            case CMD_WRITE_REG:
                if (0 == fro_module_event_process(protocol_buf.arry))
                {
                    //数据写入成功，将原数据包原样返回
                    old_flag = uart_model->open_flag;
                    uart_model->open_flag = RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX;
                    rt_device_write(uart_model, 0, &(protocol_buf.arry), len);
                    uart_model->open_flag = old_flag;
                }
                else
                {
                    // todo: return error code
                }
                break;
            case CMD_READ_DEVICE_ADDR:
                /**
                 * @brief 返回节点参数格式：
                 * +----+----------------------+-----------+----------+-------+--------+
                 * | 00 |          01          |     02    |    03    |   04  |   05   |
                 * +----+----------------------+-----------+----------+-------+--------+
                 * | xx | CMD_READ_DEVICE_ADDR | addr_high | addr_low | trans | status |
                 * +----+----------------------+-----------+----------+-------+--------+
                 * eeprom储存格式：
                 * +------+-------+--------+
                 * |  03  |   04  |   05   |
                 * +------+-------+--------+
                 * | addr | trans | status |
                 * +------+-------+--------+
                 *
                 */
                ret_read->head = protocol_buf.read.head;
                ret_buf[2] = 00;
                at24cxx_read_n_byte(EEPROM_ADDR_OF_DEVICE_ADDR, &ret_buf[3], 3);
                device_addr = ret_buf[3];
                old_flag = uart_model->open_flag;
                uart_model->open_flag = RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX;
                rt_device_write(uart_model, 0, &ret_buf, 6);
                uart_model->open_flag = old_flag;
                break;
            case CMD_SET_DEVICE_ADDR:
                /**
                 * @brief 设置节点参数返回格式：
                 * +----+---------------------+-----------+----------+-------+--------+
                 * | 00 |          01         |     02    |    03    |   04  |   05   |
                 * +----+---------------------+-----------+----------+-------+--------+
                 * | xx | CMD_SET_DEVICE_ADDR | addr_high | addr_low | trans | status |
                 * +----+---------------------+-----------+----------+-------+--------+
                 * eeprom储存格式：
                 * +------+-------+--------+
                 * |  03  |   04  |   05   |
                 * +------+-------+--------+
                 * | addr | trans | status |
                 * +------+-------+--------+
                 */
                at24cxx_write_n_byte(EEPROM_ADDR_OF_DEVICE_ADDR, &protocol_buf.arry[3], 3);
                ret_read->head = protocol_buf.read.head;
                ret_buf[2] = 0;
                at24cxx_read_n_byte(EEPROM_ADDR_OF_DEVICE_ADDR, &ret_buf[3], 3);
                device_addr = ret_buf[3];
                old_flag = uart_model->open_flag;
                uart_model->open_flag = RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX;
                rt_device_write(uart_model, 0, &ret_buf, 6);
                uart_model->open_flag = old_flag;
                break;
            }
        }
    }
}

static rt_err_t uart1_rx_ind(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(&rx_sem);
    return RT_EOK;
}

static void rx_handle(void *arg)
{
    uint8_t ch;
    for (;;)
    {
        while (rt_device_read(uart_model, 0, &ch, 1) != 1)
        {
            rt_sem_take(&rx_sem, RT_WAITING_FOREVER);
        }
        fro_put_data(ch);
        fro_protocol_handle();
    }
}

int32_t fro_protocol_init(void)
{
    int32_t ret;
    rt_ringbuffer_init(&rb, (rt_uint8_t *)rx_ringbuf, sizeof(rx_ringbuf));
    ret = rt_sem_init(&rx_sem, "rx sem", 0, RT_IPC_FLAG_FIFO);
    if (RT_EOK != ret) return ret;
    uart_model = rt_device_find("uart1");
    if (RT_NULL == uart_model)
    {
        return -RT_ERROR;
    }
    else
    {
        rt_device_init(uart_model);
        rt_device_open(uart_model, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
        rt_device_set_rx_indicate(uart_model, uart1_rx_ind);
    }

    tid = rt_thread_create("rx", rx_handle, (void *)0, THREAD_STACK_SIZE, THREAD_PRIORITY, THREAD_TIMESLICE);
    if (tid != RT_NULL)
    {
        rt_thread_startup(tid);
    }
    else
    {
        return -RT_ERROR;
    }

    ret = at24cxx_read_byte(EEPROM_ADDR_OF_DEVICE_ADDR, &device_addr);

    return ret;
}
INIT_APP_EXPORT(fro_protocol_init);
