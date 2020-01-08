/**
 * @file fro_module.c
 * @author z14git
 * @brief
 * @version 0.1
 * @date 2019-11-12
 *
 * @copyright Copyright (c) 2019
 *
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "fro_module.h"
#include "frotech_protocol.h"
#include "cJSON_util.h"

#define PC0 GET_PIN(C, 0)
#define PC1 GET_PIN(C, 1)
#define PC2 GET_PIN(C, 2)
#define PC3 GET_PIN(C, 3)
#define PC4 GET_PIN(C, 4)
#define PC5 GET_PIN(C, 5)

#define THREAD_PRIORITY   25
#define THREAD_STACK_SIZE 2048
#define THREAD_TIMESLICE  5

static rt_thread_t tid = RT_NULL;

static rt_slist_t   _module_list;
static fro_module_t current_module = RT_NULL;

static uint8_t servo_test_mode = 0;

const char *fro_module_name     = RT_NULL;
char *      fro_module_info_str = RT_NULL;

static uint8_t get_fro_module_type(void)
{
    uint8_t module_type;
    uint8_t tmp;
    uint8_t retry = 3;

    do {
        module_type = 0;
        module_type |= rt_pin_read(PC0) << 0;
        module_type |= rt_pin_read(PC1) << 1;
        module_type |= rt_pin_read(PC2) << 2;
        module_type |= rt_pin_read(PC3) << 3;
        module_type |= rt_pin_read(PC4) << 4;
        module_type |= rt_pin_read(PC5) << 5;

        rt_thread_mdelay(100);

        tmp = 0;
        tmp |= rt_pin_read(PC0) << 0;
        tmp |= rt_pin_read(PC1) << 1;
        tmp |= rt_pin_read(PC2) << 2;
        tmp |= rt_pin_read(PC3) << 3;
        tmp |= rt_pin_read(PC4) << 4;
        tmp |= rt_pin_read(PC5) << 5;

        if (tmp == module_type) {
            if (tmp == M_NONE) {
                /**
                 * FIXME: 当前舵机调试板上没有识别电阻，所以采用这种取巧的方式；
                 * 当识别不到模块，且收到"name"值为"舵机调试"的JSON格式指令，
                 * 则判断当前模块为"舵机调试"
                 */
                if (servo_test_mode) {
                    return M_SERVO_TEST;
                }
            }
            servo_test_mode = 0;
            return module_type;
        }

    } while (retry--);

    return M_NONE;
}

int fro_module_register(fro_module_t m)
{
    RT_ASSERT(m != RT_NULL);
    RT_ASSERT(m->ops != RT_NULL);
    RT_ASSERT(m->type != RT_NULL);

    rt_slist_append(&_module_list, &m->list);

    return 0;
}

fro_module_t get_current_module(void)
{
    return current_module;
}

char *get_current_module_name(void)
{
    if (current_module != RT_NULL)
        return (char *)(current_module->name);
    else
        return "";
}

static fro_module_t detect_current_module(void)
{
    rt_slist_t * module_list    = RT_NULL;
    fro_module_t current_module = RT_NULL;
    uint8_t      type;

    type = get_fro_module_type();

    rt_slist_for_each(module_list, &_module_list)
    {
        current_module = (fro_module_t)(module_list);
        if (current_module->type == type)
            break;
        current_module = RT_NULL;
    }

    return current_module;
}

