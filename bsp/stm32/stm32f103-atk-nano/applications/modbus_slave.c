/**
 * @file modbus_slave.c
 * @author z14git
 * @brief 
 * @version 0.1
 * @date 2020-06-24
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#include "mb.h"
#include "user_mb_app.h"
#include "servo_process.h"

#ifndef ULOG_USING_SYSLOG
#define LOG_TAG "MB_SLAVE"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>
#else
#include <syslog.h>
#endif /* ULOG_USING_SYSLOG */

#define SLAVE_ADDR 14
#define PORT_NUM 2
#define PORT_BAUDRATE 115200

#define PORT_PARITY MB_PAR_NONE

#define THREAD_STACK_SIZE 2048
#define THRAD_PRIORITY 15

typedef enum {
        ADAPTER_MODBUS_CONFIG_SERVO_ON,
        ADAPTER_MODBUS_CONFIG_UP_SERVO_ANGLE,
        ADAPTER_MODBUS_CONFIG_DOWN_SERVO_ANGLE,
        ADAPTER_MODBUS_CONFIG_LEFT_PIE_TIMEOUT,
        ADAPTER_MODBUS_CONFIG_RIGHT_PIE_TIMEOUT,
        ADAPTER_MODBUS_CONFIG_MAX
} adapter_modbus_config_t;

typedef enum {
        /* 拔片动作，动作执行完成后自动归0 */
        ADAPTER_MODBUS_BIT_LEFT_BLOCK, // combine block and pass bit, 10(block:1, pass:0): do block;
        ADAPTER_MODBUS_BIT_LEFT_PASS, // 01: do pass; 11: auto
        ADAPTER_MODBUS_BIT_RIGHT_BLOCK,
        ADAPTER_MODBUS_BIT_RIGHT_PASS,
        /* 限位开关状态 */
        ADAPTER_MODBUS_BIT_LEFT_UP,
        ADAPTER_MODBUS_BIT_LEFT_DOWN,
        ADAPTER_MODBUS_BIT_RIGHT_UP,
        ADAPTER_MODBUS_BIT_RIGHT_DOWN,
        ADAPTER_MODBUS_BIT_MAX
} adapter_modbus_bit_t;

/*------------------------Slave mode use these variables----------------------*/
//Slave mode:DiscreteInputs variables
USHORT usSDiscInStart = S_DISCRETE_INPUT_START;
#if S_DISCRETE_INPUT_NDISCRETES % 8
UCHAR ucSDiscInBuf[S_DISCRETE_INPUT_NDISCRETES / 8 + 1];
#else
UCHAR ucSDiscInBuf[S_DISCRETE_INPUT_NDISCRETES / 8];
#endif
//Slave mode:Coils variables
USHORT usSCoilStart = S_COIL_START;
#if S_COIL_NCOILS % 8
UCHAR ucSCoilBuf[S_COIL_NCOILS / 8 + 1];
#else
UCHAR ucSCoilBuf[S_COIL_NCOILS / 8];
#endif
//Slave mode:InputRegister variables
USHORT usSRegInStart = S_REG_INPUT_START;
USHORT usSRegInBuf[S_REG_INPUT_NREGS];
//Slave mode:HoldingRegister variables
USHORT usSRegHoldStart = S_REG_HOLDING_START;
USHORT usSRegHoldBuf[S_REG_HOLDING_NREGS];

static void hold_reg_process(uint16_t index, uint16_t data)
{
        struct adapter_servo_msg servo_msg;
        switch (index) {
        case ADAPTER_MODBUS_CONFIG_UP_SERVO_ANGLE:
                servo_msg.cmd = ADAPTER_SERVO_CMD_UP_ANGLE;
                servo_msg.data = data;
                adapter_servo_msg_put(&servo_msg);
                break;
        case ADAPTER_MODBUS_CONFIG_DOWN_SERVO_ANGLE:
                servo_msg.cmd = ADAPTER_SERVO_CMD_DOWN_ANGLE;
                servo_msg.data = data;
                adapter_servo_msg_put(&servo_msg);
                break;
        default:
                break;
        }
}

/**
 * Modbus slave input register callback function.
 *
 * @param pucRegBuffer input register buffer
 * @param usAddress input register address
 * @param usNRegs input register number
 *
 * @return result
 */
