/**
 * @file test.cpp
 * @author z14git
 * @brief 
 * @version 0.1
 * @date 2019-09-17
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <U8g2lib.h>
#include "board.h"
#include "bsp_at24cxx.h"
#include "fro_module.h"
#include <dfs_posix.h> /* 当需要使用文件操作时，需要包含这个头文件 */

#ifndef ULOG_USING_SYSLOG
#define LOG_TAG "u8g2"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>
#else
#include <syslog.h>
#endif /* ULOG_USING_SYSLOG */

#define THREAD_PRIORITY 25
#define THREAD_STACK_SIZE 2048
#define THREAD_TIMESLICE 5

// In u8x8.h #define U8X8_USE_PINS
/* 使用该版本：https://github.com/z14git/rt-u8g2/commit/f8adcdcdf7e720f54c7cea9e5edb945fa64f0688
   确保按键可正常触发 */
#define U8G2_PIN_UP U8X8_PIN_NONE
#define U8G2_PIN_DOWN GET_PIN(C, 10)
#define U8G2_PIN_LEFT U8X8_PIN_NONE
#define U8G2_PIN_RIGHT U8X8_PIN_NONE
#define U8G2_PIN_SELECT GET_PIN(C, 11)
#define U8G2_PIN_HOME U8X8_PIN_NONE

#define OLED_SPI_PIN_CLK GET_PIN(B, 3)
#define OLED_SPI_PIN_MOSI GET_PIN(B, 4)
#define OLED_SPI_PIN_RES GET_PIN(B, 5)
#define OLED_SPI_PIN_DC GET_PIN(B, 6)
#define OLED_SPI_PIN_CS GET_PIN(B, 7)

#define SELECT_CONFIRM_BUTTON 1
#define SELECT_RETURN_BUTTON 2

/**
 * @brief 文泉驿点阵宋体
 * check https://github.com/larryli/u8g2_wqy for details
 * 
 */
#define CHINESE_FONT u8g2_font_wqy12_t_gb2312a

/**
 * @brief 显示✓✘用
 * check https://github.com/olikraus/u8g2/wiki/fntgrpgeoff#tenfatguys for detail
 */
#define SYMBOL_FONT u8g2_font_tenfatguys_tn

// Check https://github.com/olikraus/u8g2/wiki/u8g2setupcpp for all supported devices
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
static U8G2_SH1106_128X64_NONAME_F_4W_SW_SPI u8g2(U8G2_R0,
                                                  /* clock=*/OLED_SPI_PIN_CLK,
                                                  /* data=*/OLED_SPI_PIN_MOSI,
                                                  /* cs=*/OLED_SPI_PIN_CS,
                                                  /* dc=*/OLED_SPI_PIN_DC,
                                                  /* reset=*/OLED_SPI_PIN_RES);

static const char *string_list =
    "EEPROM\n"
    "OLED\n"
    "蜂鸣器\n"
    "SPI FLASH\n"
    "USB\n"
    "DB9(串口)\n"
    "板载LED";

/**
 * @brief 注意顺序与上面的string_list一致
 * 
 */
enum SELECTED_MENU_TYPE
{
    MENU_EEPROM = 1,
    MENU_OLED,
    MENU_BEEP,
    MENU_SPI_FLASH,
    MENU_USB,
    MENU_DB9,
    MENU_ONBOARD_LED,
};

#define BEEP_PIN GET_PIN(B, 2)

static const char *usb_test_explain =
    "用USB线连接电脑\n"
    "连接后，不要动鼠标,\n"
    "观察电脑屏幕上的\n"
    "鼠标是否左右移动。\n"
    "若鼠标左右移动，\n"
    "则USB功能正常\n"
    "(按KEY4返回)";

static const char *db9_test_explain =
    "用串口线连接电脑\n"
    "三档开关拔到左边\n"
    "电脑打开串口调试工具\n"
    "波特率设为115200\n"
    "单片机会每隔3秒\n"
    "发送一次\"hola\"\n"
    "观察是否收到该信息\n"
    "若发送十六进制数\"0xD\"\n"
    "则单片机会返回\n"
    "\"msh />\"\n"
    "(按KEY4返回)";

static uint8_t current_selection = 0;

static rt_thread_t tid = RT_NULL;

static void first_screen(void)
{
    int8_t event;
    u8g2.clearBuffer();
    u8g2.setCursor(0, 15);
    u8g2.print("操作说明");
    u8g2.setCursor(12, 30);
    u8g2.print("KEY3: 方向下");
    u8g2.setCursor(12, 45);
    u8g2.print("KEY4: 选择/返回");
    u8g2.setCursor(12, 45 + 15);
    u8g2.print("按“KEY4”继续");
    u8g2.sendBuffer();
    for (;;)
    {
        event = u8g2.getMenuEvent();
        if (event == U8X8_MSG_GPIO_MENU_SELECT)
        {
            break;
        }
        rt_thread_mdelay(10);
    }
}