int fro_module_handler(cJSON *req_json, cJSON *reply_json)
{
    cJSON *name = RT_NULL;

    /* 处理读取请求 */
    cJSON *requests = RT_NULL;

    /* 处理写入请求 */
    cJSON *datapoints = RT_NULL;

    int ret;

    /* 判断请求命令的设备名称是否与模块名称一致 */
    name = cJSON_GetObjectItemCaseSensitive(req_json, "name");
    if (!cJSON_IsString(name) || (name->valuestring == RT_NULL)) {
        ret = -1;
        goto __end;
    }

    /**
     * FIXME: 当前舵机调试板上没有识别电阻，所以采用这种取巧的方式；
     * 当识别不到模块，且收到"name"值为"舵机调试"的JSON格式指令，
     * 则判断当前模块为"舵机调试"
     */
    if (rt_strcmp("舵机调试", name->valuestring) == 0) {
        if (servo_test_mode == 0) {
            servo_test_mode = 1;
            ret             = 0;
            if (cJSON_AddStringToObject(reply_json,
                                        "msg",
                                        "已进入舵机调试模式") == RT_NULL) {
                ret = -1;
                goto __end;
            }
            goto __end;
        }
    }

    if (current_module == RT_NULL) {
        ret = -1;
        goto __end;
    }
    if (rt_strcmp(current_module->name, name->valuestring)) {
        ret = -1;
        goto __end;
    }

    /* 读取请求示例：
        {
            "addr": 1, //这部分由frotech_protocol.c处理
            "name": "led",
            "requests": [
                {
                    "id": "on-off"
                },
                {
                    "id": "brightness"
                }
            ]
        }
    */
    requests = cJSON_GetObjectItemCaseSensitive(req_json, "requests");
    if (requests != RT_NULL && cJSON_IsArray(requests)) {
        /* 返回示例：
            {
                "addr": 1, //这部分暂时不用准备，由上层应用(frotech_protocol.c)处理
                "name": "led",
                "datapoints": [
                    {
                        "id": "on-off",
                        "value": 1
                    },
                    {
                        "id": "brightness",
                        "value": 50
                    }
                ]
            }
         */
        cJSON *request         = RT_NULL;
        cJSON *datapoints      = RT_NULL;
        double datapoint_value = RT_NULL;
        if (cJSON_AddStringToObject(reply_json, "name", name->valuestring) ==
            RT_NULL) {
            ret = -1;
            goto __end;
        }
        datapoints = cJSON_AddArrayToObject(reply_json, "datapoints");
        if (datapoints == RT_NULL) {
            ret = -1;
            goto __end;
        }

        cJSON_ArrayForEach(request, requests)
        {
            if (current_module->ops->read == RT_NULL) {
                ret = -1;
                goto __end;
            }
            cJSON *id = cJSON_GetObjectItemCaseSensitive(request, "id");
            if (!cJSON_IsString(id) || (id->valuestring == RT_NULL)) {
                ret = -1;
                goto __end;
            }

            cJSON *datapoint = cJSON_CreateObject();
            if (datapoint == RT_NULL) {
                ret = -1;
                goto __end;
            }

            ret = current_module->ops->read(id->valuestring, &datapoint_value);
            if (ret != 0) {
                goto __end;
            }

            if (cJSON_AddStringToObject(datapoint, "id", id->valuestring) ==
                RT_NULL) {
                ret = -1;
                goto __end;
            }
            if (cJSON_AddNumberToObject(datapoint, "value", datapoint_value) ==
                RT_NULL) {
                ret = -1;
                goto __end;
            }

            cJSON_AddItemToArray(datapoints, datapoint);
        }
        return 0;
    }

    /* 写入请求示例：
        {
            "addr": 1,
            "name": "led",
            "datapoints": [
                {
                    "id": "on-off",
                    "value": 1
                },
                {
                    "id": "brightness",
                    "value": 50
                }
            ]
        }
    */
    datapoints = cJSON_GetObjectItemCaseSensitive(req_json, "datapoints");
    if (datapoints != RT_NULL && cJSON_IsArray(datapoints)) {
        /* 返回示例：
            {
                "addr": 1,
                "name": "led",
                "errno": 0,
                "error": "succ"
            }
        */
        cJSON *datapoint = RT_NULL;

        if (cJSON_AddStringToObject(reply_json, "name", name->valuestring) ==
            RT_NULL) {
            ret = -1;
            goto __end;
        }

        cJSON_ArrayForEach(datapoint, datapoints)
        {
            if (current_module->ops->write == RT_NULL) {
                ret = -1;
                goto __end;
            }

            cJSON *id = cJSON_GetObjectItemCaseSensitive(datapoint, "id");
            if (!cJSON_IsString(id) || (id->valuestring == RT_NULL)) {
                ret = -1;
                goto __end;
            }

            cJSON *value = cJSON_GetObjectItemCaseSensitive(datapoint, "value");
            if (!cJSON_IsNumber(value)) {
                ret = -1;
                goto __end;
            }

            int wr_value = value->valuedouble;
            ret =
                current_module->ops->write(id->valuestring, (void *)(wr_value));
            if (ret != 0) {
                if (cJSON_AddNumberToObject(reply_json, "errno", ret) ==
                    RT_NULL) {
                    ret = -1;
                    goto __end;
                }
                return 0;
            }
        }
        if (cJSON_AddNumberToObject(reply_json, "errno", ret) == RT_NULL) {
            ret = -1;
            goto __end;
        }
        return 0;
    }

__end:
    return ret;
}

