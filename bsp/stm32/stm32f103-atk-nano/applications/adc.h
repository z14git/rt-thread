/**
 * @file adc.h
 * @author z14git
 * @brief 
 * @version 0.1
 * @date 2019-11-13
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#ifndef __ADC_H
#define __ADC_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "board.h"

    int adc_init(void);
    void adc_deinit(void);
    uint16_t get_adc(uint8_t ch);

#ifdef __cplusplus
}
#endif

#endif // __ADC_H