eMBErrorCode eMBRegInputCB(UCHAR *pucRegBuffer, USHORT usAddress,
                           USHORT usNRegs)
{
        eMBErrorCode eStatus = MB_ENOERR;
        USHORT iRegIndex;
        USHORT *pusRegInputBuf;
        USHORT REG_INPUT_START;
        USHORT REG_INPUT_NREGS;
        USHORT usRegInStart;

        pusRegInputBuf = usSRegInBuf;
        REG_INPUT_START = S_REG_INPUT_START;
        REG_INPUT_NREGS = S_REG_INPUT_NREGS;
        usRegInStart = usSRegInStart;

        /* it already plus one in modbus function method. */
        usAddress--;

        if ((usAddress >= REG_INPUT_START) &&
            (usAddress + usNRegs <= REG_INPUT_START + REG_INPUT_NREGS)) {
                iRegIndex = usAddress - usRegInStart;
                while (usNRegs > 0) {
                        *pucRegBuffer++ =
                                (UCHAR)(pusRegInputBuf[iRegIndex] >> 8);
                        *pucRegBuffer++ =
                                (UCHAR)(pusRegInputBuf[iRegIndex] & 0xFF);
                        iRegIndex++;
                        usNRegs--;
                }
        } else {
                eStatus = MB_ENOREG;
        }

        return eStatus;
}

/**
 * Modbus slave holding register callback function.
 *
 * @param pucRegBuffer holding register buffer
 * @param usAddress holding register address
 * @param usNRegs holding register number
 * @param eMode read or write
 *
 * @return result
 */
eMBErrorCode eMBRegHoldingCB(UCHAR *pucRegBuffer, USHORT usAddress,
                             USHORT usNRegs, eMBRegisterMode eMode)
{
        eMBErrorCode eStatus = MB_ENOERR;
        USHORT iRegIndex;
        USHORT *pusRegHoldingBuf;
        USHORT REG_HOLDING_START;
        USHORT REG_HOLDING_NREGS;
        USHORT usRegHoldStart;

        pusRegHoldingBuf = usSRegHoldBuf;
        REG_HOLDING_START = S_REG_HOLDING_START;
        REG_HOLDING_NREGS = S_REG_HOLDING_NREGS;
        usRegHoldStart = usSRegHoldStart;

        /* it already plus one in modbus function method. */
        usAddress--;

        if ((usAddress >= REG_HOLDING_START) &&
            (usAddress + usNRegs <= REG_HOLDING_START + REG_HOLDING_NREGS)) {
                iRegIndex = usAddress - usRegHoldStart;
                switch (eMode) {
                /* read current register values from the protocol stack. */
                case MB_REG_READ:
                        while (usNRegs > 0) {
                                *pucRegBuffer++ = (UCHAR)(
                                        pusRegHoldingBuf[iRegIndex] >> 8);
                                *pucRegBuffer++ = (UCHAR)(
                                        pusRegHoldingBuf[iRegIndex] & 0xFF);
                                iRegIndex++;
                                usNRegs--;
                        }
                        break;

                /* write current register values with new values from the protocol stack. */
                case MB_REG_WRITE:
                        while (usNRegs > 0) {
                                pusRegHoldingBuf[iRegIndex] = *pucRegBuffer++
                                                              << 8;
                                pusRegHoldingBuf[iRegIndex] |= *pucRegBuffer++;
                                hold_reg_process(iRegIndex,
                                                 pusRegHoldingBuf[iRegIndex]);
                                LOG_D("MB_REG_WRITE INDEX: %d, data: %d",
                                      iRegIndex, pusRegHoldingBuf[iRegIndex]);
                                iRegIndex++;
                                usNRegs--;
                        }
                        break;
                }
        } else {
                eStatus = MB_ENOREG;
        }
        return eStatus;
}

/**
 * Modbus slave coils callback function.
 *
 * @param pucRegBuffer coils buffer
 * @param usAddress coils address
 * @param usNCoils coils number
 * @param eMode read or write
 *
 * @return result
 */
