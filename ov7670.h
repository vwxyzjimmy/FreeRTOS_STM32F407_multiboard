#ifndef _OV7670_H
#define _OV7670_H
#include "reg.h"
#include <stdio.h>
//#include "stm32f4xx.h"

/*
#define OV7670_RST_PW_RCC		RCC_AHB1Periph_GPIOG
#define OV7670_RST_PW_Pin		GPIO_Pin_4|GPIO_Pin_5
#define OV7670_RST_PW_GPIO		GPIOB

#define OV7670_PWDN_H  			GPIO_SetBits(GPIOG,GPIO_Pin_9)
#define OV7670_PWDN_L  			GPIO_ResetBits(GPIOG,GPIO_Pin_9)

#define OV7670_RST_H  			GPIO_SetBits(GPIOG,GPIO_Pin_8)
#define OV7670_RST_L  			GPIO_ResetBits(GPIOG,GPIO_Pin_8)
*/

#define OV7670_PWDN_H  			SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_BSRR_OFFSET, BSy_BIT(4));
#define OV7670_PWDN_L  			SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_BSRR_OFFSET, BRy_BIT(4));

#define OV7670_RST_H  			SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_BSRR_OFFSET, BSy_BIT(5));
#define OV7670_RST_L  			SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_BSRR_OFFSET, BRy_BIT(5));


#if(USE_CAMERA == 1)
    #define PIC_START_X				32
    #define PIC_START_Y				32
    #define PIC_WIDTH				64
    #define PIC_HEIGHT				64
#else
    #define PIC_START_X				0
    #define PIC_START_Y				0
    #define PIC_WIDTH				1
    #define PIC_HEIGHT				1
#endif
extern uint16_t camera_buffer[PIC_WIDTH*PIC_HEIGHT];

//////////////////////////////////////////////////////////////////////////////////
#define OV7670_MID				0X7FA2
#define OV7670_PID				0X7673
/////////////////////////////////////////

uint8_t   OV7670_Init(void);
void OV7670_Light_Mode(uint8_t mode);
void OV7670_Color_Saturation(uint8_t sat);
void OV7670_Brightness(uint8_t bright);
void OV7670_Contrast(uint8_t contrast);
void OV7670_Special_Effects(uint8_t eft);
void OV7670_Window_Set(uint16_t sx,uint16_t sy,uint16_t width,uint16_t height);
void set_cif(void);

#endif
