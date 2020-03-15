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
#define HEAP_MAX (32 * 1024) 	// 128 KB


extern void vPortSVCHandler();
void vApplicationTickHook() {;}
void vApplicationStackOverflowHook() {;}
void vApplicationIdleHook() {;}
void vApplicationMallocFailedHook() {;}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void init_dac(void);
void DAC_SetChannel1Data(uint8_t vol);
void init_usart1(void);
void init_usart2(void);
void usart1_send_char(const char ch);
void usart2_send_char(const char ch);
void print_sys(char str[255]);
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Distributed_TaskHandle_List_t* Distributed_Dispatch_Task(void* data_info, uint32_t sp, uint32_t lr);
uint32_t Got_sp_minus_immediate(uint32_t addr);
void Distributed_Local_Subtask_Done(Distributed_TaskHandle_List_t* s, uint32_t* Result_Data_addr, uint32_t Result_Data_size);
void Distributed_Insert_Finish_Node(Distributed_TaskHandle_List_t* NewDTaskControlBlock);
void Distributed_Create_Task(void* task, Distributed_Data_t *s, uint32_t Stack_size);
Distributed_TaskHandle_List_t* Distributed_GetNode(uint32_t Return_addr, Distributed_TaskHandle_List_t* Lastnode);
Distributed_Data_t* Distributed_Set_Traget_Data(uint32_t* data_addr, uint32_t data_size, uint32_t split_size);
void Distributed_Add_Target_Data(Distributed_Data_t* S, uint32_t* data_addr, uint32_t data_size, uint32_t split_size);
extern Distributed_TaskHandle_List_t* Distributed_Start(void *data_info);
void List_FreeBlock();
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint8_t init_eth(uint16_t PHYAddress, uint8_t *Addr);
uint32_t ETH_WritePHYRegister(uint16_t PHYAddress, uint16_t PHYReg, uint16_t PHYValue);
uint32_t ETH_ReadPHYRegister(uint16_t PHYAddress, uint16_t PHYReg);
void ETH_DMATxDescChainInit(ETH_DMADESCTypeDef *DMATxDescTab, uint8_t* TxBuff, uint32_t TxBuffCount);
void ETH_DMARxDescChainInit(ETH_DMADESCTypeDef *DMARxDescTab, uint8_t *RxBuff, uint32_t RxBuffCount);
uint8_t DP83848Send(uint8_t* data, uint16_t length);
void eth_handler(void);
uint32_t ETH_CheckFrameReceived(void);
FrameTypeDef ETH_Get_Received_Frame(void);
FrameTypeDef Pkt_Handle(void);
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DistributedNodeGetID();
void DistributedNodeGetIDAgain();
void DistributedNodeResponseID();
uint8_t DistributedNodeCheck(uint32_t Target_Node_id, uint32_t Needsize);
void DistributedNodeCheckback(uint32_t Target_Node_id, uint8_t checkback_flag);
void DistributedNodeBackupMaster(uint32_t Target_Node_id);
void DistributedNodeInvalid(uint32_t Target_Node_id);
uint32_t DistributedNodeSendFreespace(uint32_t Target_Node_id, uint32_t Node_id);
void DistributedNodeSendSubtask(uint32_t Target_Node_id, uint8_t* Subtask_addr, uint32_t Subtask_size);
void DistributedNodeResponseSubtask(uint32_t Target_Node_id);
void DistributedNodeDisablePublish();
void DistributedNodeEnablePublish();
void DistributedNodeSubtaskFinish(uint32_t Target_Node_id, uint32_t Task_id, uint32_t Subtask_id, uint32_t Size);
uint8_t DistributedNodeCheckSizeTimeout(uint32_t tick, uint32_t Target_Node_id, uint32_t Needsize);
void DistributedNodeRequestResult(uint32_t Target_Node_id, uint32_t Task_id, uint32_t Subtask_id);
void DistributedNodeResponseResult(uint32_t Target_Node_id, uint8_t* Result_addr, uint32_t Result_size);
void DistributedNodeRemoveTask(uint32_t Target_Node_id, uint32_t Task_id);
void DistributedSendMsg(uint8_t* MyMacAddr, uint8_t* Target_Addr, uint32_t Size);
void UpdateLocalFreeBlock();
uint8_t Check_Sendable();
Distributed_FreeBlock* GetFreeBlockNode(uint32_t Node_id);
void Distributed_Show_FreeBlock();
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Distributed_Manager_Task();
void Distributed_task();
void task3();
void eth_send(void);
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void swap(int *a, int *b);
int Partition(int *arr, int front, int end);
void QuickSort(int *arr, int front, int end);
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------

volatile uint8_t rec_play_buf_fir[200], rec_play_buf_sec[200];
volatile uint8_t *rece_ptr;
volatile uint8_t *play_ptr;
volatile uint8_t receive_count = 0;
TaskHandle_t TaskHandle_1;
TaskHandle_t TaskHandle_2;
TaskHandle_t TaskHandle_3;
extern BlockLink_t xStart;
Distributed_TaskHandle_List_t* DStart = NULL;											//	Distributed_task list
Distributed_TaskHandle_List_t* DFinish = NULL;											//	Distributed_task Finish list
Distributed_TaskHandle_List_t* DDelete = NULL;											//	Distributed_task Delete list
uint8_t Msg_event = 0;
uint32_t Global_Node_id = 0;
uint32_t Global_Node_count = 0;
uint32_t Global_Node_Master = 0;
uint32_t Global_Node_Backup_Master = 0;
uint32_t Global_Node_Master_Token = 0;
uint32_t Global_Task_id = 0;
Distributed_FreeBlock* DF_Start;
uint32_t DisrtibutedNodeCheckIDFlag = 0;
uint8_t CheckMasterNodeFlag = 0;
uint8_t SendFreespaceFlag = 0;
uint32_t RecvFreespaceFlag = 0;
uint32_t CheckbackFlag = 0;
uint32_t ReceiveSubtaskFlag = 0;
uint32_t PublishFlag = 1;
uint32_t TaskDoneFlag = 0;
uint32_t RequestResultFlag = 0;
uint32_t DispatchSuccessFlag = 0;
extern uint8_t BlockChangeFlag;
uint32_t tickcount_lo_bound = 0;
uint32_t tickcount_hi_bound = 0xFFFFFFFF;
uint32_t unmerge_finish_distributed_task = 0;

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Distributed_Data_t* Distributed_Set_Traget_Data(uint32_t* data_addr, uint32_t data_size, uint32_t split_size){
	Distributed_Data_t* data_info = pvPortMalloc(sizeof(Distributed_Data_t));
	data_info->Data_addr = data_addr;
	data_info->Data_size = data_size;
	data_info->Split_size = split_size;
	data_info->Next_Distributed_Data = NULL;
	return data_info;
}

void Distributed_Add_Target_Data(Distributed_Data_t* data_info, uint32_t* data_addr, uint32_t data_size, uint32_t split_size){
	Distributed_Data_t* tmp_Distributed_Data_t = data_info;
	while(tmp_Distributed_Data_t->Next_Distributed_Data != NULL)
		tmp_Distributed_Data_t = tmp_Distributed_Data_t->Next_Distributed_Data;
	Distributed_Data_t* new_data_info = pvPortMalloc(sizeof(Distributed_Data_t));
	tmp_Distributed_Data_t->Next_Distributed_Data = new_data_info;
	new_data_info->Data_addr = data_addr;
	new_data_info->Data_size = data_size;
	new_data_info->Split_size = split_size;
	new_data_info->Next_Distributed_Data = NULL;
}

Distributed_TaskHandle_List_t* Distributed_GetNode(uint32_t Return_addr, Distributed_TaskHandle_List_t* Lastnode){
	while(Lastnode != NULL){
		if (((uint32_t)Lastnode->Instruction_addr<=Return_addr) && (Return_addr<=(uint32_t)Lastnode->Instruction_addr_end)){
			break;
		}
		Lastnode = Lastnode->Next_TaskHandle_List;
	}
	return Lastnode;
}

void List_FreeBlock(){
	BlockLink_t* tmp_block = &xStart;
	printf("------------------------------------------------------------\r\n");
	while((tmp_block->pxNextFreeBlock)!=NULL){
		tmp_block = tmp_block->pxNextFreeBlock;
		printf("Free  xBlockAddr	0x%lX, xBlockSize:	0x%lX\r\n", (uint32_t)tmp_block, (uint32_t)tmp_block->xBlockSize);
	}
	printf("------------------------------------------------------------\r\n");
}

void Distributed_Create_Task(void* task, Distributed_Data_t *data_info, uint32_t Stack_size){
	QueueHandle_t xQueue = xQueueCreate(1, sizeof(uint32_t*));
	data_info->xQueue = &xQueue;
	TaskHandle_t TaskHandle;
	xTaskCreate(task, "Dtask", Stack_size, data_info, 1, &TaskHandle);
	Distributed_Data_t* Recv_S;
	xQueueReceive(xQueue, (void*)&Recv_S, 0);
	/*
	while(xQueueReceive(xQueue, (void*)&Recv_S, 0) == 0);
	vQueueDelete(xQueue);
	vPortFree(Recv_S->Data_addr);
	vTaskDelete(TaskHandle);
	vPortFree(Recv_S);
	*/
}

/*
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
		if(((Lastnode->DTask_id) == s->DTask_id)&&((Lastnode->Source_Processor_id) == s->Source_Processor_id)){
			for(uint32_t i=0;i<Lastnode->Data_number;i++){
				uint32_t* tmp_Data_addr = Data_addr;
				uint32_t* From_Data_addr = Lastnode->Data_addr;
				for(uint32_t j=0;j<i;j++){
					tmp_Data_addr += Every_Data_Size[j];
					From_Data_addr += *(Lastnode->Data_size+j);
				}
				for(uint32_t j=0;j< *(Lastnode->Data_size+i);j++){
					*(tmp_Data_addr + (Lastnode->DSubTask_id)*(*(Lastnode->Data_Max_size+i)) + j) = *(From_Data_addr+j);
					//printf("DTask_id: %d, Source_Processor_id: %d, DSubTask_id: %d, Data_number: %d/%d, Data_size: %d/%d, \r\n", Lastnode->DTask_id, Lastnode->Source_Processor_id, Lastnode->DSubTask_id, i, Lastnode->Data_number, j, *(Lastnode->Data_size+i), *(From_Data_addr+j));
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
		printf("0x%lX	0x%lX\r\n", (uint32_t)(Data_addr+i), *(Data_addr+i));

	Distributed_Data_t* Send_S = pvPortMalloc(sizeof(Distributed_Data_t));		//	Send result back to  caller, important
	Send_S->Data_addr = Data_addr;
	Send_S->Data_size = Total_Data_Size;
	Send_S->xQueue = s->xQueue;
	xQueueSendToBack((void*)Send_S->xQueue, (void*)Send_S, 0);
	while(1);
}
*/

void Distributed_Local_Subtask_Done(Distributed_TaskHandle_List_t* s, uint32_t* Result_Data_addr, uint32_t Result_Data_size){
	printf("In Distributed_Local_Subtask_Done\r\n");
	Distributed_TaskHandle_List_t *tmp_NewDTaskControlBlock = pvPortMalloc(sizeof(Distributed_TaskHandle_List_t)+ Result_Data_size*sizeof(uint32_t));					//	Copy a DTCB and copy from sour DTCB
	for(uint8_t i=0;i<sizeof(Distributed_TaskHandle_List_t);i++)
		*((uint8_t*)tmp_NewDTaskControlBlock+i) = *((uint8_t*)s+i);
	tmp_NewDTaskControlBlock->Next_TaskHandle_List = NULL;
	tmp_NewDTaskControlBlock->Data_addr = (uint32_t*)((uint8_t*)tmp_NewDTaskControlBlock + sizeof(Distributed_TaskHandle_List_t));											//	Copy result from sour data
	tmp_NewDTaskControlBlock->Data_number = Result_Data_size;
	printf("Result data, Result_Data_size: 0x%lX\r\n", Result_Data_size);
	for(uint32_t i=0;i<Result_Data_size;i++){
		*(tmp_NewDTaskControlBlock->Data_addr+i) = *(Result_Data_addr+i);
		printf("0x%lX, 0x%lX\r\n", (uint32_t)(tmp_NewDTaskControlBlock->Data_addr+i), *(tmp_NewDTaskControlBlock->Data_addr+i));
	}
	tmp_NewDTaskControlBlock->Finish_Flag = 1;
	Distributed_Insert_Finish_Node(tmp_NewDTaskControlBlock);																		//	INser to Fiish DTCB list
	printf("Delete: s\r\n");
	vPortFree(s);																													//	Free sour DTCB
	printf("delete distributed task in major node: 0x%lX\r\n", (uint32_t)(*(s->TaskHandlex)));
	vTaskDelete(NULL);																												//	Kill local task(parameter eith NULL)
	printf("Entry while loop forever, Should not be here\r\n");
	//	need to wait for and merge all data
	/*
	Distributed_Data_t* Send_S = pvPortMalloc(sizeof(Distributed_Data_t));
	Send_S->Data_addr = Data_addr;
	Send_S->Data_size = Total_Data_Size;
	Send_S->xQueue = s->xQueue;
	xQueueSendToBack(Send_S->xQueue, Send_S, 0);
	while(1);
	*/
	while(1);
}

