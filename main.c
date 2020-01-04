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
void task1();
void Distributed_task();
void task3();
Distributed_TaskHandle_List_t* Distributed_manager_task(void* S, uint32_t sp, uint32_t lr);
uint32_t Got_sp_minus_immediate(uint32_t addr);
void Distributed_Check(Distributed_TaskHandle_List_t* s, uint32_t* Result_Data_addr, uint32_t Result_Data_size);
void Distributed_TaskCreate(void* task, Distributed_Data_t *s, uint32_t Stack_size);
Distributed_TaskHandle_List_t* Distributed_GetNode(uint32_t Return_addr);
Distributed_Data_t* Distributed_Set_Traget_Data(uint32_t* addr, uint32_t size);
void Distributed_Add_Traget_Data(Distributed_Data_t* S, uint32_t* addr, uint32_t size);

void List_FreeBlock();

volatile uint8_t rec_play_buf_fir[200], rec_play_buf_sec[200];
volatile uint8_t *rece_ptr;
volatile uint8_t *play_ptr;
volatile uint8_t receive_count = 0;
volatile TaskHandle_t TaskHandle_1;
volatile TaskHandle_t TaskHandle_2;
volatile TaskHandle_t TaskHandle_3;
extern BlockLink_t xStart;
Distributed_TaskHandle_List_t* DStart;
uint32_t Global_Task_id = 0;

Distributed_Data_t* Distributed_Set_Traget_Data(uint32_t* addr, uint32_t size){
	Distributed_Data_t* s = pvPortMalloc(sizeof(Distributed_Data_t));
	s->Data_addr = addr;
	s->Data_size = size;
	s->Next_Distributed_Data = NULL;
	return s;
}

void Distributed_Add_Traget_Data(Distributed_Data_t* S, uint32_t* addr, uint32_t size){
	Distributed_Data_t* tmp_Distributed_Data_t = S;
	while(tmp_Distributed_Data_t->Next_Distributed_Data != NULL)
		tmp_Distributed_Data_t = tmp_Distributed_Data_t->Next_Distributed_Data;
	Distributed_Data_t* s = pvPortMalloc(sizeof(Distributed_Data_t));
	tmp_Distributed_Data_t->Next_Distributed_Data = s;
	s->Data_addr = addr;
	s->Data_size = size;
	s->Next_Distributed_Data = NULL;
}

Distributed_TaskHandle_List_t* Distributed_GetNode(uint32_t Return_addr){
	Distributed_TaskHandle_List_t* Lastnode = &DStart;
	while(Lastnode->Next_TaskHandle_List != NULL){
		Lastnode = Lastnode->Next_TaskHandle_List;
		if ((Lastnode->Instruction_addr<=Return_addr) && (Return_addr<=Lastnode->Instruction_addr_end)){
			break;
		}
	}
	return Lastnode;
}

void List_FreeBlock(){
	BlockLink_t* tmp_block = &xStart;
	printf("------------------------------------------------------------\r\n");
	while((tmp_block->pxNextFreeBlock)!=NULL){
		tmp_block = tmp_block->pxNextFreeBlock;
		printf("Free  xBlockAddr	0x%X, xBlockSize:	0x%X\r\n", tmp_block, tmp_block->xBlockSize);
	}
	printf("------------------------------------------------------------\r\n");
}

void Distributed_TaskCreate(void* task, Distributed_Data_t *S, uint32_t Stack_size){
	QueueHandle_t xQueue = xQueueCreate(1, sizeof(uint32_t*));
	S->xQueue = xQueue;
	TaskHandle_t TaskHandle;
	xTaskCreate(task, "Dtask", Stack_size, S, 1, &TaskHandle);
	while(xQueueReceive(xQueue, S, 0) == 0);
	vQueueDelete(xQueue);
	vPortFree(S->Data_addr);
	vTaskDelete(TaskHandle);
	vPortFree(S);
}

