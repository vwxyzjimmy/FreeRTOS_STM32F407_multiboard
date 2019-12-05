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
#include "struct.h"
#define HEAP_MAX (128 * 1024) //64 KB

void vApplicationTickHook() {;}
void vApplicationStackOverflowHook() {;}
void vApplicationIdleHook() {;}
void vApplicationMallocFailedHook() {;}

void setup_mpu(void);
void init_dac(void);
void DAC_SetChannel1Data(uint8_t vol);
void init_usart2(void);
void usart2_send_char(const char ch);
void init_usart1(void);
void usart1_send_char(const char ch);
void print_sys(char str[255]);
void test_add_all(void);
Distributed_TaskHandle_List_t* distributed_manager_task(uint32_t Data_addr, uint32_t Data_size, uint32_t sp, uint32_t lr);
uint32_t got_sp_minus_immediate(uint32_t addr);
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
extern BlockLink_t xStart;
Distributed_TaskHandle_List_t* DStart;

uint32_t got_sp_minus_immediate(uint32_t addr){
	uint32_t sp_T1_bit_mask = 0xB080;
	uint32_t sp_T2_bit_mask = 0xF1AD0D00;
	uint32_t sp_T3_bit_mask = 0xF2AD0D00;
	uint32_t immediate = 0;

	uint32_t tmp = *((uint32_t *)addr);
	uint32_t tmp_high = tmp >> 16;
	uint32_t tmp_low = tmp << 16;
	tmp = tmp_high|tmp_low;
	if ((tmp & 0xB0FFFFFF)==tmp){
		tmp = (tmp>>16);
		if ((tmp &sp_T1_bit_mask)==sp_T1_bit_mask){
			immediate = (tmp & 0x7F)<<2;
		}
	}
	else {
		if ((tmp &sp_T2_bit_mask)==sp_T2_bit_mask){
			uint32_t tmp_immediate = ((tmp & 0x4000000)>>15)|((tmp & 0x7000)>>4)|(tmp & 0xFF);
			immediate = ((0x80|(tmp_immediate&0x7F))>>(tmp_immediate>>7))|((0x80|(tmp_immediate&0x7F))<<(32-(tmp_immediate>>7)));
		}
		else if ((tmp &sp_T3_bit_mask)==sp_T3_bit_mask){
			immediate = ((tmp & 0x4000000)>>15)|((tmp & 0x7000)>>4)|(tmp & 0xFF);
		}
	}
	return immediate;
}