void Distributed_Insert_Finish_Node(Distributed_TaskHandle_List_t* NewDTaskControlBlock){
	Distributed_TaskHandle_List_t* Lastnode = DFinish;
	Distributed_TaskHandle_List_t* pre_Lastnode = Lastnode;
	NewDTaskControlBlock->Next_TaskHandle_List = NULL;
	while((Lastnode != NULL) && ((Lastnode->Source_Processor_id != NewDTaskControlBlock->Source_Processor_id) || (Lastnode->DTask_id != NewDTaskControlBlock->DTask_id))){
		pre_Lastnode = Lastnode;
		Lastnode = Lastnode->Next_TaskHandle_List;
	}
	if(Lastnode == NULL){
		NewDTaskControlBlock->Next_TaskHandle_List = NULL;
		if(Lastnode == DFinish){
			DFinish = NewDTaskControlBlock;
		}
		else{
			pre_Lastnode->Next_TaskHandle_List = NewDTaskControlBlock;
		}
	}
	else{
		while((Lastnode != NULL) && (Lastnode->Source_Processor_id == NewDTaskControlBlock->Source_Processor_id) &&  (Lastnode->DTask_id == NewDTaskControlBlock->DTask_id) && (Lastnode->DSubTask_id < NewDTaskControlBlock->DSubTask_id)){
			pre_Lastnode = Lastnode;
			Lastnode = Lastnode->Next_TaskHandle_List;
		}
		if((Lastnode == DFinish) && (Lastnode->DSubTask_id >= NewDTaskControlBlock->DSubTask_id)){
			NewDTaskControlBlock->Next_TaskHandle_List = Lastnode;
			DFinish = NewDTaskControlBlock;
		}
		else{
			NewDTaskControlBlock->Next_TaskHandle_List = pre_Lastnode->Next_TaskHandle_List;
			pre_Lastnode->Next_TaskHandle_List = NewDTaskControlBlock;
		}
	}
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

Distributed_TaskHandle_List_t* Distributed_Dispatch_Task(void* data_info, uint32_t sp, uint32_t lr){
	DistributedNodeDisablePublish();
	printf("Start to manager_task\r\n");
	Global_Task_id++;
	uint32_t Data_number = 0;
	Distributed_Data_t* tmp_Distributed_Data = ((Distributed_Data_t*)data_info);
	while(tmp_Distributed_Data != NULL){										//	Count the number of data
		tmp_Distributed_Data = tmp_Distributed_Data->Next_Distributed_Data;
		Data_number++;
	}

	uint32_t Data_size_array[Data_number];
	uint32_t Data_split_size_array[Data_number];

	tmp_Distributed_Data = ((Distributed_Data_t*)data_info);					//	Put the datas info into array, make it more convenience
	for(uint32_t i=0;i<Data_number;i++){
		Data_size_array[i] = tmp_Distributed_Data->Data_size;
		Data_split_size_array[i] = tmp_Distributed_Data->Split_size;
		tmp_Distributed_Data = tmp_Distributed_Data->Next_Distributed_Data;
	}

	uint32_t *pc_start = 0;
	uint32_t *pc_end = 0;
	uint32_t *lr_addr = 0;

	uint32_t stack_size = 0;
	lr_addr = (uint32_t *)((lr & 0xFFFFFFFE)-4);								//	Get return addr

	uint32_t tmp_lr = lr & 0xFFFFFFFE;
	while(*((uint16_t *)tmp_lr) != 0xb580){										//	To find the first push	{r7, sp} instruction	as the begin of distributed task text section
		stack_size = stack_size + Got_sp_minus_immediate(tmp_lr);				//	decode to find sp_minus_immediate instruction and accumulate the stack_size
		tmp_lr = (uint32_t)((uint16_t *)tmp_lr-1);
	}
	pc_start = (((uint32_t*)tmp_lr));											//	To find the secnod push	{r7, sp} instruction 	as the end of distributed task text section
	tmp_lr = lr & 0xFFFFFFFE;
	while(*((uint16_t *)tmp_lr)!= 0xb580){
		tmp_lr = (uint32_t)((uint16_t *)tmp_lr+1);
	}
	pc_end = (uint32_t*)((uint16_t *)tmp_lr);
	uint32_t instruction_size = ((uint32_t)pc_end-(uint32_t)pc_start);			//	Get the size of distributed task text section

	Distributed_TaskHandle_List_t* Subscriber_task;
	uint32_t Distributed_subtask_size = 104;									//	xTaskCreate need at least 104 bytes
	Distributed_subtask_size += (4*2);											//	malloc twice		(malloc every time need more 4 bytes)
	Distributed_subtask_size += stack_size;										//	D_Task Stack size
	Distributed_subtask_size += instruction_size;								// 	D_Task_Instruction_size
	Distributed_subtask_size += Data_number*sizeof(uint32_t);					//	D_Task_Data_Max_size_split_record
	Distributed_subtask_size += Data_number*sizeof(uint32_t);					//	D_Task_Data_size_split_record
	Distributed_subtask_size += Data_number*sizeof(Distributed_Data_t);			//	D_Task_Distributed_Data_List
	Distributed_subtask_size += sizeof(TaskHandle_t);							//	D_Task_TaskHandle_t
	Distributed_subtask_size += sizeof(Distributed_TaskHandle_List_t);			//	D_Task_NewDTaskControlBlock
	printf("Distributed_subtask need size: 0x%lX bytes\r\n", Distributed_subtask_size);

	uint32_t split_num = 0;
	Distributed_FreeBlock* free_block = DF_Start;
	while(free_block != NULL){													//	Calculate the number of node may be dispatched
		if(free_block->Node_id != Global_Node_id)								//	Except itself	(Must dispatch one of subtask to itself)
			split_num++;
		free_block = free_block->Next_Distributed_FreeBlock;
	}

	uint32_t free_block_Max[2][split_num];										//	free_block_Max[1]: Node_id, free_block_Max[2]: Max_block_size
	uint32_t free_block_sort[split_num];
	uint32_t split_num_index = 0;
	free_block = DF_Start;
	while(free_block != NULL){													//	Find the largest block in every node
		if(free_block->Node_id != Global_Node_id){								//	Except itself	(Must dispatch one of subtask to itself)
			free_block_Max[0][split_num_index] = free_block->Node_id;
			free_block_Max[1][split_num_index] = 0;
			for(uint32_t i=0;i<free_block->Block_number;i++){
				if(*(free_block->Block_size_array+i) > free_block_Max[1][split_num_index]){
						free_block_Max[1][split_num_index] = *(free_block->Block_size_array+i);
						free_block_sort[split_num_index] = *(free_block->Block_size_array+i);
					}
			}
			split_num_index++;
		}
		free_block = free_block->Next_Distributed_FreeBlock;
	}

	QuickSort((int*)free_block_sort, (int)0, (int)(split_num-1));				//	Sort the block list

	for(uint32_t i=0;i<split_num;i++){											//	Update 2-D array free_block_Max as sorted order
		for(uint32_t j=0;j<split_num;j++){
			if(free_block_sort[i] == free_block_Max[1][j]){
				if(i != j){
					swap((int*)&free_block_Max[0][i], (int*)&free_block_Max[0][j]);
					swap((int*)&free_block_Max[1][i], (int*)&free_block_Max[1][j]);
				}
				break;
			}
		}
	}

	uint32_t satisfy_split_num = 0;												//	Final split number
	uint32_t* Distributed_dispatch_node;										//	Final dispatch node id array
	uint32_t* Distributed_data_need_size;										//	Final dispatch node need size array
	uint32_t** TwoD_Data_Max_size_split_record;
	uint32_t** TwoD_Data_size_split_record;
	uint32_t* Fail_node;														//	Fail node array
	uint32_t Fail_node_num = 0;													//	Fail node number
	uint8_t success_dispatch_flag = 0;											//	Split and check node sucess flag
	uint32_t act_split_num = 0;													//	Tmp split number
	while(success_dispatch_flag == 0){

		uint32_t split_num_without_fail_node = split_num;
		if(Fail_node_num > 0){													//	If fail node array is not empty(at the first time Fail_node_num is zero)
			printf("Fail_node_num: 0x%lX\r\n", Fail_node_num);
			for(uint32_t i=0;i<Fail_node_num;i++)
				printf("Fail_node: 0x%lX\r\n", Fail_node[i]);
																				//	remove fail node from quick sort array by change the node id to zero
			for(uint32_t i=0;i<Fail_node_num;i++){								//	Compare fail node array and free_block_Max[0] to find the fail node id and change to 0 (indicate fail)
				for(uint32_t j=0;j<split_num;j++){								//	Calculate  the valid node number: split_num_without_fail_node
					if(Fail_node[i] == free_block_Max[0][j]){
						free_block_Max[0][j] = 0;
						split_num_without_fail_node--;
						break;
					}
				}
			}
		}
		uint32_t free_block_Max_without_fail_node[2][split_num_without_fail_node];									//	Create 2d array from free_block_Max and without fail node
		split_num_without_fail_node = 0;
		for(uint32_t i=0;i<split_num;i++){																			//	Copy to 2d array without fail node
			if(free_block_Max[0][i] != 0){
				free_block_Max_without_fail_node[0][split_num_without_fail_node] = free_block_Max[0][i];			//	free_block_Max_without_fail_node is sorted, small to large
				free_block_Max_without_fail_node[1][split_num_without_fail_node] = free_block_Max[1][i];
				split_num_without_fail_node++;
			}
		}
		printf("free_block_Max_without_fail_node:\r\n");
		for(uint32_t i=0;i<split_num_without_fail_node;i++)
			printf("0x%lX	Node id: 0x%lX, size: 0x%lX\r\n", i, free_block_Max_without_fail_node[0][i], free_block_Max_without_fail_node[1][i]);
		uint32_t decrease_node_num = 0;																				//	accumulate the number freespace not enough node number
		uint32_t split_num_with_local = split_num_without_fail_node + 1;											//	Plus 1 is the local subtask	(Must dispatch one of subtask to local)
		while(split_num_with_local > decrease_node_num){															//	Calculate a suitable way to dispatch data and distributed task
																													//	If success_dispatch_flag == 0 (fail), abandon the least size node
			success_dispatch_flag = 1;
			act_split_num = split_num_with_local - decrease_node_num;												//	actually split numumber, abandon the not enough size node number
			Distributed_dispatch_node = pvPortMalloc(act_split_num*sizeof(uint32_t));								//	Final dispatch node id array
			Distributed_data_need_size = pvPortMalloc(act_split_num*sizeof(uint32_t));								//	Every node need size array
			for(uint32_t i=0;i<act_split_num;i++){																	//	Clear array for insurance
				Distributed_dispatch_node[i] = 0;
				Distributed_data_need_size[i] = 0;
			}
			TwoD_Data_size_split_record = (uint32_t**)pvPortMalloc(act_split_num*sizeof(uint32_t*));				//	2D array to store every node(act_split_num) and every(Data_number) data split size
			TwoD_Data_Max_size_split_record = (uint32_t**)pvPortMalloc(act_split_num*sizeof(uint32_t*));			//	2D array to store every node(act_split_num) and every(Data_number) data split max size
			for(uint32_t i=0;i<act_split_num;i++){
				TwoD_Data_size_split_record[i] = (uint32_t*)pvPortMalloc(Data_number*sizeof(uint32_t));
				TwoD_Data_Max_size_split_record[i] = (uint32_t*)pvPortMalloc(Data_number*sizeof(uint32_t));
			}
			printf("act_split_num: 0x%lX\r\n", act_split_num);															//	Split datas into property size
			for(uint32_t split_num_th=0;split_num_th<act_split_num;split_num_th++){										//	For loop for every node
				uint32_t Data_size_split = 0;
				for(uint32_t Data_number_th=0;Data_number_th<Data_number;Data_number_th++){								//	For loop for every data
					uint32_t tmp_data_size = 0;
					uint32_t split_base_data_size = Data_size_array[Data_number_th];									//	Data_number_th data total size
					if(Data_split_size_array[Data_number_th] > 1){														//	If indicate minimum split size
						split_base_data_size = Data_size_array[Data_number_th]/Data_split_size_array[Data_number_th];	//	split_base_data_size = total size / minimum split size
					}
					else{
						Data_split_size_array[Data_number_th] = 1;
					}
					if ((split_base_data_size%act_split_num) == 0){														//	split_base_data_size is divisible to act_split_num
						tmp_data_size = (split_base_data_size/act_split_num)*Data_split_size_array[Data_number_th];		//	every node split equally whole split_base_data_size
						TwoD_Data_Max_size_split_record[split_num_th][Data_number_th] = tmp_data_size;
					}
					else{																									//	split_base_data_size is not divisible to act_split_num
						tmp_data_size = ((split_base_data_size/act_split_num) + 1)*Data_split_size_array[Data_number_th];	//	every node get ((split_base_data_size/act_split_num)+1)*Data_split_size_array[Data_number_th]
						TwoD_Data_Max_size_split_record[split_num_th][Data_number_th] = tmp_data_size;

						if (((split_num_th+1)*tmp_data_size) <= Data_size_array[Data_number_th]){							// if ((split_base_data_size/act_split_num)+1)*Data_split_size_array[Data_number_th] > data total size
							;																								//	Get the remain part
						}
						else if ((((split_num_th+1)*tmp_data_size) > Data_size_array[Data_number_th]) &&  ((split_num_th*tmp_data_size) <= Data_size_array[Data_number_th])){
							//tmp_data_size = (split_base_data_size % tmp_data_size)*Data_split_size_array[Data_number_th];
							tmp_data_size = Data_size_array[Data_number_th] % tmp_data_size;
						}
						else{
							tmp_data_size = 0;
						}
					}
					//printf("split_num_th: 0x%lX, Data_number_th: 0x%lX, tmp_data_size: 0x%lX\r\n", split_num_th, Data_number_th, tmp_data_size);
					TwoD_Data_size_split_record[split_num_th][Data_number_th] = tmp_data_size;
					Data_size_split += tmp_data_size;
				}
				Distributed_data_need_size[split_num_th] = Data_size_split*sizeof(uint32_t) + Distributed_subtask_size;
			}
			uint8_t Local_satisfy_subtask_flag = 0;
			BlockLink_t* tmp_block = &xStart;															//	Check local freespace whether satisfy Distributed_data_need_size[0]
			while(tmp_block != NULL){
				if(tmp_block->xBlockSize > Distributed_data_need_size[0]){
					Local_satisfy_subtask_flag = 1;
					break;
				}
				tmp_block = tmp_block->pxNextFreeBlock;
			}
			if(Local_satisfy_subtask_flag == 0){
				decrease_node_num = split_num_with_local + 1;											//	decrease_node_num = split_num_with_local + 1 mean local freespace not enough to execute the task
				success_dispatch_flag = 0;
				printf("Local Freeblock not satisfy the minimum subtask size, dame it.\r\n");
				break;
			}
			else{
				Distributed_dispatch_node[0] = Global_Node_id;											// Check local freespace satisfy Distributed_data_need_size[0]
			}																							//	Distributed_dispatch_node[0] is local node
			for(uint32_t i=1;i<act_split_num;i++){														//	i=0, dispatch to local subtask, i mean the need block size
				for(uint32_t j=0;j<act_split_num-1;j++){												//	j mean the Free block size[1] and Node id[0]
					uint8_t node_dispatch_flag = 0;
					for(uint32_t k=1;k<act_split_num;k++){												//	k used to check the j node id whether been dispatch
						if(Distributed_dispatch_node[k] == free_block_Max_without_fail_node[0][decrease_node_num+j]){
							node_dispatch_flag = free_block_Max_without_fail_node[0][decrease_node_num+j];
							break;
						}
					}
					if((Distributed_data_need_size[i] < free_block_Max_without_fail_node[1][decrease_node_num+j]) && (node_dispatch_flag == 0)){	//	free_block_Max[decrease_node_num+j] satisfy Distributed_data_need_size[i]
						Distributed_dispatch_node[i] = free_block_Max_without_fail_node[0][decrease_node_num+j];	// decrease_node_num to base mean abandon least decrease_node_num node(freespace is not enough)
						break;
					}
				}
			}
			for(uint32_t i=0;i<act_split_num;i++){														//	check every Distributed_dispatch_node could be dispatch
				if(Distributed_dispatch_node[i] == 0){
					success_dispatch_flag = 0;
				}
			}
			if(success_dispatch_flag > 0){																//	every Distributed_dispatch_node has been dispatch
				break;
			}
			else{																//	dispatch fail, abandon the least freespace node(decrease_node_num++), then try again
				vPortFree(Distributed_dispatch_node);							//	free Distributed_dispatch_node array
				vPortFree(Distributed_data_need_size);							//	free Distributed_dispatch_node array
				for(uint32_t i=0;i<act_split_num;i++){							//	free 2d TwoD_Data_Max_size_split_record and TwoD_Data_size_split_record array
					vPortFree(TwoD_Data_Max_size_split_record[i]);
					vPortFree(TwoD_Data_size_split_record[i]);
				}
				vPortFree(TwoD_Data_Max_size_split_record);
				vPortFree(TwoD_Data_size_split_record);
				decrease_node_num++;
			}
		}
		if(success_dispatch_flag > 0){																		//	All node could be dispatch
			if(Fail_node_num > 0){
				vPortFree(Fail_node);
				Fail_node_num = 0;
			}
			Fail_node_num = 0;																				//	Check the node exist and freespace enough
			uint32_t tmp_invalid_node[act_split_num];
			for(uint32_t split_num_th=0;split_num_th<act_split_num;split_num_th++){
				if(Distributed_dispatch_node[split_num_th] != Global_Node_id){
					uint8_t timeout_flag = DistributedNodeCheckSizeTimeout(210000, Distributed_dispatch_node[split_num_th], Distributed_data_need_size[split_num_th]);
					if(timeout_flag == 0xFF){																//	0xFF mean node not response, not exist
						printf("Without check back, can't DistributedNodeSendSubtask, invalid Node id: 0x%lX\r\n", Distributed_dispatch_node[split_num_th]);
						tmp_invalid_node[split_num_th] = Distributed_dispatch_node[split_num_th];
						Fail_node_num++;
					}
					else if(timeout_flag == 0){																//	0 mean Node Freespace is not enough to dispatch
						printf("Got check back but freespace not satisfy, Node id: 0x%lX\r\n", Distributed_dispatch_node[split_num_th]);
						tmp_invalid_node[split_num_th] = Distributed_dispatch_node[split_num_th];
						Fail_node_num++;
					}
					else{																					//	other mean the node exist and Freespace is enough to dispatch
						printf("Got check back and ready to DistributedNodeSendSubtask, Node id: 0x%lX\r\n", Distributed_dispatch_node[split_num_th]);
						;
					}
				}
			}
			if(Fail_node_num > 0){
				Fail_node = pvPortMalloc(Fail_node_num*sizeof(uint32_t));									//	Record Fail node (not exist or Freespace not enough)
				uint32_t tmp_index = 0;
				for(uint32_t i=0;i<act_split_num;i++){
					if(tmp_invalid_node[i] != 0){
						Fail_node[tmp_index] = tmp_invalid_node[i];
						tmp_index++;
					}
				}
				success_dispatch_flag = 0;
			}
		}
		if(success_dispatch_flag > 0){																		//	every Distributed_dispatch_node has been dispatch
			satisfy_split_num = act_split_num;
			if(Fail_node_num > 0){
				vPortFree(Fail_node);
				Fail_node_num = 0;
			}
			break;
		}
		else{																//	dispatch fail, because check node fail(freespace not enough or without checkback), mark as Fail_node, then try again
			vPortFree(Distributed_dispatch_node);							//	free Distributed_dispatch_node array
			vPortFree(Distributed_data_need_size);							//	free Distributed_dispatch_node array
			for(uint32_t i=0;i<act_split_num;i++){							//	free 2d TwoD_Data_Max_size_split_record and TwoD_Data_size_split_record array
				vPortFree(TwoD_Data_Max_size_split_record[i]);
				vPortFree(TwoD_Data_size_split_record[i]);
			}
			vPortFree(TwoD_Data_Max_size_split_record);
			vPortFree(TwoD_Data_size_split_record);
		}
	}

	printf("satisfy_split_num: 0x%lX\r\n", satisfy_split_num);
	if(satisfy_split_num == 0){
		printf("Dame it fail to dispatch\r\n");
	}
	else{
		for(uint32_t split_num_th=0;split_num_th<satisfy_split_num;split_num_th++){
			uint32_t Data_size_split = 0;
			for(uint32_t i=0;i<Data_number;i++){												//	Calculate sum of data size and copy to array
				Data_size_split += TwoD_Data_size_split_record[split_num_th][i];
			}

			uint8_t* Distributed_Send_Addr;
			uint32_t Distributed_Send_Size = 0;

			Distributed_TaskHandle_List_t* NewDTaskControlBlock;
			uint32_t* Data_size_split_record;
			uint32_t* Data_Max_size_split_record;
			Distributed_Data_t* Start_Distributed_Data_List;
			TaskHandle_t* Subtask_handler;
			uint16_t* dest_instruction_addr;
			uint32_t* dest_data_addr;

			if(split_num_th != 0){
				Distributed_Send_Size = 13;														//	eth send header need at least 13 bytes
				Distributed_Send_Size += instruction_size;
			}
			Distributed_Send_Size += sizeof(Distributed_TaskHandle_List_t);						//	Malloc order:
			Distributed_Send_Size += Data_number*sizeof(uint32_t);								//		1.	Header							(split_num_th != 0)
			Distributed_Send_Size += Data_number*sizeof(uint32_t);								//		2.	NewDTaskControlBlock
			Distributed_Send_Size += Data_number*sizeof(Distributed_Data_t);					//		3.	Data_size_split_record
			Distributed_Send_Size += sizeof(TaskHandle_t);										//		4.	Data_Max_size_split_record
			Distributed_Send_Size += Data_size_split*sizeof(uint32_t);							//		5.	Start_Distributed_Data_List
																								//		6.	Subtask_handler
																								//		7.	dest_instruction_addr			(split_num_th != 0)
																								//		8.	dest_data_addr
			Distributed_Send_Addr = pvPortMalloc(Distributed_Send_Size);						//	Allocate property size to every subtask
			if(split_num_th != 0)
				NewDTaskControlBlock = (Distributed_TaskHandle_List_t*)((uint8_t*)Distributed_Send_Addr + 13);
			else
				NewDTaskControlBlock = (Distributed_TaskHandle_List_t*)Distributed_Send_Addr;
			Data_size_split_record = (uint32_t*)((uint8_t*)NewDTaskControlBlock + sizeof(Distributed_TaskHandle_List_t));
			Data_Max_size_split_record = (uint32_t*)((uint8_t*)Data_size_split_record + Data_number*sizeof(uint32_t));
			Start_Distributed_Data_List = (Distributed_Data_t*)((uint8_t*)Data_Max_size_split_record + Data_number*sizeof(uint32_t));
			Subtask_handler = (TaskHandle_t*)((uint8_t*)Start_Distributed_Data_List + Data_number*sizeof(Distributed_Data_t));
			if(split_num_th != 0){
				dest_instruction_addr = (uint16_t*)((uint8_t*)Subtask_handler + sizeof(TaskHandle_t));
				dest_data_addr = (uint32_t*)((uint8_t*)dest_instruction_addr + instruction_size);
			}
			else{
				dest_instruction_addr = (uint16_t*)pc_start;
				dest_data_addr = (uint32_t*)((uint8_t*)Subtask_handler + sizeof(TaskHandle_t));
			}

			for(uint32_t i=0;i<Data_number;i++){																			//	Copy Data_size_split_record and Data_Max_size_split_record
				Data_size_split_record[i] = TwoD_Data_size_split_record[split_num_th][i];
				Data_Max_size_split_record[i] = TwoD_Data_Max_size_split_record[split_num_th][i];
			}

			if(split_num_th != 0){																							//	Copy instruction
				for(uint32_t i=0;i<(instruction_size/2);i++){
					if((lr_addr<(uint32_t*)((uint16_t*)pc_start+i)) && ((lr_addr+1)>(uint32_t*)((uint16_t*)pc_start+i))){	//	Overwrite lr addr instruction to nop instruction
						*((uint16_t*)dest_instruction_addr+i) = 0xbf00;
					}
					else if (lr_addr == (uint32_t*)((uint16_t*)pc_start+i)){												//	Overwrite lr addr instruction to svc 1 instruction
						*((uint16_t*)dest_instruction_addr+i) = 0xdf01;
					}
					else																									//	Copy instructions
						*((uint16_t*)dest_instruction_addr+i) = *((uint16_t*)pc_start+i);
				}
			}

			Distributed_Data_t* tmp_Distributed_Data_List;
			Distributed_Data_t* tmp_Distributed_Data = ((Distributed_Data_t*)data_info);
			uint32_t* tmp_dest_data_addr = dest_data_addr;
			for(uint32_t Data_number_i=0;Data_number_i<Data_number;Data_number_i++){										//	Copy datas and create Distributed_Data_t List
				for(uint32_t i=0;i<Data_size_split_record[Data_number_i];i++){
					*(tmp_dest_data_addr+i) = *(tmp_Distributed_Data->Data_addr + split_num_th*Data_Max_size_split_record[Data_number_i] + i);
				}
				tmp_Distributed_Data = tmp_Distributed_Data->Next_Distributed_Data;
				tmp_Distributed_Data_List = (Distributed_Data_t*)((uint8_t*)Start_Distributed_Data_List + Data_number_i*sizeof(Distributed_Data_t));
				if(Data_number_i == (Data_number-1))
					tmp_Distributed_Data_List->Next_Distributed_Data = NULL;
				else
					tmp_Distributed_Data_List->Next_Distributed_Data = (Distributed_Data_t*)((uint8_t*)tmp_Distributed_Data_List + sizeof(Distributed_Data_t));
				tmp_Distributed_Data_List->Data_addr = tmp_dest_data_addr;
				tmp_Distributed_Data_List->Data_size = Data_size_split_record[Data_number_i];
				tmp_dest_data_addr += Data_size_split_record[Data_number_i];
			}

			NewDTaskControlBlock->Source_Processor_id = Global_Node_id;														//	Fullfill the distributed task control block
			NewDTaskControlBlock->Destinate_Processor_id = Distributed_dispatch_node[split_num_th];
		    NewDTaskControlBlock->DTask_id = Global_Task_id;
			NewDTaskControlBlock->DSubTask_id = split_num_th;
			NewDTaskControlBlock->Instruction_addr = (uint32_t*)dest_instruction_addr;
			NewDTaskControlBlock->Instruction_addr_end = (uint32_t*)((uint8_t*)dest_instruction_addr + instruction_size);
			NewDTaskControlBlock->Data_addr = dest_data_addr;
			NewDTaskControlBlock->Data_size = Data_size_split_record;
			NewDTaskControlBlock->Data_Max_size  = Data_Max_size_split_record;
			NewDTaskControlBlock->Data_number = Data_number;
			NewDTaskControlBlock->Remaind_Data_number = 0;
			NewDTaskControlBlock->Stack_size = stack_size;
			NewDTaskControlBlock->Finish_Flag = 0;
			NewDTaskControlBlock->TaskHandlex = Subtask_handler;
			NewDTaskControlBlock->Distributed_Data_List = Start_Distributed_Data_List;
			NewDTaskControlBlock->Next_TaskHandle_List = NULL;

			if(split_num_th == 0){
				*(NewDTaskControlBlock->TaskHandlex) = NULL;
				NewDTaskControlBlock->xQueue = ((Distributed_Data_t*)data_info)->xQueue;
				Subscriber_task = NewDTaskControlBlock;
			}
			else{
				NewDTaskControlBlock->xQueue = NULL;
				//	After send to other board remember to free the msg, jsut remaind the Distributed_TaskHandle_List_t
				//	Distributed_dispatch_node[satisfy_split_num] is the destinate node, Distributed_dispatch_node[0] is local node id
				//	Distributed_dispatch_node[satisfy_split_num], Distributed_Send_Addr, Distributed_Send_Size
				//	???????
				while(1){
					printf("Send to 0x%lX\r\n", Distributed_dispatch_node[split_num_th]);
					DistributedNodeSendSubtask(Distributed_dispatch_node[split_num_th], Distributed_Send_Addr, Distributed_Send_Size);	//	Dispatch by ethernet
					uint32_t base_tick = xTaskGetTickCount();
					uint32_t timeout_tick = base_tick + 100000;
					while(DispatchSuccessFlag == 0){
						uint32_t now_tick = xTaskGetTickCount();
						if(timeout_tick > base_tick){
							if((now_tick > timeout_tick) || (now_tick < base_tick))
								break;
						}
						else{
							if((now_tick > timeout_tick) && (now_tick < base_tick))
								break;
						}
					}
					if(DispatchSuccessFlag != 0){
						printf("Dispatch success, node id: 0x%lX\r\n", DispatchSuccessFlag);
						DispatchSuccessFlag = 0;
						break;
					}
					else{
						printf("Timeout in DistributedNodeSendSubtask\r\n");
					}
				}
				uint32_t subtask_Distributed_TaskHandle_List_size = sizeof(Distributed_TaskHandle_List_t);							//	Copy Distributed_TaskHandle_List_t part
				Distributed_TaskHandle_List_t* tmp_NewDTaskControlBlock = (Distributed_TaskHandle_List_t *)pvPortMalloc(subtask_Distributed_TaskHandle_List_size);
				for(uint8_t i=0;i<sizeof(Distributed_TaskHandle_List_t);i++)
					*((uint8_t*)tmp_NewDTaskControlBlock+i) = *((uint8_t*)NewDTaskControlBlock+i);

				NewDTaskControlBlock = tmp_NewDTaskControlBlock;
				vPortFree(Distributed_Send_Addr);																					//	Free the origin part (already sent)
			}

			Distributed_TaskHandle_List_t* Lastnode = DStart;																		//	Insert to Local Distributed List
			if(Lastnode == NULL)
				DStart = NewDTaskControlBlock;
			else{
				while(Lastnode->Next_TaskHandle_List != NULL)
					Lastnode = Lastnode->Next_TaskHandle_List;
				NewDTaskControlBlock->Next_TaskHandle_List = Lastnode->Next_TaskHandle_List;
				Lastnode->Next_TaskHandle_List = NewDTaskControlBlock;
			}
		}

		vPortFree(Distributed_dispatch_node);									//	dispatch sucess, free useless array
		vPortFree(Distributed_data_need_size);									//	free Distributed_dispatch_node array
		for(uint32_t i=0;i<satisfy_split_num;i++){								//	free Distributed_dispatch_node array
			vPortFree(TwoD_Data_Max_size_split_record[i]);						//	free 2d TwoD_Data_Max_size_split_record and TwoD_Data_size_split_record array
			vPortFree(TwoD_Data_size_split_record[i]);
		}
		vPortFree(TwoD_Data_Max_size_split_record);
		vPortFree(TwoD_Data_size_split_record);

		Distributed_Data_t* reomve_s = data_info;								//	remove	data_info list
		while(reomve_s != NULL){
			Distributed_Data_t* s_delete = reomve_s;
			reomve_s = reomve_s->Next_Distributed_Data;
			vPortFree(s_delete);
		}
	}
	Distributed_TaskHandle_List_t* Lastnode = DStart;
	printf("Show Unfinish node\r\n");
	while(Lastnode != NULL){
		printf("Source_Processor_id: 0x%lX, Destinate_Processor_id: 0x%lX, DTask_id: 0x%lX, DSubTask_id: 0x%lX\r\n", Lastnode->Source_Processor_id, Lastnode->Destinate_Processor_id, Lastnode->DTask_id, Lastnode->DSubTask_id);
		Lastnode = Lastnode->Next_TaskHandle_List;
	}
	DistributedNodeEnablePublish();												//	Enable otehr processor to publish freespace
	return Subscriber_task;
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
		 *(stack_frame_ptr) = (uint32_t)Distributed_GetNode(stacked_return_addr, DStart);
		 printf("i think the program is runnung: 0x%lX\r\n", (uint32_t)stack_frame_ptr);
	}
	else if (svc_num == 2){
		printf("i think the program is done\r\n");
		Distributed_TaskHandle_List_t* Lastnode = Distributed_GetNode(stacked_return_addr, DStart);
		Distributed_TaskHandle_List_t* tmp_Lastnode = DStart;												//	Remove subtask TCB from DStart list
		Distributed_TaskHandle_List_t* pre_tmp_Lastnode = tmp_Lastnode;
		while((tmp_Lastnode != Lastnode) && (tmp_Lastnode != NULL)){
			pre_tmp_Lastnode = tmp_Lastnode;
			tmp_Lastnode = tmp_Lastnode->Next_TaskHandle_List;
		}
		if(tmp_Lastnode == Lastnode){
			if(tmp_Lastnode == DStart){
				if(DStart->Next_TaskHandle_List != NULL)
					DStart = DStart->Next_TaskHandle_List;
				else
					DStart = NULL;
			}
			else
				pre_tmp_Lastnode->Next_TaskHandle_List = tmp_Lastnode->Next_TaskHandle_List;
		}
		Lastnode->Next_TaskHandle_List = NULL;

		if(Lastnode->Source_Processor_id == Global_Node_id){												//	Check Task done, DStart list without task_id(all subtask done)
			uint32_t tmp_count = 0;
			Distributed_TaskHandle_List_t* check_Lastnode = DStart;
			while(check_Lastnode != NULL){
				if(check_Lastnode->DTask_id == Lastnode->DTask_id)
					tmp_count++;
				check_Lastnode = check_Lastnode->Next_TaskHandle_List;
			}
			if(tmp_count == 0){
				TaskDoneFlag = Lastnode->DTask_id;
				printf("1 DTask_id: 0x%lX done, DSubTask_id: 0x%lX  is the last===============================================\r\n", Lastnode->DTask_id, Lastnode->DSubTask_id);
			}
		}

		if (Lastnode->DSubTask_id != 0){
			*((uint16_t*)(stacked_return_addr&0xFFFFFFFE)) = 0xe7fe;										//	modify return addr instruction to bx here
			Lastnode->Finish_Flag = 0;																		//	Finish_Flag = 0 mean that the data has not merge yet
			Distributed_Insert_Finish_Node(Lastnode);														//	Insert to Finish list
			unmerge_finish_distributed_task++;
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
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

void DAC_SetChannel1Data(uint8_t vol){
	WRITE_BITS( DAC_BASE + DAC_DHR8R1_OFFSET, DAC_DHR8R1_DACC1DHR_7_BIT, DAC_DHR8R1_DACC1DHR_0_BIT, vol);
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
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint8_t init_eth(uint16_t PHYAddress, uint8_t *Addr){
	SET_BIT(NVIC_ISER_BASE + NVIC_ISERn_OFFSET(1), 29); //IRQ61 => (m+(32*n)) | m=29, n=1

	SET_BIT(RCC_BASE + RCC_AHB1ENR_OFFSET, GPIO_EN_BIT(GPIO_PORTA));
	SET_BIT(RCC_BASE + RCC_AHB1ENR_OFFSET, GPIO_EN_BIT(GPIO_PORTB));
	SET_BIT(RCC_BASE + RCC_AHB1ENR_OFFSET, GPIO_EN_BIT(GPIO_PORTC));

	SET_BIT(RCC_BASE + RCC_APB2ENR_OFFSET, SYSCFGEN_BIT);
	SET_BIT(SYSCFG_BASE + SYSCFG_PMC_OFFSET, MII_RMII_SEL_BIT);

	SET_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_MODER_OFFSET, MODERy_1_BIT(1));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_MODER_OFFSET, MODERy_0_BIT(1));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_OTYPER_OFFSET, OTy_BIT(1));
	SET_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(1));
	SET_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(1));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(1));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(1));
	WRITE_BITS(GPIO_BASE(GPIO_PORTA) + GPIOx_AFRL_OFFSET, AFRLy_3_BIT(1), AFRLy_0_BIT(1), 11);

	SET_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_MODER_OFFSET, MODERy_1_BIT(2));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_MODER_OFFSET, MODERy_0_BIT(2));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_OTYPER_OFFSET, OTy_BIT(2));
	SET_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(2));
	SET_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(2));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(2));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(2));
	WRITE_BITS(GPIO_BASE(GPIO_PORTA) + GPIOx_AFRL_OFFSET, AFRLy_3_BIT(2), AFRLy_0_BIT(2), 11);

	SET_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_MODER_OFFSET, MODERy_1_BIT(7));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_MODER_OFFSET, MODERy_0_BIT(7));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_OTYPER_OFFSET, OTy_BIT(7));
	SET_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(7));
	SET_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(7));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(7));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(7));
	WRITE_BITS(GPIO_BASE(GPIO_PORTA) + GPIOx_AFRL_OFFSET, AFRLy_3_BIT(7), AFRLy_0_BIT(7), 11);

	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_MODER_OFFSET, MODERy_1_BIT(11));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_MODER_OFFSET, MODERy_0_BIT(11));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OTYPER_OFFSET, OTy_BIT(11));
	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(11));
	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(11));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(11));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(11));
	WRITE_BITS(GPIO_BASE(GPIO_PORTB) + GPIOx_AFRH_OFFSET, AFRHy_3_BIT(11), AFRHy_0_BIT(11), 11);

	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_MODER_OFFSET, MODERy_1_BIT(12));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_MODER_OFFSET, MODERy_0_BIT(12));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OTYPER_OFFSET, OTy_BIT(12));
	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(12));
	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(12));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(12));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(12));
	WRITE_BITS(GPIO_BASE(GPIO_PORTB) + GPIOx_AFRH_OFFSET, AFRHy_3_BIT(12), AFRHy_0_BIT(12), 11);

	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_MODER_OFFSET, MODERy_1_BIT(13));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_MODER_OFFSET, MODERy_0_BIT(13));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OTYPER_OFFSET, OTy_BIT(13));
	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(13));
	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(13));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(13));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(13));
	WRITE_BITS(GPIO_BASE(GPIO_PORTB) + GPIOx_AFRH_OFFSET, AFRHy_3_BIT(13), AFRHy_0_BIT(13), 11);

	SET_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_MODER_OFFSET, MODERy_1_BIT(1));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_MODER_OFFSET, MODERy_0_BIT(1));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_OTYPER_OFFSET, OTy_BIT(1));
	SET_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(1));
	SET_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(1));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(1));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(1));
	WRITE_BITS(GPIO_BASE(GPIO_PORTC) + GPIOx_AFRL_OFFSET, AFRLy_3_BIT(1), AFRLy_0_BIT(1), 11);

	SET_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_MODER_OFFSET, MODERy_1_BIT(4));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_MODER_OFFSET, MODERy_0_BIT(4));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_OTYPER_OFFSET, OTy_BIT(4));
	SET_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(4));
	SET_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(4));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(4));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(4));
	WRITE_BITS(GPIO_BASE(GPIO_PORTC) + GPIOx_AFRL_OFFSET, AFRLy_3_BIT(4), AFRLy_0_BIT(4), 11);

	SET_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_MODER_OFFSET, MODERy_1_BIT(5));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_MODER_OFFSET, MODERy_0_BIT(5));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_OTYPER_OFFSET, OTy_BIT(5));
	SET_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(5));
	SET_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(5));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(5));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTC) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(5));
	WRITE_BITS(GPIO_BASE(GPIO_PORTC) + GPIOx_AFRL_OFFSET, AFRLy_3_BIT(5), AFRLy_0_BIT(5), 11);

	SET_BIT(RCC_BASE + RCC_AHB1ENR_OFFSET, ETHMACEN);
	SET_BIT(RCC_BASE + RCC_AHB1ENR_OFFSET, ETHMACRXEN);
	SET_BIT(RCC_BASE + RCC_AHB1ENR_OFFSET, ETHMACTXEN);

	SET_BIT(RCC_BASE + RCC_AHB1RSTR_OFFSET, ETHMACRST);
	CLEAR_BIT(RCC_BASE + RCC_AHB1RSTR_OFFSET, ETHMACRST);

	SET_BIT(ETHERNET_MAC_BASE + ETH_DMABMR_OFFSET, DMABMR_SR);
	while(READ_BIT(ETHERNET_MAC_BASE + ETH_DMABMR_OFFSET, DMABMR_SR) != 0);

	WRITE_BITS(ETHERNET_MAC_BASE + ETH_MACMIIAR_OFFSET, CR_2_BIT, CR_0_BIT, 0b100);

	uint32_t result;
	//volatile uint16_t ReadPHYRegister = 0;
	//volatile uint16_t tmp_ReadPHYRegister = 0;

	//uint32_t TO_LIMIT = 0x0004FFFF;
	//uint32_t TO_COUNT = 0;
	uint32_t ETH_Mode;
	uint32_t ETH_Speed;


	result = ETH_WritePHYRegister(PHYAddress, 0, 0x8000);	// PHY_BCR	PHY_Reset
	if (result == 0){
		printf("Fail: PHY_BCR	PHY_Reset, result: 	0x%lX\r\n", result);
		return 0;
	}

	for(uint32_t i=0;i<0x0000FFFF;i++)	// PHY_RESET_DELAY
		;

	ETH_Mode = ETH_MODE_FULLDUPLEX;
	//ETH_Mode = ETH_MODE_HALFDUPLEX;
	ETH_Speed = ETH_SPEED_10M;
	//ETH_Speed = ETH_SPEED_100M;

	result = ETH_WritePHYRegister(PHYAddress, 0, ((ETH_Mode >> 3) | (ETH_Speed >> 1)));	//PHY_BCR	Disable PHY_AutoNegotiation
	if (result == 0){
		printf("Fail: PHY_BCR	Disable PHY_AutoNegotiation, result:	0x%lX\r\n", result);
		return 0;
	}
	for(uint32_t i=0;i<0x000FFFF;i++)	// PHY_RESET_DELAY
		;
	/*Auto Negotiation-----------------------------------------------------------------------------------------------------------------------------------------------------------
	result = ETH_WritePHYRegister(PHYAddress, 0, 0x8000);	// PHY_BCR	PHY_Reset
	if (result == 0){
		printf("Fail: PHY_BCR	PHY_Reset, result: 	0x%X\r\n", result);
		return 0;
	}

	for(uint32_t i=0;i<0x0000FFFF;i++)	// PHY_RESET_DELAY
		;

	ReadPHYRegister = 0;
	TO_COUNT = 0;
	while((ReadPHYRegister!=0x784D) && (TO_COUNT<TO_LIMIT)){
		tmp_ReadPHYRegister = ETH_ReadPHYRegister(PHYAddress, 1);
		//ReadPHYRegister = (tmp_ReadPHYRegister&0x0004);	//PHY_BSR	PHY_Linked_Status
		ReadPHYRegister = (tmp_ReadPHYRegister);
		TO_COUNT++;
	}
	if(TO_LIMIT == TO_COUNT){
		printf("Time Out PHY_BSR	PHY_Linked_Status, ReadPHYRegister:	0x%X\r\n", tmp_ReadPHYRegister);
		TO_COUNT = 0;
		return 0;
	}
	printf("Pass PHY_BSR	PHY_Linked_Status, ReadPHYRegister:	0x%X\r\n", tmp_ReadPHYRegister);

	result = ETH_WritePHYRegister(PHYAddress, 0, 0x1000);	//PHY_BCR	PHY_AutoNegotiation
	if (result == 0){
		printf("Fail: PHY_BCR	PHY_AutoNegotiation, result:	0x%X\r\n", result);
		return 0;
	}

	for(uint32_t i=0;i<0x0000FFFF;i++)	// PHY_RESET_DELAY
		;

	ReadPHYRegister = 0;
	TO_COUNT = 0;
	while((!ReadPHYRegister) && (TO_COUNT<TO_LIMIT)){
		tmp_ReadPHYRegister = ETH_ReadPHYRegister(PHYAddress, 1);
		ReadPHYRegister = (tmp_ReadPHYRegister&0x0020);	//PHY_BSR	PHY_AutoNego_Complete
		TO_COUNT++;
	}
	if(TO_LIMIT == TO_COUNT){
		printf("Time Out: PHY_BSR	PHY_AutoNego_Complete, ReadPHYRegister:	0x%X\r\n", tmp_ReadPHYRegister);
		TO_COUNT = 0;
		return 0;
	}
	printf("Pass PHY_AutoNego_Complete: 0x%X\r\n", tmp_ReadPHYRegister);
	for(uint32_t i=0;i<0x0000FFFF;i++)	// PHY_RESET_DELAY
		;
	ReadPHYRegister = ETH_ReadPHYRegister(PHYAddress, 0x0010);	//PHY_SR
	printf("PHY_SR: 0x%X\r\n", ReadPHYRegister);
	if ((ReadPHYRegister & 0x0004) != 0){						//PHY_DUPLEX_STATUS
		ETH_Mode = ((uint32_t)0x00000800);						//ETH_Mode_FullDuplex
	}
	else{
		ETH_Mode = ((uint32_t)0x00000000);						//ETH_Mode_HalfDuplex
	}
	if ((ReadPHYRegister & 0x0002) != 0){						//PHY_SPEED_STATUS
		ETH_Speed  = ((uint32_t)0x00000000);					//ETH_Speed_10M
	}
	else{
		ETH_Speed  = ((uint32_t)0x00004000);					//ETH_Speed_100M
	}
	//Auto Negotiation-----------------------------------------------------------------------------------------------------------------------------------------------------------
	*/
	//ETH_MACCR
	if (ETH_Speed==0x00004000){
		SET_BIT(ETHERNET_MAC_BASE + ETH_MACCR_OFFSET, FES);
	}
	else{
		CLEAR_BIT(ETHERNET_MAC_BASE + ETH_MACCR_OFFSET, FES);
	}

	CLEAR_BIT(ETHERNET_MAC_BASE + ETH_MACCR_OFFSET, LM);

	if (ETH_Mode==0x00000800){
		SET_BIT(ETHERNET_MAC_BASE + ETH_MACCR_OFFSET, DM);
	}
	else{
		CLEAR_BIT(ETHERNET_MAC_BASE + ETH_MACCR_OFFSET, DM);
	}
	SET_BIT(ETHERNET_MAC_BASE + ETH_MACCR_OFFSET, IPCO);
	SET_BIT(ETHERNET_MAC_BASE + ETH_MACCR_OFFSET, RD);
	CLEAR_BIT(ETHERNET_MAC_BASE + ETH_MACCR_OFFSET, APCS);

	for(uint32_t i=0;i<0x0000FFFF;i++)
		;
	//ETH_MACFFR
	SET_BIT(ETHERNET_MAC_BASE + ETH_MACFFR_OFFSET, RA);
	CLEAR_BIT(ETHERNET_MAC_BASE + ETH_MACFFR_OFFSET, BFD);

	CLEAR_BIT(ETHERNET_MAC_BASE + ETH_MACFFR_OFFSET, PM);
	CLEAR_BIT(ETHERNET_MAC_BASE + ETH_MACFFR_OFFSET, HM);
	CLEAR_BIT(ETHERNET_MAC_BASE + ETH_MACFFR_OFFSET, HPF);

	CLEAR_BIT(ETHERNET_MAC_BASE + ETH_MACFFR_OFFSET, HU);

	for(uint32_t i=0;i<0x0000FFFF;i++)
		;

	//ETH_DMAOMR
	CLEAR_BIT(ETHERNET_MAC_BASE + ETH_DMAOMR_OFFSET, DTCEFD);
	SET_BIT(ETHERNET_MAC_BASE + ETH_DMAOMR_OFFSET, RSF);
	SET_BIT(ETHERNET_MAC_BASE + ETH_DMAOMR_OFFSET, TSF);
	CLEAR_BIT(ETHERNET_MAC_BASE + ETH_DMAOMR_OFFSET, FEF);
	CLEAR_BIT(ETHERNET_MAC_BASE + ETH_DMAOMR_OFFSET, FUGF);
	SET_BIT(ETHERNET_MAC_BASE + ETH_DMAOMR_OFFSET, OSF);

	//ETH_DMABMR
	SET_BIT(ETHERNET_MAC_BASE + ETH_DMABMR_OFFSET, AAB);
	SET_BIT(ETHERNET_MAC_BASE + ETH_DMABMR_OFFSET, FB);
	WRITE_BITS(ETHERNET_MAC_BASE + ETH_DMABMR_OFFSET, RDP_5_BIT, RDP_0_BIT, 0x20);
	WRITE_BITS(ETHERNET_MAC_BASE + ETH_DMABMR_OFFSET, PBL_5_BIT, PBL_0_BIT, 0x20);
	WRITE_BITS(ETHERNET_MAC_BASE + ETH_DMABMR_OFFSET, PM_1_BIT, PM_0_BIT, 0x1);
	CLEAR_BIT(ETHERNET_MAC_BASE + ETH_DMABMR_OFFSET, DA);

	for(uint32_t i=0;i<0x0000FFFF;i++)
		;

	SET_BIT(ETHERNET_MAC_BASE + ETH_DMAIER_OFFSET, NISE);
	SET_BIT(ETHERNET_MAC_BASE + ETH_DMAIER_OFFSET, RIE);
	uint32_t ETH_MAC_ADDRESS = ETH_MAC_ADDRESS0;
	/* initialize MAC address in ethernet MAC */
	uint32_t MAC_addr_high_reg = ((uint32_t)Addr[5] << 8) | (uint32_t)Addr[4];
	uint32_t MAC_addr_low_reg = ((uint32_t)Addr[3] << 24) | ((uint32_t)Addr[2] << 16) | ((uint32_t)Addr[1] << 8) | Addr[0];
	WRITE_BITS(ETHERNET_MAC_BASE + ETH_MACAxHR_OFFSET + ETH_MAC_ADDRESS, MACAxH_15_BIT, MACAxH_0_BIT, MAC_addr_high_reg);
	WRITE_BITS(ETHERNET_MAC_BASE + ETH_MACAxLR_OFFSET + ETH_MAC_ADDRESS, MACAxL_31_BIT, MACAxL_0_BIT, MAC_addr_low_reg);

	/* Initialize Tx Rx Descriptors list: Chain Mode */
	ETH_DMATxDescChainInit(DMATxDscrTab, &Tx_Buff[0][0], ETH_TXBUFNB);	// ETH_TXBUFNB 5
	ETH_DMARxDescChainInit(DMARxDscrTab, &Rx_Buff[0][0], ETH_RXBUFNB);	// ETH_RXBUFNB 5

	for(uint32_t i=0; i<ETH_TXBUFNB; i++)
		(&DMATxDscrTab[i])->Status |= 0x00C00000;	// DMATxDesc_Checksum 0x00C00000

	SET_BIT(ETHERNET_MAC_BASE + ETH_MACCR_OFFSET, TE);
	for(uint32_t i=0;i<0x0000FFFF;i++)
		;
	SET_BIT(ETHERNET_MAC_BASE + ETH_MACCR_OFFSET, RE);
	for(uint32_t i=0;i<0x0000FFFF;i++)
		;
	SET_BIT(ETHERNET_MAC_BASE + ETH_DMAOMR_OFFSET, FTF);
	for(uint32_t i=0;i<0x0000FFFF;i++)
		;
	SET_BIT(ETHERNET_MAC_BASE + ETH_DMAOMR_OFFSET, ST);
	for(uint32_t i=0;i<0x0000FFFF;i++)
		;
	SET_BIT(ETHERNET_MAC_BASE + ETH_DMAOMR_OFFSET, DMAOMR_SR);
	for(uint32_t i=0;i<0x0000FFFF;i++)
		;
	return 1;
}

