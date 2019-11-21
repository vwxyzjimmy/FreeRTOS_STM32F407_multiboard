#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include "reg.h"
#include "blink.h"
#include "asm_func.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"


#define HEAP_MAX (192 * 1024) //64 KB

void vApplicationTickHook() {}
void vApplicationStackOverflowHook() {}
void vApplicationIdleHook() {}
void vApplicationMallocFailedHook() {}

void setup_mpu(void);
void init_dac(void);
void DAC_SetChannel1Data(uint8_t vol);
void init_usart2(void);
void usart2_send_char(const char ch);
void print_sys(char str[255]);
void test_add_all(void);
void task1();
void task2();
void task3();
volatile uint8_t rec_play_buf_fir[200], rec_play_buf_sec[200];
volatile uint8_t *rece_ptr;
volatile uint8_t *play_ptr;
volatile uint8_t receive_count = 0;
volatile TaskHandle_t TaskHandle_1;
volatile TaskHandle_t TaskHandle_2;
volatile TaskHandle_t TaskHandle_3;
uint32_t test_target_array[] = {1, 2, 3, 4, 5};

void setup_mpu(void)
{
	//set region 0: flash (0x00000000), 1MB, allow execution, full access, enable all subregion
	//?????
	REG(MPU_BASE + MPU_RBAR_OFFSET) = MPU_RBAR_VALUE(0x00000000, 0);
	REG(MPU_BASE + MPU_RASR_OFFSET) = MPU_RASR_VALUE(MPU_XN_DISABLE, MPU_AP_FULL_ACCESS, MPU_TYPE_FLASH, 0, MPU_REGION_SIZE_1MB);
	//set region 1: sram (0x20000000), 128KB, forbid execution, full access, enable all subregion
	//?????
	REG(MPU_BASE + MPU_RBAR_OFFSET) = MPU_RBAR_VALUE(0x20000000, 1);
	REG(MPU_BASE + MPU_RASR_OFFSET) = MPU_RASR_VALUE(MPU_XN_DISABLE, MPU_AP_FULL_ACCESS, MPU_TYPE_SRAM, 0, MPU_REGION_SIZE_128KB);
	//set region 3: GPIOD, 32B, forbid execution, full access, enable all subregion
	//?????
	REG(MPU_BASE + MPU_RBAR_OFFSET) = MPU_RBAR_VALUE(USART1_BASE , 2);
	REG(MPU_BASE + MPU_RASR_OFFSET) = MPU_RASR_VALUE(MPU_XN_ENABLE, MPU_AP_PRIV_ACCESS, MPU_TYPE_PERIPHERALS, 0, MPU_REGION_SIZE_32B);
	//set region 2: RCC_AHB1ENR, 32B, forbid execution, full access, enable all subregion
	//?????
	REG(MPU_BASE + MPU_RBAR_OFFSET) = MPU_RBAR_VALUE(RCC_BASE + RCC_AHB1ENR_OFFSET, 3);
	REG(MPU_BASE + MPU_RASR_OFFSET) = MPU_RASR_VALUE(MPU_XN_ENABLE, MPU_AP_FULL_ACCESS, MPU_TYPE_PERIPHERALS, 0, MPU_REGION_SIZE_32B);
	//set region 3: GPIOD, 32B, forbid execution, full access, enable all subregion
	//?????
	REG(MPU_BASE + MPU_RBAR_OFFSET) = MPU_RBAR_VALUE(GPIO_BASE(GPIO_PORTD), 4);
	REG(MPU_BASE + MPU_RASR_OFFSET) = MPU_RASR_VALUE(MPU_XN_ENABLE, MPU_AP_FULL_ACCESS, MPU_TYPE_PERIPHERALS, 0, MPU_REGION_SIZE_32B);
	//disable region 4 ~ 7
	for (int i=5;i<8;i++)
	{
		//?????
		REG(MPU_BASE + MPU_RBAR_OFFSET) = MPU_RBAR_VALUE(0x00000000, i);
		REG(MPU_BASE + MPU_RASR_OFFSET) = 0;
	}
	//enable the default memory map as a background region for privileged access (PRIVDEFENA)
	//?????
	SET_BIT(MPU_BASE + MPU_CTRL_OFFSET, MPU_PRIVDEFENA_BIT);
	//enable mpu
	//?????
	SET_BIT(MPU_BASE + MPU_CTRL_OFFSET, MPU_ENABLE_BIT);
}