Distributed_TaskHandle_List_t* distributed_manager_task(uint32_t Data_addr, uint32_t Data_size, uint32_t sp, uint32_t lr){
	uint32_t slit_num = 3;
	static uint32_t *sp_start = 0;
	static uint32_t *sp_end = 0;
	static uint32_t *pc_start = 0;
	static uint32_t *pc_end = 0;
	static uint32_t *lr_addr = 0;
	uint32_t stack_size = 0;
	lr_addr = (uint32_t *)((lr & 0xFFFFFFFE)-4);

	uint32_t tmp_lr = lr & 0xFFFFFFFE;
	tmp_lr = ((uint16_t *)tmp_lr-3);
	while(*((uint16_t *)tmp_lr)!=0xb580){
		stack_size = stack_size + got_sp_minus_immediate(tmp_lr);
		tmp_lr = ((uint16_t *)tmp_lr-1);
	}
	sp_end = (uint32_t*)sp;
	sp_start =(uint32_t*)(sp_end+stack_size);
	pc_start = (((uint32_t*)tmp_lr));
	tmp_lr = lr & 0xFFFFFFFE;
	while(*((uint16_t *)tmp_lr)!=0xb580){
		tmp_lr = ((uint16_t *)tmp_lr+1);
	}
	pc_end = ((uint16_t *)tmp_lr);
	uint32_t instruction_size = ((uint32_t)pc_end-(uint32_t)pc_start);
	Distributed_TaskHandle_List_t *Subscriber_task;

	for(uint32_t slit_num_th=0;slit_num_th<slit_num;slit_num_th++){
		uint32_t Data_size_split = ((Data_size/(slit_num))+1);
		if (slit_num_th == (slit_num-1))
			Data_size_split = Data_size%Data_size_split;
		printf("slit_num_th:	0x%X, Data_size_split:	0x%X\r\n", slit_num_th, Data_size_split);
		if (slit_num_th == 0){
			Distributed_TaskHandle_List_t *NewDTaskControlBlock = pvPortMalloc(sizeof(Distributed_TaskHandle_List_t));
			NewDTaskControlBlock->Processor_id = 0;
		    NewDTaskControlBlock->DTask_id = 0;
			NewDTaskControlBlock->DSubTask_id = slit_num_th;
			NewDTaskControlBlock->Instruction_addr = pc_start;
			NewDTaskControlBlock->Instruction_addr_end = pc_end;
			NewDTaskControlBlock->Data_addr = Data_addr;
			NewDTaskControlBlock->Data_size = Data_size_split;
			NewDTaskControlBlock->Finish_Flag = 0;
			Distributed_TaskHandle_List_t* Lastnode = &DStart;
			while(Lastnode->Next_TaskHandle_List != NULL)
				Lastnode = Lastnode->Next_TaskHandle_List;
			Lastnode->Next_TaskHandle_List = NewDTaskControlBlock;
			Subscriber_task = NewDTaskControlBlock;
		}
		else{
			uint32_t Data_size_split = ((Data_size/(slit_num))+1);
			if (slit_num_th == (slit_num-1))
				Data_size_split = Data_size%Data_size_split;
			uint32_t total_size = instruction_size + Data_size_split;
			uint32_t malloc_size = 4*(total_size);
			uint32_t *instruction = pvPortMalloc(4*malloc_size);
			for(uint32_t i=0;i<(instruction_size)/2;i++){
				if((lr_addr<((uint16_t*)pc_start+i)) && ((lr_addr+1)>((uint16_t*)pc_start+i))){
					*((uint16_t*)instruction+i) = 0xbf00;
				}
				else if (lr_addr == ((uint16_t*)pc_start+i)){
					*((uint16_t*)instruction+i) = 0xdf01;
				}
				else
					*((uint16_t*)instruction+i) = *((uint16_t*)pc_start+i);
			}
			for(uint32_t i=0;i<Data_size_split;i++){
				*(instruction+(instruction_size/4)+i) = *((uint32_t *)(Data_addr)+(((Data_size/(slit_num))+1)*slit_num_th)+i);
			}

			Distributed_TaskHandle_List_t *NewDTaskControlBlock = pvPortMalloc(sizeof(Distributed_TaskHandle_List_t));
			NewDTaskControlBlock->Next_TaskHandle_List = NULL;
			NewDTaskControlBlock->Processor_id = 0;
		    NewDTaskControlBlock->DTask_id = 0;
			NewDTaskControlBlock->DSubTask_id = slit_num_th;
			NewDTaskControlBlock->Instruction_addr = instruction;
			NewDTaskControlBlock->Instruction_addr_end = instruction + (instruction_size/4);
			NewDTaskControlBlock->Data_addr = (instruction+(instruction_size/4));
			NewDTaskControlBlock->Data_size = Data_size_split;
			NewDTaskControlBlock->Finish_Flag = 0;
			NewDTaskControlBlock->TaskHandlex = pvPortMalloc(sizeof(TaskHandle_t));
			Distributed_TaskHandle_List_t* Lastnode = &DStart;
			while(Lastnode->Next_TaskHandle_List != NULL)
				Lastnode = Lastnode->Next_TaskHandle_List;
			Lastnode->Next_TaskHandle_List = NewDTaskControlBlock;
			void (*func_ptr)() = ((uint32_t)instruction)+1;
			printf("Generate task%d\r\n", slit_num_th);
			xTaskCreate((uint16_t*)func_ptr, "task", (stack_size*4), NULL, 1, NewDTaskControlBlock->TaskHandlex);
		}
	}
	return Subscriber_task;
}

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
	while(1){
		if ((READ_BIT(USART1_BASE + USART_SR_OFFSET, RXNE_BIT)) || (READ_BIT(USART1_BASE + USART_SR_OFFSET, ORE_BIT))){
			char rec_cmd = (char)REG(USART1_BASE + USART_DR_OFFSET);
			printf("%c\r\n", rec_cmd);
			if (rec_cmd == 'a'){
				xTaskCreate(task2, "task2", 1000, NULL, 1, &TaskHandle_2);
			}
			else if (rec_cmd == 'b'){
				vTaskDelete(TaskHandle_2);
				printf("kill task2\r\n");
				SET_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_BSRR_OFFSET, BRy_BIT(LED_BLUE));
			}
		}
	}
}