void Distributed_Check(Distributed_TaskHandle_List_t* s, uint32_t* Result_Data_addr, uint32_t Result_Data_size){

	Distributed_TaskHandle_List_t* Lastnode = DStart;
	Distributed_TaskHandle_List_t* pre_Lastnode = DStart;
	uint32_t All_Subtask_Done = 0;
	uint32_t Total_Data_Size = 0;
	uint32_t Every_Data_Size[s->Data_number];
	while(All_Subtask_Done == 0){
		All_Subtask_Done = 1;
		Lastnode = DStart;
		Total_Data_Size = 0;
		for(uint32_t i=0;i<s->Data_number;i++)
			Every_Data_Size[i] = 0;
		while(Lastnode != NULL){
			if((Lastnode->DTask_id) == s->DTask_id){
				if((Lastnode->Finish_Flag) == 0){
					All_Subtask_Done = 0;
					break;
				}
				else{
					for(uint32_t i=0;i<Lastnode->Data_number;i++){
						Total_Data_Size += *(Lastnode->Data_size+i);
						Every_Data_Size[i] = Every_Data_Size[i] + *(Lastnode->Data_size+i);
					}
				}
			}
			Lastnode = Lastnode->Next_TaskHandle_List;
		}
	}

	uint32_t* Data_addr = pvPortMalloc(Total_Data_Size*sizeof(Total_Data_Size));
	Lastnode = DStart;
	while(Lastnode != NULL){
		if(((Lastnode->DTask_id) == s->DTask_id)&&((Lastnode->Processor_id) == s->Processor_id)){
			for(uint32_t i=0;i<Lastnode->Data_number;i++){
				uint32_t* tmp_Data_addr = Data_addr;
				uint32_t* From_Data_addr = Lastnode->Data_addr;
				for(uint32_t j=0;j<i;j++){
					tmp_Data_addr += Every_Data_Size[j];
					From_Data_addr += *(Lastnode->Data_size+j);
				}
				for(uint32_t j=0;j< *(Lastnode->Data_size+i);j++){
					*(tmp_Data_addr + (Lastnode->DSubTask_id)*(*(Lastnode->Data_Max_size+i)) + j) = *(From_Data_addr+j);
				}
			}

			if ((Lastnode->DSubTask_id) != 0){
				vTaskDelete(*(Lastnode->TaskHandlex));
				vPortFree(Lastnode->TaskHandlex);
				vPortFree(Lastnode->Instruction_addr);
			}
			else{
				vPortFree(Lastnode->Data_addr);
			}
			Distributed_Data_t* tmp_array = Lastnode->Distributed_Data_List;
			for(uint32_t i=0;i<Lastnode->Data_number;i++){
				Distributed_Data_t* Delete_tmp_array = tmp_array;
				tmp_array = tmp_array->Next_Distributed_Data;
				vPortFree(Delete_tmp_array);
			}
			vPortFree(Lastnode->Data_size);
			vPortFree(Lastnode->Data_Max_size);
			Distributed_TaskHandle_List_t* Delete_node;
			if((pre_Lastnode == DStart) && (Lastnode == DStart)){
				Delete_node = Lastnode;
				if (Lastnode->Next_TaskHandle_List != NULL)
					DStart = Lastnode->Next_TaskHandle_List;
				else
					DStart = NULL;
				pre_Lastnode = DStart;
				Lastnode = pre_Lastnode;
			}
			else{
				Delete_node = Lastnode;
				pre_Lastnode->Next_TaskHandle_List = Lastnode->Next_TaskHandle_List;
				Lastnode = Lastnode->Next_TaskHandle_List;
			}
			vPortFree(Delete_node);
		}
		else{
			pre_Lastnode = Lastnode;
			Lastnode = Lastnode->Next_TaskHandle_List;
		}
	}

	printf("Result:\r\n");
	for(uint32_t i=0;i<Total_Data_Size;i++)
		printf("0x%X	0x%X\r\n", (Data_addr+i), *(Data_addr+i));

	Distributed_Data_t* Send_S = pvPortMalloc(sizeof(Distributed_Data_t));
	Send_S->Data_addr = Data_addr;
	Send_S->Data_size = Total_Data_Size;
	Send_S->xQueue = s->xQueue;
	xQueueSendToBack(Send_S->xQueue, Send_S, 0);
	while(1);
}

