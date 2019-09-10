/*********************************************************
**版权所有: GuangZhou F.R.O Electronic Technology Co.,LTD.
**文件名称: oled.h
**功能描述: 用于初始化SPI屏
**			D0   接PC5（CLK）
**			D1   接PC4（MOSI）
**			RES  接PA7
**			DC   接PA6
**			CS   接PA5
**编写作者: 
**当前版本: V1.0
**修改时间: 2016-8-1
**历史版本: 无
***********************************************************/
#ifndef __OLED_H
#define __OLED_H

#include "board.h"
#include "stm32f1xx_ll_gpio.h"

//0:4线串行模式
//1:并行8080模式
#define SIZE 16
#define XLevelL 0x00
#define XLevelH 0x10
#define Max_Column 128
#define Max_Row 64
#define Brightness 0xFF
#define X_WIDTH 128
#define Y_WIDTH 64
//-----------------OLED端口定义----------------
#define OLED_CS_Clr() LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_7 ) //CS
#define OLED_CS_Set() LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_7 )

#define OLED_RST_Clr() LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_5) //RES
#define OLED_RST_Set() LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_5)

#define OLED_DC_Clr() LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_6) //DC
#define OLED_DC_Set() LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_6)

#define OLED_SCLK_Clr() LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_3) //CLK
#define OLED_SCLK_Set() LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_3)

#define OLED_SDIN_Clr() LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_4) //MOSI
#define OLED_SDIN_Set() LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_4)

#define OLED_CMD 0  //写命令
#define OLED_DATA 1 //写数据

//OLED控制用函数
void OLED_WR_Byte(uint8_t dat, uint8_t cmd);
void OLED_Display_On(void);
void OLED_Display_Off(void);
int OLED_Init(void);
void OLED_Clear(void);
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t t);
void OLED_Fill(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t dot);
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size);
void OLED_ShowString(uint8_t x, uint8_t y, char *p);
void OLED_Set_Pos(unsigned char x, unsigned char y);
void OLED_ShowCHinese(uint8_t x, uint8_t y, uint8_t no, uint8_t fontsize);
void OLED_DrawBMP(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, unsigned char BMP[]);
#endif