uint32_t ETH_WritePHYRegister(uint16_t PHYAddress, uint16_t PHYReg, uint16_t PHYValue){
	WRITE_BITS(ETHERNET_MAC_BASE + ETH_MACMIIAR_OFFSET, PA_4_BIT, PA_0_BIT, PHYAddress);
	WRITE_BITS(ETHERNET_MAC_BASE + ETH_MACMIIAR_OFFSET, MR_4_BIT, MR_0_BIT, PHYReg);
	SET_BIT(ETHERNET_MAC_BASE + ETH_MACMIIAR_OFFSET, MW);
	SET_BIT(ETHERNET_MAC_BASE + ETH_MACMIIAR_OFFSET, MB);
	WRITE_BITS(ETHERNET_MAC_BASE + ETH_MACMIIDR_OFFSET, MD_15_BIT, MD_0_BIT, PHYValue);
	volatile uint8_t read_ETH_MACMIIAR_MB = 1;
	uint32_t TO_LIMIT = 0x0003FFFF;
	uint32_t TO_COUNT = 0;
	while(read_ETH_MACMIIAR_MB && (TO_COUNT<TO_LIMIT)){
		read_ETH_MACMIIAR_MB = READ_BIT(ETHERNET_MAC_BASE + ETH_MACMIIAR_OFFSET, MB);
		TO_COUNT++;
	}
	if (TO_COUNT>=TO_LIMIT){
		//printf("Turn Over Write ETH_MACMIIAR_MB\r\n");
		return 0;
	}
	else{
		//printf("Pass Write ETH_MACMIIAR_MB\r\n");
		return 1;
	}
}