uint32_t Got_sp_minus_immediate(uint32_t addr){

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

Distributed_TaskHandle_List_t* Distributed_manager_task(void* S, uint32_t sp, uint32_t lr){
	Global_Task_id++;
	uint32_t Data_number = 1;
	Distributed_Data_t* tmp_Distributed_Data = ((Distributed_Data_t*)S);
	while(tmp_Distributed_Data->Next_Distributed_Data != NULL){
		tmp_Distributed_Data = tmp_Distributed_Data->Next_Distributed_Data;
		Data_number++;
	}

	uint32_t Data_addr_array[Data_number];
	uint32_t Data_size_array[Data_number];

	tmp_Distributed_Data = ((Distributed_Data_t*)S);
	for(uint32_t i=0;i<Data_number;i++){
		Data_addr_array[i] = tmp_Distributed_Data->Data_addr;
		Data_size_array[i] = tmp_Distributed_Data->Data_size;
		tmp_Distributed_Data = tmp_Distributed_Data->Next_Distributed_Data;
	}

	uint32_t slit_num = 4;
	static uint32_t *sp_start = 0;
	static uint32_t *sp_end = 0;
	static uint32_t *pc_start = 0;
	static uint32_t *pc_end = 0;
	static uint32_t *lr_addr = 0;

	uint32_t stack_size = 0;
	lr_addr = (uint32_t *)((lr & 0xFFFFFFFE)-4);

	uint32_t tmp_lr = lr & 0xFFFFFFFE;
	while(*((uint16_t *)tmp_lr)!=0xb580){
		stack_size = stack_size + Got_sp_minus_immediate(tmp_lr);
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
		uint32_t Data_size_split = 0;
		uint32_t* Data_Max_size_split_record = pvPortMalloc(Data_number*sizeof(uint32_t));
		uint32_t* Data_size_split_record = pvPortMalloc(Data_number*sizeof(uint32_t));

		for(uint32_t Data_number_th=0;Data_number_th<Data_number;Data_number_th++){
			uint32_t tmp_data_size = 0;
			if ((Data_size_array[Data_number_th]%slit_num)==0){
				tmp_data_size = Data_size_array[Data_number_th]/slit_num;
				*(Data_Max_size_split_record+Data_number_th)= tmp_data_size;
			}
			else{
				tmp_data_size = (Data_size_array[Data_number_th]/slit_num) + 1;
				*(Data_Max_size_split_record+Data_number_th) = tmp_data_size;
				if (slit_num_th == (slit_num-1))
					tmp_data_size = Data_size_array[Data_number_th]%tmp_data_size;
			}
			*(Data_size_split_record+Data_number_th) = tmp_data_size;
			Data_size_split = Data_size_split + tmp_data_size;
		}

		if (slit_num_th == 0){
			uint32_t malloc_size = 4*Data_size_split;
			uint32_t* Data_addr = pvPortMalloc(malloc_size);
			Distributed_Data_t* tmp_Distributed_Data = ((Distributed_Data_t*)S);
			uint32_t* tmp_Data_addr = Data_addr;

			Distributed_Data_t* Start_Distributed_Data_List;
			Distributed_Data_t* Stop_Distributed_Data_List;

			for(uint32_t Data_number_i=0;Data_number_i<Data_number;Data_number_i++){
				for(uint32_t i=0;i<Data_size_split_record[Data_number_i];i++){
					*(tmp_Data_addr+i) = *(tmp_Distributed_Data->Data_addr + slit_num_th*Data_Max_size_split_record[Data_number_i] + i);
				}
				Distributed_Data_t* tmp_Distributed_Data_List = pvPortMalloc(sizeof(Distributed_Data_t));
				tmp_Distributed_Data_List->Next_Distributed_Data = NULL;
				if(Data_number_i == 0){
					Start_Distributed_Data_List = tmp_Distributed_Data_List;
					Stop_Distributed_Data_List = Start_Distributed_Data_List;
				}
				else{
					Stop_Distributed_Data_List->Next_Distributed_Data = tmp_Distributed_Data_List;
					Stop_Distributed_Data_List = Stop_Distributed_Data_List->Next_Distributed_Data;
				}
				tmp_Distributed_Data_List->Data_addr = tmp_Data_addr;
				tmp_Distributed_Data_List->Data_size = Data_size_split_record[Data_number_i];

				tmp_Data_addr = tmp_Data_addr + Data_size_split_record[Data_number_i];
				tmp_Distributed_Data = tmp_Distributed_Data->Next_Distributed_Data;
			}

			Distributed_TaskHandle_List_t *NewDTaskControlBlock = pvPortMalloc(sizeof(Distributed_TaskHandle_List_t));
			NewDTaskControlBlock->Next_TaskHandle_List = NULL;
			NewDTaskControlBlock->Processor_id = 1;
		    NewDTaskControlBlock->DTask_id = Global_Task_id;
			NewDTaskControlBlock->DSubTask_id = slit_num_th;
			NewDTaskControlBlock->Instruction_addr = pc_start;
			NewDTaskControlBlock->Instruction_addr_end = pc_end;
			NewDTaskControlBlock->Data_addr = Data_addr;
			NewDTaskControlBlock->Data_size = Data_size_split_record;
			NewDTaskControlBlock->Data_Max_size  = Data_Max_size_split_record;
			NewDTaskControlBlock->Data_number = Data_number;
			NewDTaskControlBlock->Remaind_Data_number = 0;
			NewDTaskControlBlock->Finish_Flag = 0;
			NewDTaskControlBlock->xQueue = ((Distributed_Data_t*)S)->xQueue;
			NewDTaskControlBlock->Distributed_Data_List = Start_Distributed_Data_List;
			Distributed_TaskHandle_List_t* Lastnode = DStart;

			if(Lastnode == NULL)
				DStart = NewDTaskControlBlock;
			else{
				while(Lastnode->Next_TaskHandle_List != NULL)
					Lastnode = Lastnode->Next_TaskHandle_List;
				Lastnode->Next_TaskHandle_List = NewDTaskControlBlock;
			}
			Subscriber_task = NewDTaskControlBlock;
		}
		else{
			uint32_t malloc_size = instruction_size + 4*Data_size_split;
			uint32_t *instruction = pvPortMalloc(malloc_size);

			for(uint32_t i=0;i<(instruction_size/2);i++){
				if((lr_addr<((uint16_t*)pc_start+i)) && ((lr_addr+1)>((uint16_t*)pc_start+i))){
					*((uint16_t*)instruction+i) = 0xbf00;
				}
				else if (lr_addr == ((uint16_t*)pc_start+i)){
					*((uint16_t*)instruction+i) = 0xdf01;
				}
				else
					*((uint16_t*)instruction+i) = *((uint16_t*)pc_start+i);
			}

			uint32_t* Data_addr = instruction+(instruction_size/4);
			Distributed_Data_t* tmp_Distributed_Data = ((Distributed_Data_t*)S);
			uint32_t* tmp_Data_addr = Data_addr;

			Distributed_Data_t* Start_Distributed_Data_List;
			Distributed_Data_t* Stop_Distributed_Data_List;
			for(uint32_t Data_number_i=0;Data_number_i<Data_number;Data_number_i++){
				for(uint32_t i=0;i<Data_size_split_record[Data_number_i];i++){
					*(tmp_Data_addr+i) = *(tmp_Distributed_Data->Data_addr + slit_num_th*Data_Max_size_split_record[Data_number_i] + i);
					//printf("slit_num_th: 0x%X, Data_number_i: 0x%X, Data_addr: 0x%X	0x%X\r\n", slit_num_th, Data_number_i, (tmp_Data_addr+i), *(tmp_Data_addr+i));
				}

				Distributed_Data_t* tmp_Distributed_Data_List = pvPortMalloc(sizeof(Distributed_Data_t));
				tmp_Distributed_Data_List->Next_Distributed_Data = NULL;
				if(Data_number_i == 0){
					Start_Distributed_Data_List = tmp_Distributed_Data_List;
					Stop_Distributed_Data_List = Start_Distributed_Data_List;
				}
				else{
					Stop_Distributed_Data_List->Next_Distributed_Data = tmp_Distributed_Data_List;
					Stop_Distributed_Data_List = Stop_Distributed_Data_List->Next_Distributed_Data;
				}
				tmp_Distributed_Data_List->Data_addr = tmp_Data_addr;
				tmp_Distributed_Data_List->Data_size = Data_size_split_record[Data_number_i];
				tmp_Data_addr = tmp_Data_addr + Data_size_split_record[Data_number_i];
				tmp_Distributed_Data = tmp_Distributed_Data->Next_Distributed_Data;
			}

			Distributed_TaskHandle_List_t *NewDTaskControlBlock = pvPortMalloc(sizeof(Distributed_TaskHandle_List_t));
			NewDTaskControlBlock->Next_TaskHandle_List = NULL;
			NewDTaskControlBlock->Processor_id = 1;
		    NewDTaskControlBlock->DTask_id = Global_Task_id;
			NewDTaskControlBlock->DSubTask_id = slit_num_th;
			NewDTaskControlBlock->Instruction_addr = instruction;
			NewDTaskControlBlock->Instruction_addr_end = instruction + (instruction_size/4);
			NewDTaskControlBlock->Data_addr = Data_addr;
			NewDTaskControlBlock->Data_size = Data_size_split_record;
			NewDTaskControlBlock->Data_Max_size  = Data_Max_size_split_record;
			NewDTaskControlBlock->Data_number = Data_number;
			NewDTaskControlBlock->Remaind_Data_number = 0;
			NewDTaskControlBlock->Finish_Flag = 0;
			NewDTaskControlBlock->TaskHandlex = pvPortMalloc(sizeof(TaskHandle_t));
			NewDTaskControlBlock->Distributed_Data_List = Start_Distributed_Data_List;
			NewDTaskControlBlock->Next_TaskHandle_List = NULL;
			Distributed_TaskHandle_List_t* Lastnode = DStart;

			if(Lastnode == NULL)
				DStart = NewDTaskControlBlock;
			else{
				while(Lastnode->Next_TaskHandle_List != NULL){
					Lastnode = Lastnode->Next_TaskHandle_List;
				}
				Lastnode->Next_TaskHandle_List = NewDTaskControlBlock;
			}
			void (*func_ptr)() = ((uint32_t)instruction)+1;
			xTaskCreate((uint16_t*)func_ptr, "task", (stack_size*4), NULL, 1, NewDTaskControlBlock->TaskHandlex);
		}
	}

	Distributed_Data_t* reomve_s = S;
	while(reomve_s != NULL){
		Distributed_Data_t* s_delete = reomve_s;
		reomve_s = reomve_s->Next_Distributed_Data;
		vPortFree(s_delete);
	}
	return Subscriber_task;
}

void setup_mpu(void){
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
	uint32_t count = 0;
	while(1){
		if ((READ_BIT(USART1_BASE + USART_SR_OFFSET, RXNE_BIT)) || (READ_BIT(USART1_BASE + USART_SR_OFFSET, ORE_BIT))){
			char rec_cmd = (char)REG(USART1_BASE + USART_DR_OFFSET);
			printf("%c\r\n", rec_cmd);
			if (rec_cmd == 'a'){
				count++;
				for(uint32_t i=0;i<16;i++){
					*(((uint32_t*)0x10000000)+i) = 0;
					*(((uint32_t*)0x10000100)+i) = 0;
					*(((uint32_t*)0x10000200)+i) = 0;
				}
				List_FreeBlock();
				Distributed_Data_t* s = Distributed_Set_Traget_Data(0x10000000, 16);
				Distributed_Add_Traget_Data(s, 0x10000100, 8);
				Distributed_Add_Traget_Data(s, 0x10000200, 13);

				Distributed_TaskCreate(Distributed_task, s, 1000);
				List_FreeBlock();
			}
			else if (rec_cmd == 'b'){
				vTaskDelete(TaskHandle_2);
				printf("kill Distributed_task\r\n");
				SET_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_BSRR_OFFSET, BRy_BIT(LED_BLUE));
			}
		}
	}
}