void task2(){
	uint32_t *Data_start_addr = 0x10000010;
	uint32_t Data_start_size = 16;
	Distributed_TaskHandle_List_t *s = Distributed_Start(Data_start_addr, Data_start_size);
	for(uint32_t i=0;i<s->Data_size;i++){
		*(s->Data_addr + i) = i;
	}
	/*
	while(1){
		for(uint32_t i=0;i<500000;i++){
			;
		}
		SET_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_BSRR_OFFSET, BSy_BIT(LED_ORANGE));
		for(uint32_t i=0;i<500000;i++){
			;
		}
		SET_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_BSRR_OFFSET, BRy_BIT(LED_ORANGE));
	}
	*/
	Distributed_End();
}

void task3(){
	init_dac();
	init_timer4();
	led_init(LED_BLUE);
	printf("task3\r\n");
	unsigned int play_count = 0;
	unsigned int led_state = 0;
	rece_ptr = rec_play_buf_fir;
	SET_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_BSRR_OFFSET, BSy_BIT(LED_BLUE));
	while(1);
	while(1){
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
	DStart->Next_TaskHandle_List = NULL;
	init_usart1();
	led_init(LED_GREEN);
	led_init(LED_ORANGE);
	void (*func_ptr)() = task1;
	REG(AIRCR_BASE) = NVIC_AIRCR_RESET_VALUE | NVIC_PRIORITYGROUP_4;
	xTaskCreate(func_ptr, "task1", 1000, NULL, 1, &TaskHandle_1);
	SET_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_BSRR_OFFSET, BSy_BIT(LED_GREEN));
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

	//uint32_t stacked_r0 = *(stack_frame_ptr);
	//uint32_t stacked_r1 = *(stack_frame_ptr+1);
	uint32_t stacked_return_addr = *(stack_frame_ptr+6);

	uint16_t svc_instruction = *((uint16_t *)stacked_return_addr - 1);
	uint8_t svc_num = (uint8_t)svc_instruction;
	/*
	printf("[SVC Handler] Stacked R0: 0x%X \r\n", (unsigned int)stacked_r0);
	printf("[SVC Handler] Stacked R1: 0x%X \r\n", (unsigned int)stacked_r1);
	printf("[SVC Handler] SVC number: 0x%X \r\n\n", (unsigned int)svc_num);
	*/
	if(svc_num == 0)
		vPortSVCHandler();
	else if (svc_num == 1){
		Distributed_TaskHandle_List_t* Lastnode = &DStart;
		while(Lastnode->Next_TaskHandle_List != NULL){
			Lastnode = Lastnode->Next_TaskHandle_List;
			if ((Lastnode->Instruction_addr<=stacked_return_addr) && (stacked_return_addr<=Lastnode->Instruction_addr_end)){
				break;
			}
		}
		 *(stack_frame_ptr) = Lastnode;
	}
	else if (svc_num == 2){
		Distributed_TaskHandle_List_t* Lastnode = &DStart;
		while(Lastnode->Next_TaskHandle_List != NULL){
			Lastnode = Lastnode->Next_TaskHandle_List;
			if ((Lastnode->Instruction_addr<=stacked_return_addr) && (stacked_return_addr<=Lastnode->Instruction_addr_end)){
				printf("\r\nSVC 2, DSubTask_id:0x%X	done\r\n\r\nInstruction_addr    :0x%X\r\nstacked_return_addr  :0x%X\r\nInstruction_addr_end:0x%X\r\n\r\n", Lastnode->DSubTask_id, Lastnode->Instruction_addr, stacked_return_addr, Lastnode->Instruction_addr_end);

				for(uint32_t i=0;i<Lastnode->Data_size;i++){
					printf(" DSubTask_id:0x%X	0x%X	0x%X\r\n", Lastnode->DSubTask_id, (Lastnode->Data_addr+i), *(Lastnode->Data_addr+i));
				}

				break;
			}
		}
	}
		//return 0x10000000;
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