uint32_t ETH_ReadPHYRegister(uint16_t PHYAddress, uint16_t PHYReg){
	WRITE_BITS(ETHERNET_MAC_BASE + ETH_MACMIIAR_OFFSET, PA_4_BIT, PA_0_BIT, PHYAddress);
	WRITE_BITS(ETHERNET_MAC_BASE + ETH_MACMIIAR_OFFSET, MR_4_BIT, MR_0_BIT, PHYReg);
	CLEAR_BIT(ETHERNET_MAC_BASE + ETH_MACMIIAR_OFFSET, MW);
	SET_BIT(ETHERNET_MAC_BASE + ETH_MACMIIAR_OFFSET, MB);
	volatile uint8_t read_ETH_MACMIIAR_MB = 1;
	uint32_t TO_LIMIT = 0x0003FFFF;
	uint32_t TO_COUNT = 0;

	while(read_ETH_MACMIIAR_MB && (TO_COUNT<TO_LIMIT)){
		read_ETH_MACMIIAR_MB = READ_BIT(ETHERNET_MAC_BASE + ETH_MACMIIAR_OFFSET, MB);
		TO_COUNT++;
	}
	if (TO_COUNT>=TO_LIMIT){
		return 0;
	}
	else{
		volatile uint16_t ret = (uint16_t)REG(ETHERNET_MAC_BASE + ETH_MACMIIDR_OFFSET);
		return ret;
	}
}

void ETH_DMATxDescChainInit(ETH_DMADESCTypeDef *DMATxDescTab, uint8_t* TxBuff, uint32_t TxBuffCount){
  uint32_t i = 0;
  ETH_DMADESCTypeDef *DMATxDesc;
  DMATxDescToSet = DMATxDescTab;
  for(i=0; i < TxBuffCount; i++){
	DMATxDesc = DMATxDescTab + i;
	DMATxDesc->Status = 0x00100000 ;														// ETH_DMATxDesc_TCH 0x00100000;
	DMATxDesc->Buffer1Addr = (uint32_t)(&TxBuff[i*ETH_TX_BUF_SIZE]);						// ETH_TX_BUF_SIZE ETH_MAX_PACKET_SIZE 1524U
	if(i < (TxBuffCount-1))
		DMATxDesc->Buffer2NextDescAddr = (uint32_t)(DMATxDescTab+i+1);
	else
		DMATxDesc->Buffer2NextDescAddr = (uint32_t) DMATxDescTab;
  }
  REG(ETHERNET_MAC_BASE + ETH_DMATDLAR_OFFSET) = (uint32_t) DMATxDescTab;
}

void ETH_DMARxDescChainInit(ETH_DMADESCTypeDef *DMARxDescTab, uint8_t *RxBuff, uint32_t RxBuffCount){
	uint32_t i = 0;
	ETH_DMADESCTypeDef *DMARxDesc;
	DMARxDescToGet = DMARxDescTab;
	for(i=0; i < RxBuffCount; i++){
		DMARxDesc = DMARxDescTab+i;
		DMARxDesc->Status = 0x80000000;														// ETH_DMARxDesc_OWN
		DMARxDesc->ControlBufferSize = 0x00004000 | (uint32_t)ETH_RX_BUF_SIZE;				// ETH_DMARxDesc_RCH 0x00004000	ETH_RX_BUF_SIZE ETH_MAX_PACKET_SIZE   1524
		DMARxDesc->Buffer1Addr = (uint32_t)(&RxBuff[i*ETH_RX_BUF_SIZE]);					// ETH_RX_BUF_SIZE 1524
		if(i < (RxBuffCount-1))
			DMARxDesc->Buffer2NextDescAddr = (uint32_t)(DMARxDescTab+i+1);
		else
			DMARxDesc->Buffer2NextDescAddr = (uint32_t)(DMARxDescTab);
	}
	REG(ETHERNET_MAC_BASE + ETH_DMARDLAR_OFFSET) = (uint32_t) DMARxDescTab;
	DMA_RX_FRAME_infos = &RX_Frame_Descriptor;
}

