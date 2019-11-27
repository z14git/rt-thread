/**
 * @file frotech_protocol.h
 * @author z14git
 * @brief
 * @version 0.1
 * @date 2018-11-22
 *
 * @copyright Copyright (c) 2018
 *
 */

#ifndef __FROTECH_PROTOCOL_H
#define __FROTECH_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <board.h>

#define JSON_MODE

#ifndef JSON_MODE
#define PROTOCOL_READ_LEN 8
#define PROTOCOL_WRITE_BASE_LEN 7
#define PROTOCOL_SET_ADDR_LEN 6
#define PROTOCOL_BUF_MAX_LEN 128

typedef enum
{
    CMD_READ_REG = 0x03,    // 读取设备数据功能码
    CMD_WRITE_REG = 0x10,   // 写入设备数据功能码
    CMD_SET_DEVICE_ADDR = 0x30, // 设置设备地址功能码
    CMD_READ_DEVICE_ADDR = 0x31,// 读取设备地址功能码
    CMD_READ_REG_ERROR = 0x83, // 读取异常时的错误代码
    CMD_WRITE_REG_ERROR = 0x90, // 写入异常时的错误代码
} PROTOCOL_CMDTYPE;

typedef enum
{
    ERR_CMD = 0x01, // 功能码不支持
    ERR_ADDR = 0x02, // StartAddr验证错误
    ERR_REG_NUM = 0x03, // 寄存器个数错误
    ERR_RW_MULTI_REG = 0x04, // 不支持读/写多个寄存器
    ERR_HOST = 0x05, // 主机号不存在
} PROTOCOL_ERROR_CODE_TYPE;

#pragma pack(1)
typedef struct
{
    uint8_t crc_lo;     // crc 检验低8位
    uint8_t crc_hi;     // crc 检验高8位
} protocol_crc_type_t;

typedef struct
{
    uint8_t device_addr; // 设备地址
    uint8_t cmd; // 功能码
} protocol_head_type_t;

typedef struct protocol_reg_data_node
{
    uint8_t reg_data_hi;
    uint8_t reg_data_lo;
    struct protocol_reg_data_node *next;
} protocol_data_node_t;

typedef struct
{
    protocol_head_type_t head;
    uint8_t reg_addr_hi; // 传感器地址的高8位
    uint8_t reg_addr_lo; // 传感器地址的低8位
    uint8_t num_hi; // 要读取的寄存器数量的高8位
    uint8_t num_lo; // 要读取的寄存器数量的低8位
    protocol_crc_type_t crc;
} protocol_read_t;

typedef struct
{
    protocol_head_type_t head;
    uint8_t count;  // 要传输的数据的字节数，一个数据占两字节
    protocol_data_node_t data; // 要传输的数据
    protocol_crc_type_t crc;
} protocol_ret_read_t;

typedef struct
{
    protocol_head_type_t head;
    uint8_t reg_addr_hi; // 传感器地址的高8位
    uint8_t reg_addr_lo; // 传感器地址的低8位
    uint8_t num_hi; // 要写入的寄存器数量的高8位
    uint8_t num_lo; // 要写入的寄存器数量的低8位
    uint8_t count; // 要写入的数据的字节数，等于寄存器数量的两倍
    protocol_data_node_t data; // 要写入的数据
    protocol_crc_type_t crc;
} protocol_write_t;

typedef union
{
    uint8_t arry[PROTOCOL_BUF_MAX_LEN];
    protocol_read_t read;
    protocol_write_t write;
} protocol_rx_buf_t;

/**
 * @brief 设备检测到传输错误时，返回错误信息的协议格式
 *
 */
typedef struct
{
    protocol_head_type_t head;
    uint8_t err_code;
    protocol_crc_type_t crc;
} protocol_err_type_t;

#pragma pack()
#endif /* ndef JSON_MODE */
#ifdef JSON_MODE

#endif /* ndef JSON_MODE */


#ifdef __cplusplus
}
#endif

#endif // __FROTECH_PROTOCOL_H