void task1(){
	printf("task1\r\n");
	led_init(LED_GREEN);
	SET_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_BSRR_OFFSET, BSy_BIT(LED_GREEN));
	while(1){
		if ((READ_BIT(USART2_BASE + USART_SR_OFFSET, RXNE_BIT)) || (READ_BIT(USART2_BASE + USART_SR_OFFSET, ORE_BIT))){
			char rec_cmd = (char)REG(USART2_BASE + USART_DR_OFFSET);
			printf("%c\r\n", rec_cmd);
			if (rec_cmd == 'a'){
				xTaskCreate(task2, "task2", 1000, NULL, 1, TaskHandle_2);
			}
			else if (rec_cmd == 'b'){
				printf("%c\r\n", rec_cmd);
				printf("kill task2\r\n");
				if (TaskHandle_2 == NULL){
					printf("dame that null\r\n");
				}
				vTaskDelete(TaskHandle_2);
				printf("already clean task2\r\n");
				SET_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_BSRR_OFFSET, BRy_BIT(LED_BLUE));
			}
		}
	}
	//blink(LED_GREEN);
}
void task2(){
	uint32_t tmp1[120];
	for(int i=0;i<120;i++)
		tmp1[i] = *((uint32_t*)(0x200199fc-(4*i)));
	blink(LED_BLUE);
}
void task3(){
	int tmp_count = 0;
	for(int i=0;i<100;i++){
		tmp_count++;
		;
	}
	printf("%d", tmp_count);

	init_dac();
	init_timer4();
	led_init(LED_BLUE);
	printf("task3\r\n");
	unsigned int play_count = 0;
	unsigned int led_state = 0;
	rece_ptr = rec_play_buf_fir;
	while(1){
		SET_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_BSRR_OFFSET, BRy_BIT(LED_ORANGE));
		volatile unsigned int read_uv = READ_BIT(TIM4_BASE + TIMx_SR_OFFSET, TIMx_SR_UIF);
		if(read_uv > 0){
			CLEAR_BIT(TIM4_BASE + TIMx_SR_OFFSET, TIMx_SR_UIF);
				if (led_state==0){
					led_state = 1;
					SET_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_BSRR_OFFSET, BSy_BIT(LED_BLUE));
				}
				else{
					led_state = 0;
					SET_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_BSRR_OFFSET, BRy_BIT(LED_BLUE));
				}
				DAC_SetChannel1Data(((uint8_t)*(play_ptr+play_count)));
				play_count++;
				if(play_count>=200){
					play_count = 0;
				}
		}
	}
}

int main(void)
{
	init_usart2();
	void (*func_ptr)() = task1;
	*((uint32_t*)(0x10010000-4)) = (uint32_t)0x0000000F;
	printf("i pass that\r\n");
	REG(AIRCR_BASE) = NVIC_AIRCR_RESET_VALUE | NVIC_PRIORITYGROUP_4;
	uint32_t *tmp_ptr = task2;
	uint32_t *tmp_ptr2 = task3;

	printf("task2: 0x%X\r\n", (unsigned int)tmp_ptr);
	xTaskCreate(func_ptr, "task1", 1000, NULL, 1, TaskHandle_1);
	vTaskStartScheduler();

	while (1)
		;
}

