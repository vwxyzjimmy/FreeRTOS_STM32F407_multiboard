#include "ov7670.h"
#include "ov7670config.h"
//#include "sys_delay.h"
#include "sccb.h"
#include "dcmi.h"
//////////////////////////////////////////////////////////////////////////////////
//�������ο�������guanfu_wang���롣
//ALIENTEKս��STM32������
//OV7670 ��������
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2012/9/14
//�汾��V1.0
//////////////////////////////////////////////////////////////////////////////////
uint16_t camera_buffer[PIC_WIDTH*PIC_HEIGHT]={0};

//����RESET��PWDN����
void OV7670_RST_PW_Init(void){

	//����IO
	/*
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(OV7670_RST_PW_RCC, ENABLE);

	//RESET,PWDN��ʼ������
	GPIO_InitStructure.GPIO_Pin = OV7670_RST_PW_Pin;//RESET,PWDN��������
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT; //��������
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//��������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
	GPIO_Init(OV7670_RST_PW_GPIO, &GPIO_InitStructure);//��ʼ��
	*/
	SET_BIT(RCC_BASE + RCC_AHB1ENR_OFFSET, GPIO_EN_BIT(GPIO_PORTB));
	//MODER led pin = 01 => General purpose output mode
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_MODER_OFFSET, MODERy_1_BIT(4));
	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_MODER_OFFSET, MODERy_0_BIT(4));
	//OT led pin = 0 => Output push-pull
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OTYPER_OFFSET, OTy_BIT(4));
	//OSPEEDR led pin = 00 => Low speed
	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(4));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(4));
	//PUPDR led pin = 00 => pull-uP
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(4));
	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(4));

	//MODER led pin = 01 => General purpose output mode
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_MODER_OFFSET, MODERy_1_BIT(5));
	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_MODER_OFFSET, MODERy_0_BIT(5));
	//OT led pin = 0 => Output push-pull
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OTYPER_OFFSET, OTy_BIT(5));
	//OSPEEDR led pin = 00 => Low speed
	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(5));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(5));
	//PUPDR led pin = 00 => pull-up
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(5));
	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(5));
}

