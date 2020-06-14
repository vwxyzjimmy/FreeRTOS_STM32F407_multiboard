#ifndef __SCCB_H
#define __SCCB_H
#include "reg.h"
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"

/*
#define OV7670_XCLK_RCC		RCC_AHB1Periph_GPIOA
#define OV7670_XCLK_Pin		GPIO_Pin_8
#define OV7670_XCLK_GPIO	GPIOA
#define STM32_MCO1_DIV		RCC_MCO1Div_4

#define OV7670_SCCB_RCC		RCC_AHB1Periph_GPIOB
#define OV7670_SCCB_Pin		GPIO_Pin_|GPIO_Pin_9
#define OV7670_SCCB_GPIO	GPIOF

#define SCCB_SDA_IN()  		{GPIOF->MODER&=~(0x0003<<18);GPIOF->MODER|=(0x0000<<18);}	//PF9 ���� �Ĵ�������2*9=18λ
#define SCCB_SDA_OUT() 		{GPIOF->MODER&=~(0x0003<<18);GPIOF->MODER|=(0x0001<<18);} 	//PF9 ����

#define SCCB_SCL_H    		GPIO_SetBits(GPIOF,GPIO_Pin_10)	 		//SCL
#define SCCB_SCL_L    		GPIO_ResetBits(GPIOF,GPIO_Pin_10)	 	//SCL

#define SCCB_SDA_H    		GPIO_SetBits(GPIOF,GPIO_Pin_9) 			//SDA
#define SCCB_SDA_L    		GPIO_ResetBits(GPIOF,GPIO_Pin_9) 		//SDA

#define SCCB_READ_SDA   	GPIO_ReadInputDataBit(GPIOF,GPIO_Pin_9) //����SDA
*/
#define SCCB_SDA_IN()  		WRITE_BITS(GPIO_BASE(GPIO_PORTB) + GPIOx_MODER_OFFSET, MODERy_1_BIT(9), MODERy_0_BIT(9), 0b00)  //PB9 ���� �Ĵ�������2*9=18λ
#define SCCB_SDA_OUT() 		WRITE_BITS(GPIO_BASE(GPIO_PORTB) + GPIOx_MODER_OFFSET, MODERy_1_BIT(9), MODERy_0_BIT(9), 0b01) 	//PB9 ����

#define SCCB_SCL_H    		SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_BSRR_OFFSET, BSy_BIT(8))	 		//SCL
#define SCCB_SCL_L    		SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_BSRR_OFFSET, BRy_BIT(8))	    	//SCL

#define SCCB_SDA_H    		SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_BSRR_OFFSET, BSy_BIT(9)) 		//SDA
#define SCCB_SDA_L    		SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_BSRR_OFFSET, BRy_BIT(9)) 		//SDA

#define SCCB_READ_SDA   	READ_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_IDR_OFFSET, IDy_BIT(9)) //����SDA

/*************************��Ҫ�޸ĵĵط�*************************/

#define SCCB_ID   			0X42  			//OV7670��ID

///////////////////////////////////////////
void SCCB_Init(void);
void SCCB_Start(void);
void SCCB_Stop(void);
void SCCB_No_Ack(void);
uint8_t SCCB_WR_Byte(uint8_t dat);
uint8_t SCCB_RD_Byte(void);
uint8_t SCCB_WR_Reg(uint8_t reg,uint8_t data);
uint8_t SCCB_RD_Reg(uint8_t reg);
#endif