void svc_handler_c(uint32_t LR, uint32_t MSP)
{
	/*
	printf("[SVC Handler] LR: 0x%X\r\n", (unsigned int)LR);
	printf("[SVC Handler] MSP Backup: 0x%X \r\n", (unsigned int)MSP);
	printf("[SVC Handler] Control: 0x%X\r\n", (unsigned int)read_ctrl());
	printf("[SVC Handler] SP: 0x%X \r\n", (unsigned int)read_sp());
	printf("[SVC Handler] MSP: 0x%X \r\n", (unsigned int)read_msp());
	printf("[SVC Handler] PSP: 0x%X \r\n\n", (unsigned int)read_psp());
	*/
	uint32_t *stack_frame_ptr;
	if (LR & 0x4) //Test bit 2 of EXC_RETURN
	//if (READ_BIT(((uint32_t *)LR), 2)) //Test bit 2 of EXC_RETURN
	{
		stack_frame_ptr = (uint32_t *)read_psp(); //if 1, stacking used PSP
		//printf("[SVC Handler] Stacking used PSP: 0x%X \r\n\n", (unsigned int)stack_frame_ptr);
	}
	else
	{
		stack_frame_ptr = (uint32_t *)MSP; //if 0, stacking used MSP
		//printf("[SVC Handler] Stacking used MSP: 0x%X \r\n\n", (unsigned int)stack_frame_ptr);
	}

	uint32_t stacked_r0 = *(stack_frame_ptr);
	uint32_t stacked_r1 = *(stack_frame_ptr+1);
	uint32_t stacked_return_addr = *(stack_frame_ptr+6);

	uint16_t svc_instruction = *((uint16_t *)stacked_return_addr - 1);
	uint8_t svc_num = (uint8_t)svc_instruction;
	/*
	printf("[SVC Handler] Stacked R0: 0x%X \r\n", (unsigned int)stacked_r0);
	printf("[SVC Handler] Stacked R1: 0x%X \r\n", (unsigned int)stacked_r1);
	printf("[SVC Handler] SVC number: 0x%X \r\n\n", (unsigned int)svc_num);
	*/
	if (svc_num == 0xA)
		//return r0 + r1
		 *(stack_frame_ptr) = (stacked_r0 + stacked_r1);

	else if (svc_num == 0x4){
		for (unsigned int i = 0; i < (unsigned int)stacked_r1; i++)
			usart2_send_char( *((char *)stacked_r0++) );
 	}

	else
		//return 0
		*(stack_frame_ptr) = 0;
}