/**
 * @brief 显示 ✘
 * 
 */
static void show_X_mark(void)
{
    u8g2.setFont(SYMBOL_FONT);
    u8g2.print("*");            // 在u8g2_font_tenfatguys_tn字体下会显示为✘
    u8g2.setFont(CHINESE_FONT); //恢复为中文字体
}

/**
 * @brief 显示 ✓
 * 
 */
static void show_check_mark(void)
{
    u8g2.setFont(SYMBOL_FONT);
    u8g2.print("/");            // 在u8g2_font_tenfatguys_tn字体下会显示为✓
    u8g2.setFont(CHINESE_FONT); //恢复为中文字体
}

static void show_scroll_text(const char *str)
{
    char buf[64]; // there are at most 8 chinese glyphs per line, max buf size is 8*3 = 24

    uint8_t total_lines;    // the total number of lines in the story
    uint8_t i;              // loop variable for the lines
    uint8_t line_cnt;       // number of lines to draw, usually equal to lines_per_draw
    uint8_t start_line;     // topmost visible line, derived from top_window_pos
    uint8_t lines_per_draw; // how many lines to draw on the screen, derived from font and display height
    uint16_t glyph_height;  // height of the glyphs

    uint16_t top_window_pos; // defines the display position in pixel within the text
    u8g2_uint_t top_offset;  // offset between the first visible line and the display
    int8_t event;
    /* calculate the length of the text in lines */
    total_lines = u8x8_GetStringLineCnt(str);

    /* get the height of the glyphs */
    glyph_height = u8g2.getMaxCharHeight();

    /* calculate how many lines must be drawn on the screen */
    lines_per_draw = u8g2.getDisplayHeight() / glyph_height;
    lines_per_draw += 2;

    /* start at the top of the text */
    top_window_pos = 0;

    for (;;)
    {
        start_line = top_window_pos / glyph_height;
        top_offset = top_window_pos % glyph_height;

        line_cnt = total_lines - start_line;
        if (line_cnt > lines_per_draw)
            line_cnt = lines_per_draw;

        u8g2.clearBuffer();            // clear the internal memory
        for (i = 0; i < line_cnt; i++) // draw visible lines
        {
            /* copy a line of the text to the local buffer */
            u8x8_CopyStringLine(buf, i + start_line, str);

            /* draw the content of the local buffer */
            u8g2.drawUTF8(0, i * glyph_height - top_offset + glyph_height, buf);
        }
        u8g2.sendBuffer(); // transfer internal memory to the display
        for (;;)
        {
            event = u8g2.getMenuEvent();
            if (event == U8X8_MSG_GPIO_MENU_DOWN)
            {
                top_window_pos += 8;
                break;
            }
            else if (event == U8X8_MSG_GPIO_MENU_SELECT)
            {
                return;
            }
            rt_thread_mdelay(10);
        }
    }
}

static void draw_usb_test_explain(void)
{
    show_scroll_text(usb_test_explain);
}

static int exec_eeprom_test(void)
{
#define TEST_DATA1 (0xaa)
#define TEST_DATA2 (0xcc)
    int ret;
    uint8_t buf;

    /* 先读取0和1地址的数据，判断是否为期望的数据 */
    ret = at24cxx_read_byte(0, &buf);
    if (ret != 0)
    {
        return ret;
    }
    if (buf != TEST_DATA1)
    {
        goto __write_test;
    }
    ret = at24cxx_read_byte(1, &buf);
    if (ret != 0)
    {
        return ret;
    }
    if (buf != TEST_DATA2)
    {
        goto __write_test;
    }
    /* 读取的数据符合期望，省略写入测试 */
    return 0;

/* 若读取的数据与期望数据不一致，则进行写入测试 */
__write_test:
    ret = at24cxx_write_byte(0, TEST_DATA1);
    if (ret != 0)
    {
        return ret;
    }
    ret = at24cxx_read_byte(0, &buf);
    if (ret != 0)
    {
        return ret;
    }
    if (buf != TEST_DATA1)
    {
        return -10;
    }

    ret = at24cxx_write_byte(1, TEST_DATA2);
    if (ret != 0)
    {
        return ret;
    }
    ret = at24cxx_read_byte(1, &buf);
    if (ret != 0)
    {
        return ret;
    }
    if (buf != TEST_DATA2)
    {
        return -11;
    }
    return 0;
}

