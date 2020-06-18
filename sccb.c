#include "sccb.h"
//#include "sys_delay.h"
extern uint32_t SystemTICK_RATE_HZ;
extern uint32_t SystemCoreClockl;
#define portTICK_PERIOD_US			( (uint32_t) 1000000 / SystemTICK_RATE_HZ )

void SCCB_Init(void){
    SET_BIT(RCC_BASE + RCC_AHB1ENR_OFFSET, GPIO_EN_BIT(GPIO_PORTA));
    SET_BIT(RCC_BASE + RCC_AHB1ENR_OFFSET, GPIO_EN_BIT(GPIO_PORTB));

    WRITE_BITS(RCC_BASE + RCC_CFGR_OFFSET, MCO1_1_BIT, MCO1_0_BIT, 0);
    WRITE_BITS(RCC_BASE + RCC_CFGR_OFFSET, MCO1PRE_2_BIT, MCO1PRE_0_BIT, 0b110);

    SET_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_MODER_OFFSET, MODERy_1_BIT(8));							//MODER => 10
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_MODER_OFFSET, MODERy_0_BIT(8));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_OTYPER_OFFSET, OTy_BIT(8));								//OT => 0
	SET_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(8));						//OSPEEDR => 10
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(8));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(8));							//PUPDR = 00 => No pull-up, pull-down
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(8));
	WRITE_BITS(GPIO_BASE(GPIO_PORTA) + GPIOx_AFRH_OFFSET, AFRHy_3_BIT(8), AFRHy_0_BIT(8), 0);		//AF sel	dcmi	13

    CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_MODER_OFFSET, MODERy_1_BIT(8));
	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_MODER_OFFSET, MODERy_0_BIT(8));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OTYPER_OFFSET, OTy_BIT(8));
	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(8));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(8));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(8));
	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(8));

    CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_MODER_OFFSET, MODERy_1_BIT(9));
	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_MODER_OFFSET, MODERy_0_BIT(9));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OTYPER_OFFSET, OTy_BIT(9));
	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(9));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(9));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(9));
	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(9));

    SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_BSRR_OFFSET, BSy_BIT(8));
    SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_BSRR_OFFSET, BSy_BIT(9));
    /*
  	GPIO_InitTypeDef  GPIO_InitStructure;

  	RCC_AHB1PeriphClockCmd(OV7670_SCCB_RCC, ENABLE);
  	RCC_AHB1PeriphClockCmd(OV7670_XCLK_RCC, ENABLE);

	//STM32F4ʱ������XCLK
  	GPIO_InitStructure.GPIO_Pin = OV7670_XCLK_Pin;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//50MHz

  	GPIO_Init(OV7670_XCLK_GPIO, &GPIO_InitStructure);

  	RCC_MCO1Config(RCC_MCO1Source_HSI,STM32_MCO1_DIV);

  	//GPIOF9,F10��ʼ������
  	GPIO_InitStructure.GPIO_Pin = OV7670_SCCB_Pin;//PD6,7 ��������
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;  //PD6,7 ��������
  	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//��������
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//100MHz
  	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
  	GPIO_Init(OV7670_SCCB_GPIO, &GPIO_InitStructure);//��ʼ��

	GPIO_SetBits(OV7670_SCCB_GPIO,OV7670_SCCB_Pin);
	SCCB_SDA_OUT();
    */
}

void SCCB_Start(void){
    SCCB_SDA_H;
	//delay_us(500);
    vTaskDelay(500/portTICK_PERIOD_US);
    SCCB_SCL_H;
    //delay_us(500);
    vTaskDelay(500/portTICK_PERIOD_US);
    SCCB_SDA_L;
    //delay_us(500);
    vTaskDelay(500/portTICK_PERIOD_US);
    SCCB_SCL_L;
	//delay_us(500);
    vTaskDelay(500/portTICK_PERIOD_US);
}