uint8_t DP83848Send(uint8_t* data, uint16_t length){
	for(uint16_t i;i<length;i++){
		*(((uint8_t *)DMATxDescToSet->Buffer1Addr)+i) = *(data+i);
	}

	volatile ETH_DMADESCTypeDef *DMATxDesc;
	if (DMATxDescToSet->Status & 0x80000000){												//ETH_DMATxDesc_OWN
		printf("Error: ETHERNET DMA OWN descriptor\r\n");
		return 0;
	}

	uint32_t buf_count = 0;
	uint32_t size = 0;

	DMATxDesc = DMATxDescToSet;
	if (length > ETH_TX_BUF_SIZE){
		buf_count = length/ETH_TX_BUF_SIZE;
		if (length%ETH_TX_BUF_SIZE)
			buf_count++;
	}
	else
		buf_count = 1;
	if (buf_count == 1){
		/*set LAST and FIRST segment */
		DMATxDesc->Status |= (0x10000000|0x20000000);	// ETH_DMATxDesc_FS ETH_DMATxDesc_LS
		/* Set frame size */
		DMATxDesc->ControlBufferSize = (length & 0x00001FFF);	// ETH_DMATxDesc_TBS1
		/* Set Own bit of the Tx descriptor Status: gives the buffer back to ETHERNET DMA */
		DMATxDesc->Status |= 0x80000000;	// ETH_DMATxDesc_OWN
		DMATxDesc= (ETH_DMADESCTypeDef *)(DMATxDesc->Buffer2NextDescAddr);
	}
	else{
		for (uint32_t i=0; i<buf_count; i++){
			/* Clear FIRST and LAST segment bits */
			DMATxDesc->Status &= ~(0x10000000|0x20000000);	// ETH_DMATxDesc_FS ETH_DMATxDesc_LS
			if (i == 0) {
				/* Setting the first segment bit */
				DMATxDesc->Status |= 0x10000000;	// ETH_DMATxDesc_FS
			}
			/* Program size */
			DMATxDesc->ControlBufferSize = (ETH_TX_BUF_SIZE & 0x00001FFF);	// ETH_DMATxDesc_TBS1
			if (i == (buf_count-1)){
				/* Setting the last segment bit */
				DMATxDesc->Status |= 0x20000000;	// ETH_DMATxDesc_LS
				size = length - (buf_count-1)*ETH_TX_BUF_SIZE;
				DMATxDesc->ControlBufferSize = (size & 0x00001FFF);	// ETH_DMATxDesc_TBS1
			}
			/* Set Own bit of the Tx descriptor Status: gives the buffer back to ETHERNET DMA */
			DMATxDesc->Status |= 0x80000000;	// ETH_DMATxDesc_OWN
			DMATxDesc = (ETH_DMADESCTypeDef *)(DMATxDesc->Buffer2NextDescAddr);
		}
	}
	DMATxDescToSet = DMATxDesc;
	/* When Tx Buffer unavailable flag is set: clear it and resume transmission */
	if(READ_BIT(ETHERNET_MAC_BASE + ETH_DMASR_OFFSET, TBUS) != 0){
		/* Clear TBUS ETHERNET DMA flag */
		SET_BIT(ETHERNET_MAC_BASE + ETH_DMASR_OFFSET, TBUS);
		/* Resume DMA transmission*/
		REG(ETHERNET_MAC_BASE + ETH_DMATPDR_OFFSET) = 0;
		//return 0;
	}
	/* Return SUCCESS */
	return 1;
}

void eth_handler(void){
	/* Handles all the received frames */
	/* check if any packet received */
	FrameTypeDef frame;

	while(ETH_CheckFrameReceived()){
	    /* process received ethernet packet */
	    frame = Pkt_Handle();
	}
	uint32_t Dest = *((uint32_t*)((uint8_t*)frame.buffer+2));
	uint32_t Sour = *((uint32_t*)((uint8_t*)frame.buffer+8));
	if ((Dest == 0xffffffff) || (Dest == Global_Node_id)){
		//-------------------------------------------------------------------------------------------------------------------------------
		if (Dest == 0xffffffff){
			tickcount_lo_bound = xTaskGetTickCount();
			uint32_t multi = 0;
			if( (Sour <= Global_Node_id) && (Sour > 0))
				multi = (Global_Node_id-Sour-1);
			else if((Sour > Global_Node_id) && (Sour <= Global_Node_count))
				multi = (Global_Node_id+(Global_Node_count-Sour)-1);
			else
				multi = (Global_Node_id-1);
			tickcount_hi_bound = tickcount_lo_bound + 100000*multi + 10;
			//printf("Updata delay time, multi: 0x%lX\r\n", multi);
		}
		//-------------------------------------------------------------------------------------------------------------------------------
		Msg_event = *((uint8_t*)frame.buffer+12);
		if (Msg_event == 1){
			if((Global_Node_Master == Global_Node_id) && (Global_Node_Master != 0)){
				printf("Get DistributedNodeGetID\r\n");
				DistributedNodeResponseID();
			}
		}
		else if (Msg_event == 2){
			if((Global_Node_Backup_Master == Global_Node_id) && (Global_Node_Backup_Master != 0)){
				printf("Get DistributedNodeGetIDAgain\r\n");
				CheckMasterNodeFlag = 1;
			}
			else if((Global_Node_id == Global_Node_Master) && (Global_Node_Backup_Master == 0)){
				printf("Master get DistributedNodeGetIDAgain and no BackupMaster\r\n");
				DistributedNodeResponseID();
			}
		}
		else if (Msg_event == 3){
			printf("Get DistributedNodeResponseID\r\n");
			Global_Node_Master = Sour;
			if(Global_Node_id == 0){
				Global_Node_count = *((uint32_t*)((uint8_t*)frame.buffer+13));
				Global_Node_id = Global_Node_count;
				printf("Global_Node_id: 0x%lX, Global_Node_count: 0x%lX\r\n", Global_Node_id, Global_Node_count);
				SendFreespaceFlag = 1;																			// New Node to Master Node
			}
		}
		else if (Msg_event == 4){
			printf("Get DistributedNodeCheck\r\n");
			uint32_t Needsize = *((uint32_t*)((uint8_t*)frame.buffer+13));
			uint8_t freespace_satisfy_flag = 1;
			if(Needsize > 0){
				freespace_satisfy_flag = 0;
				BlockLink_t* tmp_block = &xStart;
				while(tmp_block != NULL){
					if(tmp_block->xBlockSize >= Needsize){
						freespace_satisfy_flag = 1;
						break;
					}
					tmp_block = tmp_block->pxNextFreeBlock;
				}
			}
			DistributedNodeCheckback(Sour, freespace_satisfy_flag);
		}
		else if (Msg_event == 5){
			if(DisrtibutedNodeCheckIDFlag == Sour){
				CheckbackFlag = *((uint8_t*)frame.buffer+13);
				printf("Get DistributedNodeCheckback, checkback_flag: 0x%lX\r\n",  (uint32_t)CheckbackFlag);
				DisrtibutedNodeCheckIDFlag = 0;
			}
		}
		else if (Msg_event == 6){
			printf("Get DistributedNodeBackupMaster\r\n");
			Global_Node_Backup_Master = Global_Node_id;
			printf("Global_Node_Backup_Master: 0x%lX\r\n", Global_Node_Backup_Master);
		}
		else if (Msg_event == 7){
			printf("Get DistributedNodeInvalid\r\n");
			uint32_t Invalid_Node = *((uint32_t*)((uint8_t*)frame.buffer+13));
			if(Invalid_Node == Global_Node_Master)
				Global_Node_Master = Sour;										//	?????	should be BackupMaster, need to fix
			printf("Global_Node_Master: 0x%lX\r\n", Global_Node_Master);
		}
		else if (Msg_event == 8){
			printf("Get DistributedNodeSendFreespace\r\n");
			if ((Sour <= Global_Node_count) || (Global_Node_id == Global_Node_Master)){
				RecvFreespaceFlag = Sour;
			}
		}
		else if (Msg_event == 9){
			ReceiveSubtaskFlag = 1;
			DistributedNodeResponseSubtask(Sour);
			printf("Get DistributedNodeSendSubtask\r\n");
		}
		else if (Msg_event == 0x0a){
			DispatchSuccessFlag = Sour;
			printf("Get DistributedNodeResponseSubtask\r\n");
		}
		else if (Msg_event == 0x0b){
			PublishFlag = 0;
			printf("Get DistributedNodeDisablePublish\r\n");
		}
		else if (Msg_event == 0x0c){
			PublishFlag = 1;
			printf("Get DistributedNodeEnablePublish\r\n");
		}
		else if (Msg_event == 0x0d){
			uint32_t task_id = *((uint32_t*)((uint8_t*)frame.buffer+13));
			uint32_t subtask_id = *((uint32_t*)((uint8_t*)frame.buffer+17));
			uint32_t size = *((uint32_t*)((uint8_t*)frame.buffer+21));

			Distributed_TaskHandle_List_t* Lastnode = DStart;
			Distributed_TaskHandle_List_t* pre_Lastnode;
			while((!((Lastnode->DTask_id == task_id)&&(Lastnode->DSubTask_id == subtask_id))) && (Lastnode != NULL)){					//	Remove subtask TCB from DStart list
				pre_Lastnode = Lastnode;
				Lastnode = Lastnode->Next_TaskHandle_List;
			}
			if(Lastnode != NULL){
				if(Lastnode == DStart)
					DStart = DStart->Next_TaskHandle_List;
				else
					pre_Lastnode->Next_TaskHandle_List = Lastnode->Next_TaskHandle_List;
				Lastnode->Next_TaskHandle_List = NULL;
				Lastnode->Finish_Flag = 1;
				Lastnode->Data_number = size;
				Distributed_Insert_Finish_Node(Lastnode);
				printf("Find task_id: 0x%lX, subtask_id: 0x%lX in DTCB List in final\r\n", Lastnode->DTask_id, Lastnode->DSubTask_id);
				if(Lastnode->Source_Processor_id == Global_Node_id){							//	Check Task done, DStart list without task_id(all subtask done)
					uint32_t tmp_count = 0;
					Distributed_TaskHandle_List_t* check_Lastnode = DStart;
					while(check_Lastnode != NULL){
						if(check_Lastnode->DTask_id == task_id)
							tmp_count++;
						check_Lastnode = check_Lastnode->Next_TaskHandle_List;
					}
					if(tmp_count == 0){
						TaskDoneFlag = task_id;
						printf("2 DTask_id: 0x%lX done, DSubTask_id: 0x%lX  is the last===============================================\r\n", task_id, subtask_id);
					}
				}
			}
			else
				printf("Can't find task_id: 0x%lX, subtask_id: 0x%lX in DTCB List in final\r\n", task_id, subtask_id);
			printf("Get DistributedNodeSubtaskFinish, task_id: 0x%lX, subtask_id: 0x%lX, size: 0x%lX\r\n", task_id, subtask_id, size);
		}
		else if (Msg_event == 0x0e){
			uint32_t task_id = *((uint32_t*)((uint8_t*)frame.buffer+13));
			uint32_t subtask_id = *((uint32_t*)((uint8_t*)frame.buffer+17));
			Distributed_TaskHandle_List_t* Lastnode = DFinish;
			while((Lastnode->Source_Processor_id != Sour) && (Lastnode->DTask_id != task_id) && (Lastnode->DSubTask_id != subtask_id) && (Lastnode != NULL))
				Lastnode = Lastnode->Next_TaskHandle_List;
			if(Lastnode != NULL){
				DistributedNodeResponseResult(Lastnode->Source_Processor_id, ((uint8_t*)Lastnode->Data_addr-13), (13+Lastnode->Data_number*sizeof(uint32_t)));
			}
			printf("Get DistributedNodeRequestResult\r\n");
		}
		else if (Msg_event == 0x0f){
			RequestResultFlag = Sour;
			printf("Get DistributedNodeResponseResult\r\n");
		}
		else if (Msg_event == 0x10){
			uint32_t processor_id = *((uint32_t*)((uint8_t*)frame.buffer+13));
			uint32_t task_id = *((uint32_t*)((uint8_t*)frame.buffer+17));
			printf("processor_id: 0x%lX, task_id: 0x%lX\r\n", processor_id, task_id);
			Distributed_TaskHandle_List_t* Insert_Lastnode = DDelete;
			if(DDelete != NULL){
				while(Insert_Lastnode->Next_TaskHandle_List != NULL){
					Insert_Lastnode = Insert_Lastnode->Next_TaskHandle_List;
				}
			}

			Distributed_TaskHandle_List_t* Lastnode = DFinish;
			Distributed_TaskHandle_List_t* pre_Lastnode = DFinish;
			while(Lastnode != NULL){
				if((Lastnode->Source_Processor_id == processor_id) && (Lastnode->DTask_id == task_id)){
					printf("God node, Subtask_id: 0x%lX\r\n", Lastnode->DSubTask_id);
					if(Lastnode == DFinish){
						DFinish = Lastnode->Next_TaskHandle_List;
						pre_Lastnode = DFinish;
					}
					else
						pre_Lastnode->Next_TaskHandle_List = Lastnode->Next_TaskHandle_List;
					Lastnode->Next_TaskHandle_List = NULL;
					if(DDelete == NULL)
						DDelete = Lastnode;
					else
						Insert_Lastnode->Next_TaskHandle_List = Lastnode;
					Insert_Lastnode = Lastnode;
					Lastnode = pre_Lastnode;
				}
				pre_Lastnode = Lastnode;
				printf("Lastnode: 0x%lX\r\n", (uint32_t)Lastnode);
				if(Lastnode != NULL)
					Lastnode = Lastnode->Next_TaskHandle_List;
			}
			printf("Get DistributedNodeRemoveTask\r\n");
		}
		//printf("Node_id: 0x%lX, Node_count: 0x%lX, Master: 0x%lX, Backup_Master: 0x%lX, Dest: 0x%lX, Sour: 0x%lX\r\n", Global_Node_id, Global_Node_count, Global_Node_Master, Global_Node_Backup_Master, Dest, Sour);
	}
	/* Clear the Eth DMA Rx IT pending bits */
	SET_BIT(ETHERNET_MAC_BASE + ETH_DMASR_OFFSET, RS);
	SET_BIT(ETHERNET_MAC_BASE + ETH_DMASR_OFFSET, NIS);
}

uint32_t ETH_CheckFrameReceived(void){
  /* check if last segment */
  if(((DMARxDescToGet->Status & 0x80000000) == (uint32_t)0) &&								// ETH_DMARxDesc_OWN	RESET
  	((DMARxDescToGet->Status & 0x00000100) != (uint32_t)0)){								// ETH_DMARxDesc_LS		RESET
    DMA_RX_FRAME_infos->Seg_Count++;
    if (DMA_RX_FRAME_infos->Seg_Count == 1){
      DMA_RX_FRAME_infos->FS_Rx_Desc = DMARxDescToGet;
    }
    DMA_RX_FRAME_infos->LS_Rx_Desc = DMARxDescToGet;
    return 1;
  }
  /* check if first segment */
  else if(((DMARxDescToGet->Status & 0x80000000) == (uint32_t)0) &&						// ETH_DMARxDesc_OWN RESET
          ((DMARxDescToGet->Status & 0x00000200) != (uint32_t)0)&&						// ETH_DMARxDesc_FS  RESET
            ((DMARxDescToGet->Status & 0x00000100) == (uint32_t)0)){					// ETH_DMARxDesc_LS	 RESET
    DMA_RX_FRAME_infos->FS_Rx_Desc = DMARxDescToGet;
    DMA_RX_FRAME_infos->LS_Rx_Desc = NULL;
    DMA_RX_FRAME_infos->Seg_Count = 1;
    DMARxDescToGet = (ETH_DMADESCTypeDef*) (DMARxDescToGet->Buffer2NextDescAddr);
  }
  /* check if intermediate segment */
  else if(((DMARxDescToGet->Status & 0x80000000) == (uint32_t)0) &&						// ETH_DMARxDesc_OWN RESET
          ((DMARxDescToGet->Status & 0x00000200) == (uint32_t)0)&&						// ETH_DMARxDesc_FS  RESET
            ((DMARxDescToGet->Status & 0x00000100) == (uint32_t)0)){					// ETH_DMARxDesc_LS 	 RESET
    (DMA_RX_FRAME_infos->Seg_Count) ++;
    DMARxDescToGet = (ETH_DMADESCTypeDef*) (DMARxDescToGet->Buffer2NextDescAddr);
  }
  return 0;
}

FrameTypeDef ETH_Get_Received_Frame(void){
  uint32_t framelength = 0;
  FrameTypeDef frame = {0,0,0};

  /* Get the Frame Length of the received packet: substruct 4 bytes of the CRC */
  framelength = ((DMARxDescToGet->Status & 0x3FFF0000) >> 16) - 4;	// ETH_DMARxDesc_FL ETH_DMARxDesc_FrameLengthShift
  frame.length = framelength;
  /* Get the address of the first frame descriptor and the buffer start address */
  frame.descriptor = DMA_RX_FRAME_infos->FS_Rx_Desc;
  frame.buffer =(DMA_RX_FRAME_infos->FS_Rx_Desc)->Buffer1Addr;

  /* Update the ETHERNET DMA global Rx descriptor with next Rx descriptor */
  /* Chained Mode */
  /* Selects the next DMA Rx descriptor list for next buffer to read */
  DMARxDescToGet = (ETH_DMADESCTypeDef*) (DMARxDescToGet->Buffer2NextDescAddr);

  /* Return Frame */
  return (frame);
}