void init_eth(void)
{
	SET_BIT(NVIC_ISER_BASE + NVIC_ISERn_OFFSET(1), 29); //IRQ61 => (m+(32*n)) | m=29, n=1
	SET_BIT(RCC_BASE + RCC_APB2ENR_OFFSET, SYSCFGEN_BIT);
	SET_BIT(SYSCFG_BASE + SYSCFG_PMC_OFFSET, MII_RMII_SEL_BIT);

	SET_BIT(RCC_BASE + RCC_AHB1ENR_OFFSET, GPIO_EN_BIT(GPIO_PORTA));

	SET_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_MODER_OFFSET, MODERy_1_BIT(1));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_MODER_OFFSET, MODERy_0_BIT(1));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_OTYPER_OFFSET, OTy_BIT(1));
	SET_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(1));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(1));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(1));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(1));
	WRITE_BITS(GPIO_BASE(GPIO_PORTA) + GPIOx_AFRL_OFFSET, AFRLy_3_BIT(1), AFRLy_0_BIT(1), 11);

	SET_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_MODER_OFFSET, MODERy_1_BIT(2));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_MODER_OFFSET, MODERy_0_BIT(2));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_OTYPER_OFFSET, OTy_BIT(2));
	SET_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(2));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(2));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(2));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(2));
	WRITE_BITS(GPIO_BASE(GPIO_PORTA) + GPIOx_AFRL_OFFSET, AFRLy_3_BIT(2), AFRLy_0_BIT(2), 11);

	SET_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_MODER_OFFSET, MODERy_1_BIT(7));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_MODER_OFFSET, MODERy_0_BIT(7));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_OTYPER_OFFSET, OTy_BIT(7));
	SET_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(7));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(7));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(7));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(7));
	WRITE_BITS(GPIO_BASE(GPIO_PORTA) + GPIOx_AFRL_OFFSET, AFRLy_3_BIT(7), AFRLy_0_BIT(7), 11);

	SET_BIT(RCC_BASE + RCC_AHB1ENR_OFFSET, GPIO_EN_BIT(GPIO_PORTC));

	SET_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_MODER_OFFSET, MODERy_1_BIT(1));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_MODER_OFFSET, MODERy_0_BIT(1));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_OTYPER_OFFSET, OTy_BIT(1));
	SET_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(1));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(1));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(1));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(1));
	WRITE_BITS(GPIO_BASE(GPIO_PORTC) + GPIOx_AFRL_OFFSET, AFRLy_3_BIT(1), AFRLy_0_BIT(1), 11);

	SET_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_MODER_OFFSET, MODERy_1_BIT(4));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_MODER_OFFSET, MODERy_0_BIT(4));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_OTYPER_OFFSET, OTy_BIT(4));
	SET_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(4));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(4));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(4));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(4));
	WRITE_BITS(GPIO_BASE(GPIO_PORTC) + GPIOx_AFRL_OFFSET, AFRLy_3_BIT(4), AFRLy_0_BIT(4), 11);

	SET_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_MODER_OFFSET, MODERy_1_BIT(5));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_MODER_OFFSET, MODERy_0_BIT(5));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_OTYPER_OFFSET, OTy_BIT(5));
	SET_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(5));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(5));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(5));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(5));
	WRITE_BITS(GPIO_BASE(GPIO_PORTC) + GPIOx_AFRL_OFFSET, AFRLy_3_BIT(5), AFRLy_0_BIT(5), 11);

	SET_BIT(RCC_BASE + RCC_AHB1ENR_OFFSET, GPIO_EN_BIT(GPIO_PORTG));

	SET_BIT(GPIO_BASE(GPIO_PORTG) + GPIOx_MODER_OFFSET, MODERy_1_BIT(11));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTG) + GPIOx_MODER_OFFSET, MODERy_0_BIT(11));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTG) + GPIOx_OTYPER_OFFSET, OTy_BIT(11));
	SET_BIT(GPIO_BASE(GPIO_PORTG) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(11));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTG) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(11));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTG) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(11));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTG) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(11));
	WRITE_BITS(GPIO_BASE(GPIO_PORTG) + GPIOx_AFRL_OFFSET, AFRLy_3_BIT(11), AFRLy_0_BIT(11), 11);

	SET_BIT(GPIO_BASE(GPIO_PORTG) + GPIOx_MODER_OFFSET, MODERy_1_BIT(13));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTG) + GPIOx_MODER_OFFSET, MODERy_0_BIT(13));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTG) + GPIOx_OTYPER_OFFSET, OTy_BIT(13));
	SET_BIT(GPIO_BASE(GPIO_PORTG) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(13));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTG) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(13));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTG) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(13));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTG) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(13));
	WRITE_BITS(GPIO_BASE(GPIO_PORTG) + GPIOx_AFRL_OFFSET, AFRLy_3_BIT(13), AFRLy_0_BIT(13), 11);

	SET_BIT(GPIO_BASE(GPIO_PORTG) + GPIOx_MODER_OFFSET, MODERy_1_BIT(14));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTG) + GPIOx_MODER_OFFSET, MODERy_0_BIT(14));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTG) + GPIOx_OTYPER_OFFSET, OTy_BIT(14));
	SET_BIT(GPIO_BASE(GPIO_PORTG) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(14));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTG) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(14));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTG) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(14));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTG) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(14));
	WRITE_BITS(GPIO_BASE(GPIO_PORTG) + GPIOx_AFRL_OFFSET, AFRLy_3_BIT(14), AFRLy_0_BIT(14), 11);

	SET_BIT(RCC_BASE + RCC_AHB1ENR_OFFSET, ETHMACRXEN);
	SET_BIT(RCC_BASE + RCC_AHB1ENR_OFFSET, ETHMACTXEN);
	SET_BIT(RCC_BASE + RCC_AHB1ENR_OFFSET, ETHMACEN);

	SET_BIT(RCC_BASE + RCC_AHB1RSTR_OFFSET, ETHMACRST);
	CLEAR_BIT(RCC_BASE + RCC_AHB1RSTR_OFFSET, ETHMACRST);

	SET_BIT(ETHERNET_MAC_BASE + ETH_DMABMR_OFFSET, SR);

	while(READ_BIT(ETHERNET_MAC_BASE + ETH_DMABMR_OFFSET, SR) != 0);

}

