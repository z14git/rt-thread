#include "crc.h"
//************************************************************
//函数名：cal_crc(uint8_t *snd, uint8_t num)
//输入：需要生成CRC校验码的字符串首地址，字符串的字节个数
//输出：16位的CRC校验码
//功能描述：生成CRC校验码
//************************************************************
uint16_t cal_crc(uint8_t *snd, uint8_t num)
{
    uint8_t i, j;
    uint16_t  c, crc = 0xFFFF;

    for (i = 0; i < num; i++)
    {
        c = snd[i] & 0x00FF;
        crc ^= c;
        for (j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
                crc >>= 1;
        }
    }

    return (crc);
}