FrameTypeDef Pkt_Handle(void){
	volatile ETH_DMADESCTypeDef *DMARxNextDesc;
    FrameTypeDef frame;
    /* get received frame */
    frame = ETH_Get_Received_Frame();
    /* Obtain the size of the packet and put it into the "len" variable. */
    //uint32_t receiveLen = (uint32_t)frame.length;
    //uint8_t *receiveBuffer = (uint8_t*)frame.buffer;
    /* Check if frame with multiple DMA buffer segments */
    if (DMA_RX_FRAME_infos->Seg_Count > 1) {
        DMARxNextDesc = DMA_RX_FRAME_infos->FS_Rx_Desc;
    }
	else {
        DMARxNextDesc = frame.descriptor;
    }
    /* Set Own bit in Rx descriptors: gives the buffers back to DMA */
    for (uint32_t i = 0; i < DMA_RX_FRAME_infos->Seg_Count; i++) {
        DMARxNextDesc->Status = 0x80000000;		//	ETH_DMARxDesc_OWN
        DMARxNextDesc = (ETH_DMADESCTypeDef *)(DMARxNextDesc->Buffer2NextDescAddr);
    }
    /* Clear Segment_Count */
    DMA_RX_FRAME_infos->Seg_Count = 0;
    /* When Rx Buffer unavailable flag is set: clear it and resume reception */
	if (READ_BIT(ETHERNET_MAC_BASE + ETH_DMASR_OFFSET, RBUS) != (uint32_t)0){
        /* Clear RBUS ETHERNET DMA flag */
		SET_BIT(ETHERNET_MAC_BASE + ETH_DMASR_OFFSET, RBUS);
        /* Resume DMA reception */
        REG(ETHERNET_MAC_BASE + ETH_DMATPDR_OFFSET) = 0;
    }
	return frame;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Distributed_Manager_Task(){
	uint32_t count = 0;
	while(1){
		if ((READ_BIT(USART1_BASE + USART_SR_OFFSET, RXNE_BIT)) || (READ_BIT(USART1_BASE + USART_SR_OFFSET, ORE_BIT))){
			char rec_cmd = (char)REG(USART1_BASE + USART_DR_OFFSET);
			printf("%c\r\n", rec_cmd);
			/*
			if (rec_cmd == 'a'){
				count++;
				for(uint32_t i=0;i<16;i++){
					*(((uint32_t*)0x10000000)+i) = i;
					*(((uint32_t*)0x10000100)+i) = i;
					*(((uint32_t*)0x10000200)+i) = i;
				}
				List_FreeBlock();
				Distributed_Data_t* data_info = Distributed_Set_Traget_Data(0x10000000, 16, 4);
				Distributed_Add_Target_Data(data_info, 0x10000100, 8, 2);
				Distributed_Add_Target_Data(data_info, 0x10000200, 13, 1);

				Distributed_Create_Task(Distributed_task, data_info, 1000);
				List_FreeBlock();
			}
			else if (rec_cmd == 'b'){
				vTaskDelete(TaskHandle_2);
				printf("kill Distributed_task\r\n");
				SET_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_BSRR_OFFSET, BRy_BIT(LED_BLUE));
			}
			*/
			if (rec_cmd == 'c'){
				Msg_event = 0;
				Global_Node_id = 0;
				Global_Node_count = 0;
				Global_Node_Master = 0;
				Global_Node_Backup_Master = 0;
				Global_Task_id = 0;
				DisrtibutedNodeCheckIDFlag = 0;
				CheckMasterNodeFlag = 0;

				uint8_t MyMacAddr[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
				while(!init_eth(DP83848_PHY_ADDRESS, MyMacAddr)){
					printf("Reset eth\r\n");
					for(uint32_t i=0;i<0x00000FFF;i++)
						;
				}

				uint32_t TO_COUNT = 0;
				DistributedNodeGetID();
				while((TO_COUNT < 1680000) && (Global_Node_id == 0)){
					TO_COUNT++;
				}

				if(TO_COUNT >= 1680000){
					TO_COUNT = 0;
					DistributedNodeGetIDAgain();
					while((TO_COUNT < 1680000) && (Global_Node_id == 0)){
						TO_COUNT++;
					}
				}
				else{
					TO_COUNT = 0;
				}

				if(TO_COUNT >= 1680000){
					TO_COUNT = 0;
					Global_Node_id = 1;
					Global_Node_count++;
					Global_Node_Master = Global_Node_id;
				}
				TO_COUNT = 0;
				printf("Got Global_Node_id: 0x%lX\r\n", Global_Node_id);
				BlockChangeFlag = 0;

				while(1){
					if ((READ_BIT(USART1_BASE + USART_SR_OFFSET, RXNE_BIT)) || (READ_BIT(USART1_BASE + USART_SR_OFFSET, ORE_BIT))){
						rec_cmd = (char)REG(USART1_BASE + USART_DR_OFFSET);
						printf("%c\r\n", rec_cmd);
					}
					if (rec_cmd == 'w'){
						Distributed_TaskHandle_List_t* Lastnode = DStart;
						uint32_t tmp_count = 0;
						printf("Unfinish task List:\r\n");
						while(Lastnode != NULL){
							printf("%d, Source_Processor_id: 0x%lX, Destinate_Processor_id: 0x%lX, DTask_id: 0x%lX, DSubTask_id: 0x%lX\r\n", (int)tmp_count, Lastnode->Source_Processor_id, Lastnode->Destinate_Processor_id, Lastnode->DTask_id, Lastnode->DSubTask_id);
							tmp_count++;
							Lastnode = Lastnode->Next_TaskHandle_List;
						}
						rec_cmd = '\0';
					}
					if (rec_cmd == 'W'){
						Distributed_TaskHandle_List_t* Lastnode = DFinish;
						uint32_t tmp_count = 0;
						printf("Finish task List:\r\n");
						while(Lastnode != NULL){
							printf("%d, Source_Processor_id: 0x%lX, Destinate_Processor_id: 0x%lX, DTask_id: 0x%lX, DSubTask_id: 0x%lX, Data_size: 0x%lX\r\n", (int)tmp_count, Lastnode->Source_Processor_id, Lastnode->Destinate_Processor_id, Lastnode->DTask_id, Lastnode->DSubTask_id, Lastnode->Data_number);
							tmp_count++;
							Lastnode = Lastnode->Next_TaskHandle_List;
						}
						rec_cmd = '\0';
					}

					if (rec_cmd == 'e'){
						Distributed_TaskHandle_List_t* Lastnode = DDelete;
						uint32_t tmp_count = 0;
						printf("DDelete task List:\r\n");
						while(Lastnode != NULL){
							printf("%d, Source_Processor_id: 0x%lX, Destinate_Processor_id: 0x%lX, DTask_id: 0x%lX, DSubTask_id: 0x%lX\r\n", (int)tmp_count, Lastnode->Source_Processor_id, Lastnode->Destinate_Processor_id, Lastnode->DTask_id, Lastnode->DSubTask_id);
							tmp_count++;
							Lastnode = Lastnode->Next_TaskHandle_List;
						}
						rec_cmd = '\0';
					}

					if (rec_cmd == 's'){										//	Distributed_Show_FreeBlock
						Distributed_Show_FreeBlock();
						rec_cmd = '\0';
					}
					if (rec_cmd == 'S'){										//	List_FreeBlock
						List_FreeBlock();
						rec_cmd = '\0';
					}
					if (rec_cmd == 'f'){										//	send check all node
						uint32_t free_block_number = 0;
						Distributed_FreeBlock* local_free_block = DF_Start;
						while(local_free_block != NULL){
							local_free_block = local_free_block->Next_Distributed_FreeBlock;
							free_block_number++;
						}
						uint32_t Allfreeblocknode[free_block_number];
						free_block_number = 0;
						local_free_block = DF_Start;
						while(local_free_block != NULL){
							Allfreeblocknode[free_block_number] = local_free_block->Node_id;
							free_block_number++;
							local_free_block = local_free_block->Next_Distributed_FreeBlock;
						}
						for(uint32_t i=0;i<free_block_number;i++){
							uint8_t timeout_flag = 0;
							if(Allfreeblocknode[i] != Global_Node_id){
								printf("Ready to send DistributedNodeCheckSizeTimeout to 0x%lX\r\n", Allfreeblocknode[i]);
								timeout_flag = DistributedNodeCheckSizeTimeout(105000, Allfreeblocknode[i], 0);

								if(timeout_flag != 0xff)
									printf("Got response: 0x%lX\r\n",  (uint32_t)timeout_flag);
								else
									printf("MTFK no response: 0x%lX\r\n", (uint32_t)timeout_flag);
							}
						}
						rec_cmd = '\0';
					}
					if (rec_cmd == 'd'){										//	dispatch distributed task
						printf("distributed task test start\r\n");
						count++;
						for(uint32_t i=0;i<16;i++){
							*(((uint32_t*)0x10000000)+i) = i;
							*(((uint32_t*)0x10000100)+i) = 2*i;
							*(((uint32_t*)0x10000200)+i) = 3*i;
						}
						Distributed_Data_t* data_info = Distributed_Set_Traget_Data((uint32_t*)0x10000000, 16, 4);
						Distributed_Add_Target_Data(data_info, (uint32_t*)0x10000100, 8, 2);
						Distributed_Add_Target_Data(data_info, (uint32_t*)0x10000200, 13, 1);
						Distributed_Create_Task(Distributed_task, data_info, 1000);
						rec_cmd = '\0';
					}

					if (rec_cmd == 'm'){										//	malloc large size
						uint32_t Max_block_size = 0;
						List_FreeBlock();
						BlockLink_t* tmp_block = &xStart;
						while((tmp_block->pxNextFreeBlock)!= NULL){
							if(tmp_block->xBlockSize > Max_block_size)
								Max_block_size = tmp_block->xBlockSize;
							tmp_block = tmp_block->pxNextFreeBlock;
						}
						if(Max_block_size > 0xf0){
							Max_block_size = Max_block_size - 0xf0;
							uint32_t* max_malloc = pvPortMalloc(Max_block_size);
							max_malloc[0]++;
							printf("Malloc Max block: 0x%lX\r\n", Max_block_size);
							BlockChangeFlag = 0;
						}
						List_FreeBlock();
						rec_cmd = '\0';
					}

					if(CheckMasterNodeFlag == 1){
						uint8_t timeout_flag = DistributedNodeCheckSizeTimeout(105000, Global_Node_Master, 0);
						if(timeout_flag == 0xff){
							printf("time out\r\n");
							portDISABLE_INTERRUPTS();
							DistributedNodeInvalid(Global_Node_Master);			//	Master node not exist
							DisrtibutedNodeCheckIDFlag = 0;
							Global_Node_Master = Global_Node_id;
							DistributedNodeResponseID();
							portENABLE_INTERRUPTS();
						}
						else{
							DistributedNodeGetID();								//	Master node exist
						}
						portDISABLE_INTERRUPTS();
						CheckMasterNodeFlag = 0;
						portENABLE_INTERRUPTS();
					}

					if(SendFreespaceFlag == 1){									//	New Node to Master Node or Master Node to New Node
						portDISABLE_INTERRUPTS();
						DistributedNodeSendFreespace(0xffffffff, 0);
						SendFreespaceFlag = 0;
						portENABLE_INTERRUPTS();
					}

					if(RecvFreespaceFlag > 0){
						portDISABLE_INTERRUPTS();
						volatile uint32_t tmp_RecvFreespaceFlag = RecvFreespaceFlag;
						uint8_t* frame_addr = (uint8_t*)((DMA_RX_FRAME_infos->FS_Rx_Desc)->Buffer1Addr);
						uint8_t block_number = *((uint8_t*)frame_addr+13);
						uint32_t tmp_node_data_count = 0;
						//printf("Destinate, block_number: 0x%X, --------------------------------------\r\n", block_number);
						for(uint8_t i=0;i<block_number;i++){
							Distributed_FreeBlock* tmp_block = (Distributed_FreeBlock*)((uint8_t*)frame_addr+14+i*sizeof(Distributed_FreeBlock));
							if(tmp_block->Node_id != Global_Node_id){
								if((tmp_block->Node_id > Global_Node_count) && (Global_Node_id != Global_Node_Master))
									Global_Node_count = tmp_block->Node_id;
								//printf("tmp_block: 0x%lX, Node_id: 0x%lX, Block_number: 0x%lX, Block_size_array: 0x%lX\r\n", (uint32_t)tmp_block, tmp_block->Node_id, tmp_block->Block_number, (uint32_t)tmp_block->Block_size_array);
								Distributed_FreeBlock* Local_Node = GetFreeBlockNode(tmp_block->Node_id);
								if(Local_Node->Block_number != tmp_block->Block_number){
									if(Local_Node->Block_number > 0)
										vPortFree(Local_Node->Block_size_array);
									Local_Node->Block_number = tmp_block->Block_number;
									Local_Node->Block_size_array = pvPortMalloc(Local_Node->Block_number*sizeof(uint32_t));
								}
								//printf("BLock: ");
								for(uint32_t j=0;j<tmp_block->Block_number;j++){
									uint32_t* tmp_addr = ((uint32_t*)((uint8_t*)frame_addr+14+block_number*sizeof(Distributed_FreeBlock))+tmp_node_data_count);
									tmp_node_data_count++;
									*(Local_Node->Block_size_array+j) = *tmp_addr;
									//printf("0x%lX, ",  (uint32_t)*tmp_addr);
								}
								//printf("\r\n");
							}
							else{
								tmp_node_data_count += tmp_block->Block_number;
							}
						}
						portENABLE_INTERRUPTS();

						//printf("Destinate End-------------------------------------------------------\r\n");
						uint32_t tmp_count = 0;
						for(uint32_t i=0;i<block_number*sizeof(Distributed_FreeBlock);i++){
							tmp_count++;
						}
						UpdateLocalFreeBlock();
						if(RecvFreespaceFlag > Global_Node_count){
							Global_Node_count = RecvFreespaceFlag;
							printf("New Node and update Global_Node_count: 0x%lX\r\n", Global_Node_count);
							if (Global_Node_id == Global_Node_Master){
								if(Global_Node_Backup_Master == 0){
									printf("Dispatch BackupMaster to %dth Node\r\n", (int)RecvFreespaceFlag);
									DistributedNodeBackupMaster(RecvFreespaceFlag);
									Global_Node_Backup_Master = RecvFreespaceFlag;
								}
								SendFreespaceFlag = 1;
								BlockChangeFlag = 0;// important!!!
							}
						}
						if (tmp_RecvFreespaceFlag == RecvFreespaceFlag)
							RecvFreespaceFlag = 0;
					}

					if (unmerge_finish_distributed_task > 0){
						Distributed_TaskHandle_List_t* Lastnode = DFinish;
						Distributed_TaskHandle_List_t* pre_Lastnode = DFinish;
						while((Lastnode != NULL) && (Lastnode->Finish_Flag != 0)){
							pre_Lastnode = Lastnode;
							Lastnode = Lastnode->Next_TaskHandle_List;
						}
						if(Lastnode != NULL){
							uint32_t Total_malloc_size = sizeof(Distributed_TaskHandle_List_t) + (uint32_t)(Lastnode->Data_number)*sizeof(uint32_t) + 13;
							Distributed_TaskHandle_List_t* tmp_NewDTaskControlBlock = pvPortMalloc(Total_malloc_size);
							for(uint8_t i=0;i<sizeof(Distributed_TaskHandle_List_t);i++){
								*((uint8_t*)tmp_NewDTaskControlBlock+i) = *((uint8_t*)Lastnode+i);
							}
							printf("Result:\r\n");
							tmp_NewDTaskControlBlock->Data_addr = (uint32_t*)((uint8_t*)tmp_NewDTaskControlBlock + sizeof(Distributed_TaskHandle_List_t) + 13);
							tmp_NewDTaskControlBlock->Data_number = Lastnode->Data_number;
							for(uint32_t i=0;i<Lastnode->Data_number;i++){
								*(tmp_NewDTaskControlBlock->Data_addr+i) = *(Lastnode->Data_addr+i);
								printf("0x%lX, 0x%lX\r\n", (uint32_t)(tmp_NewDTaskControlBlock->Data_addr+i), *(tmp_NewDTaskControlBlock->Data_addr+i));
							}
							tmp_NewDTaskControlBlock->Finish_Flag = 1;
							tmp_NewDTaskControlBlock->Next_TaskHandle_List = Lastnode->Next_TaskHandle_List;
							if(Lastnode == DFinish)
								DFinish = tmp_NewDTaskControlBlock;
							else
								pre_Lastnode->Next_TaskHandle_List = tmp_NewDTaskControlBlock;
							printf("delete distributed task in other node\r\n");
							vTaskDelete(*(Lastnode->TaskHandlex));
							vPortFree(Lastnode);
							printf("1 Target to delete Lastnode: 0x%lX\r\n", (uint32_t)Lastnode);
							while(!(Check_Sendable())){
								for(uint32_t i=0;i<100;i++)
									;
							}
							DistributedNodeSubtaskFinish(tmp_NewDTaskControlBlock->Source_Processor_id, tmp_NewDTaskControlBlock->DTask_id, tmp_NewDTaskControlBlock->DSubTask_id, tmp_NewDTaskControlBlock->Data_number);
							portDISABLE_INTERRUPTS();
							unmerge_finish_distributed_task--;
							portENABLE_INTERRUPTS();
							printf("After allocate\r\n");
						}
					}

					if (ReceiveSubtaskFlag > 0){
						portDISABLE_INTERRUPTS();
						printf("ReceiveSubtaskFlag: 0x%lX\r\n", ReceiveSubtaskFlag);
						uint8_t* frame_addr = (uint8_t*)((DMA_RX_FRAME_infos->FS_Rx_Desc)->Buffer1Addr);
						Distributed_TaskHandle_List_t* TmpDTaskControlBlock = (Distributed_TaskHandle_List_t*)((uint8_t*)frame_addr+13);
						uint32_t Data_number = TmpDTaskControlBlock->Data_number;
						uint32_t instruction_size = (uint8_t*)TmpDTaskControlBlock->Instruction_addr_end - (uint8_t*)TmpDTaskControlBlock->Instruction_addr;
						uint32_t Data_size_split = 0;

						uint32_t* Data_size_split_record;
						uint32_t* Data_Max_size_split_record;
						Distributed_Data_t* Start_Distributed_Data_List;
						TaskHandle_t* Subtask_handler;
						uint16_t* dest_instruction_addr;
						uint32_t* dest_data_addr;

						Data_size_split_record = (uint32_t*)((uint8_t*)TmpDTaskControlBlock + sizeof(Distributed_TaskHandle_List_t));

						for(uint32_t i=0;i<Data_number;i++)												//	Calculate sum of data size and copy to array
							Data_size_split += Data_size_split_record[i];

						uint32_t Distributed_Recv_Size = 0;
						Distributed_Recv_Size += sizeof(Distributed_TaskHandle_List_t);
						Distributed_Recv_Size += Data_number*sizeof(uint32_t);
						Distributed_Recv_Size += Data_number*sizeof(uint32_t);
						Distributed_Recv_Size += Data_number*sizeof(Distributed_Data_t);
						Distributed_Recv_Size += sizeof(TaskHandle_t);
						Distributed_Recv_Size += instruction_size;
						Distributed_Recv_Size += Data_size_split*sizeof(uint32_t);

						Distributed_TaskHandle_List_t* NewDTaskControlBlock = pvPortMalloc(Distributed_Recv_Size);
						for(uint32_t i=0;i<Distributed_Recv_Size;i++){
							*((uint8_t*)NewDTaskControlBlock+i) = *((uint8_t*)TmpDTaskControlBlock+i);
						}
						portENABLE_INTERRUPTS();
						Data_size_split_record = (uint32_t*)((uint8_t*)NewDTaskControlBlock + sizeof(Distributed_TaskHandle_List_t));
						Data_Max_size_split_record = (uint32_t*)((uint8_t*)Data_size_split_record + Data_number*sizeof(uint32_t));
						Start_Distributed_Data_List = (Distributed_Data_t*)((uint8_t*)Data_Max_size_split_record + Data_number*sizeof(uint32_t));
						Subtask_handler = (TaskHandle_t*)((uint8_t*)Start_Distributed_Data_List + Data_number*sizeof(Distributed_Data_t));
						dest_instruction_addr = (uint16_t*)((uint8_t*)Subtask_handler + sizeof(TaskHandle_t));
						dest_data_addr = (uint32_t*)((uint8_t*)dest_instruction_addr + instruction_size);

						NewDTaskControlBlock->Instruction_addr = (uint32_t*)dest_instruction_addr;
						NewDTaskControlBlock->Instruction_addr_end = (uint32_t*)((uint8_t*)dest_instruction_addr + instruction_size);
						NewDTaskControlBlock->Data_addr = dest_data_addr;
						NewDTaskControlBlock->Data_size = Data_size_split_record;
						NewDTaskControlBlock->Data_Max_size  = Data_Max_size_split_record;
						NewDTaskControlBlock->TaskHandlex = Subtask_handler;
						NewDTaskControlBlock->Distributed_Data_List = Start_Distributed_Data_List;
						NewDTaskControlBlock->Next_TaskHandle_List = NULL;

						uint32_t* tmp_Data_addr = dest_data_addr;
						Distributed_Data_t* tmp_Distributed_Data_List = Start_Distributed_Data_List;
						for(uint32_t i=0;i<Data_number;i++){
							tmp_Distributed_Data_List->Next_Distributed_Data = (Distributed_Data_t*)((uint8_t*)tmp_Distributed_Data_List + sizeof(Distributed_Data_t));
							tmp_Distributed_Data_List->Data_addr = tmp_Data_addr;
							tmp_Data_addr += Data_size_split_record[i];
							tmp_Distributed_Data_List = tmp_Distributed_Data_List->Next_Distributed_Data;
						}

						Distributed_TaskHandle_List_t* Lastnode = DStart;														//	Insert to Local DTCB List
						if(Lastnode == NULL)
							DStart = NewDTaskControlBlock;
						else{
							while(Lastnode->Next_TaskHandle_List != NULL)
								Lastnode = Lastnode->Next_TaskHandle_List;
							NewDTaskControlBlock->Next_TaskHandle_List = Lastnode->Next_TaskHandle_List;
							Lastnode->Next_TaskHandle_List = NewDTaskControlBlock;
						}
						printf("NewDTaskControlBlock->Stack_size: 0x%lX\r\n", NewDTaskControlBlock->Stack_size);
						printf("sizeof(TaskHandle_t): 0x%X\r\n", sizeof(TaskHandle_t));
						xTaskCreate((void*)NewDTaskControlBlock->Instruction_addr, "Distributed task", (NewDTaskControlBlock->Stack_size), NULL, 1, NewDTaskControlBlock->TaskHandlex);
						portDISABLE_INTERRUPTS();
						ReceiveSubtaskFlag = 0;
						portENABLE_INTERRUPTS();
					}

					if(TaskDoneFlag > 0){																	//	Ready to recycle task_id = TaskDoneFlag result
						printf("Disable all publish and check every node and recycle all subtask result then malloc a property size the send the result to the queue\r\n");
						Distributed_TaskHandle_List_t* Lastnode = DFinish;
						Distributed_TaskHandle_List_t* before_target_node;
						Distributed_TaskHandle_List_t* Last_target_node;
						while( !((Lastnode->Source_Processor_id == Global_Node_id) && (Lastnode->DTask_id = TaskDoneFlag)) && (Lastnode != NULL)){					//	Find the target node DTCB head in DF_Start
							before_target_node = Lastnode;
							Lastnode = Lastnode->Next_TaskHandle_List;
						}
						if(Lastnode == DFinish)
							before_target_node = NULL;
						uint32_t Total_result_size = 0;
						uint32_t target_node_count = 0;
						Distributed_TaskHandle_List_t* Targetnodehead = Lastnode;
						if(Lastnode != NULL){																//	Calculate target result size and node count
							while((Lastnode->Source_Processor_id == Global_Node_id) && (Lastnode->DTask_id == TaskDoneFlag) && (Lastnode != NULL)){
								Last_target_node = Lastnode;
								target_node_count++;
								Total_result_size += Lastnode->Data_number;
								Lastnode = Lastnode->Next_TaskHandle_List;
							}
							Distributed_TaskHandle_List_t*  target_node_array[target_node_count];
							Distributed_TaskHandle_List_t* Subtask_node_zero;

							Lastnode = Targetnodehead;
							target_node_count = 0;
							while((Lastnode->DTask_id == TaskDoneFlag) && (Lastnode != NULL)){					//	record the target Node_id
								if(Lastnode->DSubTask_id == 0)
									Subtask_node_zero = Lastnode;
								target_node_array[target_node_count] = Lastnode;
								target_node_count++;
								Lastnode = Lastnode->Next_TaskHandle_List;
							}
							printf("target_node_count: 0x%lX, Total_result_size: 0x%lX\r\n", target_node_count, Total_result_size);
							for(uint32_t i=0;i<target_node_count;i++){
								printf("Source_Processor_id: 0x%lX, Destinate_Processor_id: 0x%lX, DTask_id: 0x%lX, DSubTask_id: 0x%lX, Data_number: 0x%lX\r\n", target_node_array[i]->Source_Processor_id, target_node_array[i]->Destinate_Processor_id, target_node_array[i]->DTask_id, target_node_array[i]->DSubTask_id, target_node_array[i]->Data_number);
							}
							Distributed_Data_t* Send_S = pvPortMalloc(sizeof(Distributed_Data_t));				//	Send result back to  caller, important
							Send_S->Data_addr = pvPortMalloc(Total_result_size*sizeof(uint32_t));;
							Send_S->Data_size = Total_result_size;
							Send_S->xQueue = Subtask_node_zero->xQueue;

							DistributedNodeDisablePublish();													//	Ready to recycle result, Node_id: target_node_array, size: Total_result_size
							uint32_t* tmp_Target_Addr = Send_S->Data_addr;
							for(uint32_t i=0;i<target_node_count;i++){
								printf("Recycle Result, DTask_id: 0x%lX, DSubTask_id: 0x%lX, Data_number: 0x%lX\r\n", target_node_array[i]->DTask_id, target_node_array[i]->DSubTask_id, target_node_array[i]->Data_number);
								if(target_node_array[i]->DSubTask_id == 0){
									uint32_t* result_addr = target_node_array[i]->Data_addr;
									for(uint32_t j=0;j<(target_node_array[i]->Data_number);j++){
										*(tmp_Target_Addr+j)= *(result_addr+j);
										printf("%d, 0x%lX, 0x%lX\r\n", (int)j, (uint32_t)(tmp_Target_Addr+j), *(tmp_Target_Addr+j));
									}
								}
								else{
									while(1){
										printf("DistributedNodeRequestResult, Destinate_Processor_id: 0x%lX, DTask_id: 0x%lX, DSubTask_id: 0x%lX\r\n", target_node_array[i]->Destinate_Processor_id, target_node_array[i]->DTask_id, target_node_array[i]->DSubTask_id);
										DistributedNodeRequestResult(target_node_array[i]->Destinate_Processor_id, target_node_array[i]->DTask_id, target_node_array[i]->DSubTask_id);
										uint32_t base_tick = xTaskGetTickCount();
										uint32_t timeout_tick = base_tick + 800000;
										while(RequestResultFlag == 0){
											uint32_t now_tick = xTaskGetTickCount();
											if(timeout_tick > base_tick){
												if((now_tick > timeout_tick) || (now_tick < base_tick))
													break;
											}
											else{
												if((now_tick > timeout_tick) && (now_tick < base_tick))
													break;
											}
										}
										if(RequestResultFlag == target_node_array[i]->Destinate_Processor_id){
											portDISABLE_INTERRUPTS();
											uint8_t* frame_addr = (uint8_t*)((DMA_RX_FRAME_infos->FS_Rx_Desc)->Buffer1Addr);
											uint32_t* result_addr = (uint32_t*)((uint8_t*)frame_addr+13);
											for(uint32_t j=0;j<target_node_array[i]->Data_number;j++){
												*(tmp_Target_Addr+j)= *(result_addr+j);
												printf("0x%lX, 0x%lX\r\n", (uint32_t)(tmp_Target_Addr+j), *(tmp_Target_Addr+j));
											}
											portENABLE_INTERRUPTS();
											RequestResultFlag = 0;
											break;
										}
										else{
											printf("Dame, Timeout, without feedback, Destinate_Processor_id: 0x%lX, DTask_id: 0x%lX, DSubTask_id: 0x%lX\r\n", target_node_array[i]->Destinate_Processor_id, target_node_array[i]->DTask_id, target_node_array[i]->DSubTask_id);
										}
									}
								}
								tmp_Target_Addr += target_node_array[i]->Data_number;
							}
							if(before_target_node != NULL)
								before_target_node->Next_TaskHandle_List = Last_target_node->Next_TaskHandle_List;
							else{
								DFinish = Last_target_node->Next_TaskHandle_List;
							}
							Last_target_node->Next_TaskHandle_List = NULL;
							Lastnode = Targetnodehead;
							while(Lastnode != NULL){
								Lastnode = Lastnode->Next_TaskHandle_List;
							}
							Lastnode = Targetnodehead;
							DistributedNodeRemoveTask(Global_Node_id, Lastnode->DTask_id);
							while(Lastnode != NULL){
								Distributed_TaskHandle_List_t* tmp_node = Lastnode;
								Lastnode = Lastnode->Next_TaskHandle_List;
								vPortFree(tmp_node);
							}
							DistributedNodeEnablePublish();
							xQueueSendToBack((void*)Send_S->xQueue, (void*)Send_S, 0);
							TaskDoneFlag = 0;
						}
						else{
							printf("Can't find target node\r\n");
						}
					}

					/*
					if((Global_Node_Master == Global_Node_id) && (Global_Node_count > Global_Node_id) && (Global_Node_Backup_Master <= Global_Node_id) && (SendFreespaceFlag == 0)){
						for(uint32_t i=(Global_Node_id+1);i<=Global_Node_count;i++){
							while(!(DistributedNodeCheck(i)));
							TO_COUNT = 0;
							while((TO_COUNT < 1680000) && (DisrtibutedNodeCheckIDFlag != 0)){
								TO_COUNT++;
							}
							if (TO_COUNT < 1680000){
								printf("Dispatch BackupMaster to %dth Node\r\n", i);
								DistributedNodeBackupMaster(i);
								Global_Node_Backup_Master = i;
								break;
							}
							else{
								portDISABLE_INTERRUPTS();
								DisrtibutedNodeCheckIDFlag = 0;
								portENABLE_INTERRUPTS();
								printf("Timeout, %dth Node not exist\r\n", i);
							}
						}
					}
					*/
					/*
					if((BlockChangeFlag > 0) && (PublishFlag > 0)){
						uint8_t bool_send_flag = 0;
						uint32_t tickcount = xTaskGetTickCount();
						if(tickcount_hi_bound > tickcount_lo_bound){
							if((tickcount>tickcount_hi_bound) || (tickcount<tickcount_lo_bound)){
								bool_send_flag = 1;
							}
						}
						else{
							if((tickcount>tickcount_hi_bound) && (tickcount<tickcount_lo_bound)){
								bool_send_flag = 1;
							}
						}
						if(bool_send_flag != 0){
							DistributedNodeSendFreespace(0xffffffff, Global_Node_id);
						}
					}
					*/
					if(Check_Sendable()){
						if(BlockChangeFlag > 0){
							DistributedNodeSendFreespace(0xffffffff, Global_Node_id);
						}
					}
					while(DDelete != NULL){
						portDISABLE_INTERRUPTS();
						Distributed_TaskHandle_List_t* remove_node = DDelete;
						DDelete = DDelete->Next_TaskHandle_List;
						portENABLE_INTERRUPTS();
						printf("Remove node from DDelete List, Source_Processor_id: 0x%lX, DTask_id: 0x%lX, DSubTask_id: 0x%lX\r\n", remove_node->Source_Processor_id, remove_node->DTask_id, remove_node->DSubTask_id);
						vPortFree(remove_node);
					}
				}
				rec_cmd = '\0';
			}
		}
	}
}

void Distributed_task(void *task_info){
	Distributed_TaskHandle_List_t *data_info = Distributed_Start(task_info);
	Distributed_Data_t* array1 = Distributed_Get_Traget_Data(data_info);
	Distributed_Data_t* array2 = Distributed_Get_Traget_Data(data_info);
	Distributed_Data_t* array3 = Distributed_Get_Traget_Data(data_info);

	for(uint32_t i=0;i<array1->Data_size;i++){
		//*(array1->Data_addr + i) = *(array1->Data_addr + i)+1;
		*(array1->Data_addr + i) = *(array1->Data_addr + i)*2;
	}
	for(uint32_t i=0;i<array2->Data_size;i++){
		//*(array2->Data_addr + i) = *(array2->Data_addr + i)+1;
		*(array2->Data_addr + i) = *(array2->Data_addr + i);
	}

	for(uint32_t i=0;i<array3->Data_size;i++){
		//*(array3->Data_addr + i) = *(array3->Data_addr + i)+1;
		*(array3->Data_addr + i) = *(array3->Data_addr + i);
	}

	Distributed_End(data_info, array1->Data_addr, array1->Data_size);
}

void task3(){
	printf("task3\r\n");
	led_init(LED_BLUE);
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

void eth_send(void){
	printf("eth_send\r\n");
	uint8_t MyMacAddr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	while(!init_eth(DP83848_PHY_ADDRESS, MyMacAddr)){
		printf("Reset eth\r\n");
		for(uint32_t i=0;i<0x00000FFF;i++)
			;
	}

	printf("init_eth success\r\n");
	uint8_t mydata[60] = {	 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
							 0x00, 0x01, 0x08, 0x06, 0x00, 0x01, 0x08, 0x00, 0x06, 0x04,
							 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xc0, 0xa8,
							 0x02, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xa8,
							 0x02, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
							 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00};
	uint8_t green_led = 0;
	uint8_t send_count = 0;
	while(1){
		uint32_t clock = 8400000;
		while(clock--);
		mydata[59] = send_count;

		uint8_t Send_success_flag = 0;
		while(!Send_success_flag){
			Send_success_flag = DP83848Send(mydata, 60);
			if (!Send_success_flag){
				while(!init_eth(DP83848_PHY_ADDRESS, MyMacAddr)){
					printf("Reset eth\r\n");
					for(uint32_t i=0;i<0x00000FFF;i++)
						;
				}
				send_count = 0;
			}
		}
		printf("DP83848Send: %d\r\n", send_count);

		send_count++;
		if (send_count>255)
			send_count = 0;

		if (green_led==0){
			SET_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_BSRR_OFFSET, BSy_BIT(LED_GREEN));
			green_led = 1;
		}
		else{
			SET_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_BSRR_OFFSET, BRy_BIT(LED_GREEN));
			green_led = 0;
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DistributedNodeGetID(){
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[13] = {  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
	}
	DistributedSendMsg(MyMacAddr, mydata, 13);
	printf("Broadcast DistributedNodeGetID\r\n");
}

void DistributedNodeGetIDAgain(){
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[13] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
	}
	DistributedSendMsg(MyMacAddr, mydata, 13);
	printf("Broadcast DistributedNodeGetIDAgain\r\n");
}

void DistributedNodeResponseID(){
	uint32_t Dispatch_id = Global_Node_count + 1;
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[17] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+13] = *((uint8_t*)&Dispatch_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
	}
	DistributedSendMsg(MyMacAddr, mydata, 17);
	printf("Send DistributedNodeResponseID, Dispatch_id: 0x%lX\r\n", Dispatch_id);
}

uint8_t DistributedNodeCheck(uint32_t Target_Node_id, uint32_t Needsize){
	if (DisrtibutedNodeCheckIDFlag == 0){
		portDISABLE_INTERRUPTS();
		DisrtibutedNodeCheckIDFlag = Target_Node_id;
		portENABLE_INTERRUPTS();
		uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
		uint8_t mydata[17] = { 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00};
		for(uint8_t i=0;i<4;i++){
			MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
			mydata[i+2] = *((uint8_t*)&Target_Node_id+i);
			mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
			mydata[i+13] = *((uint8_t*)&Needsize+i);
		}
		DistributedSendMsg(MyMacAddr, mydata, 17);
		printf("Send DistributedNodeCheck to Node 0x%lX\r\n", Target_Node_id);
		return 1;
	}
	else
		return 0;
}

void DistributedNodeCheckback(uint32_t Target_Node_id, uint8_t checkback_flag){
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[14] = { 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+2] = *((uint8_t*)&Target_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
	}
	mydata[13] = *((uint8_t*)&checkback_flag);
	DistributedSendMsg(MyMacAddr, mydata, 14);
	printf("Send DistributedNodeCheckback to Node 0x%lX, checkback_flag: 0x%X\r\n", Target_Node_id, checkback_flag);
}

void DistributedNodeBackupMaster(uint32_t Target_Node_id){
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[13] = { 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+2] = *((uint8_t*)&Target_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
	}
	DistributedSendMsg(MyMacAddr, mydata, 13);
	printf("Send DistributedNodeBackupMaster to Node 0x%lX\r\n", Target_Node_id);
}

void DistributedNodeInvalid(uint32_t Target_Node_id){
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[17] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+13] = *((uint8_t*)&Target_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
	}
	DistributedSendMsg(MyMacAddr, mydata, 17);
	printf("Broadcast DistributedNodeInvalid Node 0x%lX\r\n", Target_Node_id);
}

uint32_t DistributedNodeSendFreespace(uint32_t Target_Node_id, uint32_t Node_id){
	if(Target_Node_id == 0)
		Target_Node_id = 0xFFFFFFFF;
	UpdateLocalFreeBlock();
	uint32_t node_number = 0;
	uint32_t block_number = 0;
	Distributed_FreeBlock* FreespaceStart = DF_Start;
	Distributed_FreeBlock* tmp_block = FreespaceStart;
	if(Node_id > 0){
		while((tmp_block != NULL) && (tmp_block->Node_id != Node_id)){
			tmp_block = tmp_block->Next_Distributed_FreeBlock;
		}
		if(tmp_block == NULL){
			printf("DistributedNodeSendFreespace Fail, Without Node_id: 0x%lX\r\n", Node_id);
			return 0;
		}
		else{
			node_number++;
			block_number += tmp_block->Block_number;
		}
	}
	else{
		while(tmp_block != NULL){
			node_number++;
			block_number += tmp_block->Block_number;
			tmp_block = tmp_block->Next_Distributed_FreeBlock;
		}
	}
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint32_t Send_size = 14+(node_number*sizeof(Distributed_FreeBlock))+block_number*sizeof(uint32_t);
	uint8_t mydata[Send_size];
	mydata[0] = 0xff;
	mydata[1] = 0xff;
	mydata[12] = 0x08;
	mydata[13] = node_number;
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+2] = *((uint8_t*)&Target_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
	}
	uint32_t tmp_node_number = 0;
	uint32_t tmp_node_data_count = 0;
	uint32_t source_node_count = 0;
	//printf("Source, node_number: 0x%lX--------------------------------------------\r\n", node_number);
	if(Node_id == 0){
		tmp_block = FreespaceStart;
		while(tmp_block != NULL){
			source_node_count++;
			//printf("tmp_block: 0x%lX, Node_id: 0x%lX, Block_number: 0x%lX, Block_size_array: 0x%lX\r\n", (uint32_t)tmp_block, tmp_block->Node_id, tmp_block->Block_number, (uint32_t)tmp_block->Block_size_array);
			for(uint8_t i=0;i<sizeof(Distributed_FreeBlock);i++){
				*((uint8_t*)(mydata+14+tmp_node_number*sizeof(Distributed_FreeBlock)+i)) = *((uint8_t*)tmp_block+i);
			}
			//printf("BLock: ");
			for(uint32_t i=0;i<tmp_block->Block_number;i++){
				for(uint32_t j=0;j<sizeof(uint32_t);j++){
					*((uint8_t*)(mydata+14+node_number*sizeof(Distributed_FreeBlock)+tmp_node_data_count)) = *((uint8_t*)(tmp_block->Block_size_array+i)+j);
					tmp_node_data_count++;
				}
				//printf("0x%lX, ", *((uint32_t*)(mydata+14+node_number*sizeof(Distributed_FreeBlock)+tmp_node_data_count-4)));
			}
			//printf("\r\n");
			tmp_node_number++;
			tmp_block = tmp_block->Next_Distributed_FreeBlock;
		}
	}
	else{
		if (tmp_block->Node_id == Node_id){
			printf("Send Specifid Node_id Block: 0x%lX\r\n", Node_id);
			//printf("tmp_block: 0x%lX, Node_id: 0x%lX, Block_number: 0x%lX, Block_size_array: 0x%lX\r\n", (uint32_t)tmp_block, tmp_block->Node_id, tmp_block->Block_number, (uint32_t)tmp_block->Block_size_array);
			for(uint8_t i=0;i<sizeof(Distributed_FreeBlock);i++){
				*((uint8_t*)(mydata+14+i)) = *((uint8_t*)tmp_block+i);
			}
			//printf("BLock: ");
			for(uint32_t i=0;i<tmp_block->Block_number;i++){
				for(uint32_t j=0;j<sizeof(uint32_t);j++){
					*((uint8_t*)(mydata+14+node_number*sizeof(Distributed_FreeBlock)+4*i+j)) = *((uint8_t*)tmp_block->Block_size_array+4*i+j);
				}
				//printf("0x%lX, ", *((uint32_t*)(mydata+14+node_number*sizeof(Distributed_FreeBlock)+4*i)));
			}
			//printf("\r\n");
		}
	}
	//	printf("Source End, source_node_count: 0x%lX----------------------------------\r\n", source_node_count);
	DistributedSendMsg(MyMacAddr, mydata, Send_size);
	BlockChangeFlag = 0;
	return 0;
}

