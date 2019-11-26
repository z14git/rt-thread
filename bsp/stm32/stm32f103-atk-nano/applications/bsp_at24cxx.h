/**
 * @file bsp_at24cxx.h
 * @author z14git
 * @brief
 * @version 0.1
 * @date 2019-09-19
 *
 * @copyright Copyright (c) 2019
 *
 */

#ifndef __BSP_AT24CXX_H
#define __BSP_AT24CXX_H
#ifdef __cplusplus
extern "C" {
#endif

#include <rtthread.h>
rt_err_t at24cxx_write_byte(rt_uint8_t wr_addr, rt_uint8_t data);
rt_err_t at24cxx_read_byte(rt_uint8_t rd_addr, rt_uint8_t *rd_buf);
rt_err_t
at24cxx_read_n_byte(rt_uint8_t rd_addr, rt_uint8_t *rd_buf, rt_uint8_t num);
rt_err_t
at24cxx_write_n_byte(rt_uint8_t wr_addr, rt_uint8_t *wr_buf, rt_uint8_t num);
#ifdef __cplusplus
}
#endif

#endif // __BSP_AT24CXX_H