void Distributed_task(void *S){
	Distributed_TaskHandle_List_t *s = Distributed_Start(S);
	Distributed_Data_t* array1 = Distributed_Get_Traget_Data(s);
	Distributed_Data_t* array2 = Distributed_Get_Traget_Data(s);
	Distributed_Data_t* array3 = Distributed_Get_Traget_Data(s);

	for(uint32_t i=0;i<array1->Data_size;i++){
		//*(array1->Data_addr + i) = *(array1->Data_addr + i)+1;
		*(array1->Data_addr + i) = i;
	}
	for(uint32_t i=0;i<array2->Data_size;i++){
		//*(array2->Data_addr + i) = *(array2->Data_addr + i)+1;
		*(array2->Data_addr + i) = i;
	}

	for(uint32_t i=0;i<array3->Data_size;i++){
		//*(array3->Data_addr + i) = *(array3->Data_addr + i)+1;
		*(array3->Data_addr + i) = i;
	}

	Distributed_End(s, array1->Data_addr, array1->Data_size);
}

void task3(){
	led_init(LED_BLUE);
	printf("task3\r\n");

	while(1){
		for(uint32_t i=0;i<500000;i++){
			;
		}
		SET_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_BSRR_OFFSET, BSy_BIT(LED_BLUE));
		for(uint32_t i=0;i<500000;i++){
			;
		}
		SET_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_BSRR_OFFSET, BRy_BIT(LED_BLUE));
	}
}