void DistributedNodeSendSubtask(uint32_t Target_Node_id, uint8_t* Subtask_addr, uint32_t Subtask_size){
	printf("Subtask_size: 0x%lX\r\n", Subtask_size);
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	*((uint8_t*)Subtask_addr) = 0xff;
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		*(((uint8_t*)Subtask_addr+2)+i) = *((uint8_t*)&Target_Node_id+i);
		*(((uint8_t*)Subtask_addr+8)+i) = *((uint8_t*)&Global_Node_id+i);
		*((uint8_t*)Subtask_addr+12) = 0x09;
	}
	DistributedSendMsg(MyMacAddr, Subtask_addr, Subtask_size);
	printf("Broadcast DistributedNodeSubtask Node 0x%lX\r\n", Target_Node_id);
}

void DistributedNodeResponseSubtask(uint32_t Target_Node_id){
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[13] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+2] = *((uint8_t*)&Target_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
	}
	DistributedSendMsg(MyMacAddr, mydata, 13);
	printf("Broadcast DistributedNodeResponseSubtask Node\r\n");
}

void DistributedNodeDisablePublish(){
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[13] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0b};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
	}
	PublishFlag = 0;
	DistributedSendMsg(MyMacAddr, mydata, 13);
	printf("Broadcast DistributedNodeDisablePublish Node\r\n");
}