void init_dac(void)
{
	SET_BIT(RCC_BASE + RCC_AHB1ENR_OFFSET, GPIO_EN_BIT(GPIO_PORTA));
	SET_BIT(RCC_BASE + RCC_APB1ENR_OFFSET, DACEN);

	SET_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_MODER_OFFSET, MODERy_1_BIT(4));
	SET_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_MODER_OFFSET, MODERy_0_BIT(4));

	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(4));
	SET_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(4));

	CLEAR_BIT(DAC_BASE + DAC_CR_OFFSET, DAC_CR_DMAUDRIE1);
	CLEAR_BIT(DAC_BASE + DAC_CR_OFFSET, DAC_CR_DMAEN1);
	WRITE_BITS(DAC_BASE + DAC_CR_OFFSET, DAC_CR_MAMP1_3_BIT, DAC_CR_MAMP1_0_BIT, 0);
	WRITE_BITS(DAC_BASE + DAC_CR_OFFSET, DAC_CR_WAVE1_1_BIT, DAC_CR_WAVE1_0_BIT, 0);
	WRITE_BITS(DAC_BASE + DAC_CR_OFFSET, DAC_CR_TSEL1_2_BIT, DAC_CR_TSEL1_0_BIT, 7);
	CLEAR_BIT(DAC_BASE + DAC_CR_OFFSET, DAC_CR_TEN1);
	SET_BIT(DAC_BASE + DAC_CR_OFFSET, DAC_CR_BOFF1);
	SET_BIT(DAC_BASE + DAC_CR_OFFSET, DAC_CR_EN1);
	DAC_SetChannel1Data(0);
}

void init_timer4(void)
{
	SET_BIT(RCC_BASE + RCC_APB1ENR_OFFSET, TIM4EN);
	WRITE_BITS(TIM4_BASE + TIMx_CR1_OFFSET, TIMx_CKD_1_BIT, TIMx_CKD_0_BIT, 0b00);
	SET_BIT(TIM4_BASE + TIMx_CR1_OFFSET, TIMx_ARPE);
	WRITE_BITS(TIM4_BASE + TIMx_CR1_OFFSET, TIMx_CMS_1_BIT, TIMx_CMS_0_BIT, 0b00);
	CLEAR_BIT(TIM4_BASE + TIMx_CR1_OFFSET, TIMx_DIR);
	CLEAR_BIT(TIM4_BASE + TIMx_CR1_OFFSET, TIMx_OPM);
	CLEAR_BIT(TIM4_BASE + TIMx_CR1_OFFSET, TIMx_URS);
	CLEAR_BIT(TIM4_BASE + TIMx_CR1_OFFSET, TIMx_UDIS);
	WRITE_BITS(TIM4_BASE + TIMx_PSC_OFFSET, TIMx_PSC_15_BIT, TIMx_PSC_0_BIT, 3);
	WRITE_BITS(TIM4_BASE + TIMx_ARR_OFFSET, TIMx_ARR_15_BIT, TIMx_ARR_0_BIT, 952);
	SET_BIT(TIM4_BASE + TIMx_CR1_OFFSET, TIMx_CEN);
}

void DAC_SetChannel1Data(uint8_t vol)
{
	WRITE_BITS( DAC_BASE + DAC_DHR8R1_OFFSET, DAC_DHR8R1_DACC1DHR_7_BIT, DAC_DHR8R1_DACC1DHR_0_BIT, vol);
}