static void draw_eeprom_test(void)
{
    int ret;
    uint8_t sel;
    sel = u8g2.userInterfaceMessage("说明", "", "确定开始测试?", "确定\n返回");
    if (sel == SELECT_CONFIRM_BUTTON)
    {
        ret = exec_eeprom_test();
        if (ret == 0)
        {
            u8g2.userInterfaceMessage("结果", "", "EEPROM测试成功", "返回");
        }
        else
        {
            u8g2.userInterfaceMessage("结果", "", "EEPROM测试失败", "返回");
        }
    }
}

static void oled_test(void)
{
    /* 全屏显示，检测是否有坏点 */
    u8g2.clearBuffer();
    u8g2.drawBox(0, 0, u8g2.getDisplayWidth(), u8g2.getDisplayHeight());
    u8g2.sendBuffer();
}

static void draw_oled_test(void)
{
    uint8_t sel;
    sel = u8g2.userInterfaceMessage("说明\n该测试检测屏幕是否有", "坏点", "", "确认\n返回");
    if (sel == SELECT_CONFIRM_BUTTON)
    {
        oled_test();
        rt_thread_mdelay(2000);
    }
    return;
}

#define TEST_FN "/test.dat"
/* 测试用的数据和缓冲 */
static char test_data[120], buffer[120];

static int exec_spi_flash_test(void)
{
    int fd;
    int index, length;

    /* 只写 & 创建 打开 */
    fd = open(TEST_FN, O_WRONLY | O_CREAT | O_TRUNC, 0);
    if (fd < 0)
    {
        rt_kprintf("open file for write failed\n");
        return -1;
    }

    /* 准备写入数据 */
    for (index = 0; index < sizeof(test_data); index++)
    {
        test_data[index] = index + 27;
    }

    /* 写入数据 */
    length = write(fd, test_data, sizeof(test_data));
    if (length != sizeof(test_data))
    {
        rt_kprintf("write data failed\n");
        close(fd);
        return -1;
    }

    /* 关闭文件 */
    close(fd);

    /* 只写并在末尾添加打开 */
    fd = open(TEST_FN, O_WRONLY | O_CREAT | O_APPEND, 0);
    if (fd < 0)
    {
        rt_kprintf("open file for append write failed\n");
        return -1;
    }

    length = write(fd, test_data, sizeof(test_data));
    if (length != sizeof(test_data))
    {
        rt_kprintf("append write data failed\n");
        close(fd);
        return -1;
    }
    /* 关闭文件 */
    close(fd);

    /* 只读打开进行数据校验 */
    fd = open(TEST_FN, O_RDONLY, 0);
    if (fd < 0)
    {
        rt_kprintf("check: open file for read failed\n");
        return -1;
    }

    /* 读取数据(应该为第一次写入的数据) */
    length = read(fd, buffer, sizeof(buffer));
    if (length != sizeof(buffer))
    {
        rt_kprintf("check: read file failed\n");
        close(fd);
        return -1;
    }

    /* 检查数据是否正确 */
    for (index = 0; index < sizeof(test_data); index++)
    {
        if (test_data[index] != buffer[index])
        {
            rt_kprintf("check: check data failed at %d\n", index);
            close(fd);
            return -1;
        }
    }

    /* 读取数据(应该为第二次写入的数据) */
    length = read(fd, buffer, sizeof(buffer));
    if (length != sizeof(buffer))
    {
        rt_kprintf("check: read file failed\n");
        close(fd);
        return -1;
    }

    /* 检查数据是否正确 */
    for (index = 0; index < sizeof(test_data); index++)
    {
        if (test_data[index] != buffer[index])
        {
            rt_kprintf("check: check data failed at %d\n", index);
            close(fd);
            return -1;
        }
    }

    /* 检查数据完毕，关闭文件 */
    close(fd);
    /* 打印结果 */
    rt_kprintf("read/write done.\n");
    return 0;
}

static void draw_spi_flash_test(void)
{
    int ret;
    uint8_t sel;
    sel = u8g2.userInterfaceMessage("说明", "", "确定开始测试?", "确定\n返回");
    if (sel == SELECT_CONFIRM_BUTTON)
    {
        ret = exec_spi_flash_test();
        if (ret == 0)
        {
            u8g2.userInterfaceMessage("结果", "", "SPI FLASH测试成功", "返回");
        }
        else
        {
            u8g2.userInterfaceMessage("结果", "", "SPI FLASH测试失败", "返回");
        }
    }
}

static void draw_db9_test(void)
{
    show_scroll_text(db9_test_explain);
}