#ifndef JSON_MODE
int fro_module_collect_data(uint8_t *              protocol_buf,
                            protocol_data_node_t **ptr_ptr_node)
{
    int      ret      = 0;
    uint8_t  reg_addr = protocol_buf[3];
    uint32_t cnt      = protocol_buf[5] << 1;
    if (current_module == RT_NULL) {
        return -1;
    }

    if (current_module->addr != reg_addr) {
        return -1;
    }

    if (current_module->ops->read == RT_NULL) {
        return -1;
    }
    ret = current_module->ops->read((void *)cnt, ptr_ptr_node);
    return ret;
}

int fro_module_event_process(uint8_t *protocol_buf)
{
    /* 该功能暂时未完成 */
    return 0;
}
#endif /* ndef JSON_MODE */

static int fro_module_list_init(void)
{
    rt_slist_init(&_module_list);

    return RT_EOK;
}
INIT_BOARD_EXPORT(fro_module_list_init);

static int fro_module_init(void)
{
    current_module = detect_current_module();
    return RT_EOK;
}
INIT_ENV_EXPORT(fro_module_init);

static int fro_module_type_detect_io_init(void)
{
    rt_pin_mode(PC0, PIN_MODE_INPUT);
    rt_pin_mode(PC1, PIN_MODE_INPUT);
    rt_pin_mode(PC2, PIN_MODE_INPUT);
    rt_pin_mode(PC3, PIN_MODE_INPUT);
    rt_pin_mode(PC4, PIN_MODE_INPUT);
    rt_pin_mode(PC5, PIN_MODE_INPUT);
    return 0;
}
INIT_ENV_EXPORT(fro_module_type_detect_io_init);

static void module_test_thread_entry(void *parameter)
{
    fro_module_t         last_module = RT_NULL;
    struct rt_workqueue *wq          = RT_NULL;
    struct rt_work       work;
    uint32_t             work_status;

    wq = rt_workqueue_create("m_work", 2048, 24);
    if (wq == RT_NULL)
        return;

    for (;;) {
        current_module = detect_current_module();
        if (current_module != RT_NULL) {
            if (last_module != current_module) {
                if (last_module != RT_NULL) {
                    // 处理识别错误的情况（偶尔接触不良会发生这种情况）
                    work_status = 0;
                    rt_workqueue_cancel_work_sync(wq, &work);
                    if (last_module->ops->deinit != RT_NULL)
                        last_module->ops->deinit();
                }

                if (current_module->ops->init != RT_NULL)
                    current_module->ops->init();
                rt_work_init(&work, current_module->ops->run, &work_status);
                work_status = 1;
                rt_workqueue_dowork(wq, &work);
            }
            // get module info
            if (current_module->ops->read != RT_NULL) {
                current_module->ops->read(0, &fro_module_info_str);
            }
            fro_module_name = current_module->name;
        } else {
            if (last_module != RT_NULL) {
                work_status = 0;
                rt_workqueue_cancel_work_sync(wq, &work);
                if (last_module->ops->deinit != RT_NULL)
                    last_module->ops->deinit();
            }
            fro_module_name     = "";
            fro_module_info_str = "";
        }
        last_module = current_module;

        rt_thread_mdelay(200);
    }
}

static int module_test_init(void)
{
    tid = rt_thread_create("m_test",
                           module_test_thread_entry,
                           RT_NULL,
                           THREAD_STACK_SIZE,
                           THREAD_PRIORITY,
                           THREAD_TIMESLICE);
    if (tid != RT_NULL)
        rt_thread_startup(tid);
    return 0;
}
INIT_APP_EXPORT(module_test_init);