void SCCB_Stop(void){
    SCCB_SDA_L;
    //delay_us(500);
    vTaskDelay(500/portTICK_PERIOD_US);
    SCCB_SCL_H;
    //delay_us(500);
    vTaskDelay(500/portTICK_PERIOD_US);
    SCCB_SDA_H;
    //delay_us(500);
    vTaskDelay(500/portTICK_PERIOD_US);
}

void SCCB_No_Ack(void){
	//delay_us(500);
    vTaskDelay(500/portTICK_PERIOD_US);
	SCCB_SDA_H;
	SCCB_SCL_H;
	//delay_us(500);
    vTaskDelay(500/portTICK_PERIOD_US);
	SCCB_SCL_L;
	//delay_us(500);
    vTaskDelay(500/portTICK_PERIOD_US);
	SCCB_SDA_L;
	//delay_us(500);
    vTaskDelay(500/portTICK_PERIOD_US);
}

uint8_t SCCB_WR_Byte(uint8_t dat){
	uint8_t j,res;
	for(j=0;j<8;j++)
	{
		if(dat&0x80)
            SCCB_SDA_H;
		else
            SCCB_SDA_L;
		dat<<=1;
		//delay_us(500);
        vTaskDelay(500/portTICK_PERIOD_US);
		SCCB_SCL_H;
		//delay_us(500);
        vTaskDelay(500/portTICK_PERIOD_US);
		SCCB_SCL_L;
	}
	SCCB_SDA_IN();
	//delay_us(500);
    vTaskDelay(500/portTICK_PERIOD_US);
	SCCB_SCL_H;
	//delay_us(100);
    vTaskDelay(500/portTICK_PERIOD_US);
	if(SCCB_READ_SDA)
        res=1;
	else
        res=0;
	SCCB_SCL_L;
	SCCB_SDA_OUT();
	return res;
}

uint8_t SCCB_RD_Byte(void){
	uint8_t temp=0,j;
	SCCB_SDA_IN();
	for(j=8;j>0;j--){
		//delay_us(500);
        vTaskDelay(500/portTICK_PERIOD_US);
		SCCB_SCL_H;
		temp=temp<<1;
		if(SCCB_READ_SDA)
            temp++;
		//delay_us(500);
        vTaskDelay(500/portTICK_PERIOD_US);
		SCCB_SCL_L;
	}
	SCCB_SDA_OUT();
	return temp;
}

uint8_t SCCB_WR_Reg(uint8_t reg,uint8_t data){
	uint8_t res=0;
	SCCB_Start();
	if(SCCB_WR_Byte(SCCB_ID))
        res=1;
	//delay_us(100);
    vTaskDelay(100/portTICK_PERIOD_US);
  	if(SCCB_WR_Byte(reg))
        res=1;
	//delay_us(100);
    vTaskDelay(100/portTICK_PERIOD_US);
  	if(SCCB_WR_Byte(data))
        res=1;
  	SCCB_Stop();
    vTaskDelay(100/portTICK_PERIOD_US);
  	return	res;
}

uint8_t SCCB_RD_Reg(uint8_t reg){
	uint8_t val=0;
	SCCB_Start();
	SCCB_WR_Byte(SCCB_ID);
	//delay_us(100);
    vTaskDelay(100/portTICK_PERIOD_US);
  	SCCB_WR_Byte(reg);
	//delay_us(100);
    vTaskDelay(100/portTICK_PERIOD_US);
	SCCB_Stop();
	//delay_us(100);
    vTaskDelay(100/portTICK_PERIOD_US);

	SCCB_Start();
	SCCB_WR_Byte(SCCB_ID|0X01);
	//delay_us(100);
    vTaskDelay(100/portTICK_PERIOD_US);
  	val=SCCB_RD_Byte();
  	SCCB_No_Ack();
  	SCCB_Stop();
    vTaskDelay(100/portTICK_PERIOD_US);
  	return val;
}