void init_usart2(void)
{
	//RCC EN GPIO
	SET_BIT(RCC_BASE + RCC_AHB1ENR_OFFSET, GPIO_EN_BIT(GPIO_PORTA));

	//MODER => 10
	SET_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_MODER_OFFSET, MODERy_1_BIT(2));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_MODER_OFFSET, MODERy_0_BIT(2));

	SET_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_MODER_OFFSET, MODERy_1_BIT(3));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_MODER_OFFSET, MODERy_0_BIT(3));

	//OT => 0
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_OTYPER_OFFSET, OTy_BIT(2));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_OTYPER_OFFSET, OTy_BIT(3));

	//OSPEEDR => 10
	SET_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(2));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(2));

	SET_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(3));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(3));

	//PUPDR = 00 => No pull-up, pull-down
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(2));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(2));

	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(3));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(3));

	//AF sel
	WRITE_BITS(GPIO_BASE(GPIO_PORTA) + GPIOx_AFRL_OFFSET, AFRLy_3_BIT(2), AFRLy_0_BIT(2), 7);
	WRITE_BITS(GPIO_BASE(GPIO_PORTA) + GPIOx_AFRL_OFFSET, AFRLy_3_BIT(3), AFRLy_0_BIT(3), 7);

	//RCC EN USART2
	SET_BIT(RCC_BASE + RCC_APB1ENR_OFFSET, USART2EN);

	//baud rate
	const unsigned int BAUDRATE = 115200;
	//const unsigned int BAUDRATE = 529200;
	//const unsigned int BAUDRATE = 115200;
	const unsigned int SYSCLK_MHZ = 168;
	const double USARTDIV = SYSCLK_MHZ * 1.0e6 / 8 / 2 / BAUDRATE;

	const uint32_t DIV_MANTISSA = (uint32_t)USARTDIV;
	const uint32_t DIV_FRACTION = (uint32_t)((USARTDIV - DIV_MANTISSA) * 16);

	WRITE_BITS(USART2_BASE + USART_BRR_OFFSET, DIV_MANTISSA_11_BIT, DIV_MANTISSA_0_BIT, DIV_MANTISSA);
	WRITE_BITS(USART2_BASE + USART_BRR_OFFSET, DIV_FRACTION_3_BIT, DIV_FRACTION_0_BIT, DIV_FRACTION);

	//usart2 enable
	SET_BIT(USART2_BASE + USART_CR1_OFFSET, UE_BIT);

	//set TE
	SET_BIT(USART2_BASE + USART_CR1_OFFSET, TE_BIT);

	//set RE
	SET_BIT(USART2_BASE + USART_CR1_OFFSET, RE_BIT);

	//set RXNEIE
	SET_BIT(USART2_BASE + USART_CR1_OFFSET, RXNEIE_BIT);

	//set NVIC
	//SET_BIT(NVIC_ISER_BASE + NVIC_ISERn_OFFSET(1), 6); //IRQ37 => (m+(32*n)) | m=5, n=1
}

void init_usart6(void)
{
	//RCC EN GPIO
	SET_BIT(RCC_BASE + RCC_AHB1ENR_OFFSET, GPIO_EN_BIT(GPIO_PORTC));

	//MODER => 10
	SET_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_MODER_OFFSET, MODERy_1_BIT(6));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_MODER_OFFSET, MODERy_0_BIT(6));

	SET_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_MODER_OFFSET, MODERy_1_BIT(7));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_MODER_OFFSET, MODERy_0_BIT(7));

	//OT => 0
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_OTYPER_OFFSET, OTy_BIT(6));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_OTYPER_OFFSET, OTy_BIT(7));

	//OSPEEDR => 10
	SET_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(6));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(6));

	SET_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(7));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(7));

	//PUPDR = 00 => No pull-up, pull-down
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(6));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(6));

	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(7));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(7));

	//AF sel
	WRITE_BITS(GPIO_BASE(GPIO_PORTC) + GPIOx_AFRL_OFFSET, AFRLy_3_BIT(6), AFRLy_0_BIT(6), 8);
	WRITE_BITS(GPIO_BASE(GPIO_PORTC) + GPIOx_AFRL_OFFSET, AFRLy_3_BIT(7), AFRLy_0_BIT(7), 8);

	//RCC EN USART2
	SET_BIT(RCC_BASE + RCC_APB2ENR_OFFSET, USART6EN);

	//baud rate
	const unsigned int BAUDRATE = 115200;
	const unsigned int SYSCLK_MHZ = 168;
	const double USARTDIV = SYSCLK_MHZ * 1.0e6 / 8 / 2 / BAUDRATE;

	const uint32_t DIV_MANTISSA = (uint32_t)USARTDIV;
	const uint32_t DIV_FRACTION = (uint32_t)((USARTDIV - DIV_MANTISSA) * 16);

	WRITE_BITS(USART6_BASE + USART_BRR_OFFSET, DIV_MANTISSA_11_BIT, DIV_MANTISSA_0_BIT, DIV_MANTISSA);
	WRITE_BITS(USART6_BASE + USART_BRR_OFFSET, DIV_FRACTION_3_BIT, DIV_FRACTION_0_BIT, DIV_FRACTION);

	//usart2 enable
	SET_BIT(USART6_BASE + USART_CR1_OFFSET, UE_BIT);

	//set TE
	SET_BIT(USART6_BASE + USART_CR1_OFFSET, TE_BIT);

	//set RE
	SET_BIT(USART6_BASE + USART_CR1_OFFSET, RE_BIT);

	//set RXNEIE
	SET_BIT(USART6_BASE + USART_CR1_OFFSET, RXNEIE_BIT);

	//set NVIC
	SET_BIT(NVIC_ISER_BASE + NVIC_ISERn_OFFSET(2), 7); //IRQ71 => (m+(32*n)) | m=7, n=2
}