int main(void){
	DStart->Next_TaskHandle_List = NULL;
	init_usart1();
	led_init(LED_GREEN);
	led_init(LED_ORANGE);
	void (*func_ptr)() = task1;
	REG(AIRCR_BASE) = NVIC_AIRCR_RESET_VALUE | NVIC_PRIORITYGROUP_4;
	xTaskCreate(task1, "task1", 1000, NULL, 1, &TaskHandle_1);
	xTaskCreate(task3, "task3", 1000, NULL, 1, &TaskHandle_3);

	SET_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_BSRR_OFFSET, BSy_BIT(LED_GREEN));
	vTaskStartScheduler();

	while (1)
		;
}

void svc_handler_c(uint32_t LR, uint32_t MSP){
	uint32_t *stack_frame_ptr;
	if (LR & 0x4){
		stack_frame_ptr = (uint32_t *)read_psp();
	}
	else{
		stack_frame_ptr = (uint32_t *)MSP;
	}
	uint32_t stacked_return_addr = *(stack_frame_ptr+6);
	uint16_t svc_instruction = *((uint16_t *)stacked_return_addr - 1);
	uint8_t svc_num = (uint8_t)svc_instruction;
	if(svc_num == 0)
		vPortSVCHandler();
	else if (svc_num == 1){
		 *(stack_frame_ptr) = Distributed_GetNode(stacked_return_addr);
	}
	else if (svc_num == 2){
		Distributed_TaskHandle_List_t* Lastnode = Distributed_GetNode(stacked_return_addr);
		if (Lastnode->DSubTask_id != 0){
			*((uint32_t*)(stacked_return_addr&0xFFFFFFFE)) = 0xe7fe;
		}
		Lastnode->Finish_Flag = 1;
	}
}