void init_usart1(void)
{
	//RCC EN GPIO
	SET_BIT(RCC_BASE + RCC_AHB1ENR_OFFSET, GPIO_EN_BIT(GPIO_PORTB));

	//MODER => 10
	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_MODER_OFFSET, MODERy_1_BIT(6));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_MODER_OFFSET, MODERy_0_BIT(6));

	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_MODER_OFFSET, MODERy_1_BIT(7));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_MODER_OFFSET, MODERy_0_BIT(7));

	//OT => 0
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OTYPER_OFFSET, OTy_BIT(6));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OTYPER_OFFSET, OTy_BIT(7));

	//OSPEEDR => 10
	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(6));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(6));

	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(7));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(7));

	//PUPDR = 00 => No pull-up, pull-down
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(6));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(6));

	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(7));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(7));

	//AF sel
	WRITE_BITS(GPIO_BASE(GPIO_PORTB) + GPIOx_AFRL_OFFSET, AFRLy_3_BIT(6), AFRLy_0_BIT(6), 7);
	WRITE_BITS(GPIO_BASE(GPIO_PORTB) + GPIOx_AFRL_OFFSET, AFRLy_3_BIT(7), AFRLy_0_BIT(7), 7);

	//RCC EN USART2
	SET_BIT(RCC_BASE + RCC_APB2ENR_OFFSET, USART1EN);

	//baud rate
	const unsigned int BAUDRATE = 115200;
	const unsigned int SYSCLK_MHZ = 168;
	const double USARTDIV = SYSCLK_MHZ * 1.0e6 / 8 / 2 / BAUDRATE;

	const uint32_t DIV_MANTISSA = (uint32_t)USARTDIV;
	const uint32_t DIV_FRACTION = (uint32_t)((USARTDIV - DIV_MANTISSA) * 16);

	WRITE_BITS(USART1_BASE + USART_BRR_OFFSET, DIV_MANTISSA_11_BIT, DIV_MANTISSA_0_BIT, DIV_MANTISSA);
	WRITE_BITS(USART1_BASE + USART_BRR_OFFSET, DIV_FRACTION_3_BIT, DIV_FRACTION_0_BIT, DIV_FRACTION);

	//usart2 enable
	SET_BIT(USART1_BASE + USART_CR1_OFFSET, UE_BIT);

	//set TE
	SET_BIT(USART1_BASE + USART_CR1_OFFSET, TE_BIT);

	//set RE
	SET_BIT(USART1_BASE + USART_CR1_OFFSET, RE_BIT);

	//set RXNEIE
	SET_BIT(USART1_BASE + USART_CR1_OFFSET, RXNEIE_BIT);

	//set NVIC
	//SET_BIT(NVIC_ISER_BASE + NVIC_ISERn_OFFSET(1), 5); //IRQ71 => (m+(32*n)) | m=7, n=2
}

void usart1_send_char(const char ch)
{
	//wait util TXE == 1
	while (!READ_BIT(USART1_BASE + USART_SR_OFFSET, TXE_BIT))
		;
	REG(USART1_BASE + USART_DR_OFFSET) = (uint8_t)ch;
}

void usart1_handler(void)
{

	if (READ_BIT(USART1_BASE + USART_SR_OFFSET, ORE_BIT))
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
		uint8_t tmp = (uint8_t)REG(USART1_BASE + USART_DR_OFFSET);
		usart2_send_char(receive_count);
		receive_count = 0;
	}

	else if (READ_BIT(USART1_BASE + USART_SR_OFFSET, RXNE_BIT))
	{
		/*
		char ch = (char)REG(USART2_BASE + USART_DR_OFFSET);
		if (ch == '\r')
			usart2_send_char('\n');
		usart2_send_char(ch);
		*/
		*(rece_ptr+receive_count) = (uint8_t)REG(USART1_BASE + USART_DR_OFFSET);
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
			usart1_send_char(receive_count);
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

	for (unsigned int i = 0; i < len; i++)
		usart1_send_char(*ptr++);

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