static void auto_test(void)
{
    int ret;
    /* OLED测试 */
    oled_test();

    /* 蜂鸣器测试 */
    rt_pin_mode(BEEP_PIN, PIN_MODE_OUTPUT);
    for (uint8_t i = 0; i < 2; i++)
    {
        rt_pin_write(BEEP_PIN, PIN_HIGH); //蜂鸣器响
        rt_thread_mdelay(500);
        rt_pin_write(BEEP_PIN, PIN_LOW); //关闭蜂鸣器
        rt_thread_mdelay(500);
    }

    u8g2.clearBuffer();

    /* SPI FLASH测试 */
    ret = exec_spi_flash_test();
    u8g2.setCursor(0, 15);
    if (ret != 0)
    {
        u8g2.print("SPI FLASH 测试失败");
        show_X_mark();
    }
    else
    {
        u8g2.print("SPI FLASH 测试成功");
        show_check_mark();
    }
    u8g2.sendBuffer();

    /* EEPROM测试 */
    ret = exec_eeprom_test();
    u8g2.setCursor(0, 30);
    if (ret != 0)
    {
        u8g2.print("EEPROM 测试失败");
        show_X_mark();
    }
    else
    {
        u8g2.print("EEPROM 测试成功");
        show_check_mark();
    }
    u8g2.sendBuffer();

    u8g2.setCursor(0, 50);
    u8g2.print("按KEY4进入下一页");
    u8g2.sendBuffer();

    for (;;)
    {
        int8_t event;
        event = u8g2.getMenuEvent();
        if (event == U8X8_MSG_GPIO_MENU_SELECT)
        {
            break;
        }
        rt_thread_mdelay(10);
    }
}

static void board_test_thread_entry(void *parameter)
{
    auto_test();
    first_screen();

    while (1)
    {
        current_selection = u8g2.userInterfaceSelectionList(
            "选择测试项目",
            current_selection,
            string_list);

        switch (current_selection)
        {
        case MENU_EEPROM:
            draw_eeprom_test();
            break;
        case MENU_OLED:
            draw_oled_test();
            break;
        case MENU_BEEP:
            rt_pin_mode(BEEP_PIN, PIN_MODE_OUTPUT);
            rt_pin_write(BEEP_PIN, PIN_HIGH); //蜂鸣器响
            u8g2.userInterfaceMessage("说明", "蜂鸣器发出声音则正常", "", "返回");
            rt_pin_write(BEEP_PIN, PIN_LOW); //关闭蜂鸣器
            break;
        case MENU_SPI_FLASH:
            draw_spi_flash_test();
            break;
        case MENU_USB:
            draw_usb_test_explain();
            break;
        case MENU_DB9:
            draw_db9_test();
            break;
        case MENU_ONBOARD_LED:
            u8g2.userInterfaceMessage("说明", "观察PD2与PC13是否闪烁,", "频率2HZ", "返回");
            break;
        default:
            LOG_D("error selection");
            break;
        }
    }
}

static void module_info_thread(void *parameter)
{
    for (;;)
    {
        u8g2.clearBuffer();
        u8g2.setCursor(0, 12);
        u8g2.print("当前设备:");
        u8g2.setCursor(0, 27);
        u8g2.print(fro_module_name);
        u8g2.setCursor(0, 48);
        u8g2.print("设备信息:");
        u8g2.setCursor(0, 63);
        u8g2.print(fro_module_info_str);
        u8g2.sendBuffer();
        rt_thread_mdelay(100);
    }
}

static void u8g2_init(void)
{
    u8g2.begin(/*Select=*/U8G2_PIN_SELECT, /*Right/Next=*/U8G2_PIN_RIGHT, /*Left/Prev=*/U8G2_PIN_LEFT, /*Up=*/U8G2_PIN_UP, /*Down=*/U8G2_PIN_DOWN, /*Home/Cancel=*/U8G2_PIN_HOME);
    u8g2.enableUTF8Print(); // enable UTF8 support for the Arduino print() function
    u8g2.setFont(CHINESE_FONT);
    u8g2.setFontDirection(0);
}

static int test_init(void)
{
    u8g2_init();

    if (get_current_module() == RT_NULL)
    {
        tid = rt_thread_create("BoardTest",
                               board_test_thread_entry, RT_NULL,
                               THREAD_STACK_SIZE,
                               THREAD_PRIORITY, THREAD_TIMESLICE);
    }
    else
    {
        tid = rt_thread_create("ModuleInfo",
                               module_info_thread, RT_NULL,
                               THREAD_STACK_SIZE,
                               THREAD_PRIORITY, THREAD_TIMESLICE);
    }
    if (tid != RT_NULL)
        rt_thread_startup(tid);

    return 0;
}
INIT_APP_EXPORT(test_init);