void init_eth(void){
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

void init_dac(void){
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

void init_timer4(void){
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

void DAC_SetChannel1Data(uint8_t vol){
	WRITE_BITS( DAC_BASE + DAC_DHR8R1_OFFSET, DAC_DHR8R1_DACC1DHR_7_BIT, DAC_DHR8R1_DACC1DHR_0_BIT, vol);
}

void init_usart2(void){
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

void usart2_send_char(const char ch){
	//wait util TXE == 1
	while (!READ_BIT(USART2_BASE + USART_SR_OFFSET, TXE_BIT))
		;
	REG(USART2_BASE + USART_DR_OFFSET) = (uint8_t)ch;
}

void usart2_handler(void){
	if (READ_BIT(USART2_BASE + USART_SR_OFFSET, ORE_BIT))
	{
		char ch = (char)REG(USART2_BASE + USART_DR_OFFSET);
		for (unsigned int i = 0; i < 5000000; i++)
			;
		if (ch == '\r')
			usart2_send_char('\n');
		usart2_send_char(ch);
		usart2_send_char('~');
	}
	else if (READ_BIT(USART2_BASE + USART_SR_OFFSET, RXNE_BIT))
	{
		char ch = (char)REG(USART2_BASE + USART_DR_OFFSET);
		if (ch == '\r')
			usart2_send_char('\n');
		usart2_send_char(ch);
	}
}

void init_usart1(void){
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

void usart1_send_char(const char ch){
	//wait util TXE == 1
	while (!READ_BIT(USART1_BASE + USART_SR_OFFSET, TXE_BIT))
		;
	REG(USART1_BASE + USART_DR_OFFSET) = (uint8_t)ch;
}

void usart1_handler(void){

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

void *_sbrk(int incr){
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

int _write(int file, char *ptr, int len){

	for (unsigned int i = 0; i < len; i++)
		usart1_send_char(*ptr++);

	return len;
}

int _close(int file){
	return -1;
}

int _lseek(int file, int ptr, int dir){
	return 0;
}

int _read(int file, char *ptr, int len){
	return 0;
}

int _fstat(int file, struct stat *st){
	st->st_mode = S_IFCHR;
	return 0;
}

int _isatty(int file){
	return 1;
}