eMBErrorCode eMBRegCoilsCB(UCHAR *pucRegBuffer, USHORT usAddress,
                           USHORT usNCoils, eMBRegisterMode eMode)
{
        eMBErrorCode eStatus = MB_ENOERR;
        USHORT iRegIndex, iRegBitIndex, iNReg;
        UCHAR *pucCoilBuf;
        USHORT COIL_START;
        USHORT COIL_NCOILS;
        USHORT usCoilStart;
        iNReg = usNCoils / 8 + 1;

        pucCoilBuf = ucSCoilBuf;
        COIL_START = S_COIL_START;
        COIL_NCOILS = S_COIL_NCOILS;
        usCoilStart = usSCoilStart;

        /* it already plus one in modbus function method. */
        usAddress--;

        if ((usAddress >= COIL_START) &&
            (usAddress + usNCoils <= COIL_START + COIL_NCOILS)) {
                iRegIndex = (USHORT)(usAddress - usCoilStart) / 8;
                iRegBitIndex = (USHORT)(usAddress - usCoilStart) % 8;
                switch (eMode) {
                /* read current coil values from the protocol stack. */
                case MB_REG_READ:
                        while (iNReg > 0) {
                                *pucRegBuffer++ =
                                        xMBUtilGetBits(&pucCoilBuf[iRegIndex++],
                                                       iRegBitIndex, 8);
                                iNReg--;
                        }
                        pucRegBuffer--;
                        /* last coils */
                        usNCoils = usNCoils % 8;
                        /* filling zero to high bit */
                        *pucRegBuffer = *pucRegBuffer << (8 - usNCoils);
                        *pucRegBuffer = *pucRegBuffer >> (8 - usNCoils);
                        break;

                        /* write current coil values with new values from the protocol stack. */
                case MB_REG_WRITE:
                        while (iNReg > 1) {
                                xMBUtilSetBits(&pucCoilBuf[iRegIndex++],
                                               iRegBitIndex, 8,
                                               *pucRegBuffer++);
                                iNReg--;
                        }
                        /* last coils */
                        usNCoils = usNCoils % 8;
                        /* xMBUtilSetBits has bug when ucNBits is zero */
                        if (usNCoils != 0) {
                                xMBUtilSetBits(&pucCoilBuf[iRegIndex++],
                                               iRegBitIndex, usNCoils,
                                               *pucRegBuffer++);
                        }
                        break;
                }
        } else {
                eStatus = MB_ENOREG;
        }
        return eStatus;
}

/**
 * Modbus slave discrete callback function.
 *
 * @param pucRegBuffer discrete buffer
 * @param usAddress discrete address
 * @param usNDiscrete discrete number
 *
 * @return result
 */
eMBErrorCode eMBRegDiscreteCB(UCHAR *pucRegBuffer, USHORT usAddress,
                              USHORT usNDiscrete)
{
        eMBErrorCode eStatus = MB_ENOERR;
        USHORT iRegIndex, iRegBitIndex, iNReg;
        UCHAR *pucDiscreteInputBuf;
        USHORT DISCRETE_INPUT_START;
        USHORT DISCRETE_INPUT_NDISCRETES;
        USHORT usDiscreteInputStart;
        iNReg = usNDiscrete / 8 + 1;

        pucDiscreteInputBuf = ucSDiscInBuf;
        DISCRETE_INPUT_START = S_DISCRETE_INPUT_START;
        DISCRETE_INPUT_NDISCRETES = S_DISCRETE_INPUT_NDISCRETES;
        usDiscreteInputStart = usSDiscInStart;

        /* it already plus one in modbus function method. */
        usAddress--;

        if ((usAddress >= DISCRETE_INPUT_START) &&
            (usAddress + usNDiscrete <=
             DISCRETE_INPUT_START + DISCRETE_INPUT_NDISCRETES)) {
                iRegIndex = (USHORT)(usAddress - usDiscreteInputStart) / 8;
                iRegBitIndex = (USHORT)(usAddress - usDiscreteInputStart) % 8;

                while (iNReg > 0) {
                        *pucRegBuffer++ = xMBUtilGetBits(
                                &pucDiscreteInputBuf[iRegIndex++], iRegBitIndex,
                                8);
                        iNReg--;
                }
                pucRegBuffer--;
                /* last discrete */
                usNDiscrete = usNDiscrete % 8;
                /* filling zero to high bit */
                *pucRegBuffer = *pucRegBuffer << (8 - usNDiscrete);
                *pucRegBuffer = *pucRegBuffer >> (8 - usNDiscrete);
        } else {
                eStatus = MB_ENOREG;
        }

        return eStatus;
}

static void mb_slave_handler(void *arg)
{
        eMBInit(MB_RTU, SLAVE_ADDR, PORT_NUM, PORT_BAUDRATE, PORT_PARITY);
        eMBEnable();
        for (;;) {
                eMBPoll();
        }
}

static int mb_slave_start(void)
{
        rt_thread_t tid = RT_NULL;
        tid = rt_thread_create("mb_slave", mb_slave_handler, RT_NULL,
                               THREAD_STACK_SIZE, THRAD_PRIORITY, 20);
        if (tid != RT_NULL) {
                rt_thread_startup(tid);
                return 0;
        }
        return -1;
}
INIT_APP_EXPORT(mb_slave_start);