//��ʼ��OV7670
//����0:�ɹ�
//��������ֵ:��������
uint8_t OV7670_Init(void){
	uint16_t i=0;
	uint16_t reg=0;

	OV7670_RST_PW_Init();

	OV7670_PWDN_L;	//POWER ON
	for(uint32_t i=0;i<5000;i++)
		;
	//delay_ms(100);
	OV7670_RST_L;	//��λOV7670
	for(uint32_t i=0;i<5000;i++)
		;
	//delay_ms(100);
	OV7670_RST_H;	//������λ

	SCCB_Init();        		//��ʼ��SCCB ��IO��
	SCCB_WR_Reg(0X12, 0x80);	//����λOV7670
	for(uint32_t i=0;i<5000;i++)
		;
	//delay_ms(50);

	reg=SCCB_RD_Reg(0x1c);	//��ȡ����ID �߰�λ
	reg<<=8;
	reg|=SCCB_RD_Reg(0x1d);	//��ȡ����ID �Ͱ�λ
	if(reg != OV7670_MID){
		printf("\r\nMID:%d\r\n",reg);
		return 1;
	}

	reg=SCCB_RD_Reg(0x0a);	//��ȡ����ID �߰�λ
	reg<<=8;
	reg|=SCCB_RD_Reg(0x0b);	//��ȡ����ID �Ͱ�λ
	if(reg != OV7670_PID){
		printf("HID:%d\r\n\r\n",reg);
		return 2;
	}

	//��ʼ�� OV7670,����QVGA�ֱ���(320*240)
	for(i=0;i<sizeof(ov7670_init_reg_tbl)/sizeof(ov7670_init_reg_tbl[0]);i++)
	{
		SCCB_WR_Reg(ov7670_init_reg_tbl[i][0],ov7670_init_reg_tbl[i][1]);
	}

	OV7670_Window_Set(PIC_START_X,PIC_START_Y,PIC_WIDTH,PIC_HEIGHT);
	for(uint32_t i=0;i<5;i++){
		OV7670_Light_Mode(0);
		OV7670_Color_Saturation(2);
		OV7670_Brightness(1);
		OV7670_Contrast(2);
		OV7670_Special_Effects(0);
	}
	My_DCMI_Init();
	DCMI_DMA_Init((uint32_t)&camera_buffer,sizeof(camera_buffer)/4);//DCMI DMA

	return 0x00; 	//ok
}
////////////////////////////////////////////////////////////////////////////
//OV7670��������
//��ƽ������
//0:�Զ�
//1:̫��sunny
//2,����cloudy
//3,�칫��office
//4,����home
void OV7670_Light_Mode(uint8_t mode)
{
	uint8_t reg13val=0XE7;
	uint8_t reg01val=0;
	uint8_t reg02val=0;
	switch(mode)
	{
		case 1://sunny
			reg13val=0XE5;
			reg01val=0X5A;
			reg02val=0X5C;
			break;
		case 2://cloudy
			reg13val=0XE5;
			reg01val=0X58;
			reg02val=0X60;
			break;
		case 3://office
			reg13val=0XE5;
			reg01val=0X84;
			reg02val=0X4c;
			break;
		case 4://home
			reg13val=0XE5;
			reg01val=0X96;
			reg02val=0X40;
			break;
	}
	SCCB_WR_Reg(0X13,reg13val);//COM8����
	SCCB_WR_Reg(0X01,reg01val);//AWB��ɫͨ������
	SCCB_WR_Reg(0X02,reg02val);//AWB��ɫͨ������
}
//ɫ������
//0:-2
//1:-1
//2,0
//3,1
//4,2
void OV7670_Color_Saturation(uint8_t sat){

	uint8_t reg4f5054val=0X80;//Ĭ�Ͼ���sat=2,��������ɫ�ȵ�����
 	uint8_t reg52val=0X22;
	uint8_t reg53val=0X5E;
 	switch(sat)
	{
		case 0://-2
			reg4f5054val=0X40;
			reg52val=0X11;
			reg53val=0X2F;
			break;
		case 1://-1
			reg4f5054val=0X66;
			reg52val=0X1B;
			reg53val=0X4B;
			break;
		case 3://1
			reg4f5054val=0X99;
			reg52val=0X28;
			reg53val=0X71;
			break;
		case 4://2
			reg4f5054val=0XC0;
			reg52val=0X33;
			reg53val=0X8D;
			break;
	}
	SCCB_WR_Reg(0X4F,reg4f5054val);	//ɫ�ʾ���ϵ��1
	SCCB_WR_Reg(0X50,reg4f5054val);	//ɫ�ʾ���ϵ��2
	SCCB_WR_Reg(0X51,0X00);			//ɫ�ʾ���ϵ��3
	SCCB_WR_Reg(0X52,reg52val);		//ɫ�ʾ���ϵ��4
	SCCB_WR_Reg(0X53,reg53val);		//ɫ�ʾ���ϵ��5
	SCCB_WR_Reg(0X54,reg4f5054val);	//ɫ�ʾ���ϵ��6
	SCCB_WR_Reg(0X58,0X9E);			//MTXS
}
//��������
//0:-2
//1:-1
//2,0
//3,1
//4,2
void OV7670_Brightness(uint8_t bright){

	uint8_t reg55val=0X00;//Ĭ�Ͼ���bright=2
  	switch(bright)
	{
		case 0://-2
			reg55val=0XB0;
			break;
		case 1://-1
			reg55val=0X98;
			break;
		case 3://1
			reg55val=0X18;
			break;
		case 4://2
			reg55val=0X30;
			break;
	}
	SCCB_WR_Reg(0X55,reg55val);	//���ȵ���
}
//�Աȶ�����
//0:-2
//1:-1
//2,0
//3,1
//4,2
void OV7670_Contrast(uint8_t contrast){

	uint8_t reg56val=0X40;//Ĭ�Ͼ���contrast=2
  	switch(contrast)
	{
		case 0://-2
			reg56val=0X30;
			break;
		case 1://-1
			reg56val=0X38;
			break;
		case 3://1
			reg56val=0X50;
			break;
		case 4://2
			reg56val=0X60;
			break;
	}
	SCCB_WR_Reg(0X56,reg56val);	//�Աȶȵ���
}
//��Ч����
//0:��ͨģʽ
//1,��Ƭ
//2,�ڰ�
//3,ƫ��ɫ
//4,ƫ��ɫ
//5,ƫ��ɫ
//6,����
void OV7670_Special_Effects(uint8_t eft){

	uint8_t reg3aval=0X04;//Ĭ��Ϊ��ͨģʽ
	uint8_t reg67val=0XC0;
	uint8_t reg68val=0X80;
	switch(eft)
	{
		case 1://��Ƭ
			reg3aval=0X24;
			reg67val=0X80;
			reg68val=0X80;
			break;
		case 2://�ڰ�
			reg3aval=0X14;
			reg67val=0X80;
			reg68val=0X80;
			break;
		case 3://ƫ��ɫ
			reg3aval=0X14;
			reg67val=0XC0;
			reg68val=0X80;
			break;
		case 4://ƫ��ɫ
			reg3aval=0X14;
			reg67val=0X40;
			reg68val=0X40;
			break;
		case 5://ƫ��ɫ
			reg3aval=0X14;
			reg67val=0X80;
			reg68val=0XC0;
			break;
		case 6://����
			reg3aval=0X14;
			reg67val=0XA0;
			reg68val=0X40;
			break;
	}
	SCCB_WR_Reg(0X3A,reg3aval);//TSLB����
	SCCB_WR_Reg(0X68,reg67val);//MANU,�ֶ�Uֵ
	SCCB_WR_Reg(0X67,reg68val);//MANV,�ֶ�Vֵ
}
//����ͼ����������
//��QVGA���á�
void OV7670_Window_Set(uint16_t sx,uint16_t sy,uint16_t width,uint16_t height){

	uint16_t endx;
	uint16_t endy;
	uint8_t temp;

	if ((sx+width) > 320)
	{
		width = 320 - sx;
	}

	if ((sy+height) > 240)
	{
		height = 240 - sy;
	}

	sx += 176;
	sy += 12;

	endx=sx+width*2;	//HREF
 	endy=sy+height*2;	//VREF
	if(endx>784)
	{
		endx-=784;
	}

	temp=SCCB_RD_Reg(0X32);				//��ȡHref֮ǰ��ֵ
	temp&=0XC0;
	temp|=((endx&0X07)<<3)|(sx&0X07);
	SCCB_WR_Reg(0X32,temp);
	SCCB_WR_Reg(0X17,sx>>3);			//����Href��start��8λ
	SCCB_WR_Reg(0X18,endx>>3);			//����Href��end�ĸ�8λ

	temp=SCCB_RD_Reg(0X03);				//��ȡVref֮ǰ��ֵ
	temp&=0XF0;
	temp|=((endy&0X03)<<2)|(sy&0X03);
	SCCB_WR_Reg(0X03,temp);				//����Vref��start��end������2λ
	SCCB_WR_Reg(0X19,sy>>2);			//����Vref��start��8λ
	SCCB_WR_Reg(0X1A,endy>>2);			//����Vref��end�ĸ�8λ
}