void DistributedNodeEnablePublish(){
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[13] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
	}
	PublishFlag = 1;
	DistributedSendMsg(MyMacAddr, mydata, 13);
	printf("Broadcast DistributedNodeEnablePublish Node\r\n");
}

void DistributedNodeSubtaskFinish(uint32_t Target_Node_id, uint32_t Task_id, uint32_t Subtask_id, uint32_t Size){
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[25] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+2] = *((uint8_t*)&Target_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+13] = *((uint8_t*)&Task_id+i);
		mydata[i+17] = *((uint8_t*)&Subtask_id+i);
		mydata[i+21] = *((uint8_t*)&Size+i);
	}
	DistributedSendMsg(MyMacAddr, mydata, 25);
	printf("Broadcast DistributedNodeSubtaskFinish to Node: 0x%lX, Task_id: 0x%lX, Subtask_id: 0x%lX, Size: 0x%lX\r\n", Target_Node_id, Task_id, Subtask_id, Size);
}

void DistributedNodeRequestResult(uint32_t Target_Node_id, uint32_t Task_id, uint32_t Subtask_id){
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[21] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+2] = *((uint8_t*)&Target_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+13] = *((uint8_t*)&Task_id+i);
		mydata[i+17] = *((uint8_t*)&Subtask_id+i);
	}
	DistributedSendMsg(MyMacAddr, mydata, 21);
	printf("Broadcast DistributedNodeRequestResult, Target_Node_id: 0x%lX, Task_id: 0x%lX, Subtask_id: 0x%lX\r\n", Target_Node_id, Task_id, Subtask_id);
}

void DistributedNodeResponseResult(uint32_t Target_Node_id, uint8_t* Result_addr, uint32_t Result_size){
	printf("Result_size: 0x%lX\r\n", Result_size);
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	*((uint8_t*)Result_addr) = 0xff;
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		*(((uint8_t*)Result_addr+2)+i) = *((uint8_t*)&Target_Node_id+i);
		*(((uint8_t*)Result_addr+8)+i) = *((uint8_t*)&Global_Node_id+i);
		*((uint8_t*)Result_addr+12) = 0x0f;
	}
	DistributedSendMsg(MyMacAddr, Result_addr, Result_size);
	printf("Broadcast DistributedNodeResponseResult Node 0x%lX\r\n", Target_Node_id);
}

void DistributedNodeRemoveTask(uint32_t Target_Node_id, uint32_t Task_id){
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[21] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+13] = *((uint8_t*)&Target_Node_id+i);
		mydata[i+17] = *((uint8_t*)&Task_id+i);
	}
	DistributedSendMsg(MyMacAddr, mydata, 21);
	printf("Broadcast DistributedNodeRemoveTask, Target_Node_id: 0x%lX, Task_id: 0x%lX\r\n", Target_Node_id, Task_id);
}

void DistributedSendMsg(uint8_t* MyMacAddr, uint8_t* Target_Addr, uint32_t Size){
	while(!init_eth(DP83848_PHY_ADDRESS, MyMacAddr)){
		printf("Reset eth\r\n");
		for(uint32_t i=0;i<0x00000FFF;i++)
			;
	}

	uint8_t Send_success_flag = 0;
	while(!Send_success_flag){
		Send_success_flag = DP83848Send(Target_Addr, Size);
		if (!Send_success_flag){
			while(!init_eth(DP83848_PHY_ADDRESS, MyMacAddr)){
				printf("Reset eth\r\n");
				for(uint32_t i=0;i<0x00000FFF;i++)
					;
			}
		}
	}
	tickcount_lo_bound = xTaskGetTickCount();
	tickcount_hi_bound = tickcount_lo_bound + 100000*Global_Node_count + 10;
}

uint8_t DistributedNodeCheckSizeTimeout(uint32_t tick, uint32_t Target_Node_id, uint32_t Needsize){
	if(Target_Node_id != Global_Node_id){
		uint32_t base_tick = xTaskGetTickCount();
		uint32_t timeout_tick = base_tick + tick;
		while(!(DistributedNodeCheck(Target_Node_id, Needsize)));
		uint32_t bool_timeout_flag = 0;
		uint8_t success_flag = 0xFF;
		printf("In DistributedNodeCheckSizeTimeout DisrtibutedNodeCheckIDFlag: 0x%lX\r\n", DisrtibutedNodeCheckIDFlag);
		while((bool_timeout_flag == 0) && (DisrtibutedNodeCheckIDFlag != 0)){
			uint32_t now_tick = xTaskGetTickCount();
			if(timeout_tick > base_tick){
				if((now_tick > timeout_tick) || (now_tick < base_tick))
					bool_timeout_flag = 1;
			}
			else{
				if((now_tick > timeout_tick) && (now_tick < base_tick))
					bool_timeout_flag = 1;
			}
		}
		if(DisrtibutedNodeCheckIDFlag == 0){
			printf("Got check back!?\r\n");
			success_flag = CheckbackFlag;
		}
		else{
			printf("Without check back, timeout!?\r\n");
			DisrtibutedNodeCheckIDFlag = 0;
		}
		return success_flag;
	}
	else{
		printf("Dame you should not check yourself node id: 0x%lX\r\n", Target_Node_id);
		return 0xff;
	}
}

void UpdateLocalFreeBlock(){
	//printf("  UpdateLocalFreeBlock Start\r\n");
	Distributed_FreeBlock* local_free_block = DF_Start;
	while((local_free_block != NULL) && (local_free_block->Node_id != Global_Node_id)){
		local_free_block = local_free_block->Next_Distributed_FreeBlock;
	}
	if(local_free_block == NULL){
		local_free_block = pvPortMalloc(sizeof(Distributed_FreeBlock));
		local_free_block->Node_id = Global_Node_id;
		local_free_block->Block_number = 0;
		Distributed_FreeBlock* tmp_free_block = DF_Start;
		while((tmp_free_block->Next_Distributed_FreeBlock != NULL) && ((tmp_free_block->Next_Distributed_FreeBlock)->Node_id <= Global_Node_id)){
			tmp_free_block = tmp_free_block->Next_Distributed_FreeBlock;
		}
		if(tmp_free_block != DF_Start){
			local_free_block->Next_Distributed_FreeBlock = tmp_free_block->Next_Distributed_FreeBlock;
			tmp_free_block->Next_Distributed_FreeBlock = local_free_block;
		}
		else{
			if (DF_Start == NULL){
				DF_Start = local_free_block;
			}
			else{
				if (DF_Start->Node_id > Global_Node_id){
					local_free_block->Next_Distributed_FreeBlock = DF_Start;
					DF_Start = local_free_block;
				}
				else{
					local_free_block->Next_Distributed_FreeBlock = DF_Start->Next_Distributed_FreeBlock;
					DF_Start->Next_Distributed_FreeBlock = local_free_block;
				}
			}
		}
	}

	uint32_t block_number = 0;
	BlockLink_t* tmp_block = &xStart;
	while(tmp_block != NULL){
		if(tmp_block->xBlockSize > 0){
			block_number++;
		}
		tmp_block = tmp_block->pxNextFreeBlock;
	}
	if(block_number != local_free_block->Block_number){
		if(local_free_block->Block_number > 0){
			vPortFree(local_free_block->Block_size_array);
			block_number = 0;
			tmp_block = &xStart;
			while(tmp_block != NULL){
				if(tmp_block->xBlockSize > 0){
					block_number++;
				}
				tmp_block = tmp_block->pxNextFreeBlock;
			}
		}
		local_free_block->Block_size_array = pvPortMalloc(block_number*sizeof(uint32_t));
	}
	block_number = 0;
	tmp_block = &xStart;
	while(tmp_block != NULL){
		if(tmp_block->xBlockSize > 0){
			*(local_free_block->Block_size_array+block_number) = tmp_block->xBlockSize;
			block_number++;
		}
		tmp_block = tmp_block->pxNextFreeBlock;
	}
	local_free_block->Block_number = block_number;

}

Distributed_FreeBlock* GetFreeBlockNode(uint32_t Node_id){
	Distributed_FreeBlock* free_block = DF_Start;
	while((free_block != NULL) && (free_block->Node_id != Node_id)){
		free_block = free_block->Next_Distributed_FreeBlock;
	}
	if(free_block == NULL){
		free_block = pvPortMalloc(sizeof(Distributed_FreeBlock));
		free_block->Node_id = Node_id;
		free_block->Block_number = 0;
		Distributed_FreeBlock* tmp_free_block = DF_Start;
		while((tmp_free_block->Next_Distributed_FreeBlock != NULL) && ((tmp_free_block->Next_Distributed_FreeBlock)->Node_id <= Node_id)){
			tmp_free_block = tmp_free_block->Next_Distributed_FreeBlock;
		}
		if(tmp_free_block != DF_Start){
			free_block->Next_Distributed_FreeBlock = tmp_free_block->Next_Distributed_FreeBlock;
			tmp_free_block->Next_Distributed_FreeBlock = free_block;
		}
		else{
			if (DF_Start == NULL){
				DF_Start = free_block;
			}
			else{
				if (DF_Start->Node_id > Node_id){
					free_block->Next_Distributed_FreeBlock = DF_Start;
					DF_Start = free_block;
				}
				else{
					free_block->Next_Distributed_FreeBlock = DF_Start->Next_Distributed_FreeBlock;
					DF_Start->Next_Distributed_FreeBlock = free_block;
				}
			}
		}
	}
	return free_block;
}

uint8_t Check_Sendable(){
	uint8_t bool_send_flag = 0;
	uint32_t tickcount = xTaskGetTickCount();
	if(tickcount_hi_bound > tickcount_lo_bound){
		if((tickcount>tickcount_hi_bound) || (tickcount<tickcount_lo_bound)){
			bool_send_flag = 1;
		}
	}
	else{
		if((tickcount>tickcount_hi_bound) && (tickcount<tickcount_lo_bound)){
			bool_send_flag = 1;
		}
	}
	if (PublishFlag == 0){
		bool_send_flag = 0;
	}
	return bool_send_flag;
}

void Distributed_Show_FreeBlock(){
	printf("\r\nStart---------------------------------------\r\n");
	Distributed_FreeBlock* free_block = DF_Start;
	while(free_block != NULL){
		printf("free_block Node_id: 0x%lX, Block_number: 0x%lX, Block_size_array: 0x%lX\r\n", free_block->Node_id, free_block->Block_number, (uint32_t)free_block->Block_size_array);
		printf("Block: ");
		for(uint32_t i=0;i<free_block->Block_number;i++)
			printf("0x%lX, ", *(free_block->Block_size_array+i));
		printf("\r\n");
		free_block = free_block->Next_Distributed_FreeBlock;
	}
	printf("End  ---------------------------------------\r\n\r\n");
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int main(void){
	DStart->Next_TaskHandle_List = NULL;
	init_usart1();
	led_init(LED_GREEN);
	led_init(LED_ORANGE);
	led_init(LED_RED);

	REG(AIRCR_BASE) = NVIC_AIRCR_RESET_VALUE | NVIC_PRIORITYGROUP_4;
	xTaskCreate(Distributed_Manager_Task, "Distributed_Manager_Task", 1000, NULL, 1, &TaskHandle_1);
	xTaskCreate(task3, "task3", 1000, NULL, 1, &TaskHandle_3);
	//xTaskCreate(eth_send, "eth_send", 1000, NULL, 1, &TaskHandle_1);
	vTaskStartScheduler();
	while(1)
		;
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
void swap(int *a, int *b){
    int temp = *a;
    *a = *b;
    *b = temp;
}
int Partition(int *arr, int front, int end){
    int pivot = arr[end];
    int i = front -1;
    for (int j = front; j < end; j++) {
        if (arr[j] < pivot) {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    i++;
    swap(&arr[i], &arr[end]);
    return i;
}
void QuickSort(int *arr, int front, int end){
    if (front < end) {
        int pivot = Partition(arr, front, end);
        QuickSort(arr, front, pivot - 1);
        QuickSort(arr, pivot + 1, end);
    }
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
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