void usart2_send_char(const char ch)
{
	//wait util TXE == 1
	while (!READ_BIT(USART2_BASE + USART_SR_OFFSET, TXE_BIT))
		;
	REG(USART2_BASE + USART_DR_OFFSET) = (uint8_t)ch;
}

void usart2_handler(void)
{

	if (READ_BIT(USART2_BASE + USART_SR_OFFSET, ORE_BIT))
	{
		/*
		char ch = (char)REG(USART2_BASE + USART_DR_OFFSET);

		for (unsigned int i = 0; i < 5000000; i++)
			;

		if (ch == '\r')
			usart2_send_char('\n');

		usart2_send_char(ch);
		usart2_send_char('~');
		*/
		uint8_t tmp = (uint8_t)REG(USART2_BASE + USART_DR_OFFSET);
		usart2_send_char(receive_count);
		receive_count = 0;
	}

	else if (READ_BIT(USART2_BASE + USART_SR_OFFSET, RXNE_BIT))
	{
		/*
		char ch = (char)REG(USART2_BASE + USART_DR_OFFSET);
		if (ch == '\r')
			usart2_send_char('\n');
		usart2_send_char(ch);
		*/
		*(rece_ptr+receive_count) = (uint8_t)REG(USART2_BASE + USART_DR_OFFSET);
		receive_count++;
		if (receive_count>= 200){
			if(rece_ptr==rec_play_buf_fir){
				rece_ptr = rec_play_buf_sec;
				play_ptr = rec_play_buf_fir;
			}
			else if(rece_ptr==rec_play_buf_sec){
				rece_ptr = rec_play_buf_fir;
				play_ptr = rec_play_buf_sec;
			}
			usart2_send_char(receive_count);
			receive_count = 0;
		}
	}
}

void *_sbrk(int incr)
{
	extern uint8_t _mybss_vma_end; //Defined by the linker script
	static uint8_t *heap_end = NULL;
	uint8_t *prev_heap_end;

	if (heap_end == NULL)
		heap_end = &_mybss_vma_end;

	prev_heap_end = heap_end;
	if (heap_end + incr > &_mybss_vma_end + HEAP_MAX)
		return (void *)-1;

	heap_end += incr;
	return (void *)prev_heap_end;
}

int _write(int file, char *ptr, int len)
{
	//sys_call_write(ptr, len);

	for (unsigned int i = 0; i < len; i++)
		usart2_send_char(*ptr++);

	return len;
}

int _close(int file)
{
	return -1;
}

int _lseek(int file, int ptr, int dir)
{
	return 0;
}

int _read(int file, char *ptr, int len)
{
	return 0;
}

int _fstat(int file, struct stat *st)
{
	st->st_mode = S_IFCHR;
	return 0;
}

int _isatty(int file)
{
	return 1;
}
