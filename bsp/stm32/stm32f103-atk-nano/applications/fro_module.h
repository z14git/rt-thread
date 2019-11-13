/**
 * @file fro_module.h
 * @author z14git
 * @brief 
 * @version 0.1
 * @date 2019-11-11
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#ifndef __FRO_MODULE_H
#define __FRO_MODULE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <ipc/workqueue.h>
    typedef enum
    {
        M_NONE = 0X3F,           // 无
        M_FAN = 0X3E,            // 风扇模块
        M_SMOG = 0X3D,           // 烟雾
        M_HUMITURE = 0X3B,       // 温湿度传感器
        M_BODY = 0X2F,           // 人体
        M_GESTURE = 0X1F,        // 手势
        M_BUZZER = 0X2E,         // 蜂鸣器
        M_ILLUMINANCE = 0X37,    // 光照
        M_VOLUME = 0X3C,         // 声音
        M_INFRARED_LIMIT = 0X3A, // 红外对射
        M_INFRARED_TRANS = 0X2D, // 红外发射
        M_INFRARED_RECV = 0X1D,  // 红外接收
        M_RGB_LIGHT = 0X1E,      // RGB灯
        M_FLAME = 0X39,          // 火焰
        M_FUEL_GAS = 0X33,       // 可燃气体
        M_CONDENSATION = 0X35,   // 结露
        M_PM25 = 0X0B,           // pm2.5
        M_RFID = 0X29,           // RFID
    } FRO_MODULE_TYPE;

    struct fro_module_ops
    {
        int (*init)(void);
        void (*deinit)(void);
        void (*run)(struct rt_work *work, void *param);
        int (*write)(void *cmd, void *data);
        int (*read)(void *cmd, void *data);
    };
    typedef struct fro_module_ops *fro_module_ops_t;

    struct fro_module
    {
        rt_slist_t list;
        const struct fro_module_ops *ops;
        uint8_t type; // FRO_MODULE_TYPE
        const char *name;
        void *user_data; // 备用
    };
    typedef struct fro_module *fro_module_t;

    extern const char *fro_module_name;
    extern char *fro_module_info_str;

    int fro_module_register(fro_module_t m);
    fro_module_t get_current_module(void);

#ifdef __cplusplus
}
#endif

#endif // __FRO_MODULE_H
