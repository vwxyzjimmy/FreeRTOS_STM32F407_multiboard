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
#include "ov7670.h"
#include "dcmi.h"
#define HEAP_MAX (32 * 1024) 	// FreeRTOS manage memory 128 KB

#define portTICK_PERIOD_US			( (uint32_t) 1000000 / SystemTICK_RATE_HZ )	//	FreeRTOS Context  switch freq

// FreeRTOS porting hook function
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern void vPortSVCHandler();
void vApplicationTickHook() {;}
void vApplicationStackOverflowHook() {;}
void vApplicationIdleHook() {;}
void vApplicationMallocFailedHook() {;}

// Distributed middleware interface for user
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Distributed_Data_t* Distributed_SetTargetData(uint32_t* data_addr, uint32_t data_size, uint32_t split_size);
void Distributed_AddTargetData(Distributed_Data_t* S, uint32_t* data_addr, uint32_t data_size, uint32_t split_size);
Distributed_Result* Distributed_CreateTask(void* task, Distributed_Data_t *s, uint32_t Stack_size, uint32_t barrier);

extern Distributed_TaskHandle_List_t* Distributed_Start(void *data_info);
//Distributed_End, Distributed_pvPortMalloc, Distributed_vPortFree are macro and defined in struct.h
Distributed_Data_t* Distributed_GetResult(Distributed_Result* Result);
void Distributed_FreeResult(Distributed_Data_t* Distributed_Data);

// Distributed middleware other function
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Dispatch task
Distributed_TaskHandle_List_t* Distributed_DispatchTask(void* data_info, uint32_t sp, uint32_t lr);

// Other
Distributed_TaskHandle_List_t* Distributed_GetNode(uint32_t Return_addr, Distributed_TaskHandle_List_t* Lastnode);
uint32_t Got_sp_minus_immediate(uint32_t addr);
void Distributed_LocalSubtaskDone(Distributed_TaskHandle_List_t* s, uint32_t* Result_Data_addr, uint32_t Result_Data_size);
void Distributed_InsertFinishNode(Distributed_TaskHandle_List_t* NewDTaskControlBlock);
extern void tri_svc();
extern void vPortFree_From_ISR( void *pv );
extern void *pvPortMalloc_From_ISR( size_t xWantedSize );
void ListFreeBlock();

// Quick sort in dispatch task
void swap(int *a, int *b);
int Partition(int *arr, int front, int end);
void QuickSort(int *arr, int front, int end);

// Distributed middleware ethernet driver
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

// Distributed middleware communicate function, including init node id , request/release priviledge, check node exist, dispatch subtask, subtask finish, retrieve result, complete sequence, update free memory information and Distributed_ManageTask.
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Middleware eth funciton
void Distributed_SendMsg(uint8_t* MyMacAddr, uint8_t* Target_Addr, uint32_t Size);
// Init node id
void Distributed_NodeGetID();
void Distributed_NodeGetIDAgain();
void Distributed_NodeResponseID();
uint8_t Distributed_NodeCheck(uint32_t Target_Node_id, uint32_t Needsize);
void Distributed_NodeCheckback(uint32_t Target_Node_id, uint8_t checkback_flag);
void Distributed_NodeBackupMaster(uint32_t Target_Node_id);
void Distributed_NodeInvalid(uint32_t Target_Node_id);
// Request/release priviledge
void Distributed_NodeDisablePublish(uint32_t Target_Node_id);
void Distributed_NodeEnablePublish(uint32_t Target_Node_id);
void Distributed_NodeResponsePublish(uint32_t Target_Node_id);
void Distributed_NodeRequestKey();
void Distributed_NodeReleaseKey();
void Distributed_NodeResponseKey(uint32_t Target_Node_id, uint8_t response_flag);
uint8_t Distributed_NodeRequestReleaseSequence(uint8_t DisableEnableFlag);
// Check node exist and size
uint8_t Distributed_NodeCheckSizeTimeout(uint32_t tick, uint32_t Target_Node_id, uint32_t Needsize);
// Dispatch subtask
void Distributed_NodeSendSubtask(uint32_t Target_Node_id, uint8_t* Subtask_addr, uint32_t Subtask_size);
void Distributed_NodeSendRemainSubtask(uint32_t Target_Node_id, uint8_t* Subtask_addr, uint32_t Subtask_size, uint32_t Remain_th);
void Distributed_NodeRecvSubtask(uint32_t Target_Node_id);
void Distributed_NodeRecvRemainSubtask(uint32_t Target_Node_id, uint32_t Remain_th);
// Subtask finish
void Distributed_NodeSubtaskFinish(uint32_t Target_Node_id, uint32_t Task_id, uint32_t Subtask_id, uint32_t Size);
void Distributed_NodeResponseSubtaskFinish(uint32_t Target_Node_id, uint32_t Target_Task_id, uint32_t Target_Subtask_id);
// Retrieve result
void Distributed_NodeRequestResult(uint32_t Target_Node_id, uint32_t Task_id, uint32_t Subtask_id);
void Distributed_NodeRequestRemainResult(uint32_t Target_Node_id, uint32_t Remain_th);
void Distributed_NodeResponseResult(uint32_t Target_Node_id, uint8_t* Result_addr, uint32_t Result_size);
void Distributed_NodeResponseRemainResult(uint32_t Target_Node_id, uint8_t* Result_addr, uint32_t Result_size, uint32_t Remain_th);
void Distributed_NodeRemoveTask(uint32_t Target_Node_id, uint32_t Source_Node_id, uint32_t Task_id);
void Distributed_NodeSendComplete(uint32_t Target_Node_id, uint8_t Remain_th);
// Complete sequence
uint8_t Distributed_NodeSendCompleteSequence(uint32_t Target_Node_id);
uint8_t Distributed_NodeRecvCompleteSequence(uint32_t Target_Node_id);
// Update free memory information
void Distributed_NodeSendFreeBlock(uint32_t Target_Node_id, uint32_t Node_id);
void UpdateLocalFreeBlock();
uint8_t Check_Sendable();
uint8_t Check_Sendable_Without_Limit();
Distributed_FreeBlock* GetFreeBlockNode(uint32_t Node_id);
void Distributed_Show_FreeBlock();
// Distributed manage task
void Distributed_ManageTask();
uint32_t WaitForFlag(volatile uint32_t* Flag_Addr, uint32_t timeout_time);

// UserDefine function
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Distributed task examples
void UserDefine_Distributed_Task(void *task_info);
void UserDefine_Distributed_Task_matrix_multiplication(void *task_info);
void UserDefine_Distributed_Task_do_nothing(void *task_info);
void UserDefine_Distributed_Task_multiple(void *task_info);
void UserDefine_Distributed_Task_2d_array_convolution(void *task_info);
void UserDefine_Distributed_Task_bgr_gray_transform(void *task_info);
void UserDefine_Distributed_Task_RSA(void *task_info);
void UserDefine_Distributed_Task_bgr_gray_transform_with_2D_convolution(void *task_info);

// Local task examples
void UserDefine_Local_Task_2d_array_convolution(uint32_t rec_Count);
void UserDefine_Local_Task_RSA(uint32_t rec_Count, uint32_t e_d, uint32_t n, uint32_t array_Data_size);
void UserDefine_Local_Task_bgr_gray_transform(uint32_t rec_Count);
void UserDefine_Local_Task_bgr_gray_transform_with_2D_convolution(uint32_t rec_Count);

// RSA fuinction
uint32_t checkPrime(uint32_t n)__attribute__((always_inline));
uint32_t findGCD(uint32_t n1, uint32_t n2)__attribute__((always_inline));
uint32_t powMod(uint32_t a, uint32_t b, uint32_t n)__attribute__((always_inline));
void public_key(uint32_t* e, uint32_t* d, uint32_t* n, uint32_t p, uint32_t q)__attribute__((always_inline));

// Other fuinction
void test_inline()__attribute__((always_inline));
void UserDefine_Task();
void LED_BLUE_TASK();

// Other peripheral function, not so important
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Exti0
void init_external_interrupt(void);
void exti0_handler(void);

// Usart3
void init_usart3(void);
void usart3_send_char(const char ch);
void print_sys(char str[255]);

// Distributed_task list
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern BlockLink_t xStart;
Distributed_TaskHandle_List_t* DStart = NULL;											//	Distributed_task list
Distributed_TaskHandle_List_t* DRequest = NULL;											//	Distributed_task list
Distributed_TaskHandle_List_t* DFinish = NULL;											//	Distributed_task Finish list
Distributed_TaskHandle_List_t* DDelete = NULL;											//	Distributed_task Delete list
Distributed_TaskHandle_List_t* DSendFinish = NULL;										//	Distributed_task DSendFinish list

// Distributed_task event flag, Distributed_ManageTask will check most of these flag to do something
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint8_t Msg_event = 0;
uint32_t Global_Node_id = 0;
uint32_t Global_Node_count = 0;
uint32_t Global_Node_Master = 0;
uint32_t Global_Node_Backup_Master = 0;
uint32_t Global_Node_Master_Token = 0;
uint32_t Global_Task_id = 0;
Distributed_FreeBlock* DF_Start;
volatile uint32_t DisrtibutedNodeCheckIDFlag = 0;
volatile uint8_t CheckMasterNodeFlag = 0;
volatile uint8_t SendFreeBlockFlag = 0;
volatile uint32_t RecvFreeBlockFlag = 0;
volatile uint32_t CheckbackFlag = 0;
volatile uint32_t ReceiveSubtaskFlag = 0;
volatile uint32_t PublishFlag = 1;
volatile uint32_t PublishResponseFlag = 0;
volatile uint32_t RequestKeyFlag = 0;
volatile uint32_t Local_RequestKeyFlag = 0;
volatile uint32_t ResponseKeyFlag = 0;
volatile uint32_t TaskDoneFlag = 0;
volatile uint32_t RequestResultFlag = 0;
volatile uint32_t DispatchSuccessFlag = 0;
volatile uint32_t RemainThFlag = 0;
volatile uint32_t ResponseSubtaskFinishFlag = 0;
volatile uint32_t ResponseSubtaskFinishTaskFlag = 0;
volatile uint32_t ResponseSubtaskFinishSubtaskFlag = 0;
volatile uint32_t ResponseResultFlag = 0;
volatile uint32_t ConfirmResultFlag = 0;
volatile uint32_t RemainThResultFlag = 0;
volatile uint32_t SendCompleteFlag = 0;
volatile uint32_t NodeInvalidFlag = 0;
extern volatile uint8_t BlockChangeFlag;

// Distributed_task timeout and clock
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint32_t tickcount_lo_bound = 0;
uint32_t tickcount_hi_bound = 0xFFFFFFFF;
uint32_t Unmerge_Finish_Distributed_task = 0;
const uint32_t timeout_freq = 200;
extern uint32_t SystemCoreClock;
extern uint32_t SystemTICK_RATE_HZ;
#define Timeout_Tick_Count (SystemTICK_RATE_HZ/timeout_freq)
#define PrintSendRecv 0

volatile uint8_t exti0_flag = 0;
TaskHandle_t TaskHandle_1;
TaskHandle_t TaskHandle_2;
TaskHandle_t TaskHandle_3;

// Use to debug and test performance
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint32_t DebugFlag = 0;
uint32_t SendFlag = 0;
uint32_t RecvFlag = 0;
uint32_t SubtaskFinishArray[] = {0, 0, 0, 0, 0, 0, 0, 0};

uint32_t global_record_time;
uint32_t global_record_time_dispatch_array[52];
uint32_t global_record_requestrelease_fail_count[4];
uint32_t global_record_fail_count[8];
uint32_t checksize_count = 0;
uint32_t record_subtask_time;
uint32_t record_subtask_end_time;
uint32_t global_record_data[8];
uint32_t send_recv_data_time[16];
uint32_t send_recv_data_time_count = 8;

// Camera variable
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern volatile uint8_t ov_rev_ok;
extern volatile uint8_t ov_frame;
extern volatile uint32_t datanum;
extern uint16_t camera_buffer[PIC_WIDTH*PIC_HEIGHT];

// Distributed middleware user interface
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Distributed_Data_t* Distributed_SetTargetData(uint32_t* data_addr, uint32_t data_size, uint32_t split_size){
	Distributed_Data_t* data_info = pvPortMalloc(sizeof(Distributed_Data_t));
	data_info->Data_addr = data_addr;
	data_info->Data_size = data_size;
	data_info->Split_size = split_size;
	data_info->Next_Distributed_Data = NULL;
	return data_info;
}

void Distributed_AddTargetData(Distributed_Data_t* data_info, uint32_t* data_addr, uint32_t data_size, uint32_t split_size){
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

Distributed_Result* Distributed_CreateTask(void* task, Distributed_Data_t* data_info, uint32_t Stack_size, uint32_t barrier){
	TaskHandle_t Now_TaskHandle = xTaskGetCurrentTaskHandle();
	QueueHandle_t* xQueue = pvPortMalloc(sizeof(QueueHandle_t));
	uint32_t* barrier_flag = NULL;
	if(barrier == 0){
		barrier_flag = pvPortMalloc(sizeof(uint32_t));
		*(barrier_flag) = 0;
	}
	*(xQueue) = xQueueCreate(1, sizeof(uint32_t*));
	data_info->TaskHandle = Now_TaskHandle;
	data_info->xQueue = xQueue;
	data_info->Barrier = barrier;
	data_info->barrier_flag = barrier_flag;
	TaskHandle_t* TaskHandle = pvPortMalloc(sizeof(TaskHandle_t));
	xTaskCreate(task, "Dtask", Stack_size, data_info, 1, TaskHandle);
	vTaskSuspend(NULL);
	Distributed_Data_t** Recv_S = pvPortMalloc(sizeof(Distributed_Data_t*));
	Distributed_Result* Result = pvPortMalloc(sizeof(Distributed_Result));
	Result->Queue = xQueue;
	Result->TaskHandle = TaskHandle;
	Result->Distributed_Data = Recv_S;
	Result->barrier_flag = barrier_flag;
	return Result;
}

Distributed_Data_t* Distributed_GetResult(Distributed_Result* Result){
	portENTER_CRITICAL();
	if(Result->barrier_flag != NULL){
		if(*(Result->barrier_flag) == 0){
			*(Result->barrier_flag) = 1;
		}
		else if(*(Result->barrier_flag) == 2){
			Distributed_TaskHandle_List_t* check_Lastnode = DFinish;
			while(check_Lastnode != NULL){
				if(check_Lastnode->barrier_flag == Result->barrier_flag){
					TaskDoneFlag = check_Lastnode->DTask_id;
					*(Result->barrier_flag) = 1;
					//printf("TaskDoneFlag in Distributed_GetResult: 0x%lX\r\n", TaskDoneFlag);
					break;
				}
				check_Lastnode = check_Lastnode->Next_TaskHandle_List;
			}
		}
	}
	portEXIT_CRITICAL();
	if(	xQueueReceive(*(Result->Queue), (Result->Distributed_Data), 0) != 0){
		vTaskDelete(*(Result->TaskHandle));
		portENTER_CRITICAL();
		vPortFree(Result->TaskHandle);
		vQueueDelete(*(Result->Queue));
		vPortFree(Result->Queue);
		Distributed_Data_t* tmp_result = *(Result->Distributed_Data);
		vPortFree(Result->Distributed_Data);
		if(Result->barrier_flag != NULL){
			vPortFree(Result->barrier_flag);
		}
		vPortFree(Result);
		portEXIT_CRITICAL();
		return tmp_result;
	}
	else{
		return NULL;
	}
}

void Distributed_FreeResult(Distributed_Data_t* Distributed_Data){
	portENTER_CRITICAL();
	vPortFree(Distributed_Data);
	vPortFree(Distributed_Data->Data_addr);
	portEXIT_CRITICAL();
}

// Distributed middleware dispatch function
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Dispatch task
Distributed_TaskHandle_List_t* Distributed_DispatchTask(void* data_info, uint32_t sp, uint32_t lr){
	uint8_t Get_key = 0;
	uint32_t get_key_flag = 0;
	global_record_time = xTaskGetTickCount();
	global_record_time_dispatch_array[0] = xTaskGetTickCount() - global_record_time;
	global_record_time_dispatch_array[4] = xTaskGetTickCount() - global_record_time;
	uint32_t tmp_global_record_data_6 = global_record_time;
	while(Get_key == 0){
		if((PublishFlag != 0) && (Local_RequestKeyFlag == 0))
			Get_key = Distributed_NodeRequestReleaseSequence(Request);
	}
	get_key_flag = 1;
	Local_RequestKeyFlag = 1;
	global_record_time_dispatch_array[5] = xTaskGetTickCount() - global_record_time;
	vPortEnterCritical();
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
		global_record_data[0] += Data_size_array[i]*4;
		Data_split_size_array[i] = tmp_Distributed_Data->Split_size;
		tmp_Distributed_Data = tmp_Distributed_Data->Next_Distributed_Data;
	}
	uint32_t *pc_start = 0;
	uint32_t *pc_end = 0;
	uint32_t *lr_addr = 0;

	uint32_t stack_size = 0;
	lr_addr = (uint32_t *)((lr & 0xFFFFFFFE)-4);								//	Get return addr

	uint32_t tmp_lr = lr & 0xFFFFFFFE;
	while(1){										//	To find the first push	{r7, sp} instruction	as the begin of distributed task text section
		stack_size = stack_size + Got_sp_minus_immediate(tmp_lr);				//	decode to find sp_minus_immediate instruction and accumulate the stack_size
		tmp_lr = (uint32_t)((uint16_t *)tmp_lr-1);
		if(*((uint16_t *)tmp_lr) == 0xe92d){
			break;
		}
		else{
			uint16_t tmp = (*((uint16_t *)tmp_lr) & 0xFF0F);					//	See thumb-2 Push and STMDB instruction, r0~r3 may used to calculate, so if push them, it should not the begin of function
			if((tmp == 0xb500) || (tmp == 0xb400)){
				break;
			}
		}
	}
	pc_start = (((uint32_t*)tmp_lr));											//	To find the secnod push	{r7, sp} instruction 	as the end of distributed task text section
	tmp_lr = lr & 0xFFFFFFFE;
	while(1){
		tmp_lr = (uint32_t)((uint16_t *)tmp_lr+1);
		if(*((uint16_t *)tmp_lr) == 0xe92d){
			break;
		}
		else{
			uint16_t tmp = (*((uint16_t *)tmp_lr) & 0xFF0F);					//	See thumb-2 Push and STMDB instruction, r0~r3 may used to calculate, so if push them, it should not the begin of function
			if((tmp == 0xb500) || (tmp == 0xb400)){
				break;
			}
		}
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
			/*
			for(uint32_t i=0;i<Fail_node_num;i++)
				printf("Fail_node: 0x%lX, Fail_node_num: 0x%lX\r\n", Fail_node[i], Fail_node_num);
			*/
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
		//printf("free_block_Max_without_fail_node: 0x%lX\r\n", split_num_without_fail_node);

		uint32_t decrease_node_num = 0;																				//	accumulate the number FreeBlock not enough node number
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
			#if(PrintSendRecv > 0)
				printf("Malloc 0: 0x%lX, 0x%lX,0x%lX, 0x%lX\r\n", (uint32_t)Distributed_dispatch_node, (uint32_t)Distributed_data_need_size, (uint32_t)TwoD_Data_size_split_record, (uint32_t)TwoD_Data_Max_size_split_record);
				printf("TwoD_Data_size_split_record[i]: ");
				for(uint32_t i=0;i<act_split_num;i++)
					printf(" 0x%lX,", (uint32_t)TwoD_Data_size_split_record[i]);
				printf("\r\n");
				printf("TwoD_Data_Max_size_split_record[i]: ");
				for(uint32_t i=0;i<act_split_num;i++)
					printf(" 0x%lX,", (uint32_t)TwoD_Data_Max_size_split_record[i]);
				printf("\r\n");
			#endif
																														//	Split datas into property size
			for(uint32_t split_num_th=0;split_num_th<act_split_num;split_num_th++){										//	For loop for every node
				uint32_t Data_size_split = 0;
				for(uint32_t Data_number_th=0;Data_number_th<Data_number;Data_number_th++){								//	For loop for every data
					uint32_t tmp_data_size = 0;
					uint32_t split_base_data_size = Data_size_array[Data_number_th];									//	Data_number_th data total size
					if(Data_split_size_array[Data_number_th] > 1){														//	If indicate minimum split size
						split_base_data_size = Data_size_array[Data_number_th]/Data_split_size_array[Data_number_th];	//	split_base_data_size = total size / minimum split size
					}
					else if(Data_split_size_array[Data_number_th] == 1){
						Data_split_size_array[Data_number_th] = 1;
					}
					else if(Data_split_size_array[Data_number_th] == 0){
						Data_split_size_array[Data_number_th] = 0;
					}
					if(Data_split_size_array[Data_number_th] != 0){
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
					}
					else{
						tmp_data_size = split_base_data_size;
						TwoD_Data_Max_size_split_record[split_num_th][Data_number_th] = tmp_data_size;
					}
					//printf("split_num_th: 0x%lX, Data_number_th: 0x%lX, tmp_data_size: 0x%lX\r\n", split_num_th, Data_number_th, tmp_data_size);
					TwoD_Data_size_split_record[split_num_th][Data_number_th] = tmp_data_size;
					Data_size_split += tmp_data_size;
				}
				Distributed_data_need_size[split_num_th] = Data_size_split*sizeof(uint32_t) + Distributed_subtask_size;
			}
			uint8_t Local_satisfy_subtask_flag = 0;
			BlockLink_t* tmp_block = &xStart;															//	Check local FreeBlock whether satisfy Distributed_data_need_size[0]
			while(tmp_block != NULL){
				if(tmp_block->xBlockSize > Distributed_data_need_size[0]){
					Local_satisfy_subtask_flag = 1;
					break;
				}
				tmp_block = tmp_block->pxNextFreeBlock;
			}
			if(Local_satisfy_subtask_flag == 0){
				decrease_node_num = split_num_with_local + 1;											//	decrease_node_num = split_num_with_local + 1 mean local FreeBlock not enough to execute the task
				success_dispatch_flag = 0;
				printf("Local Freeblock not satisfy the minimum subtask size, dame it.\r\n");
				break;
			}
			else{
				Distributed_dispatch_node[0] = Global_Node_id;											// Check local FreeBlock satisfy Distributed_data_need_size[0]
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
						Distributed_dispatch_node[i] = free_block_Max_without_fail_node[0][decrease_node_num+j];	// decrease_node_num to base mean abandon least decrease_node_num node(FreeBlock is not enough)
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
			else{
																				//	dispatch fail, abandon the least FreeBlock node(decrease_node_num++), then try again
				vPortFree(Distributed_dispatch_node);							//	free Distributed_dispatch_node array
				vPortFree(Distributed_data_need_size);							//	free Distributed_dispatch_node array
				for(uint32_t i=0;i<act_split_num;i++){							//	free 2d TwoD_Data_Max_size_split_record and TwoD_Data_size_split_record array
					vPortFree(TwoD_Data_Max_size_split_record[i]);
					vPortFree(TwoD_Data_size_split_record[i]);
				}
				vPortFree(TwoD_Data_Max_size_split_record);
				vPortFree(TwoD_Data_size_split_record);
				#if(PrintSendRecv > 0)
					printf("Free 0: 0x%lX, 0x%lX,0x%lX, 0x%lX\r\n", (uint32_t)Distributed_dispatch_node, (uint32_t)Distributed_data_need_size, (uint32_t)TwoD_Data_size_split_record, (uint32_t)TwoD_Data_Max_size_split_record);
					printf("TwoD_Data_size_split_record[i]: ");
					for(uint32_t i=0;i<act_split_num;i++)
						printf(" 0x%lX,", (uint32_t)TwoD_Data_size_split_record[i]);
					printf("\r\n");
					printf("TwoD_Data_Max_size_split_record[i]: ");
					for(uint32_t i=0;i<act_split_num;i++)
						printf(" 0x%lX,", (uint32_t)TwoD_Data_Max_size_split_record[i]);
					printf("\r\n");
				#endif
				decrease_node_num++;
			}
		}
		if(success_dispatch_flag > 0){																		//	All node could be dispatch
			if(Fail_node_num > 0){
				vPortFree(Fail_node);
				#if(PrintSendRecv > 0)
					printf("Free 1-1 Fail_node: 0x%lX\r\n", (uint32_t)Fail_node);
				#endif
				Fail_node_num = 0;
			}
																											//	Check the node exist and FreeBlock enough
			uint32_t tmp_invalid_node[act_split_num];
			for(uint32_t split_num_th=0;split_num_th<act_split_num;split_num_th++){
				uint32_t checkout_limit = 5;
				uint32_t checkout_count = 0;
				if(Distributed_dispatch_node[split_num_th] != Global_Node_id){
					while(checkout_count<checkout_limit){
						vPortExitCritical();
						checksize_count++;
						uint8_t timeout_flag = Distributed_NodeCheckSizeTimeout(Timeout_Tick_Count, Distributed_dispatch_node[split_num_th], Distributed_data_need_size[split_num_th]);
						vPortEnterCritical();
						if(timeout_flag == 0xFF){																//	0xFF mean node not response, not exist
							#if(PrintSendRecv > 0)
								printf("Without check back, can't Distributed_NodeSendSubtask, invalid Node id: 0x%lX\r\n", Distributed_dispatch_node[split_num_th]);
							#endif
							tmp_invalid_node[split_num_th] = Distributed_dispatch_node[split_num_th];
							if(checkout_count == (checkout_limit-1))
								Fail_node_num++;
						}
						else if(timeout_flag == 0){																//	0 mean Node FreeBlock is not enough to dispatch
							#if(PrintSendRecv > 0)
								printf("Got check back but FreeBlock not satisfy, Node id: 0x%lX\r\n", Distributed_dispatch_node[split_num_th]);
							#endif
							tmp_invalid_node[split_num_th] = Distributed_dispatch_node[split_num_th];
							if(checkout_count == (checkout_limit-1))
								Fail_node_num++;
						}
						else{																					//	other mean the node exist and FreeBlock is enough to dispatch
							tmp_invalid_node[split_num_th] = 0;
							break;
							#if(PrintSendRecv > 0)
								printf("Got check back and ready to Distributed_NodeSendSubtask, Node id: 0x%lX\r\n", Distributed_dispatch_node[split_num_th]);
							#endif
							;
						}
						checkout_count++;
					}
				}
				else{
					tmp_invalid_node[split_num_th] = 0;
				}
			}
			#if(PrintSendRecv > 0)
				printf("Fail_node_num: 0x%lX\r\n", Fail_node_num);
			#endif
			if(Fail_node_num > 0){
				Fail_node = pvPortMalloc(Fail_node_num*sizeof(uint32_t));									//	Record Fail node (not exist or FreeBlock not enough)
				#if(PrintSendRecv > 0)
					printf("Malloc 1 Fail_node: 0x%lX\r\n", (uint32_t)Fail_node);
				#endif
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
		if(success_dispatch_flag > 0){
																										//	every Distributed_dispatch_node has been dispatch
			satisfy_split_num = act_split_num;
			if(Fail_node_num > 0){
				vPortFree(Fail_node);
				#if(PrintSendRecv > 0)
					printf("Free 1-2 Fail_node: 0x%lX\r\n", (uint32_t)Fail_node);
				#endif
				Fail_node_num = 0;
			}
			break;
		}
		else{
																			//	dispatch fail, because check node fail(FreeBlock not enough or without checkback), mark as Fail_node, then try again
			vPortFree(Distributed_dispatch_node);							//	free Distributed_dispatch_node array
			vPortFree(Distributed_data_need_size);							//	free Distributed_dispatch_node array
			for(uint32_t i=0;i<act_split_num;i++){							//	free 2d TwoD_Data_Max_size_split_record and TwoD_Data_size_split_record array
				vPortFree(TwoD_Data_Max_size_split_record[i]);
				vPortFree(TwoD_Data_size_split_record[i]);
			}
			vPortFree(TwoD_Data_Max_size_split_record);
			vPortFree(TwoD_Data_size_split_record);
			#if(PrintSendRecv > 0)
				printf("Free 1: 0x%lX, 0x%lX,0x%lX, 0x%lX\r\n", (uint32_t)Distributed_dispatch_node, (uint32_t)Distributed_data_need_size, (uint32_t)TwoD_Data_size_split_record, (uint32_t)TwoD_Data_Max_size_split_record);
				printf("TwoD_Data_size_split_record[i]: ");
				for(uint32_t i=0;i<act_split_num;i++)
					printf(" 0x%lX,", (uint32_t)TwoD_Data_size_split_record[i]);
				printf("\r\n");
				printf("TwoD_Data_Max_size_split_record[i]: ");
				for(uint32_t i=0;i<act_split_num;i++)
					printf(" 0x%lX,", (uint32_t)TwoD_Data_Max_size_split_record[i]);
				printf("\r\n");
			#endif
		}
	}

	if(satisfy_split_num == 0){
		printf("Dame it fail to dispatch\r\n");
	}
	else{
		for(uint32_t split_num_th=0;split_num_th<satisfy_split_num;split_num_th++){
			uint32_t Data_size_split = 0;
			for(uint32_t i=0;i<Data_number;i++)												//	Calculate sum of data size and copy to array
				Data_size_split += TwoD_Data_size_split_record[split_num_th][i];
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
			#if(PrintSendRecv > 0)
				printf("Malloc 2 Distributed_Send_Addr: 0x%lX\r\n", (uint32_t)Distributed_Send_Addr);
			#endif
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
					if(tmp_Distributed_Data->Split_size != 0)
						*(tmp_dest_data_addr+i) = *(tmp_Distributed_Data->Data_addr + split_num_th*Data_Max_size_split_record[Data_number_i] + i);
					else
						*(tmp_dest_data_addr+i) = *(tmp_Distributed_Data->Data_addr + i);
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
			NewDTaskControlBlock->DSubTask_number = satisfy_split_num;
			NewDTaskControlBlock->Instruction_addr = (uint32_t*)dest_instruction_addr;
			NewDTaskControlBlock->Instruction_addr_end = (uint32_t*)((uint8_t*)dest_instruction_addr + instruction_size);
			NewDTaskControlBlock->Data_addr = dest_data_addr;
			NewDTaskControlBlock->Data_size = Data_size_split_record;
			NewDTaskControlBlock->Data_Max_size  = Data_Max_size_split_record;
			NewDTaskControlBlock->Data_number = Data_number;
			NewDTaskControlBlock->Remain_Data_number = 0;
			NewDTaskControlBlock->Stack_size = stack_size;
			NewDTaskControlBlock->Finish_Flag = 0;
			NewDTaskControlBlock->Barrier = ((Distributed_Data_t*)data_info)->Barrier;
			NewDTaskControlBlock->barrier_flag = ((Distributed_Data_t*)data_info)->barrier_flag;
			NewDTaskControlBlock->TaskHandlex = Subtask_handler;
			NewDTaskControlBlock->Distributed_Data_List = Start_Distributed_Data_List;
			NewDTaskControlBlock->Next_TaskHandle_List = NULL;

			if(split_num_th == 0){
				NewDTaskControlBlock->Subtask_all_size = Distributed_Send_Size;
				NewDTaskControlBlock->TaskHandlex = (TaskHandle_t*)(((Distributed_Data_t*)data_info)->TaskHandle);
				NewDTaskControlBlock->xQueue = ((Distributed_Data_t*)data_info)->xQueue;
				#if(PrintSendRecv > 0)
					printf("1 xQueue: 0x%lX\r\n", (uint32_t)(((Distributed_Data_t*)data_info)->xQueue));
				#endif
				//printf("1 xQueue: 0x%lX\r\n", (uint32_t)(((Distributed_Data_t*)data_info)->xQueue));
				Subscriber_task = NewDTaskControlBlock;
			}
			else{
				NewDTaskControlBlock->xQueue = NULL;
				uint32_t subtask_Distributed_TaskHandle_List_size = sizeof(Distributed_TaskHandle_List_t);							//	Copy Distributed_TaskHandle_List_t part
				Distributed_TaskHandle_List_t* tmp_NewDTaskControlBlock = (Distributed_TaskHandle_List_t *)pvPortMalloc(subtask_Distributed_TaskHandle_List_size);
				for(uint8_t i=0;i<sizeof(Distributed_TaskHandle_List_t);i++)
					*((uint8_t*)tmp_NewDTaskControlBlock+i) = *((uint8_t*)NewDTaskControlBlock+i);
				NewDTaskControlBlock = tmp_NewDTaskControlBlock;
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

			if(split_num_th != 0){
				//	After send to other board remember to free the msg, jsut Remain the Distributed_TaskHandle_List_t
				//	Distributed_dispatch_node[satisfy_split_num] is the destinate node, Distributed_dispatch_node[0] is local node id
				//	Distributed_dispatch_node[satisfy_split_num], Distributed_Send_Addr, Distributed_Send_Size
				#if(PrintSendRecv > 0)
					uint32_t* package_start_addr = (uint32_t*)((uint8_t*)Distributed_Send_Addr + 13);
					uint32_t* package_stop_addr;
				#endif
				uint32_t Remain_Send_Size = Distributed_Send_Size;
				uint32_t Send_Size = 0;
				uint8_t* Send_Addr = Distributed_Send_Addr;
				uint32_t Remain_th = 0;
				global_record_time_dispatch_array[36+NewDTaskControlBlock->DSubTask_id] = xTaskGetTickCount() - global_record_time;
				global_record_data[1] += Data_size_split*sizeof(uint32_t);
				global_record_data[2] += Remain_Send_Size;
				uint32_t tmp_global_record_data_3 = xTaskGetTickCount();
				while(Remain_Send_Size > 0){
					uint32_t Compare_Size = ETH_FRAM_SIZE;
					if(Send_Addr != Distributed_Send_Addr)						//	Not the first Package
						Compare_Size = (ETH_FRAM_SIZE-17);
					if(Remain_Send_Size > Compare_Size)
						Send_Size = Compare_Size;
					else
						Send_Size = Remain_Send_Size;
					DispatchSuccessFlag = 0;
					if(Send_Addr == Distributed_Send_Addr){
						Distributed_NodeSendSubtask(Distributed_dispatch_node[split_num_th], Send_Addr, Send_Size);	//	Dispatch by ethernet
					}
					else{
						Distributed_NodeSendRemainSubtask(Distributed_dispatch_node[split_num_th], Send_Addr, Send_Size, Remain_th);	//	Dispatch by ethernet
					}
					vPortExitCritical();
					WaitForFlag(&DispatchSuccessFlag, 1);
					vPortEnterCritical();
					if((DispatchSuccessFlag == Distributed_dispatch_node[split_num_th]) && (RemainThFlag == (Remain_th+1))){
						DispatchSuccessFlag = 0;
						RemainThFlag = 0;
						Remain_Send_Size -= Send_Size;
						Send_Addr += Send_Size;
						#if(PrintSendRecv > 0)
							package_stop_addr = (uint32_t*)Send_Addr-1;
						#endif
						Remain_th++;
						if(Remain_Send_Size <= 0){
							vPortExitCritical();
							uint8_t Flag = 0;
							uint32_t start_tickcount = xTaskGetTickCount();
							uint32_t stop_tickcount = 0;
							while((Flag == 0) && (stop_tickcount < 1)){
								Flag = Distributed_NodeSendCompleteSequence(Distributed_dispatch_node[split_num_th]);
								stop_tickcount = (xTaskGetTickCount() - start_tickcount)/SystemTICK_RATE_HZ;
							}
							vPortEnterCritical();
						}
					}
					else{
						global_record_fail_count[Distributed_dispatch_node[split_num_th]-1] = global_record_fail_count[Distributed_dispatch_node[split_num_th]-1]+1;
					}
				}
				uint32_t tmp_get_tickcount = xTaskGetTickCount();
				global_record_time_dispatch_array[40+NewDTaskControlBlock->DSubTask_id] = tmp_get_tickcount - global_record_time;
				global_record_time_dispatch_array[24+NewDTaskControlBlock->DSubTask_id] = tmp_get_tickcount - global_record_time;
				global_record_data[3] += tmp_get_tickcount - tmp_global_record_data_3;
				send_recv_data_time[split_num_th] += tmp_get_tickcount - tmp_global_record_data_3;
				//printf("All Send to 0x%lX, Distributed_Send_Size: 0x%lX Done\r\n", Distributed_dispatch_node[split_num_th], Distributed_Send_Size);
				#if(PrintSendRecv > 0)
					uint32_t package_size = (uint32_t)package_stop_addr - (uint32_t)package_start_addr;
					printf("start_addr: 0x%lX, stop_addr: 0x%lX, package_size: 0x%lX\r\n0x%lX, 0x%lX\r\n", (uint32_t)package_start_addr, (uint32_t)package_stop_addr, package_size, *((uint32_t*)package_start_addr), *((uint32_t*)package_stop_addr));
				#endif
				vPortFree(Distributed_Send_Addr);																					//	Free the origin part (already sent)
				#if(PrintSendRecv > 0)
					printf("Free 2 Distributed_Send_Addr: 0x%lX\r\n", (uint32_t)Distributed_Send_Addr);
				#endif
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
		#if(PrintSendRecv > 0)
			printf("Free 3: 0x%lX, 0x%lX,0x%lX, 0x%lX\r\n", (uint32_t)Distributed_dispatch_node, (uint32_t)Distributed_data_need_size, (uint32_t)TwoD_Data_size_split_record, (uint32_t)TwoD_Data_Max_size_split_record);
			printf("TwoD_Data_size_split_record[i]: ");
			for(uint32_t i=0;i<satisfy_split_num;i++)
				printf(" 0x%lX,", (uint32_t)TwoD_Data_size_split_record[i]);
			printf("\r\n");
			printf("TwoD_Data_Max_size_split_record[i]: ");
			for(uint32_t i=0;i<satisfy_split_num;i++)
				printf(" 0x%lX,", (uint32_t)TwoD_Data_Max_size_split_record[i]);
			printf("\r\n");
		#endif
		Distributed_Data_t* reomve_s = data_info;								//	remove	data_info list
		while(reomve_s != NULL){
			Distributed_Data_t* s_delete = reomve_s;
			reomve_s = reomve_s->Next_Distributed_Data;
			vPortFree(s_delete);
		}
	}
	//	Enable otehr processor to publish FreeBlock
	vPortExitCritical();
	global_record_time_dispatch_array[6] = xTaskGetTickCount() - global_record_time;
	Get_key = 0;
	while(Get_key == 0)
		Get_key = Distributed_NodeRequestReleaseSequence(Release);
	Local_RequestKeyFlag = 0;
	global_record_time_dispatch_array[7] = xTaskGetTickCount() - global_record_time;
	global_record_time_dispatch_array[1] = xTaskGetTickCount() - global_record_time;
	vTaskResume((TaskHandle_t)Subscriber_task->TaskHandlex);
	global_record_time_dispatch_array[24] = xTaskGetTickCount() - global_record_time;
	record_subtask_time = xTaskGetTickCount();
	if(get_key_flag > 0)
		global_record_data[6] += (record_subtask_time-tmp_global_record_data_6);
	return Subscriber_task;
}

// Other
Distributed_TaskHandle_List_t* Distributed_GetNode(uint32_t Return_addr, Distributed_TaskHandle_List_t* Lastnode){
	while(Lastnode != NULL){
		if (((uint32_t)Lastnode->Instruction_addr<=Return_addr) && (Return_addr<=(uint32_t)Lastnode->Instruction_addr_end)){
			break;
		}
		Lastnode = Lastnode->Next_TaskHandle_List;
	}
	return Lastnode;
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

void Distributed_LocalSubtaskDone(Distributed_TaskHandle_List_t* s, uint32_t* Result_Data_addr, uint32_t Result_Data_size){
	record_subtask_end_time = xTaskGetTickCount();
	global_record_time_dispatch_array[28] = xTaskGetTickCount() - global_record_time;
	vPortEnterCritical();
	uint32_t Subtask_star_addr = (uint32_t)s;
	uint32_t Subtask_total_size = s->Subtask_all_size;
	Distributed_TaskHandle_List_t* tmp_NewDTaskControlBlock = pvPortMalloc(sizeof(Distributed_TaskHandle_List_t) + Result_Data_size*sizeof(uint32_t));					//	Copy a DTCB and copy from sour DTCB
	if(tmp_NewDTaskControlBlock == NULL){
		uint32_t mallocsize = (sizeof(Distributed_TaskHandle_List_t) + Result_Data_size*sizeof(uint32_t));
		uint32_t largestblock = 0;
		BlockLink_t* block_ndoe = &xStart;
		while(block_ndoe != NULL) {
			if(block_ndoe->xBlockSize > largestblock)
				largestblock = block_ndoe->xBlockSize;
			block_ndoe = block_ndoe->pxNextFreeBlock;
		}
		//printf("Malloc size: 0x%lX, but largest free block: 0x%lX\r\n", mallocsize, largestblock);
		//printf("NOt enough size to malloc in Distributed_LocalSubtaskDone\r\n");
		Distributed_TaskHandle_List_t tmp_DTCB;
		for(uint8_t i=0;i<sizeof(Distributed_TaskHandle_List_t);i++)
			*(((uint8_t*)&tmp_DTCB)+i) = *((uint8_t*)s+i);
		vPortFree(s);
		tmp_NewDTaskControlBlock = pvPortMalloc(mallocsize);
		for(uint8_t i=0;i<sizeof(Distributed_TaskHandle_List_t);i++)
			*((uint8_t*)tmp_NewDTaskControlBlock+i) = *(((uint8_t*)&tmp_DTCB)+i);
	}
	else{
		for(uint8_t i=0;i<sizeof(Distributed_TaskHandle_List_t);i++)
			*((uint8_t*)tmp_NewDTaskControlBlock+i) = *((uint8_t*)s+i);
		vPortFree(s);																													//	Free sour DTCB
	}
	tmp_NewDTaskControlBlock->Next_TaskHandle_List = NULL;
	tmp_NewDTaskControlBlock->Data_addr = (uint32_t*)((uint8_t*)tmp_NewDTaskControlBlock + sizeof(Distributed_TaskHandle_List_t));											//	Copy result from sour data
	tmp_NewDTaskControlBlock->Data_number = Result_Data_size;
	//printf("\r\n	Result data, Subtask id: 0x%lX, Result_Data_size: 0x%lX\r\n", tmp_NewDTaskControlBlock->DSubTask_id, Result_Data_size);
	for(uint32_t i=0;i<Result_Data_size;i++)
		*(tmp_NewDTaskControlBlock->Data_addr+i) = *(Result_Data_addr+i);

	if (!(((uint32_t)Result_Data_addr < (Subtask_star_addr+Subtask_total_size) ) && ((uint32_t)Result_Data_addr >= Subtask_star_addr))){
		vPortFree(Result_Data_addr);
	}

	uint32_t* Result_addr = 0;
	uint32_t Result_size = 0;
	uint32_t allsubtaskdoneflag = 0;
	tmp_NewDTaskControlBlock->Finish_Flag = 1;
	Distributed_InsertFinishNode(tmp_NewDTaskControlBlock);																			//	Inser to Fiish DTCB list
	if(tmp_NewDTaskControlBlock->Source_Processor_id == Global_Node_id){															//	Check Task done, DStart list without task_id(all subtask done)
		uint32_t tmp_count = 0;
		Distributed_TaskHandle_List_t* check_Lastnode = DFinish;
		while(check_Lastnode != NULL){
			if((check_Lastnode->DTask_id == tmp_NewDTaskControlBlock->DTask_id) && (check_Lastnode->Source_Processor_id == Global_Node_id)){
				tmp_count++;
				if(tmp_NewDTaskControlBlock->Barrier > 0)
					Result_size += check_Lastnode->Data_number;
				if(tmp_count >= tmp_NewDTaskControlBlock->DSubTask_number){
					if(tmp_NewDTaskControlBlock->Barrier > 0)
						allsubtaskdoneflag = 1;
					else{
						if(*(tmp_NewDTaskControlBlock->barrier_flag) == 1){
							TaskDoneFlag = tmp_NewDTaskControlBlock->DTask_id;	//	???	Should add with distributed_getresult
							//printf("TaskDoneFlag in Distributed_LocalSubtaskDone: 0x%lX, barrier_flag: 0x%lX, *(barrier_flag): 0x%lX\r\n", TaskDoneFlag, (uint32_t)tmp_NewDTaskControlBlock->barrier_flag, *(tmp_NewDTaskControlBlock->barrier_flag));
						}
						else{
							*(tmp_NewDTaskControlBlock->barrier_flag) = 2;
						}

					}
					break;
				}
			}
			check_Lastnode = check_Lastnode->Next_TaskHandle_List;
		}
	}
	if(allsubtaskdoneflag > 0){
		Result_addr = pvPortMalloc(Result_size*sizeof(uint32_t));
		uint32_t Subtask_id_size[tmp_NewDTaskControlBlock->DSubTask_number];
		Distributed_TaskHandle_List_t* check_Lastnode = DFinish;
		while(check_Lastnode != NULL){
			if((check_Lastnode->DTask_id == tmp_NewDTaskControlBlock->DTask_id) && (check_Lastnode->Source_Processor_id == Global_Node_id)){
				Subtask_id_size[check_Lastnode->DSubTask_id] = check_Lastnode->Data_number;
			}
			check_Lastnode = check_Lastnode->Next_TaskHandle_List;
		}
		check_Lastnode = DFinish;
		while(check_Lastnode != NULL){
			if((check_Lastnode->DTask_id == tmp_NewDTaskControlBlock->DTask_id) && (check_Lastnode->Source_Processor_id == Global_Node_id)){
				uint32_t* Dest_Result_addr = Result_addr;
				for(uint32_t i=0;i<(check_Lastnode->DSubTask_id);i++)
					Dest_Result_addr += Subtask_id_size[i];
				for(uint32_t i=0;i<check_Lastnode->Data_number;i++)
					*(Dest_Result_addr+i) = *(check_Lastnode->Data_addr+i);
				if(check_Lastnode->DSubTask_id != 0)
					vPortFree(check_Lastnode->Data_addr);
				check_Lastnode->Data_addr = Dest_Result_addr;
			}
			check_Lastnode = check_Lastnode->Next_TaskHandle_List;
		}
		TaskDoneFlag = tmp_NewDTaskControlBlock->DTask_id;
		//printf("TaskDoneFlag in LocalSubtaskDone: 0x%lX\r\n", TaskDoneFlag);
	}
	/*
	printf("Subtask 0x%lX finish, Addr: 0x%lX, Size: 0x%lX\r\n", (uint32_t)tmp_NewDTaskControlBlock->DSubTask_id, (uint32_t)tmp_NewDTaskControlBlock->Data_addr, (uint32_t)tmp_NewDTaskControlBlock->Data_size);
	for(uint32_t i=0;i<5;i++)
		printf("%d, 0x%lX, 0x%lX, %d, 0x%lX, 0x%lX\r\n", (int)i, (uint32_t)(tmp_NewDTaskControlBlock->Data_addr+i), *(tmp_NewDTaskControlBlock->Data_addr+i), (int)(tmp_NewDTaskControlBlock->Data_number-5+i), (uint32_t)(tmp_NewDTaskControlBlock->Data_addr+tmp_NewDTaskControlBlock->Data_number-5+i), *(tmp_NewDTaskControlBlock->Data_addr+tmp_NewDTaskControlBlock->Data_number-5+i));
	*/
	vPortExitCritical();
	vTaskDelete(NULL);																												//	Kill local task(parameter eith NULL)
	printf("Entry while loop forever, Should not be here\r\n");
	while(1);
}

void Distributed_InsertFinishNode(Distributed_TaskHandle_List_t* NewDTaskControlBlock){
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

void ListFreeBlock(){
	vPortEnterCritical();
	BlockLink_t* tmp_block = &xStart;
	printf("------------------------------------------------------------\r\n");
	while((tmp_block->pxNextFreeBlock)!=NULL){
		tmp_block = tmp_block->pxNextFreeBlock;
		printf("Free  xBlockAddr	0x%lX, xBlockSize:	0x%lX\r\n", (uint32_t)tmp_block, (uint32_t)tmp_block->xBlockSize);
	}
	printf("------------------------------------------------------------\r\n");
	vPortExitCritical();
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
	}
	else if (svc_num == 2){
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
		if (Lastnode->DSubTask_id != 0){
			*((uint16_t*)(stacked_return_addr&0xFFFFFFFE)) = 0xe7fe;										//	modify return addr instruction to bx here
			Lastnode->Finish_Flag = 0;																		//	Finish_Flag = 0 mean that the data has not merge yet
			Distributed_InsertFinishNode(Lastnode);															//	Insert to Finish list
			Unmerge_Finish_Distributed_task++;
		}
	}
	else if (svc_num == 3){
		printf("Success trigger svc\r\n");
	}
	else if (svc_num == 4){
		uint32_t Malloc_Size = *(stack_frame_ptr);
		uint32_t* Malloc_Addr = (uint32_t*)pvPortMalloc_From_ISR(Malloc_Size);
		*(stack_frame_ptr) = (uint32_t)Malloc_Addr;
	}
	else if (svc_num == 5){
		uint32_t* Free_Addr = (uint32_t*)*(stack_frame_ptr);
		vPortFree_From_ISR(Free_Addr);
	}
	else if (svc_num == 6){
		printf("Inline code success svc\r\n");
	}
}

// Quick sort in dispatch task
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

// Distributed middleware ethernet function
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
	/*
	for(uint32_t i=0;i<0x0000FFFF;i++)	// PHY_RESET_DELAY
		;
	*/
	//ETH_Mode = ETH_MODE_FULLDUPLEX;
	ETH_Mode = ETH_MODE_HALFDUPLEX;
	ETH_Speed = ETH_SPEED_10M;
	//ETH_Speed = ETH_SPEED_100M;

	result = ETH_WritePHYRegister(PHYAddress, 0, ((ETH_Mode >> 3) | (ETH_Speed >> 1)));	//PHY_BCR	Disable PHY_AutoNegotiation
	if (result == 0){
		printf("Fail: PHY_BCR	Disable PHY_AutoNegotiation, result:	0x%lX\r\n", result);
		return 0;
	}
	/*
	for(uint32_t i=0;i<0x000FFFF;i++)	// PHY_RESET_DELAY
		;
	*/
	/*
	//Auto Negotiation-----------------------------------------------------------------------------------------------------------------------------------------------------------
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
	/*
	for(uint32_t i=0;i<0x0000FFFF;i++)
		;
	*/
	//ETH_MACFFR
	SET_BIT(ETHERNET_MAC_BASE + ETH_MACFFR_OFFSET, RA);
	CLEAR_BIT(ETHERNET_MAC_BASE + ETH_MACFFR_OFFSET, BFD);

	CLEAR_BIT(ETHERNET_MAC_BASE + ETH_MACFFR_OFFSET, PM);
	CLEAR_BIT(ETHERNET_MAC_BASE + ETH_MACFFR_OFFSET, HM);
	CLEAR_BIT(ETHERNET_MAC_BASE + ETH_MACFFR_OFFSET, HPF);

	CLEAR_BIT(ETHERNET_MAC_BASE + ETH_MACFFR_OFFSET, HU);
	/*
	for(uint32_t i=0;i<0x0000FFFF;i++)
		;
	*/
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
	/*
	for(uint32_t i=0;i<0x0000FFFF;i++)
		;
	*/
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
	for(uint32_t i=0;i<0x0000000F;i++)
		;
	SET_BIT(ETHERNET_MAC_BASE + ETH_MACCR_OFFSET, RE);
	for(uint32_t i=0;i<0x0000000F;i++)
		;
	SET_BIT(ETHERNET_MAC_BASE + ETH_DMAOMR_OFFSET, FTF);
	//for(uint32_t i=0;i<0x00000FFF;i++)
	//	;
	SET_BIT(ETHERNET_MAC_BASE + ETH_DMAOMR_OFFSET, ST);
	//for(uint32_t i=0;i<0x00000FFF;i++)
	//	;
	SET_BIT(ETHERNET_MAC_BASE + ETH_DMAOMR_OFFSET, DMAOMR_SR);
	for(uint32_t i=0;i<0x0000000F;i++)
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
		//printf("Error: ETHERNET DMA OWN descriptor\r\n");
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

void eth_handler(void){
	uint32_t uxSavedInterruptStatus  = (uint32_t)taskENTER_CRITICAL_FROM_ISR();
	/* Handles all the received frames */
	/* check if any packet received */
	FrameTypeDef frame;

	while(ETH_CheckFrameReceived()){
	    /* process received ethernet packet */
	    frame = Pkt_Handle();
	}
	//frame has bug here, the result is less than 0x20000000 ******************************************************************
	if (((uint32_t)frame.buffer > 0x20000000) && ((uint32_t)frame.buffer <= 0x20020000)){
		uint32_t Dest = *((uint32_t*)((uint8_t*)frame.buffer+2));
		uint32_t Sour = *((uint32_t*)((uint8_t*)frame.buffer+8));
		//==========================================================================
		tickcount_lo_bound = xTaskGetTickCount();
		uint32_t multi = 0;
		if( (Sour <= Global_Node_id) && (Sour > 0))
			multi = (Global_Node_id-Sour-1);
		else if((Sour > Global_Node_id) && (Sour <= Global_Node_count))
			multi = (Global_Node_id+(Global_Node_count-Sour)-1);
		else
			multi = (Global_Node_id-1);

		tickcount_hi_bound = tickcount_lo_bound + Timeout_Tick_Count*multi + 10;
		/*
		uint32_t tmp_event = *((uint32_t*)((uint8_t*)frame.buffer+12));
		if (	(tmp_event == Distributed_NodeGetID_MSG)					\
			||	(tmp_event == Distributed_NodeGetIDAgain_MSG)			\
			||	(tmp_event == Distributed_NodeCheck_MSG)					\
			||	(tmp_event == Distributed_NodeSendSubtask_MSG)			\
			||	(tmp_event == Distributed_NodeDisablePublish_MSG)		\
			||	(tmp_event == Distributed_NodeEnablePublish_MSG)			\
			||	(tmp_event == Distributed_NodeRequestKey_MSG)			\
			||	(tmp_event == Distributed_NodeReleaseKey_MSG)			\
			||	(tmp_event == Distributed_NodeSubtaskFinish_MSG)			\
			||	(tmp_event == Distributed_NodeRequestResult_MSG)			\
			||	(tmp_event == Distributed_NodeRequestRemainResult_MSG)	\
			||	(tmp_event == Distributed_NodeResponseResult_MSG)		\
			||	(tmp_event == Distributed_NodeResponseRemainResult_MSG)	\
		){
			if(Global_Node_id != Dest)
				tickcount_hi_bound += Timeout_Tick_Count;
		}
		*/


		//==========================================================================

		if ((Dest == 0xffffffff) || (Dest == Global_Node_id)){
			Msg_event = *((uint8_t*)frame.buffer+12);
			RecvFlag = Msg_event;

			if (Msg_event == Distributed_NodeGetID_MSG){
				if((Global_Node_Master == Global_Node_id) && (Global_Node_Master != 0)){
					//printf("Get Distributed_NodeGetID\r\n");
					Distributed_NodeResponseID();
				}
			}
			else if (Msg_event == Distributed_NodeGetIDAgain_MSG){
				if((Global_Node_Backup_Master == Global_Node_id) && (Global_Node_Backup_Master != 0)){
					#if(PrintSendRecv > 0)
						printf("Get Distributed_NodeGetIDAgain\r\n");
					#endif
					CheckMasterNodeFlag = 1;
				}
				else if((Global_Node_id == Global_Node_Master) && (Global_Node_Backup_Master == 0)){
					#if(PrintSendRecv > 0)
						printf("Master get Distributed_NodeGetIDAgain and no BackupMaster\r\n");
					#endif
					Distributed_NodeResponseID();
					printf("Master get Distributed_NodeGetIDAgain and no BackupMaster\r\n");
				}
			}
			else if (Msg_event == Distributed_NodeResponseID_MSG){
				#if(PrintSendRecv > 0)
					printf("Get Distributed_NodeResponseID\r\n");
				#endif
				Global_Node_Master = Sour;
				if(Global_Node_id == 0){
					Global_Node_count = *((uint32_t*)((uint8_t*)frame.buffer+13));
					Global_Node_id = Global_Node_count;
					#if(PrintSendRecv > 0)
						printf("Global_Node_id: 0x%lX, Global_Node_count: 0x%lX\r\n", Global_Node_id, Global_Node_count);
					#endif
					SendFreeBlockFlag = 1;																			// New Node to Master Node
				}
			}
			else if (Msg_event == Distributed_NodeCheck_MSG){
				#if(PrintSendRecv > 0)
					printf("Get Distributed_NodeCheck\r\n");
				#endif
				uint32_t Needsize = *((uint32_t*)((uint8_t*)frame.buffer+13));
				uint8_t FreeBlock_satisfy_flag = 1;
				if(Needsize > 0){
					FreeBlock_satisfy_flag = 0;
					BlockLink_t* tmp_block = &xStart;
					while(tmp_block != NULL){
						if(tmp_block->xBlockSize >= Needsize){
							FreeBlock_satisfy_flag = 1;
							break;
						}
						tmp_block = tmp_block->pxNextFreeBlock;
					}
				}
				Distributed_NodeCheckback(Sour, FreeBlock_satisfy_flag);
			}
			else if (Msg_event == Distributed_NodeCheckback_MSG){
				if(DisrtibutedNodeCheckIDFlag == Sour){
					CheckbackFlag = *((uint8_t*)frame.buffer+13);
					#if(PrintSendRecv > 0)
						printf("Get Distributed_NodeCheckback, checkback_flag: 0x%lX\r\n",  (uint32_t)CheckbackFlag);
					#endif
					DisrtibutedNodeCheckIDFlag = 0;
				}
			}
			else if (Msg_event == Distributed_NodeBackupMaster_MSG){
				#if(PrintSendRecv > 0)
					printf("Get Distributed_NodeBackupMaster\r\n");
				#endif
				printf("Get Distributed_NodeBackupMaster\r\n");
				Global_Node_Backup_Master = Global_Node_id;
			}
			else if (Msg_event == Distributed_NodeInvalid_MSG){
				#if(PrintSendRecv > 0)
					printf("Get Distributed_NodeInvalid\r\n");
				#endif
				uint32_t Invalid_Node = *((uint32_t*)((uint8_t*)frame.buffer+13));
				if(Invalid_Node == Global_Node_Master)
					Global_Node_Master = Sour;										//	?????	should be BackupMaster, need to fix
				NodeInvalidFlag = Sour;
			}
			else if (Msg_event == Distributed_NodeSendFreeBlock_MSG){
				#if(PrintSendRecv > 0)
					printf("Get Distributed_NodeSendFreeBlock\r\n");
				#endif
				if ((Sour <= Global_Node_count) || (Global_Node_id == Global_Node_Master)){
					RecvFreeBlockFlag = Sour;
				}
			}
			else if (Msg_event == Distributed_NodeSendSubtask_MSG){
				Distributed_TaskHandle_List_t* TmpDTaskControlBlock = (Distributed_TaskHandle_List_t*)((uint8_t*)frame.buffer+13);
				ReceiveSubtaskFlag = Sour;
				RemainThFlag = 0;
				Distributed_TaskHandle_List_t* Lastnode = DStart;
				while(Lastnode != NULL){
					if((TmpDTaskControlBlock->Source_Processor_id == Lastnode->Source_Processor_id) && (TmpDTaskControlBlock->DTask_id == Lastnode->DTask_id)){
						ReceiveSubtaskFlag = 0;
						break;
					}
					Lastnode = Lastnode->Next_TaskHandle_List;
				}
				Lastnode = DFinish;
				while(Lastnode != NULL){
					if((TmpDTaskControlBlock->Source_Processor_id == Lastnode->Source_Processor_id) && (TmpDTaskControlBlock->DTask_id == Lastnode->DTask_id)){
						ReceiveSubtaskFlag = 0;
						break;
					}
					Lastnode = Lastnode->Next_TaskHandle_List;
				}
				#if(PrintSendRecv > 0)
					printf("Get Distributed_NodeSendSubtask\r\n");
				#endif
			}
			else if (Msg_event == Distributed_NodeSendRemainSubtask_MSG){
				ReceiveSubtaskFlag = Sour;
				RemainThFlag = *((uint32_t*)((uint8_t*)frame.buffer+13));
				#if(PrintSendRecv > 0)
					printf("Get Distributed_NodeSendRemainSubtask\r\n");
				#endif
			}
			else if (Msg_event == Distributed_NodeRecvSubtask_MSG){
				DispatchSuccessFlag = Sour;
				RemainThFlag = 1;
				#if(PrintSendRecv > 0)
					printf("Get Distributed_NodeRecvSubtask\r\n");
				#endif
			}
			else if (Msg_event == Distributed_NodeRecvRemainSubtask_MSG){
				DispatchSuccessFlag = Sour;
				RemainThFlag = *((uint32_t*)((uint8_t*)frame.buffer+13));
				#if(PrintSendRecv > 0)
					printf("Get Distributed_NodeRecvRemainSubtask\r\n");
				#endif
			}
			else if (Msg_event == Distributed_NodeDisablePublish_MSG){
				PublishFlag = 0;
				Distributed_NodeResponsePublish(Sour);
				#if(PrintSendRecv > 0)
					printf("Get Distributed_NodeDisablePublish\r\n");
				#endif
			}
			else if (Msg_event == Distributed_NodeEnablePublish_MSG){
				PublishFlag = 1;
				Distributed_NodeResponsePublish(Sour);
				#if(PrintSendRecv > 0)
					printf("Get Distributed_NodeEnablePublish\r\n");
				#endif
			}
			else if (Msg_event == Distributed_NodeResponsePublish_MSG){
				PublishResponseFlag = Sour;
				#if(PrintSendRecv > 0)
					printf("Get Distributed_NodeResponsePublish\r\n");
				#endif
			}
			else if (Msg_event == Distributed_NodeRequestKey_MSG){
				if(Global_Node_id == Global_Node_Master){
					if((RequestKeyFlag == 0) || (RequestKeyFlag == Sour)){
						Distributed_NodeResponseKey(Sour, 1);
						RequestKeyFlag = Sour;
						PublishFlag = 0;
						#if(PrintSendRecv > 0)
							printf("Request Key from Node: 0x%lX\r\n", Sour);
						#endif
					}
					else{
						Distributed_NodeResponseKey(Sour, 0);
						#if(PrintSendRecv > 0)
							printf("Not Request Key from Node: 0x%lX, Node: 0x%lX occupy the key\r\n", Sour, RequestKeyFlag);
						#endif
					}
					#if(PrintSendRecv > 0)
						printf("Get Distributed_NodeRequestKey from Node: 0x%lX\r\n", Sour);
					#endif
				}
			}
			else if (Msg_event == Distributed_NodeReleaseKey_MSG){
				if(Global_Node_id == Global_Node_Master){
					if((RequestKeyFlag == Sour) || (RequestKeyFlag == 0)){
						Distributed_NodeResponseKey(Sour, 1);
						RequestKeyFlag = 0;
						PublishFlag = 1;
						#if(PrintSendRecv > 0)
							printf("Release Key from Node: 0x%lX\r\n", Sour);
						#endif
					}
					else{
						Distributed_NodeResponseKey(Sour, 0);
						#if(PrintSendRecv > 0)
							printf("Not Release Key from Node: 0x%lX, Node : 0x%lX occupy the key\r\n", Sour, RequestKeyFlag);
						#endif
					}
					#if(PrintSendRecv > 0)
						printf("Get Distributed_NodeReleaseKey from Node: 0x%lX\r\n", Sour);
					#endif
				}
			}
			else if (Msg_event == Distributed_NodeResponseKey_MSG){
				uint8_t getresponsekey = *((uint8_t*)frame.buffer+13);
				if(getresponsekey > 0){
					ResponseKeyFlag = Sour;
					#if(PrintSendRecv > 0)
						printf("Get Key from Mastar\r\n");
					#endif
				}
				else{
					ResponseKeyFlag = Global_Node_count + 1;
					#if(PrintSendRecv > 0)
						printf("Not Get Key from Master\r\n");
					#endif
				}
				#if(PrintSendRecv > 0)
					printf("Get Distributed_NodeResponseKey\r\n");
				#endif
			}
			else if (Msg_event == Distributed_NodeSubtaskFinish_MSG){
				uint32_t task_id = *((uint32_t*)((uint8_t*)frame.buffer+13));
				uint32_t subtask_id = *((uint32_t*)((uint8_t*)frame.buffer+17));
				uint32_t size = *((uint32_t*)((uint8_t*)frame.buffer+21));
				Distributed_TaskHandle_List_t* Lastnode = DStart;
				Distributed_TaskHandle_List_t* pre_Lastnode;
				while((!((Lastnode->Destinate_Processor_id == Sour)&&(Lastnode->DTask_id == task_id)&&(Lastnode->DSubTask_id == subtask_id))) && (Lastnode != NULL)){					//	Remove subtask TCB from DStart list
					pre_Lastnode = Lastnode;
					Lastnode = Lastnode->Next_TaskHandle_List;
				}
				if(Lastnode != NULL){
					global_record_time_dispatch_array[28+Lastnode->DSubTask_id] = xTaskGetTickCount() - global_record_time;

					//printf("in DStart, Destinate_Processor_id: 0x%lX 0x%lX, DTask_id: 0x%lX 0x%lX,  DSubTask_id: 0x%lX 0x%lX\r\n", Lastnode->Destinate_Processor_id, Sour, Lastnode->DTask_id, task_id, Lastnode->DSubTask_id, subtask_id);
					if(Lastnode == DStart)
						DStart = DStart->Next_TaskHandle_List;
					else
						pre_Lastnode->Next_TaskHandle_List = Lastnode->Next_TaskHandle_List;
					Lastnode->Next_TaskHandle_List = NULL;
					Lastnode->Data_number = size;
					Distributed_NodeResponseSubtaskFinish(Sour, task_id, subtask_id);
					if(Lastnode->Barrier == 0){
						Lastnode->Finish_Flag = 1;
						Distributed_InsertFinishNode(Lastnode);
						if(Lastnode->Source_Processor_id == Global_Node_id){							//	Check Task done, DStart list without Lastnode->DTask_id(all subtask done)
							uint32_t tmp_count = 0;
							Distributed_TaskHandle_List_t* check_Lastnode = DFinish;
							while(check_Lastnode != NULL){
								if((check_Lastnode->DTask_id == Lastnode->DTask_id) && (check_Lastnode->Source_Processor_id == Global_Node_id)){
									tmp_count++;
									if(tmp_count >= Lastnode->DSubTask_number){
										if(*(Lastnode->barrier_flag) == 1){
											TaskDoneFlag = Lastnode->DTask_id;			//	???	Should add with distributed_getresult
											//printf("TaskDoneFlag in Distributed_LocalSubtaskDone: 0x%lX, barrier_flag: 0x%lX, *(barrier_flag): 0x%lX\r\n", TaskDoneFlag, (uint32_t)Lastnode->barrier_flag, *(Lastnode->barrier_flag));
										}
										else{
											*(Lastnode->barrier_flag) = 2;
										}
										break;
									}
								}
								check_Lastnode = check_Lastnode->Next_TaskHandle_List;
							}
						}
					}
					else{
						if(Lastnode->Source_Processor_id == Global_Node_id){
							Distributed_TaskHandle_List_t* tmp_Lastnode = DRequest;
							if(DRequest == NULL)
								DRequest = Lastnode;
							else{
								while(tmp_Lastnode->Next_TaskHandle_List != NULL)
									tmp_Lastnode = tmp_Lastnode->Next_TaskHandle_List;
								tmp_Lastnode->Next_TaskHandle_List = Lastnode;
							}
							Lastnode->Next_TaskHandle_List = NULL;
						}
					}
				}
				else{
					Lastnode = DFinish;
					while((!((Lastnode->Destinate_Processor_id == Sour)&&(Lastnode->DTask_id == task_id)&&(Lastnode->DSubTask_id == subtask_id))) && (Lastnode != NULL)){					//	Remove subtask TCB from DStart list
						Lastnode = Lastnode->Next_TaskHandle_List;
					}
					if(Lastnode != NULL){
						//printf("in DFinish, Destinate_Processor_id: 0x%lX 0x%lX, DTask_id: 0x%lX 0x%lX,  DSubTask_id: 0x%lX 0x%lX\r\n", Lastnode->Destinate_Processor_id, Sour, Lastnode->DTask_id, task_id, Lastnode->DSubTask_id, subtask_id);
						Distributed_NodeResponseSubtaskFinish(Sour, task_id, subtask_id);
					}
					else{
						Lastnode = DRequest;
						while((!((Lastnode->Destinate_Processor_id == Sour)&&(Lastnode->DTask_id == task_id)&&(Lastnode->DSubTask_id == subtask_id))) && (Lastnode != NULL)){					//	Remove subtask TCB from DStart list
							Lastnode = Lastnode->Next_TaskHandle_List;
						}
						if(Lastnode != NULL){
							Distributed_NodeResponseSubtaskFinish(Sour, task_id, subtask_id);
						}
						else{
							//printf("Not here, Destinate_Processor_id: 0x%lX, DTask_id: 0x%lX,  DSubTask_id: 0x%lX\r\n", Sour, task_id, subtask_id);
							Distributed_NodeResponseSubtaskFinish(Sour, task_id, subtask_id);
						}
					}
				}
				#if(PrintSendRecv > 0)
					printf("Get Distributed_NodeSubtaskFinish, Node: 0x%lX, Task: 0x%lX, Subtask: 0x%lX\r\n", Sour, task_id, subtask_id);
				#endif
			}
			else if (Msg_event == Distributed_NodeResponseSubtaskFinish_MSG){
				uint32_t task_id = *((uint32_t*)((uint8_t*)frame.buffer+13));
				uint32_t subtask_id = *((uint32_t*)((uint8_t*)frame.buffer+17));
				ResponseSubtaskFinishFlag = Sour;
				ResponseSubtaskFinishTaskFlag = task_id;
				ResponseSubtaskFinishSubtaskFlag = subtask_id;
				#if(PrintSendRecv > 0)
					printf("Get Distributed_NodeResponseSubtaskFinish\r\n");
				#endif
			}
			else if (Msg_event == Distributed_NodeRequestResult_MSG){
				uint32_t task_id = *((uint32_t*)((uint8_t*)frame.buffer+13));
				uint32_t subtask_id = *((uint32_t*)((uint8_t*)frame.buffer+17));
				ResponseSubtaskFinishFlag = Sour;
				ResponseSubtaskFinishTaskFlag = task_id;
				ResponseSubtaskFinishSubtaskFlag = subtask_id;

				Distributed_TaskHandle_List_t* Lastnode = DFinish;
				while(!((Lastnode->Source_Processor_id == Sour) && (Lastnode->DTask_id == task_id) && (Lastnode->DSubTask_id == subtask_id)) && (Lastnode != NULL))
					Lastnode = Lastnode->Next_TaskHandle_List;
				if(Lastnode != NULL){
					//printf("Source_Processor_id: 0x%lX 0x%lX, task_id: 0x%lX 0x%lX, subtask_id: 0x%lX 0x%lX, \r\n", Lastnode->Source_Processor_id, Sour, Lastnode->DTask_id, task_id, Lastnode->DSubTask_id, subtask_id);
					ResponseResultFlag = (uint32_t)Lastnode;
					/*
					ConfirmResultFlag = Sour;
					RemainThResultFlag = 1;
					*/
				}
				else{
					Lastnode = DSendFinish;
					Distributed_TaskHandle_List_t* pre_Lastnode = DSendFinish;
					while(!((Lastnode->Source_Processor_id == Sour) && (Lastnode->DTask_id == task_id) && (Lastnode->DSubTask_id == subtask_id)) && (Lastnode != NULL)){
						pre_Lastnode = Lastnode;
						Lastnode = Lastnode->Next_TaskHandle_List;
					}
					if(Lastnode != NULL){
						if(Lastnode == DSendFinish)
							DSendFinish = Lastnode->Next_TaskHandle_List;
						else
							pre_Lastnode->Next_TaskHandle_List = Lastnode->Next_TaskHandle_List;
						Lastnode->Next_TaskHandle_List = NULL;
						Distributed_InsertFinishNode(Lastnode);
						ResponseResultFlag = (uint32_t)Lastnode;
					}
				}
				#if(PrintSendRecv > 0)
					printf("Get Distributed_NodeRequestResult\r\n");
				#endif
			}
			else if(Msg_event == Distributed_NodeRequestRemainResult_MSG){
				ConfirmResultFlag = Sour;
				RemainThResultFlag =  *((uint32_t*)((uint8_t*)frame.buffer+13));
				#if(PrintSendRecv > 0)
					printf("Get Distributed_NodeRequestRemainResult, ConfirmResultFlag: 0x%lX, RemainThResultFlag: 0x%lX\r\n", ConfirmResultFlag, RemainThResultFlag);
				#endif
			}
			else if(Msg_event == Distributed_NodeResponseResult_MSG){
				RequestResultFlag = Sour;
				RemainThResultFlag = 1;
				#if(PrintSendRecv > 0)
					printf("Get Distributed_NodeResponseResult\r\n");
				#endif
			}
			else if(Msg_event == Distributed_NodeResponseRemainResult_MSG){
				RequestResultFlag = Sour;
				RemainThResultFlag = *((uint32_t*)((uint8_t*)frame.buffer+13));
				#if(PrintSendRecv > 0)
					printf("Get Distributed_NodeResponseRemainResult\r\n");
				#endif
			}
			else if (Msg_event == Distributed_NodeRemoveTask_MSG){
				uint32_t processor_id = *((uint32_t*)((uint8_t*)frame.buffer+13));
				uint32_t task_id = *((uint32_t*)((uint8_t*)frame.buffer+17));
				Distributed_TaskHandle_List_t* Lastnode = DFinish;
				Distributed_TaskHandle_List_t* pre_Lastnode = DFinish;
				while(Lastnode != NULL){
					if((Lastnode->Source_Processor_id == processor_id) && (Lastnode->DTask_id == task_id)){
						ResponseSubtaskFinishFlag = Lastnode->DSubTask_id;
						if(Lastnode == DFinish){
							DFinish = Lastnode->Next_TaskHandle_List;
							pre_Lastnode = DFinish;
						}
						else
							pre_Lastnode->Next_TaskHandle_List = Lastnode->Next_TaskHandle_List;
						Lastnode->Next_TaskHandle_List = NULL;

						Distributed_TaskHandle_List_t* Insert_Lastnode = DDelete;
						if(DDelete != NULL){
							while(Insert_Lastnode->Next_TaskHandle_List != NULL){
								Insert_Lastnode = Insert_Lastnode->Next_TaskHandle_List;
							}
							Insert_Lastnode->Next_TaskHandle_List = Lastnode;
						}
						else
							DDelete = Lastnode;
						break;
					}
					pre_Lastnode = Lastnode;
					Lastnode = Lastnode->Next_TaskHandle_List;
				}
				#if(PrintSendRecv > 0)
					printf("Get Distributed_NodeRemoveTask\r\n");
				#endif
			}

			else if (Msg_event == Distributed_NodeSendComplete_MSG){
				SendCompleteFlag = (uint32_t)*((uint8_t*)frame.buffer+13);
				#if(PrintSendRecv > 0)
					printf("Get Distributed_NodeSendComplete, Remain_th: 0x%lX\r\n", (uint32_t)SendCompleteFlag);
				#endif
			}
			//printf("Node_id: 0x%lX, Node_count: 0x%lX, Master: 0x%lX, Backup_Master: 0x%lX, Dest: 0x%lX, Sour: 0x%lX\r\n", Global_Node_id, Global_Node_count, Global_Node_Master, Global_Node_Backup_Master, Dest, Sour);
		}
	}
	/* Clear the Eth DMA Rx IT pending bits */
	SET_BIT(ETHERNET_MAC_BASE + ETH_DMASR_OFFSET, RS);
	SET_BIT(ETHERNET_MAC_BASE + ETH_DMASR_OFFSET, NIS);
	taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
}

// Distributed middleware communicate function, including init node id , request/release priviledge, check node exist, dispatch subtask, subtask finish, retrieve result, complete sequence, update free memory information and Distributed_ManageTask.
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Middleware eth funciton
void Distributed_SendMsg(uint8_t* MyMacAddr, uint8_t* Target_Addr, uint32_t Size){
	init_eth(DP83848_PHY_ADDRESS, MyMacAddr);
	uint8_t Send_success_flag = 0;
	while(!Send_success_flag){
		Send_success_flag = DP83848Send(Target_Addr, Size);
		if (!Send_success_flag){
			while(!init_eth(DP83848_PHY_ADDRESS, MyMacAddr)){
				printf("Reset eth\r\n");
				for(uint32_t i=0;i<0x000000FF;i++)
					;
			}

		}
	}
	tickcount_lo_bound = xTaskGetTickCount();
	tickcount_hi_bound = tickcount_lo_bound + Timeout_Tick_Count*Global_Node_count + 10;
}

// init node id
void Distributed_NodeGetID(){
	SendFlag = Distributed_NodeGetID_MSG;
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[13] = {  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, Distributed_NodeGetID_MSG};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
	}
	Distributed_SendMsg(MyMacAddr, mydata, 13);
	#if(PrintSendRecv > 0)
		printf("Broadcast Distributed_NodeGetID\r\n");
	#endif
}

void Distributed_NodeGetIDAgain(){
	SendFlag = Distributed_NodeGetIDAgain_MSG;
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[13] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, Distributed_NodeGetIDAgain_MSG};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
	}
	Distributed_SendMsg(MyMacAddr, mydata, 13);
	#if(PrintSendRecv > 0)
		printf("Broadcast Distributed_NodeGetIDAgain\r\n");
	#endif
}

void Distributed_NodeResponseID(){
	SendFlag = Distributed_NodeResponseID_MSG;
	uint32_t Dispatch_id = Global_Node_count + 1;
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[17] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, Distributed_NodeResponseID_MSG, 0x00, 0x00, 0x00, 0x00};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+13] = *((uint8_t*)&Dispatch_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
	}
	Distributed_SendMsg(MyMacAddr, mydata, 17);
	#if(PrintSendRecv > 0)
		printf("Send Distributed_NodeResponseID, Dispatch_id: 0x%lX\r\n", Dispatch_id);
	#endif
}

uint8_t Distributed_NodeCheck(uint32_t Target_Node_id, uint32_t Needsize){
	SendFlag = Distributed_NodeCheck_MSG;
	if (DisrtibutedNodeCheckIDFlag == 0){
		DisrtibutedNodeCheckIDFlag = Target_Node_id;
		uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
		uint8_t mydata[17] = { 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, Distributed_NodeCheck_MSG, 0x00, 0x00, 0x00, 0x00};
		for(uint8_t i=0;i<4;i++){
			MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
			mydata[i+2] = *((uint8_t*)&Target_Node_id+i);
			mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
			mydata[i+13] = *((uint8_t*)&Needsize+i);
		}
		Distributed_SendMsg(MyMacAddr, mydata, 17);
		#if(PrintSendRecv > 0)
			printf("Send Distributed_NodeCheck to Node 0x%lX\r\n", Target_Node_id);
		#endif
		return 1;
	}
	else
		return 0;
}

void Distributed_NodeCheckback(uint32_t Target_Node_id, uint8_t checkback_flag){
	SendFlag = Distributed_NodeCheckback_MSG;
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[14] = { 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, Distributed_NodeCheckback_MSG, 0x00};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+2] = *((uint8_t*)&Target_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
	}
	mydata[13] = *((uint8_t*)&checkback_flag);
	Distributed_SendMsg(MyMacAddr, mydata, 14);
	#if(PrintSendRecv > 0)
		printf("Send Distributed_NodeCheckback to Node 0x%lX, checkback_flag: 0x%X\r\n", Target_Node_id, checkback_flag);
	#endif
}

void Distributed_NodeBackupMaster(uint32_t Target_Node_id){
	SendFlag = Distributed_NodeBackupMaster_MSG;
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[13] = { 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, Distributed_NodeBackupMaster_MSG};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+2] = *((uint8_t*)&Target_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
	}
	Distributed_SendMsg(MyMacAddr, mydata, 13);
	#if(PrintSendRecv > 0)
		printf("Send Distributed_NodeBackupMaster to Node 0x%lX\r\n", Target_Node_id);
	#endif
}

void Distributed_NodeInvalid(uint32_t Target_Node_id){
	SendFlag = Distributed_NodeInvalid_MSG;
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[17] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, Distributed_NodeInvalid_MSG, 0x00, 0x00, 0x00, 0x00};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+13] = *((uint8_t*)&Target_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
	}
	Distributed_SendMsg(MyMacAddr, mydata, 17);
	#if(PrintSendRecv > 0)
		printf("Broadcast Distributed_NodeInvalid Node 0x%lX\r\n", Target_Node_id);
	#endif
}

// request/release priviledge
void Distributed_NodeDisablePublish(uint32_t Target_Node_id){
	SendFlag = Distributed_NodeDisablePublish_MSG;
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[13] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, Distributed_NodeDisablePublish_MSG};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+2] = *((uint8_t*)&Target_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
	}
	PublishFlag = 0;
	Distributed_SendMsg(MyMacAddr, mydata, 13);
	#if(PrintSendRecv > 0)
		printf("Broadcast Distributed_NodeDisablePublish Node\r\n");
	#endif
}

void Distributed_NodeEnablePublish(uint32_t Target_Node_id){
	SendFlag = Distributed_NodeEnablePublish_MSG;
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[13] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, Distributed_NodeEnablePublish_MSG};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+2] = *((uint8_t*)&Target_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
	}
	PublishFlag = 1;
	Distributed_SendMsg(MyMacAddr, mydata, 13);
	#if(PrintSendRecv > 0)
		printf("Broadcast Distributed_NodeEnablePublish Node\r\n");
	#endif
}

void Distributed_NodeResponsePublish(uint32_t Target_Node_id){
	SendFlag = Distributed_NodeResponsePublish_MSG;
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[13] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, Distributed_NodeResponsePublish_MSG};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+2] = *((uint8_t*)&Target_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
	}
	Distributed_SendMsg(MyMacAddr, mydata, 13);
	#if(PrintSendRecv > 0)
		printf("Broadcast Distributed_NodeResponsePublish Node\r\n");
	#endif
}

void Distributed_NodeRequestKey(){
	SendFlag = Distributed_NodeRequestKey_MSG;
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[13] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, Distributed_NodeRequestKey_MSG};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
	}
	Distributed_SendMsg(MyMacAddr, mydata, 13);
	#if(PrintSendRecv > 0)
		printf("Broadcast Distributed_NodeRequestKey Node\r\n");
	#endif
}

void Distributed_NodeReleaseKey(){
	SendFlag = Distributed_NodeReleaseKey_MSG;
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[13] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, Distributed_NodeReleaseKey_MSG};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
	}
	Distributed_SendMsg(MyMacAddr, mydata, 13);
	#if(PrintSendRecv > 0)
		printf("Broadcast Distributed_NodeReleaseKey Node\r\n");
	#endif
}

void Distributed_NodeResponseKey(uint32_t Target_Node_id, uint8_t response_flag){
	SendFlag = Distributed_NodeResponseKey_MSG;
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[14] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, Distributed_NodeResponseKey_MSG, response_flag};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+2] = *((uint8_t*)&Target_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
	}
	Distributed_SendMsg(MyMacAddr, mydata, 14);
	#if(PrintSendRecv > 0)
		printf("Broadcast Distributed_NodeResponseKey Node\r\n");
	#endif
}

uint8_t Distributed_NodeRequestReleaseSequence(uint8_t DisableEnableFlag){
	if(DisableEnableFlag == 0){
		while(1){
			if(Global_Node_id == Global_Node_Master){
				vPortEnterCritical();
				if((RequestKeyFlag != 0) && (RequestKeyFlag != Global_Node_id))			//	RequestKeyFlag is occupy
					ResponseKeyFlag = Global_Node_count + 1;
				else{
					ResponseKeyFlag = Global_Node_id;									//	Request RequestKeyFlag
					RequestKeyFlag = Global_Node_id;
				}
				vPortExitCritical();
			}
			else{																		//	Local node is not Master, try to request or release by communication
				ResponseKeyFlag = 0;
				while(ResponseKeyFlag == 0){
					//while(!Check_Sendable_Without_Limit());
					if(PublishFlag > 0)
						Distributed_NodeRequestKey();									//	Request by communication
					WaitForFlag(&ResponseKeyFlag, 1);
				}
			}
			if(ResponseKeyFlag > Global_Node_count){									//	RequestKeyFlag is occupy, clear ResponseKeyFlag
				ResponseKeyFlag = 0;													//	Not Get the Key
				//PublishFlag = 0;
				return	0;
			}
			else{																		//	occupy RequestKeyFlag, clear ResponseKeyFlag
				PublishFlag = 0;
				ResponseKeyFlag = 0;
				break;
			}
		}
		for(uint32_t i=1;i<=Global_Node_count;i++){										//	Disable/Enable all node
			if((i != Global_Node_Master) && (i != Global_Node_id)){
				uint32_t Timeout_count_limit = 10;
				uint32_t Timeout_count = 0;
				while(PublishResponseFlag != i){
					///while(!Check_Sendable_Without_Limit());
					PublishResponseFlag = 0;
					Distributed_NodeDisablePublish(i);
					WaitForFlag(&PublishResponseFlag, 1);
					if(PublishResponseFlag == i){										//	Got ResponsePublish
						break;
					}
					else{
						global_record_requestrelease_fail_count[i-1]++;
						//	Should publish Invalid Node
						//	??????
						Timeout_count++;
						if(Timeout_count > Timeout_count_limit){
							//printf("Over time out limit: 0x%lX\r\n", Timeout_count_limit);
							break;
						}
					}
				}
			}
			else if (i == Global_Node_id){
				PublishFlag = 0;
			}
		}
	}
	else{
		for(uint32_t i=1;i<=Global_Node_count;i++){										//	Disable/Enable all node
			if((i != Global_Node_Master) && (i != Global_Node_id)){
				uint32_t Timeout_count_limit = 20;
				uint32_t Timeout_count = 0;
				while(PublishResponseFlag != i){
					//while(!Check_Sendable_Without_Limit());
					PublishResponseFlag = 0;
					Distributed_NodeEnablePublish(i);
					WaitForFlag(&PublishResponseFlag, 1);
					if(PublishResponseFlag == i){										//	Got ResponsePublish
						break;
					}
					else{
						global_record_requestrelease_fail_count[i-1]++;
						//	Should publish Invalid Node
						//	??????
						Timeout_count++;
						if(Timeout_count > Timeout_count_limit){
							//printf("Over time out limit: 0x%lX\r\n", Timeout_count_limit);
							break;
						}
					}
				}
			}
			else if (i == Global_Node_id){
				PublishFlag = 1;
			}
		}
		while(1){
			if(Global_Node_id == Global_Node_Master){
				vPortEnterCritical();
				if((RequestKeyFlag != Global_Node_id) && (RequestKeyFlag != 0))		//	Not the RequestKeyFlag occupy node
					ResponseKeyFlag = Global_Node_count + 1;
				else{																//	Release RequestKeyFlag
					ResponseKeyFlag = Global_Node_id;
					RequestKeyFlag = 0;
				}
				vPortExitCritical();
			}
			else{																		//	Local node is not Master, try to request or release by communication
				ResponseKeyFlag = 0;
				while(ResponseKeyFlag == 0){
					Distributed_NodeReleaseKey();									//	Release by communication
					WaitForFlag(&ResponseKeyFlag, 1);
				}
			}
			/*
			if(ResponseKeyFlag > Global_Node_count){									//	RequestKeyFlag is occupy, clear ResponseKeyFlag
				#if(PrintSendRecv > 0)
					if(DisableEnableFlag == 0)
						printf("Not Request the Key, RequestKeyFlag is occupy\r\n");
					else
						printf("Not Release the Key, RequestKeyFlag is occupy\r\n");
				#endif
				ResponseKeyFlag = 0;													//	Not Get the Key
				return	0;
			}
			else{																		//	occupy RequestKeyFlag, clear ResponseKeyFlag
				ResponseKeyFlag = 0;
				break;
			}
			*/
			if(ResponseKeyFlag > Global_Node_count)
				PublishFlag = 0;
			ResponseKeyFlag = 0;
			break;
		}
	}
	return	1;
}

// check node exist
uint8_t Distributed_NodeCheckSizeTimeout(uint32_t tick, uint32_t Target_Node_id, uint32_t Needsize){
	if(Target_Node_id != Global_Node_id){
		uint32_t bool_timeout_flag = 0;
		uint8_t success_flag = 0xFF;
		while(!(Distributed_NodeCheck(Target_Node_id, Needsize)));
		uint32_t base_tick = xTaskGetTickCount();
		uint32_t timeout_tick = base_tick + tick;
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
			success_flag = CheckbackFlag;
		}
		else{
			//printf("Without check back, timeout!?\r\n");
			DisrtibutedNodeCheckIDFlag = 0;
		}
		return success_flag;
	}
	else{
		return 0xff;															//	"Dame you should not check yourself node"
	}
}

// dispatch subtask
void Distributed_NodeSendSubtask(uint32_t Target_Node_id, uint8_t* Subtask_addr, uint32_t Subtask_size){
	SendFlag = Distributed_NodeSendSubtask_MSG;
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	*((uint8_t*)Subtask_addr) = 0xff;
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		*(((uint8_t*)Subtask_addr+2)+i) = *((uint8_t*)&Target_Node_id+i);
		*(((uint8_t*)Subtask_addr+8)+i) = *((uint8_t*)&Global_Node_id+i);
		*((uint8_t*)Subtask_addr+12) = Distributed_NodeSendSubtask_MSG;
	}
	Distributed_SendMsg(MyMacAddr, Subtask_addr, Subtask_size);
	#if(PrintSendRecv > 0)
		printf("Broadcast DistributedNodeSubtask Node 0x%lX, Subtask_size: 0x%lX\r\n", Target_Node_id, Subtask_size);
	#endif
}

void Distributed_NodeSendRemainSubtask(uint32_t Target_Node_id, uint8_t* Subtask_addr, uint32_t Subtask_size, uint32_t Remain_th){
	SendFlag = Distributed_NodeSendRemainSubtask_MSG;
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t* Send_Addr = Subtask_addr - 17;
	uint8_t tmp_data[17];
	for(uint32_t i=0;i<17;i++)
		tmp_data[i] = *(Send_Addr+i);
	Subtask_size += 17;
	*((uint8_t*)Send_Addr) = 0xff;
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		*((uint8_t*)Send_Addr+2+i) = *((uint8_t*)&Target_Node_id+i);
		*((uint8_t*)Send_Addr+8+i) = *((uint8_t*)&Global_Node_id+i);
		*((uint8_t*)Send_Addr+13+i) = *((uint8_t*)&Remain_th+i);
	}
	*((uint8_t*)Send_Addr+12) = Distributed_NodeSendRemainSubtask_MSG;
	Distributed_SendMsg(MyMacAddr, Send_Addr, Subtask_size);
	for(uint32_t i=0;i<17;i++)
		*(Send_Addr+i) = tmp_data[i];

	#if(PrintSendRecv > 0)
		printf("Broadcast Distributed_NodeSendRemainSubtask Node 0x%lX, Subtask_size: 0x%lX\r\n", Target_Node_id, Subtask_size);
	#endif
}

void Distributed_NodeRecvSubtask(uint32_t Target_Node_id){
	SendFlag = Distributed_NodeRecvSubtask_MSG;
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[13] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, Distributed_NodeRecvSubtask_MSG};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+2] = *((uint8_t*)&Target_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
	}
	Distributed_SendMsg(MyMacAddr, mydata, 13);
	#if(PrintSendRecv > 0)
		printf("Broadcast Distributed_NodeRecvSubtask Node\r\n");
	#endif
}

void Distributed_NodeRecvRemainSubtask(uint32_t Target_Node_id, uint32_t Remain_th){
	SendFlag = Distributed_NodeRecvRemainSubtask_MSG;
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[17] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, Distributed_NodeRecvRemainSubtask_MSG, 0x00, 0x00, 0x00, 0x00};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+2] = *((uint8_t*)&Target_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+13] = *((uint8_t*)&Remain_th+i);
	}
	Distributed_SendMsg(MyMacAddr, mydata, 17);
	#if(PrintSendRecv > 0)
		printf("Broadcast Distributed_NodeRecvRemainSubtask Node\r\n");
	#endif
}

// subtask finish
void Distributed_NodeSubtaskFinish(uint32_t Target_Node_id, uint32_t Task_id, uint32_t Subtask_id, uint32_t Size){
	SendFlag = Distributed_NodeSubtaskFinish_MSG;
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[25] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, Distributed_NodeSubtaskFinish_MSG, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+2] = *((uint8_t*)&Target_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+13] = *((uint8_t*)&Task_id+i);
		mydata[i+17] = *((uint8_t*)&Subtask_id+i);
		mydata[i+21] = *((uint8_t*)&Size+i);
	}
	Distributed_SendMsg(MyMacAddr, mydata, 25);
	#if(PrintSendRecv > 0)
		printf("Broadcast Distributed_NodeSubtaskFinish to Node: 0x%lX, Task_id: 0x%lX, Subtask_id: 0x%lX, Size: 0x%lX\r\n", Target_Node_id, Task_id, Subtask_id, Size);
	#endif
}

void Distributed_NodeResponseSubtaskFinish(uint32_t Target_Node_id, uint32_t Target_Task_id, uint32_t Target_Subtask_id){
	SendFlag = Distributed_NodeResponseSubtaskFinish_MSG;
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[21] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, Distributed_NodeResponseSubtaskFinish_MSG, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+2] = *((uint8_t*)&Target_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+13] = *((uint8_t*)&Target_Task_id+i);
		mydata[i+17] = *((uint8_t*)&Target_Subtask_id+i);
	}
	Distributed_SendMsg(MyMacAddr, mydata, 21);
	#if(PrintSendRecv > 0)
		printf("Broadcast Distributed_NodeResponseSubtaskFinish to Node: 0x%lX, Subtask: 0x%lX\r\n", Target_Node_id, Target_Subtask_id);
	#endif
}

// retrieve result
void Distributed_NodeRequestResult(uint32_t Target_Node_id, uint32_t Task_id, uint32_t Subtask_id){
	SendFlag = Distributed_NodeRequestResult_MSG;
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[21] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, Distributed_NodeRequestResult_MSG, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+2] = *((uint8_t*)&Target_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+13] = *((uint8_t*)&Task_id+i);
		mydata[i+17] = *((uint8_t*)&Subtask_id+i);
	}
	Distributed_SendMsg(MyMacAddr, mydata, 21);
	#if(PrintSendRecv > 0)
		printf("Broadcast Distributed_NodeRequestResult, Node: 0x%lX, Task: 0x%lX, Subtask: 0x%lX\r\n", Target_Node_id, Task_id, Subtask_id);
	#endif
}

void Distributed_NodeRequestRemainResult(uint32_t Target_Node_id, uint32_t Remain_th){
	SendFlag = Distributed_NodeRequestRemainResult_MSG;
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[17] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, Distributed_NodeRequestRemainResult_MSG, 0x00, 0x00, 0x00, 0x00};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+2] = *((uint8_t*)&Target_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+13] = *((uint8_t*)&Remain_th+i);
	}
	Distributed_SendMsg(MyMacAddr, mydata, 17);
	#if(PrintSendRecv > 0)
		printf("Broadcast Distributed_NodeRequestRemainResult, Node: 0x%lX, Remain_th: 0x%lX\r\n", Target_Node_id, Remain_th);
	#endif
}

void Distributed_NodeResponseResult(uint32_t Target_Node_id, uint8_t* Result_addr, uint32_t Result_size){
	SendFlag = Distributed_NodeResponseResult_MSG;
	//printf("Result_size: 0x%lX\r\n", Result_size);
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	*((uint8_t*)Result_addr) = 0xff;
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		*(((uint8_t*)Result_addr+2)+i) = *((uint8_t*)&Target_Node_id+i);
		*(((uint8_t*)Result_addr+8)+i) = *((uint8_t*)&Global_Node_id+i);
	}
	*((uint8_t*)Result_addr+12) = Distributed_NodeResponseResult_MSG;
	Distributed_SendMsg(MyMacAddr, Result_addr, Result_size);
	#if(PrintSendRecv > 0)
		printf("Broadcast Distributed_NodeResponseResult Node 0x%lX\r\n", Target_Node_id);
	#endif
}

void Distributed_NodeResponseRemainResult(uint32_t Target_Node_id, uint8_t* Result_addr, uint32_t Result_size, uint32_t Remain_th){
	SendFlag = Distributed_NodeResponseRemainResult_MSG;
	//printf("Result_size: 0x%lX\r\n", Result_size);
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t* Send_Addr = Result_addr - 17;
	uint8_t tmp_data[17];
	for(uint32_t i=0;i<17;i++)
		tmp_data[i] = *(Send_Addr+i);
	Result_size += 17;
	*((uint8_t*)Send_Addr) = 0xff;
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		*((uint8_t*)Send_Addr+2+i) = *((uint8_t*)&Target_Node_id+i);
		*((uint8_t*)Send_Addr+8+i) = *((uint8_t*)&Global_Node_id+i);
		*((uint8_t*)Send_Addr+13+i) = *((uint8_t*)&Remain_th+i);
	}
	*((uint8_t*)Send_Addr+12) = Distributed_NodeResponseRemainResult_MSG;

	Distributed_SendMsg(MyMacAddr, Send_Addr, Result_size);
	for(uint32_t i=0;i<17;i++)
		*(Send_Addr+i) = tmp_data[i];
	#if(PrintSendRecv > 0)
		printf("Broadcast Distributed_NodeResponseRemainResult Node 0x%lX\r\n", Target_Node_id);
	#endif
}

void Distributed_NodeRemoveTask(uint32_t Target_Node_id, uint32_t Source_Node_id, uint32_t Task_id){
	SendFlag = Distributed_NodeRemoveTask_MSG;
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[21] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, Distributed_NodeRemoveTask_MSG, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+2] = *((uint8_t*)&Target_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+13] = *((uint8_t*)&Source_Node_id+i);
		mydata[i+17] = *((uint8_t*)&Task_id+i);
	}
	Distributed_SendMsg(MyMacAddr, mydata, 21);
	#if(PrintSendRecv > 0)
		printf("Broadcast Distributed_NodeRemoveTask, Source_Node_id: 0x%lX, Task_id: 0x%lX\r\n", Source_Node_id, Task_id);
	#endif
}

void Distributed_NodeSendComplete(uint32_t Target_Node_id, uint8_t Remain_th){
	SendFlag = Distributed_NodeSendComplete_MSG;
	uint8_t MyMacAddr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t mydata[21] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, Distributed_NodeSendComplete_MSG, Remain_th };
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+2] = *((uint8_t*)&Target_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
	}
	mydata[13] = Remain_th;
	Distributed_SendMsg(MyMacAddr, mydata, 14);
	#if(PrintSendRecv > 0)
		printf("Broadcast Distributed_NodeSendComplete, Target_Node_id: 0x%lX, Remain_th: 0x%lX\r\n", Target_Node_id, (uint32_t)Remain_th);
	#endif
}

// complete sequence
uint8_t Distributed_NodeSendCompleteSequence(uint32_t Target_Node_id){
	vPortEnterCritical();
	uint8_t Remain_th = 1;
	SendCompleteFlag = 0;
	Distributed_NodeSendComplete(Target_Node_id, Remain_th);
	vPortExitCritical();
	WaitForFlag(&SendCompleteFlag, 1);
	vPortEnterCritical();
	if(SendCompleteFlag == (Remain_th+1)){
		Remain_th = SendCompleteFlag + 1;
		Distributed_NodeSendComplete(Target_Node_id, Remain_th);
		SendCompleteFlag = 0;
		vPortExitCritical();
		return Remain_th;
	}
	else{
		global_record_fail_count[Target_Node_id-1] = global_record_fail_count[Target_Node_id-1]+1;
		SendCompleteFlag = 0;
		vPortExitCritical();
		return 0;
	}
}

uint8_t Distributed_NodeRecvCompleteSequence(uint32_t Target_Node_id){
	WaitForFlag(&SendCompleteFlag, 1);
	vPortEnterCritical();
	uint8_t Remain_th = 0;
	if(SendCompleteFlag == (Remain_th+1)){
		Remain_th = SendCompleteFlag + 1;
		SendCompleteFlag = 0;
		uint32_t recv_complete_count = 0;
		uint32_t recv_complete_limit = 10;
		while(1){
			Distributed_NodeSendComplete(Target_Node_id, Remain_th);
			vPortExitCritical();
			WaitForFlag(&SendCompleteFlag, 1);
			vPortEnterCritical();
			if(SendCompleteFlag == (Remain_th+1)){
				Remain_th = SendCompleteFlag + 1;
				break;
			}
			else{
				global_record_fail_count[Target_Node_id+4-1] = global_record_fail_count[Target_Node_id+4-1]+1;
				recv_complete_count++;
				if(recv_complete_count > recv_complete_limit){
					break;
				}
			}
		}
		SendCompleteFlag = 0;
		vPortExitCritical();
		return Remain_th;
	}
	else{
		global_record_fail_count[Target_Node_id+4-1] = global_record_fail_count[Target_Node_id+4-1]+1;
		SendCompleteFlag = 0;
		vPortExitCritical();
		return 0;
	}
}

// update free memory information
void Distributed_NodeSendFreeBlock(uint32_t Target_Node_id, uint32_t Node_id){
	vPortEnterCritical();

	SendFlag = Distributed_NodeSendFreeBlock_MSG;
	if(Target_Node_id == 0)
		Target_Node_id = 0xFFFFFFFF;
	UpdateLocalFreeBlock();
	uint32_t node_number = 0;
	uint32_t block_number = 0;
	Distributed_FreeBlock* FreeBlockStart = DF_Start;
	Distributed_FreeBlock* tmp_block = FreeBlockStart;
	if(Node_id > 0){
		while((tmp_block != NULL) && (tmp_block->Node_id != Node_id)){
			tmp_block = tmp_block->Next_Distributed_FreeBlock;
		}
		if(tmp_block == NULL){
			printf("Distributed_NodeSendFreeBlock Fail, Without Node_id: 0x%lX\r\n", Node_id);
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
	mydata[12] = Distributed_NodeSendFreeBlock_MSG;
	mydata[13] = node_number;
	for(uint8_t i=0;i<4;i++){
		MyMacAddr[2+i] = *((uint8_t*)&Global_Node_id+i);
		mydata[i+2] = *((uint8_t*)&Target_Node_id+i);
		mydata[i+8] = *((uint8_t*)&Global_Node_id+i);
	}
	uint32_t tmp_node_number = 0;
	uint32_t tmp_node_data_count = 0;
	uint32_t source_node_count = 0;
	if(Node_id == 0){
		tmp_block = FreeBlockStart;
		while(tmp_block != NULL){
			source_node_count++;
			for(uint8_t i=0;i<sizeof(Distributed_FreeBlock);i++){
				*((uint8_t*)(mydata+14+tmp_node_number*sizeof(Distributed_FreeBlock)+i)) = *((uint8_t*)tmp_block+i);
			}
			for(uint32_t i=0;i<tmp_block->Block_number;i++){
				for(uint32_t j=0;j<sizeof(uint32_t);j++){
					*((uint8_t*)(mydata+14+node_number*sizeof(Distributed_FreeBlock)+tmp_node_data_count)) = *((uint8_t*)(tmp_block->Block_size_array+i)+j);
					tmp_node_data_count++;
				}
			}
			tmp_node_number++;
			tmp_block = tmp_block->Next_Distributed_FreeBlock;
		}
	}
	else{
		if (tmp_block->Node_id == Node_id){
			for(uint8_t i=0;i<sizeof(Distributed_FreeBlock);i++)
				*((uint8_t*)(mydata+14+i)) = *((uint8_t*)tmp_block+i);
			for(uint32_t i=0;i<tmp_block->Block_number;i++){
				for(uint32_t j=0;j<sizeof(uint32_t);j++){
					*((uint8_t*)(mydata+14+node_number*sizeof(Distributed_FreeBlock)+4*i+j)) = *((uint8_t*)tmp_block->Block_size_array+4*i+j);
				}
			}
		}
	}
	vPortExitCritical();
	Distributed_SendMsg(MyMacAddr, mydata, Send_size);
	BlockChangeFlag = 0;
}

void UpdateLocalFreeBlock(){
	//printf("  UpdateLocalFreeBlock Start\r\n");
	vPortEnterCritical();
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
	vPortExitCritical();
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

uint8_t Check_Sendable_Without_Limit(){
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
	return bool_send_flag;
}

Distributed_FreeBlock* GetFreeBlockNode(uint32_t Node_id){
	vPortEnterCritical();
	Distributed_FreeBlock* free_block = DF_Start;
	while((free_block != NULL) && (free_block->Node_id != Node_id))
		free_block = free_block->Next_Distributed_FreeBlock;
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
	vPortExitCritical();

	return free_block;
}

void Distributed_Show_FreeBlock(){
	printf("\r\nStart---------------------------------------\r\n");
	vPortEnterCritical();

	Distributed_FreeBlock* free_block = DF_Start;
	while(free_block != NULL){
		printf("free_block Node_id: 0x%lX, Block_number: 0x%lX, Block_size_array: 0x%lX\r\n", free_block->Node_id, free_block->Block_number, (uint32_t)free_block->Block_size_array);
		printf("Block: ");
		for(uint32_t i=0;i<free_block->Block_number;i++)
			printf("0x%lX, ", *(free_block->Block_size_array+i));
		printf("\r\n");
		free_block = free_block->Next_Distributed_FreeBlock;
	}
	vPortExitCritical();

	printf("End  ---------------------------------------\r\n\r\n");
}

// Distributed manage task
void Distributed_ManageTask(){
	while(1){
		if ((READ_BIT(USART3_BASE + USART_SR_OFFSET, RXNE_BIT)) || (READ_BIT(USART3_BASE + USART_SR_OFFSET, ORE_BIT))){
			char rec_cmd = (char)REG(USART3_BASE + USART_DR_OFFSET);
			printf("%c.\r\n", rec_cmd);
			if (rec_cmd == 'c'){												// input 'c' to startup the middleware, get the id and free memory info
				Msg_event = 0;
				Global_Node_id = 0;
				Global_Node_count = 0;
				Global_Node_Master = 0;
				Global_Node_Backup_Master = 0;
				Global_Task_id = 0;
				DisrtibutedNodeCheckIDFlag = 0;
				CheckMasterNodeFlag = 0;
				NodeInvalidFlag = 0;

				uint8_t MyMacAddr[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
				while(!init_eth(DP83848_PHY_ADDRESS, MyMacAddr)){
					printf("Reset eth\r\n");
					for(uint32_t i=0;i<0x00000FFF;i++)
						;
				}
				init_eth(DP83848_PHY_ADDRESS, MyMacAddr);

				Distributed_NodeGetID();
				printf("Distributed_NodeGetID\r\n");
				uint32_t Flag = WaitForFlag(&Global_Node_id, 100);
				if(Flag == 0){
					printf("Distributed_NodeGetIDAgain\r\n");
					Distributed_NodeGetIDAgain();
					Flag = WaitForFlag(&Global_Node_id, 100);
					if(Flag == 0){
						Global_Node_id = 1;
						Global_Node_count++;
						Global_Node_Master = Global_Node_id;
					}
				}

				printf("Got Global_Node_id: 0x%lX\r\n", Global_Node_id);
				BlockChangeFlag = 0;
				while(1){
					if ((READ_BIT(USART3_BASE + USART_SR_OFFSET, RXNE_BIT)) || (READ_BIT(USART3_BASE + USART_SR_OFFSET, ORE_BIT))){
						rec_cmd = (char)REG(USART3_BASE + USART_DR_OFFSET);
						printf("%c\r\n", rec_cmd);
					}

					if(rec_cmd == 'q'){
						printf("Die in here: 0x%lX\r\n", DebugFlag);
						rec_cmd = '\0';
					}
					if (rec_cmd == 'w'){
						vPortEnterCritical();
						Distributed_TaskHandle_List_t* Lastnode = DStart;
						uint32_t tmp_count = 0;
						printf("Unfinish task List:\r\n");
						while(Lastnode != NULL){
							printf("%d, Source_Processor_id: 0x%lX, Destinate_Processor_id: 0x%lX, DTask_id: 0x%lX, DSubTask_id: 0x%lX\r\n", (int)tmp_count, Lastnode->Source_Processor_id, Lastnode->Destinate_Processor_id, Lastnode->DTask_id, Lastnode->DSubTask_id);
							tmp_count++;
							Lastnode = Lastnode->Next_TaskHandle_List;
						}
						vPortExitCritical();
						rec_cmd = '\0';
					}

					if (rec_cmd == 'W'){
						vPortEnterCritical();
						Distributed_TaskHandle_List_t* Lastnode = DFinish;
						uint32_t tmp_count = 0;
						printf("Finish task List:\r\n");
						while(Lastnode != NULL){
							printf("%d, Source_Processor_id: 0x%lX, Destinate_Processor_id: 0x%lX, DTask_id: 0x%lX, DSubTask_id: 0x%lX, Data_size: 0x%lX\r\n", (int)tmp_count, Lastnode->Source_Processor_id, Lastnode->Destinate_Processor_id, Lastnode->DTask_id, Lastnode->DSubTask_id, Lastnode->Data_number);
							tmp_count++;
							Lastnode = Lastnode->Next_TaskHandle_List;
						}
						vPortExitCritical();
						rec_cmd = '\0';
					}

					if (rec_cmd == 'E'){
						vPortEnterCritical();
						Distributed_TaskHandle_List_t* Lastnode = DRequest;
						uint32_t tmp_count = 0;
						printf("DRequest task List:\r\n");
						while(Lastnode != NULL){
							printf("%d, Source_Processor_id: 0x%lX, Destinate_Processor_id: 0x%lX, DTask_id: 0x%lX, DSubTask_id: 0x%lX, Data_size: 0x%lX\r\n", (int)tmp_count, Lastnode->Source_Processor_id, Lastnode->Destinate_Processor_id, Lastnode->DTask_id, Lastnode->DSubTask_id, Lastnode->Data_number);
							tmp_count++;
							Lastnode = Lastnode->Next_TaskHandle_List;
						}
						vPortExitCritical();
						rec_cmd = '\0';
					}

					if (rec_cmd == 'e'){
						vPortEnterCritical();
						Distributed_TaskHandle_List_t* Lastnode = DDelete;
						uint32_t tmp_count = 0;
						printf("DDelete task List:\r\n");
						while(Lastnode != NULL){
							printf("%d, Source_Processor_id: 0x%lX, Destinate_Processor_id: 0x%lX, DTask_id: 0x%lX, DSubTask_id: 0x%lX\r\n", (int)tmp_count, Lastnode->Source_Processor_id, Lastnode->Destinate_Processor_id, Lastnode->DTask_id, Lastnode->DSubTask_id);
							tmp_count++;
							Lastnode = Lastnode->Next_TaskHandle_List;
						}
						vPortExitCritical();
						rec_cmd = '\0';
					}

					if (rec_cmd == 'r'){
						vPortEnterCritical();
						Distributed_TaskHandle_List_t* Lastnode = DSendFinish;
						uint32_t tmp_count = 0;
						printf("DSendFinish task List:\r\n");
						while(Lastnode != NULL){
							printf("%d, Source_Processor_id: 0x%lX, Destinate_Processor_id: 0x%lX, DTask_id: 0x%lX, DSubTask_id: 0x%lX\r\n", (int)tmp_count, Lastnode->Source_Processor_id, Lastnode->Destinate_Processor_id, Lastnode->DTask_id, Lastnode->DSubTask_id);
							tmp_count++;
							Lastnode = Lastnode->Next_TaskHandle_List;
						}
						vPortExitCritical();
						rec_cmd = '\0';
					}

					if (rec_cmd == 's'){										//	Distributed_Show_FreeBlock
						Distributed_Show_FreeBlock();
						rec_cmd = '\0';
					}

					if (rec_cmd == 'S'){										//	ListFreeBlock
						ListFreeBlock();
						rec_cmd = '\0';
					}

					if (rec_cmd == 'f'){										//	send check all node
						uint32_t free_block_number = 0;
						vPortEnterCritical();
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
								vPortExitCritical();
								timeout_flag = Distributed_NodeCheckSizeTimeout(Timeout_Tick_Count, Allfreeblocknode[i], 0);
								vPortEnterCritical();
								if(timeout_flag != 0xff)
									printf("Got response: 0x%lX\r\n",  (uint32_t)timeout_flag);
								else
									printf("No response: 0x%lX\r\n", (uint32_t)timeout_flag);
							}
						}
						vPortExitCritical();
						rec_cmd = '\0';
					}

					if (rec_cmd == 'm'){										//	malloc large size
						vPortEnterCritical();
						uint32_t Max_block_size = 0;
						ListFreeBlock();
						BlockLink_t* tmp_block = &xStart;
						while(tmp_block!= NULL){
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
						ListFreeBlock();
						vPortExitCritical();
						rec_cmd = '\0';
					}

					if (rec_cmd == 'r'){
						printf("tri_svc\r\n");
						tri_svc();
						rec_cmd = '\0';
					}

					if(CheckMasterNodeFlag == 1){
						uint32_t checkout_limit = 3;
						uint32_t checksize_count = 0;
						while(checksize_count < checkout_limit){
							uint8_t timeout_flag = Distributed_NodeCheckSizeTimeout(Timeout_Tick_Count, Global_Node_Master, 0);
							if(timeout_flag == 0xff){
								checksize_count++;
							}
							else{
								Distributed_NodeGetID();								//	Master node exist
								break;
							}
						}
						if(checksize_count >= checkout_limit){
							printf("Timeout in CheckMasterNodeFlag\r\n");
							Distributed_NodeInvalid(Global_Node_Master);			//	Master node not exist
							DisrtibutedNodeCheckIDFlag = 0;
							Global_Node_Master = Global_Node_id;
							Distributed_NodeResponseID();
						}
						CheckMasterNodeFlag = 0;
						printf("CheckMasterNodeFlag\r\n");
					}

					if(SendFreeBlockFlag == 1){									//	New Node to Master Node or Master Node to New Node
						if(Check_Sendable());
						Distributed_NodeSendFreeBlock(0xffffffff, 0);
						SendFreeBlockFlag = 0;
						printf("SendFreeBlockFlag\r\n");
					}

					if(ReceiveSubtaskFlag > 0){
						vPortEnterCritical();
						uint32_t Source_node = ReceiveSubtaskFlag;
						ReceiveSubtaskFlag = 0;
						uint8_t* frame_addr = (uint8_t*)((DMA_RX_FRAME_infos->FS_Rx_Desc)->Buffer1Addr);
						Distributed_TaskHandle_List_t* TmpDTaskControlBlock = (Distributed_TaskHandle_List_t*)((uint8_t*)frame_addr+13);
						uint8_t tmp_event = *((uint8_t*)frame_addr+12);
						if(tmp_event == Distributed_NodeSendSubtask_MSG){
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
							#if(PrintSendRecv > 0)
								uint32_t* package_start_addr = (uint32_t*)NewDTaskControlBlock;
								uint32_t* package_stop_addr;
							#endif
							uint32_t Distributed_NodeSendSubtask_Header_Size = 13;
							uint32_t Recv_Data_Number = (ETH_FRAM_SIZE - Distributed_NodeSendSubtask_Header_Size);
							if(Distributed_Recv_Size > Recv_Data_Number){
								uint8_t* Data_Dest_ptr = (uint8_t*)NewDTaskControlBlock;
								uint32_t Remain_Data_number = Distributed_Recv_Size;
								uint32_t Remain_th = 1;
								for(uint32_t i=0;i<Recv_Data_Number;i++)
									*((uint8_t*)Data_Dest_ptr+i) = *((uint8_t*)TmpDTaskControlBlock+i);

								Data_Dest_ptr += Recv_Data_Number;
								Remain_Data_number -= Recv_Data_Number;
								while(Remain_Data_number > 0){
									ReceiveSubtaskFlag = 0;
									if(Remain_th == 1){
										Distributed_NodeRecvSubtask(Source_node);
									}
									else{
										Distributed_NodeRecvRemainSubtask(Source_node, Remain_th);
									}
									vPortExitCritical();
									WaitForFlag(&ReceiveSubtaskFlag, 1);
									vPortEnterCritical();
									if((ReceiveSubtaskFlag == NewDTaskControlBlock->Source_Processor_id) && (RemainThFlag == Remain_th)){
										Source_node = ReceiveSubtaskFlag;
										frame_addr = (uint8_t*)((DMA_RX_FRAME_infos->FS_Rx_Desc)->Buffer1Addr);
										uint32_t Distributed_NodeSendRemainSubtask_Header_Size = 17;
										uint8_t* tmp_Data_addr = ((uint8_t*)frame_addr+Distributed_NodeSendRemainSubtask_Header_Size);
										Recv_Data_Number = (ETH_FRAM_SIZE - Distributed_NodeSendRemainSubtask_Header_Size);
										if(Recv_Data_Number > Remain_Data_number)
											Recv_Data_Number = Remain_Data_number;
										for(uint32_t i=0;i<Recv_Data_Number;i++)
											*(Data_Dest_ptr+i) = *(tmp_Data_addr+i);
										Data_Dest_ptr = (uint8_t*)Data_Dest_ptr + Recv_Data_Number;
										#if(PrintSendRecv > 0)
											package_stop_addr = (uint32_t*)(Data_Dest_ptr-4);
										#endif
										Remain_Data_number -= Recv_Data_Number;
										Remain_th++;
										if(Remain_Data_number <= 0){
											Distributed_NodeRecvRemainSubtask(Source_node, Remain_th);
											SendCompleteFlag = 0;
											vPortExitCritical();
											uint8_t Flag = 0;
											while(Flag == 0){
												Flag = Distributed_NodeRecvCompleteSequence(Source_node);
												if(Flag == 0)
													Distributed_NodeRecvRemainSubtask(Source_node, Remain_th);
											}
											vPortEnterCritical();
										}
									}
								}
							}
							else{
								for(uint32_t i=0;i<Distributed_Recv_Size;i++)
									*((uint8_t*)NewDTaskControlBlock+i) = *((uint8_t*)TmpDTaskControlBlock+i);
								Distributed_NodeRecvSubtask(Source_node);
								SendCompleteFlag = 0;
								vPortExitCritical();
								uint8_t Flag = 0;
								while(Flag == 0){
									Flag = Distributed_NodeRecvCompleteSequence(Source_node);
									if(Flag == 0)
										Distributed_NodeRecvSubtask(Source_node);
								}
								vPortEnterCritical();
							}
							ReceiveSubtaskFlag = 0;
							#if(PrintSendRecv > 0)
								uint32_t package_size = (uint32_t)package_stop_addr - (uint32_t)package_start_addr;
								printf("start_addr: 0x%lX, stop_addr: 0x%lX, package_size: 0x%lX\r\n0x%lX, 0x%lX\r\n", (uint32_t)package_start_addr, (uint32_t)package_stop_addr, package_size, *((uint32_t*)package_start_addr), *((uint32_t*)package_stop_addr));
							#endif
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
							NewDTaskControlBlock->Subtask_all_size = Distributed_Recv_Size;
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
							vPortExitCritical();
							xTaskCreate((void*)NewDTaskControlBlock->Instruction_addr, "Distributed task", (NewDTaskControlBlock->Stack_size), NULL, 1, NewDTaskControlBlock->TaskHandlex);
							vPortEnterCritical();
						}
						ReceiveSubtaskFlag = 0;
						vPortExitCritical();
					}

					else if((RecvFreeBlockFlag > 0) && (PublishFlag > 0)){
						vPortEnterCritical();
						#if(PrintSendRecv > 0)
							printf("In RecvFreeBlockFlag\r\n");
						#endif
						uint8_t* frame_addr = (uint8_t*)((DMA_RX_FRAME_infos->FS_Rx_Desc)->Buffer1Addr);
						uint8_t tmp_event = *((uint8_t*)frame_addr+12);
						uint8_t block_number = *((uint8_t*)frame_addr+13);
						if(tmp_event == Distributed_NodeSendFreeBlock_MSG){
							uint32_t tmp_node_data_count = 0;
							for(uint8_t i=0;i<block_number;i++){
								Distributed_FreeBlock* tmp_block = (Distributed_FreeBlock*)((uint8_t*)frame_addr+14+i*sizeof(Distributed_FreeBlock));
								if(tmp_block->Node_id != Global_Node_id){
									if((tmp_block->Node_id > Global_Node_count) && (Global_Node_id != Global_Node_Master))
										Global_Node_count = tmp_block->Node_id;
									Distributed_FreeBlock* Local_Node = GetFreeBlockNode(tmp_block->Node_id);
									if(Local_Node->Block_number != tmp_block->Block_number){
										if(Local_Node->Block_number > 0){
											vPortFree(Local_Node->Block_size_array);
										}
										Local_Node->Block_number = tmp_block->Block_number;
										Local_Node->Block_size_array = pvPortMalloc(Local_Node->Block_number*sizeof(uint32_t));
									}
									for(uint32_t j=0;j<tmp_block->Block_number;j++){
										uint32_t* tmp_addr = ((uint32_t*)((uint8_t*)frame_addr+14+block_number*sizeof(Distributed_FreeBlock))+tmp_node_data_count);
										tmp_node_data_count++;
										*(Local_Node->Block_size_array+j) = *tmp_addr;
									}
								}
								else{
									tmp_node_data_count += tmp_block->Block_number;
								}
							}
							UpdateLocalFreeBlock();
							if(RecvFreeBlockFlag > Global_Node_count){
								Global_Node_count = RecvFreeBlockFlag;
								if (Global_Node_id == Global_Node_Master){
									if(Global_Node_Backup_Master == 0){
										printf("Dispatch backup master\r\n");
										Distributed_NodeBackupMaster(RecvFreeBlockFlag);
										vPortExitCritical();
										uint32_t base_tick  = xTaskGetTickCount();
										uint32_t timeout_tick = base_tick + SystemTICK_RATE_HZ/10;
										uint32_t tmp_tick = xTaskGetTickCount();
										while(tmp_tick < timeout_tick){
											tmp_tick = xTaskGetTickCount();
										}
										vPortEnterCritical();
										Global_Node_Backup_Master = RecvFreeBlockFlag;
									}
									SendFreeBlockFlag = 1;
									BlockChangeFlag = 0;// important!!!
								}
							}
						}
						RecvFreeBlockFlag = 0;
						vPortExitCritical();
						#if(PrintSendRecv > 0)
							printf("RecvFreeBlockFlag\r\n");
						#endif
					}

					else if(ResponseResultFlag != 0){
						vPortEnterCritical();
						Distributed_TaskHandle_List_t* Resultnode = (Distributed_TaskHandle_List_t*)ResponseResultFlag;
						//ResponseResultFlag = 0;
						uint8_t* Send_Addr = ((uint8_t*)Resultnode->Data_addr-13);
						uint32_t Send_Total_Size = (13+Resultnode->Data_number*sizeof(uint32_t));
						uint32_t Send_Remain_Size = Send_Total_Size;
						uint32_t Send_Size = 0;
						uint32_t Remain_th = 1;
						while(Send_Remain_Size > 0){
							if(Remain_th == 1){
								if(Send_Remain_Size > ETH_FRAM_SIZE)
									Send_Size = ETH_FRAM_SIZE;
								else
									Send_Size = Send_Remain_Size;
							}
							else{
								uint32_t Distributed_NodeResponseRemainResult_Header_Size = 17;
								if((Send_Remain_Size + Distributed_NodeResponseRemainResult_Header_Size) > ETH_FRAM_SIZE)
									Send_Size = ETH_FRAM_SIZE - Distributed_NodeResponseRemainResult_Header_Size;
								else
									Send_Size = Send_Remain_Size;
							}
							ConfirmResultFlag = 0;
							if(Remain_th == 1){
								Distributed_NodeResponseResult(Resultnode->Source_Processor_id, Send_Addr, Send_Size);
							}
							else{
								Distributed_NodeResponseRemainResult(Resultnode->Source_Processor_id, Send_Addr, Send_Size, Remain_th);
							}
							vPortExitCritical();
							WaitForFlag(&ConfirmResultFlag, 1);
							vPortEnterCritical();
							if((ConfirmResultFlag == Resultnode->Source_Processor_id) && (RemainThResultFlag == (Remain_th))){
								ConfirmResultFlag = 0;
								RemainThResultFlag = 0;
								Send_Addr += Send_Size;
								Send_Remain_Size -= Send_Size;
								Remain_th++;
								if(Send_Remain_Size <= 0){
									vPortExitCritical();
									uint8_t Flag = 0;
									uint32_t start_tickcount = xTaskGetTickCount();
									uint32_t stop_tickcount = 0;
									while((Flag == 0) && (stop_tickcount < 1)){
										Flag = Distributed_NodeSendCompleteSequence(Resultnode->Source_Processor_id);
										stop_tickcount = (xTaskGetTickCount() - start_tickcount)/SystemTICK_RATE_HZ;
									}
									vPortEnterCritical();
								}
							}
						}
						ResponseResultFlag = 0;

						Distributed_TaskHandle_List_t* Lastnode = DFinish;
						Distributed_TaskHandle_List_t* pre_Lastnode = DFinish;
						while(Lastnode != NULL){
							if(Lastnode == Resultnode){
								break;
							}
							pre_Lastnode = Lastnode;
							Lastnode = Lastnode->Next_TaskHandle_List;
						}
						if(Lastnode != NULL){
							if(Lastnode == DFinish)
								DFinish = Lastnode->Next_TaskHandle_List;
							else
								pre_Lastnode->Next_TaskHandle_List = Lastnode->Next_TaskHandle_List;
							Lastnode->Next_TaskHandle_List = NULL;

							Distributed_TaskHandle_List_t* Insert_Lastnode = DDelete;
							if(DDelete != NULL){
								while(Insert_Lastnode->Next_TaskHandle_List != NULL){
									Insert_Lastnode = Insert_Lastnode->Next_TaskHandle_List;
								}
								Insert_Lastnode->Next_TaskHandle_List = Lastnode;
							}
							else
								DDelete = Lastnode;
						}
						vPortExitCritical();
					}

					else if(Unmerge_Finish_Distributed_task > 0){
						vPortEnterCritical();
						Distributed_TaskHandle_List_t* Lastnode = DFinish;
						Distributed_TaskHandle_List_t* pre_Lastnode = DFinish;
						while(Lastnode != NULL){
							if(Lastnode->Finish_Flag == 0)
								break;
							pre_Lastnode = Lastnode;
							Lastnode = Lastnode->Next_TaskHandle_List;
						}
						if(Lastnode != NULL){
							uint32_t Total_malloc_size = sizeof(Distributed_TaskHandle_List_t) + (uint32_t)(Lastnode->Data_number)*sizeof(uint32_t) + 13;
							Distributed_TaskHandle_List_t* tmp_NewDTaskControlBlock = pvPortMalloc(Total_malloc_size);
							for(uint8_t i=0;i<sizeof(Distributed_TaskHandle_List_t);i++){
								*((uint8_t*)tmp_NewDTaskControlBlock+i) = *((uint8_t*)Lastnode+i);
							}
							tmp_NewDTaskControlBlock->Data_addr = (uint32_t*)((uint8_t*)tmp_NewDTaskControlBlock + sizeof(Distributed_TaskHandle_List_t) + 13);
							tmp_NewDTaskControlBlock->Data_number = Lastnode->Data_number;
							for(uint32_t i=0;i<Lastnode->Data_number;i++){
								*(tmp_NewDTaskControlBlock->Data_addr+i) = *(Lastnode->Data_addr+i);
							}
							tmp_NewDTaskControlBlock->Finish_Flag = 1;
							//tmp_NewDTaskControlBlock->Next_TaskHandle_List = Lastnode->Next_TaskHandle_List;

							if(Lastnode == DFinish)
								//DFinish = tmp_NewDTaskControlBlock;
								DFinish = Lastnode->Next_TaskHandle_List;
							else
								//pre_Lastnode->Next_TaskHandle_List = tmp_NewDTaskControlBlock;
								pre_Lastnode->Next_TaskHandle_List = Lastnode->Next_TaskHandle_List;
							ResponseSubtaskFinishFlag = 0;
							if (!(((uint32_t)(Lastnode->Data_addr) < (uint32_t)((uint8_t*)Lastnode + Lastnode->Subtask_all_size)) && ((uint32_t)(Lastnode->Data_addr) >= (uint32_t)((uint8_t*)Lastnode)))){
								vPortFree(Lastnode->Data_addr);
							}
							vPortExitCritical();
							vTaskDelete(*(Lastnode->TaskHandlex));
							vPortEnterCritical();
							vPortFree(Lastnode);
							Lastnode = DSendFinish;
							pre_Lastnode = DSendFinish;
							while(Lastnode != NULL){
								pre_Lastnode = Lastnode;
								Lastnode = Lastnode->Next_TaskHandle_List;
							}
							if(Lastnode == DSendFinish)
								DSendFinish = tmp_NewDTaskControlBlock;
							else
								pre_Lastnode->Next_TaskHandle_List = tmp_NewDTaskControlBlock;
							tmp_NewDTaskControlBlock->Next_TaskHandle_List = NULL;
							/*
							while(1){
								ResponseSubtaskFinishFlag = 0;
								//if(PublishFlag != 0)
								Distributed_NodeSubtaskFinish(tmp_NewDTaskControlBlock->Source_Processor_id, tmp_NewDTaskControlBlock->DTask_id, tmp_NewDTaskControlBlock->DSubTask_id, tmp_NewDTaskControlBlock->Data_number);
								//printf("Distributed_NodeSubtaskFinish: Source_Processor_id: 0x%lX, DTask_id: 0x%lX, DSubTask_id: 0x%lX\r\n", tmp_NewDTaskControlBlock->Source_Processor_id, tmp_NewDTaskControlBlock->DTask_id, tmp_NewDTaskControlBlock->DSubTask_id);
								vPortExitCritical();
								WaitForFlag(&ResponseSubtaskFinishFlag, 1);
								vPortEnterCritical();
								if(ResponseSubtaskFinishFlag == tmp_NewDTaskControlBlock->Source_Processor_id){
									ResponseSubtaskFinishFlag = 0;
									if((ResponseSubtaskFinishTaskFlag == tmp_NewDTaskControlBlock->DTask_id) && (ResponseSubtaskFinishSubtaskFlag == tmp_NewDTaskControlBlock->DSubTask_id)){
										ResponseSubtaskFinishTaskFlag = 0;
										ResponseSubtaskFinishSubtaskFlag = 0;
										break;
									}
								}
								else{
									//printf("Timeout in Distributed_NodeSubtaskFinish\r\n");
									;
								}
								if(tmp_NewDTaskControlBlock->Source_Processor_id > Global_Node_count){		//Dame, Source_Processor_id is invalid
									//printf("Dame, Source_Processor_id is invalid: 0x%lX, DTask_id: 0x%lX, DSubTask_id: 0x%lX\r\n", tmp_NewDTaskControlBlock->Source_Processor_id, tmp_NewDTaskControlBlock->DTask_id, tmp_NewDTaskControlBlock->DSubTask_id);
									break;
								}
							}
							*/
							//printf("SubtaskFinish, DTask_id: 0x%lX, DSubTask_id: 0x%lX\r\n", tmp_NewDTaskControlBlock->DTask_id, tmp_NewDTaskControlBlock->DSubTask_id);
							Unmerge_Finish_Distributed_task--;
						}
						else{
							;
							//printf("Can't find Unmerge_Finish_Distributed_task: 0x%lX\r\n", Unmerge_Finish_Distributed_task);
						}
						vPortExitCritical();
					}

					else if((TaskDoneFlag > 0) && (PublishFlag > 0)){
						uint32_t TaskDoneFlag_tickcount = xTaskGetTickCount();
						global_record_time_dispatch_array[2] = TaskDoneFlag_tickcount - global_record_time;
						global_record_time_dispatch_array[32] = TaskDoneFlag_tickcount - global_record_time;
						uint32_t tmp_global_record_data_6 = TaskDoneFlag_tickcount;
						uint32_t get_key_flag = 0;
						uint8_t Get_key = 0;
						uint8_t Barrier = 0;
						Distributed_TaskHandle_List_t* Lastnode = DFinish;
						while( !((Lastnode->Source_Processor_id == Global_Node_id) && (Lastnode->DTask_id == TaskDoneFlag)) && (Lastnode != NULL))
							Lastnode = Lastnode->Next_TaskHandle_List;
						if(Lastnode != NULL){
							if(Lastnode->Barrier > 0){
								Get_key = 1;
								Barrier = 1;
							}
							else{
								if((PublishFlag != 0) && (Local_RequestKeyFlag == 0))
									Get_key = Distributed_NodeRequestReleaseSequence(Request);
								if(Get_key != 0)
									Local_RequestKeyFlag = 1;
							}
						}
						if(Get_key != 0){
							get_key_flag = 1;
							global_record_time_dispatch_array[33] = xTaskGetTickCount() - global_record_time;
							vPortEnterCritical();
							Lastnode = DFinish;																											//	Ready to recycle task_id = TaskDoneFlag result
							Distributed_TaskHandle_List_t* before_target_node = NULL;
							Distributed_TaskHandle_List_t* Last_target_node;
							while( !((Lastnode->Source_Processor_id == Global_Node_id) && (Lastnode->DTask_id == TaskDoneFlag)) && (Lastnode != NULL)){	//	Find the target node DTCB head in DF_Start
								before_target_node = Lastnode;
								Lastnode = Lastnode->Next_TaskHandle_List;
							}
							if(Lastnode == DFinish)
								before_target_node = NULL;

							uint32_t Total_result_size = 0;
							uint32_t target_node_count = 0;
							Distributed_TaskHandle_List_t* Targetnodehead = Lastnode;
							if(Lastnode != NULL){
								while((Lastnode->Source_Processor_id == Global_Node_id) && (Lastnode->DTask_id == TaskDoneFlag) && (Lastnode != NULL)){
									Last_target_node = Lastnode;
									target_node_count++;
									Total_result_size += Lastnode->Data_number;
									Lastnode = Lastnode->Next_TaskHandle_List;
								}

								if(before_target_node != NULL)															//	Move all fiinish nodes from DFinish
									before_target_node->Next_TaskHandle_List = Last_target_node->Next_TaskHandle_List;
								else
									DFinish = Last_target_node->Next_TaskHandle_List;
								Last_target_node->Next_TaskHandle_List = NULL;

								Distributed_TaskHandle_List_t* target_node_array[target_node_count];
								Distributed_TaskHandle_List_t* Subtask_node_zero = NULL;
								uint32_t recompact_nodes_size = 0;
								Lastnode = Targetnodehead;
								target_node_count = 0;
								while((Lastnode->DTask_id == TaskDoneFlag) && (Lastnode != NULL)){					//	record nodes dtcb addr, node count and calculate all nodes dtcb and node 0 data size
									if(Lastnode->DSubTask_id == 0){
										Subtask_node_zero = Lastnode;
										#if (PrintSendRecv > 0)
											printf("Subtask_node_zero, DTask_id: 0x%lX, DSubTask_id: 0x%lX\r\n", Subtask_node_zero->DTask_id, Subtask_node_zero->DSubTask_id);
										#endif
										recompact_nodes_size += ((Lastnode->Data_number)*sizeof(uint32_t));
									}
									target_node_array[target_node_count] = Lastnode;
									recompact_nodes_size += sizeof(Distributed_TaskHandle_List_t);
									target_node_count++;
									Lastnode = Lastnode->Next_TaskHandle_List;
								}
								if(Subtask_node_zero == NULL){
									printf("Cannot find Subtask_node_zero\r\n");
								}
								uint32_t compact_flag = 0;
								Distributed_Data_t* Send_S = pvPortMalloc(sizeof(Distributed_Data_t));				//	Send result back to  caller, important
								Send_S->xQueue = Subtask_node_zero->xQueue;
								Send_S->TaskHandle = (TaskHandle_t)Subtask_node_zero->TaskHandlex;
								if(Subtask_node_zero->Barrier > 0){													//	Without Barrier
									Send_S->Data_addr = Subtask_node_zero->Data_addr;
									Send_S->Data_size = Total_result_size;
								}
								else{																				//	With Barrier
									Send_S->Data_addr = pvPortMalloc(Total_result_size*sizeof(uint32_t));
									Send_S->Data_size = Total_result_size;
									if(Send_S->Data_addr == NULL){
										compact_flag = 1;
										//printf("Dame 0, not enough size, need: 0x%lX bytes, try compact dtcb info\r\n", (Total_result_size*sizeof(uint32_t)));
										uint8_t* recompact_nodes_addr = pvPortMalloc(recompact_nodes_size);						//	Compact all node dtcb and node 0 data
										for(uint32_t i=0;i<target_node_count;i++){
											uint8_t* sour_node = (uint8_t*)target_node_array[i];
											uint8_t* dest_node = (uint8_t*)recompact_nodes_addr;
											Distributed_TaskHandle_List_t* tmp_node = (Distributed_TaskHandle_List_t*)dest_node;
											for(uint32_t j=0;j<sizeof(Distributed_TaskHandle_List_t);j++){
												*(dest_node+j) = *(sour_node+j);
											}
											recompact_nodes_addr += sizeof(Distributed_TaskHandle_List_t);
											if(target_node_array[i]->DSubTask_id == 0){
												dest_node = recompact_nodes_addr;
												tmp_node->Data_addr = (uint32_t*)dest_node;
												for(uint32_t j=0;j<((target_node_array[i]->Data_number*sizeof(uint32_t)));j++){
													*(dest_node+j) = *((uint8_t*)(target_node_array[i]->Data_addr)+j);
												}
												recompact_nodes_addr += ((target_node_array[i]->Data_number*sizeof(uint32_t)));
											}
											tmp_node->Next_TaskHandle_List = (Distributed_TaskHandle_List_t*)recompact_nodes_addr;
											if(i == (target_node_count-1))
												tmp_node->Next_TaskHandle_List = NULL;
											vPortFree(target_node_array[i]);
											target_node_array[i] = tmp_node;
											if(target_node_array[i]->DSubTask_id == 0)
												Subtask_node_zero = target_node_array[i];
										}
										Targetnodehead = target_node_array[0];
										Send_S->Data_addr = pvPortMalloc(Total_result_size*sizeof(uint32_t));
									}
									if(Send_S->Data_addr == NULL){
										printf("That over, just giveup mtfk\r\n");
									}
									else{																														//	Ready to recycle result, Node_id: target_node_array, size: Total_result_size
										uint32_t* tmp_Target_Addr = Send_S->Data_addr;
										for(uint32_t i=0;i<target_node_count;i++){
											#if (PrintSendRecv > 0)
												uint32_t* Result_Start_Addr = tmp_Target_Addr;
												uint32_t* Result_Stop_Addr;
											#endif
											if(target_node_array[i]->DSubTask_id == 0){
												uint32_t* result_addr = target_node_array[i]->Data_addr;
												for(uint32_t j=0;j<(target_node_array[i]->Data_number);j++)
													*(tmp_Target_Addr+j)= *(result_addr+j);
												tmp_Target_Addr += target_node_array[i]->Data_number;
											}
											else{
												uint32_t Recv_Total_Size = target_node_array[i]->Data_number*sizeof(uint32_t);
												uint32_t Recv_Remain_Size = Recv_Total_Size;
												uint8_t* Recv_Addr = (uint8_t*)tmp_Target_Addr;
												uint32_t Recv_Size = 0;
												uint32_t Remain_th = 0;
												uint32_t Distributed_NodeResponseResult_Header_Size = 13;
												global_record_time_dispatch_array[44+target_node_array[i]->DSubTask_id] = xTaskGetTickCount() - global_record_time;
												uint32_t tmp_count = 0;
												uint32_t tmp_global_record_data_4 = xTaskGetTickCount();
												global_record_data[1] += (target_node_array[i]->Data_number*sizeof(uint32_t));
												global_record_data[2] += Recv_Remain_Size;

												while(Recv_Remain_Size > 0){
													if(Remain_th > 0){
														uint32_t Distributed_NodeResponseRemainResult_Header_Size = 17;
														Distributed_NodeResponseResult_Header_Size = Distributed_NodeResponseRemainResult_Header_Size;
													}
													uint32_t Recv_Data_Number = (ETH_FRAM_SIZE - Distributed_NodeResponseResult_Header_Size);
													if(Recv_Remain_Size > Recv_Data_Number)
														Recv_Size = Recv_Data_Number;
													else
														Recv_Size = Recv_Remain_Size;
													RequestResultFlag = 0;
													RemainThResultFlag = 0;
													if(Remain_th == 0){
														Distributed_NodeRequestResult(target_node_array[i]->Destinate_Processor_id, target_node_array[i]->DTask_id, target_node_array[i]->DSubTask_id);
													}
													else{
														Distributed_NodeRequestRemainResult(target_node_array[i]->Destinate_Processor_id, Remain_th);
													}
													vPortExitCritical();
													WaitForFlag(&RequestResultFlag, 1);
													vPortEnterCritical();
													if((RequestResultFlag == target_node_array[i]->Destinate_Processor_id) && (RemainThResultFlag == (Remain_th+1))){
														uint8_t* frame_addr = (uint8_t*)((DMA_RX_FRAME_infos->FS_Rx_Desc)->Buffer1Addr);
														uint8_t* result_addr = ((uint8_t*)frame_addr+Distributed_NodeResponseResult_Header_Size);

														for(uint32_t j=0;j<Recv_Size;j++)
															*(Recv_Addr+j)= *(result_addr+j);
														Recv_Addr +=  Recv_Size;
														Recv_Remain_Size -= Recv_Size;
														Remain_th++;
														tmp_Target_Addr = (uint32_t*)Recv_Addr;
														if(Recv_Remain_Size <= 0){
															Distributed_NodeRequestRemainResult(target_node_array[i]->Destinate_Processor_id, Remain_th);
															SendCompleteFlag = 0;
															vPortExitCritical();
															uint8_t Flag = 0;
															while(Flag == 0){
																Flag = Distributed_NodeRecvCompleteSequence(target_node_array[i]->Destinate_Processor_id);
																if(Flag == 0){
																	Distributed_NodeRequestRemainResult(target_node_array[i]->Destinate_Processor_id, Remain_th);
																}
															}
															vPortEnterCritical();
														}
													}
													else{
														global_record_fail_count[target_node_array[i]->Destinate_Processor_id+4-1] = global_record_fail_count[target_node_array[i]->Destinate_Processor_id+4-1]+1;
													}
													tmp_count++;
													if((tmp_count%100) == 0){
														printf("RequestResult, count: 0x%lX, Remain_th: 0x%lX, Destinate: 0x%lX, DTask_id: 0x%lX, DSubTask_id: 0x%lX\r\n", tmp_count, Remain_th, target_node_array[i]->Destinate_Processor_id, target_node_array[i]->DTask_id, target_node_array[i]->DSubTask_id);
													}
												}
												global_record_data[4] += xTaskGetTickCount() - tmp_global_record_data_4;
												send_recv_data_time[send_recv_data_time_count] += xTaskGetTickCount() - tmp_global_record_data_4;
												send_recv_data_time_count++;
												global_record_time_dispatch_array[48+target_node_array[i]->DSubTask_id] = xTaskGetTickCount() - global_record_time;
											}
											/*
											printf("Destinate_Processor_id: 0x%lX\r\n", target_node_array[i]->Destinate_Processor_id);
											uint32_t* Result_Stop_Addr = Result_Start_Addr + target_node_array[i]->Data_number;
											for(uint32_t i=0;i<5;i++)
												printf("0x%lX, 0x%lX	0x%lX, 0x%lX\r\n", (uint32_t)(Result_Start_Addr+i), *(Result_Start_Addr+i), (uint32_t)(Result_Stop_Addr-5+i), *(Result_Stop_Addr-5+i));
											*/
											SubtaskFinishArray[target_node_array[i]->Destinate_Processor_id-1]++;
										}
									}
								}
								//Distributed_NodeRemoveTask(Broadcast_Node_id, Global_Node_id, Targetnodehead->DTask_id);
								if(compact_flag == 1)
									vPortFree(Targetnodehead);						//	After compact nodes info, just free the first node
								else{
									Lastnode = Targetnodehead;
									while(Lastnode != NULL){
										Distributed_TaskHandle_List_t* tmp_node = Lastnode;
										Lastnode = Lastnode->Next_TaskHandle_List;
										vPortFree(tmp_node);
									}
								}
								if(Send_S->Data_addr == NULL){
									printf("After Free Nodes info\r\n");
									ListFreeBlock();
								}
								/*
								for(uint32_t i=0;i<Send_S->Data_size;i++)
									printf("Send_S, 0x%lX, 0x%lX\r\n", (uint32_t)(Send_S->Data_addr+i), *(Send_S->Data_addr+i));
								*/
								vPortExitCritical();
								xQueueSendToBack(*((QueueHandle_t*)Send_S->xQueue), (void*)&Send_S, 0);
								vPortEnterCritical();
							}
							else{
								printf("Can't find target node\r\n");
							}
							TaskDoneFlag = 0;
							vPortExitCritical();
							global_record_time_dispatch_array[34] = xTaskGetTickCount() - global_record_time;
							if(Barrier <= 0){
								Get_key = 0;
								while(Get_key == 0)
									Get_key = Distributed_NodeRequestReleaseSequence(Release);
								Local_RequestKeyFlag = 0;
							}
							global_record_time_dispatch_array[35] = xTaskGetTickCount() - global_record_time;
						}
						else{
							//printf("Key is occupy, try again in future\r\n");
							;
						}
						global_record_time_dispatch_array[3] = xTaskGetTickCount() - global_record_time;
						if(get_key_flag > 0)
							global_record_data[6] += (xTaskGetTickCount()-tmp_global_record_data_6);
					}

					/*
					else if(NodeInvalidFlag != 0){
						portenterCRITICAL();
						printf("NodeInvalidFlag: 0x%lX\r\n", NodeInvalidFlag);
						Distributed_TaskHandle_List_t* Lastnode = DStart;
						Distributed_TaskHandle_List_t* pre_Lastnode = Lastnode;
						while(Lastnode != NULL){
							if(Lastnode->Source_Processor_id == NodeInvalidFlag){
								Distributed_TaskHandle_List_t* tmp_node = Lastnode;
								if(Lastnode == DStart){
									Lastnode = Lastnode->Next_TaskHandle_List;
									pre_Lastnode = Lastnode;
								}
								else{
									Lastnode = Lastnode->Next_TaskHandle_List;
									pre_Lastnode->Next_TaskHandle_List = Lastnode;
									// ??? Free Unfinish task memory
								}
							}
							else{
								pre_Lastnode = Lastnode;
								Lastnode = Lastnode->Next_TaskHandle_List;
							}
						}
						for(uint32_t i=0;i<2;i++){
							Distributed_TaskHandle_List_t* Lastnode;
							if(i == 0){
								Lastnode = DStart;
							}
							else{
								Lastnode = DFinish;
							}
							Distributed_TaskHandle_List_t* head_Lastnode = Lastnode;
							Distributed_TaskHandle_List_t* pre_Lastnode = Lastnode;
							while(Lastnode != NULL){
								if((Lastnode->Source_Processor_id == NodeInvalidFlag) || (Lastnode->Destinate_Processor_id == NodeInvalidFlag)){
									Distributed_TaskHandle_List_t* tmp_node = Lastnode;
									if(Lastnode == head_Lastnode){
										Lastnode = Lastnode->Next_TaskHandle_List;
										pre_Lastnode = Lastnode;
									}
									else{
										Lastnode = Lastnode->Next_TaskHandle_List;
										pre_Lastnode->Next_TaskHandle_List = Lastnode;
										//????? Free Unfinish task memory
										//need to free memory kill the unfinish subtask, and kill all relative task and free all memory and return fail massenge to the waiting task
									}
									if(tmp_node->Source_Processor_id == Global_Node_id){
										Distributed_TaskHandle_List_t* Lastnode = head_Lastnode;
									}
								}
								else{
									pre_Lastnode = Lastnode;
									Lastnode = Lastnode->Next_TaskHandle_List;
								}
							}
						}
						vPortExitCritical();
					}
					*/
					else if((DRequest != NULL) && (PublishFlag > 0)){
						//	have insert to request result list
						//	check WHETHER the last finish node, (check finish node same subtask number)
						//	should request result
						//	get result and insert dtcb to finish list
						uint32_t tmp_global_record_data_6 = xTaskGetTickCount();
						uint32_t DRequest_before = xTaskGetTickCount() - global_record_time;
						uint32_t Get_key = 0;
						uint32_t get_key_flag = 0;
						if((PublishFlag != 0) && (Local_RequestKeyFlag == 0))
							Get_key = Distributed_NodeRequestReleaseSequence(Request);
						if(Get_key != 0)
							Local_RequestKeyFlag = 1;
						uint32_t DRequest_after = xTaskGetTickCount() - global_record_time;
						if(Get_key > 0){
							vPortEnterCritical();
							get_key_flag = 1;
							while(DRequest != NULL){
								global_record_time_dispatch_array[8+DRequest->DSubTask_id] = DRequest_before;
								global_record_time_dispatch_array[12+DRequest->DSubTask_id] = DRequest_after;
								Distributed_TaskHandle_List_t* Lastnode = DRequest;
								uint32_t tmp_count = 1;
								uint32_t* Result_addr = NULL;
								uint32_t* unrecycle_Result_addr = NULL;
								uint32_t Result_size = Lastnode->Data_number;
								uint8_t allsubtaskdoneflag = 0;
								Distributed_TaskHandle_List_t* check_Lastnode = DFinish;
								while(check_Lastnode != NULL){
									if((check_Lastnode->DTask_id == Lastnode->DTask_id) && (check_Lastnode->Source_Processor_id == Global_Node_id)){
										tmp_count++;
										Result_size += check_Lastnode->Data_number;
										if(tmp_count >= Lastnode->DSubTask_number){
											allsubtaskdoneflag = 1;
											break;
										}
									}
									check_Lastnode = check_Lastnode->Next_TaskHandle_List;
								}
								if(allsubtaskdoneflag > 0){
									Result_addr = pvPortMalloc(Result_size*sizeof(uint32_t));
									if(Result_addr == NULL)
										printf("Dame 1, not enough size, need: 0x%lX bytes\r\n", (Result_size*sizeof(uint32_t)));
									uint32_t Subtask_id_size[Lastnode->DSubTask_number];
									check_Lastnode = DFinish;
									while(check_Lastnode != NULL){
										if((check_Lastnode->DTask_id == Lastnode->DTask_id) && (check_Lastnode->Source_Processor_id == Global_Node_id)){
											Subtask_id_size[check_Lastnode->DSubTask_id] = check_Lastnode->Data_number;
										}
										check_Lastnode = check_Lastnode->Next_TaskHandle_List;
									}
									Subtask_id_size[Lastnode->DSubTask_id] = Lastnode->Data_number;

									check_Lastnode = DFinish;
									while(check_Lastnode != NULL){
										if((check_Lastnode->DTask_id == Lastnode->DTask_id) && (check_Lastnode->Source_Processor_id == Global_Node_id)){
											uint32_t* Dest_Result_addr = Result_addr;
											for(uint32_t i=0;i<(check_Lastnode->DSubTask_id);i++)
												Dest_Result_addr += Subtask_id_size[i];
											for(uint32_t i=0;i<check_Lastnode->Data_number;i++)
												*(Dest_Result_addr+i) = *(check_Lastnode->Data_addr+i);
											if(check_Lastnode->DSubTask_id != 0)
												vPortFree(check_Lastnode->Data_addr);
											check_Lastnode->Data_addr = Dest_Result_addr;
										}
										check_Lastnode = check_Lastnode->Next_TaskHandle_List;
									}
									unrecycle_Result_addr = Result_addr;
									for(uint32_t i=0;i<(Lastnode->DSubTask_id);i++)
										unrecycle_Result_addr += Subtask_id_size[i];
								}

								if(unrecycle_Result_addr == NULL){
									unrecycle_Result_addr = pvPortMalloc(Lastnode->Data_number*sizeof(uint32_t));
									if(unrecycle_Result_addr == NULL)
										printf("Dame 2, not enough size, need: 0x%lX bytes\r\n", (Lastnode->Data_number*sizeof(uint32_t)));
								}
								Lastnode->Data_addr = unrecycle_Result_addr;
								uint32_t Recv_Total_Size = Lastnode->Data_number*sizeof(uint32_t);
								uint32_t Recv_Remain_Size = Recv_Total_Size;
								uint8_t* Recv_Addr = (uint8_t*)unrecycle_Result_addr;
								uint32_t Recv_Size = 0;
								uint32_t Remain_th = 0;
								uint32_t Distributed_NodeResponseResult_Header_Size = 13;
								global_record_time_dispatch_array[44+Lastnode->DSubTask_id] = xTaskGetTickCount() - global_record_time;
								uint32_t tmp_global_record_data_4 = xTaskGetTickCount();
								global_record_data[1] += (Lastnode->Data_number*sizeof(uint32_t));
								global_record_data[2] += Recv_Remain_Size;

								while(Recv_Remain_Size > 0){
									if(Remain_th > 0){
										uint32_t Distributed_NodeResponseRemainResult_Header_Size = 17;
										Distributed_NodeResponseResult_Header_Size = Distributed_NodeResponseRemainResult_Header_Size;
									}
									uint32_t Recv_Data_Number = (ETH_FRAM_SIZE - Distributed_NodeResponseResult_Header_Size);
									if(Recv_Remain_Size > Recv_Data_Number)
										Recv_Size = Recv_Data_Number;
									else
										Recv_Size = Recv_Remain_Size;
									RequestResultFlag = 0;
									RemainThResultFlag = 0;
									if(Lastnode->Destinate_Processor_id > Global_Node_count)
										printf("Dame, Lastnode->Destinate_Processor_id: 0x%lX\r\n", Lastnode->Destinate_Processor_id);
									if(Remain_th == 0){
										Distributed_NodeRequestResult(Lastnode->Destinate_Processor_id, Lastnode->DTask_id, Lastnode->DSubTask_id);
									}
									else{
										Distributed_NodeRequestRemainResult(Lastnode->Destinate_Processor_id, Remain_th);
									}
									vPortExitCritical();
									WaitForFlag(&RequestResultFlag, 1);
									vPortEnterCritical();
									if((RequestResultFlag == Lastnode->Destinate_Processor_id) && (RemainThResultFlag == (Remain_th+1))){
										uint8_t* frame_addr = (uint8_t*)((DMA_RX_FRAME_infos->FS_Rx_Desc)->Buffer1Addr);
										uint8_t* result_addr = ((uint8_t*)frame_addr+Distributed_NodeResponseResult_Header_Size);
										for(uint32_t i=0;i<Recv_Size;i++)
											*(Recv_Addr+i)= *(result_addr+i);
										Recv_Addr += Recv_Size;
										Recv_Remain_Size -= Recv_Size;
										Remain_th++;
										if(Recv_Remain_Size <= 0){
											Distributed_NodeRequestRemainResult(Lastnode->Destinate_Processor_id, Remain_th);
											SendCompleteFlag = 0;
											vPortExitCritical();
											uint8_t Flag = 0;
											while(Flag == 0){
												Flag = Distributed_NodeRecvCompleteSequence(Lastnode->Destinate_Processor_id);
												if(Flag == 0){
													Distributed_NodeRequestRemainResult(Lastnode->Destinate_Processor_id, Remain_th);
												}
											}
											vPortEnterCritical();
										}
									}
								}
								global_record_data[4] += xTaskGetTickCount() - tmp_global_record_data_4;
								global_record_time_dispatch_array[48+Lastnode->DSubTask_id] = xTaskGetTickCount() - global_record_time;
								send_recv_data_time[send_recv_data_time_count] += xTaskGetTickCount() - tmp_global_record_data_4;
								send_recv_data_time_count++;

								DRequest = Lastnode->Next_TaskHandle_List;
								Lastnode->Next_TaskHandle_List = NULL;
								Lastnode->Finish_Flag = 1;
								Distributed_InsertFinishNode(Lastnode);
								SubtaskFinishArray[Lastnode->Destinate_Processor_id-1]++;
								if(allsubtaskdoneflag > 0){
									TaskDoneFlag = Lastnode->DTask_id;
									//printf("TaskDoneFlag in DRequest 1: 0x%lX\r\n", TaskDoneFlag);
								}
								else{
									tmp_count = 0;
									Result_size = 0;
									check_Lastnode = DFinish;
									while(check_Lastnode != NULL){
										if((check_Lastnode->DTask_id == Lastnode->DTask_id) && (check_Lastnode->Source_Processor_id == Global_Node_id)){
											tmp_count++;
											Result_size += check_Lastnode->Data_number;
											if(tmp_count >= Lastnode->DSubTask_number){
												allsubtaskdoneflag = 1;
												break;
											}
										}
										check_Lastnode = check_Lastnode->Next_TaskHandle_List;
									}
									if(allsubtaskdoneflag > 0){
										Result_addr = pvPortMalloc(Result_size*sizeof(uint32_t));
										if(Result_addr == NULL)
											printf("Dame 3, not enough size, need: 0x%lX bytes\r\n", (Result_size*sizeof(uint32_t)));
										uint32_t Subtask_id_size[Lastnode->DSubTask_number];
										check_Lastnode = DFinish;
										while(check_Lastnode != NULL){
											if((check_Lastnode->DTask_id == Lastnode->DTask_id) && (check_Lastnode->Source_Processor_id == Global_Node_id)){
												Subtask_id_size[check_Lastnode->DSubTask_id] = check_Lastnode->Data_number;
											}
											check_Lastnode = check_Lastnode->Next_TaskHandle_List;
										}
										check_Lastnode = DFinish;
										while(check_Lastnode != NULL){
											if((check_Lastnode->DTask_id == Lastnode->DTask_id) && (check_Lastnode->Source_Processor_id == Global_Node_id)){
												uint32_t* Dest_Result_addr = Result_addr;
												for(uint32_t i=0;i<(check_Lastnode->DSubTask_id);i++)
													Dest_Result_addr += Subtask_id_size[i];
												for(uint32_t i=0;i<check_Lastnode->Data_number;i++)
													*(Dest_Result_addr+i) = *(check_Lastnode->Data_addr+i);
												if(check_Lastnode->DSubTask_id != 0)
													vPortFree(check_Lastnode->Data_addr);
												check_Lastnode->Data_addr = Dest_Result_addr;
											}
											check_Lastnode = check_Lastnode->Next_TaskHandle_List;
										}
										TaskDoneFlag = Lastnode->DTask_id;
										//printf("TaskDoneFlag in DRequest 2: 0x%lX\r\n", TaskDoneFlag);
									}
								}
							}
							vPortExitCritical();
							DRequest_before = xTaskGetTickCount() - global_record_time;
							Get_key = 0;
							while(Get_key == 0)
								Get_key = Distributed_NodeRequestReleaseSequence(Release);
							Local_RequestKeyFlag = 0;
							DRequest_after = xTaskGetTickCount() - global_record_time;
							for(uint32_t i=0;i<4;i++){
								if((global_record_time_dispatch_array[8+i] != 0) && (global_record_time_dispatch_array[16+i] == 0)){
									global_record_time_dispatch_array[16+i] = DRequest_before;
									global_record_time_dispatch_array[20+i] = DRequest_after;
								}
							}
						}
						if(get_key_flag > 0)
							global_record_data[6] += (xTaskGetTickCount() - tmp_global_record_data_6);
					}

					else if((DSendFinish != NULL) && (PublishFlag > 0)){
						vPortEnterCritical();
						Distributed_TaskHandle_List_t* Lastnode = DSendFinish;
						Distributed_TaskHandle_List_t* pre_Lastnode = DSendFinish;
						while(Lastnode != NULL){
							ResponseSubtaskFinishFlag = 0;
							//if(PublishFlag != 0)
							Distributed_NodeSubtaskFinish(Lastnode->Source_Processor_id, Lastnode->DTask_id, Lastnode->DSubTask_id, Lastnode->Data_number);
							vPortExitCritical();
							WaitForFlag(&ResponseSubtaskFinishFlag, 1);
							vPortEnterCritical();

							if(ResponseSubtaskFinishFlag == Lastnode->Source_Processor_id){
								ResponseSubtaskFinishFlag = 0;
								if((ResponseSubtaskFinishTaskFlag == Lastnode->DTask_id) && (ResponseSubtaskFinishSubtaskFlag == Lastnode->DSubTask_id)){
									ResponseSubtaskFinishTaskFlag = 0;
									ResponseSubtaskFinishSubtaskFlag = 0;
									Distributed_TaskHandle_List_t* tmp_Lastnode = DSendFinish;
									while(tmp_Lastnode != NULL){
										if(tmp_Lastnode == Lastnode)
											break;
										tmp_Lastnode = tmp_Lastnode->Next_TaskHandle_List;
									}
									if(tmp_Lastnode != NULL){
										if(Lastnode == DSendFinish)
											DSendFinish = Lastnode->Next_TaskHandle_List;
										else
											pre_Lastnode->Next_TaskHandle_List = Lastnode->Next_TaskHandle_List;
										Lastnode->Next_TaskHandle_List = NULL;
										Distributed_InsertFinishNode(Lastnode);
									}
									else{
										tmp_Lastnode = DFinish;
										while(tmp_Lastnode != NULL){
											if(tmp_Lastnode == Lastnode)
												break;
											tmp_Lastnode = tmp_Lastnode->Next_TaskHandle_List;
										}
										/*
										if(tmp_Lastnode != NULL)
											printf("Lastnode has in DFinish\r\n");
										else
											printf("Lost Lastnode\r\n");
										*/
									}
									break;
								}
							}
							else{
								//printf("Timeout in Distributed_NodeSubtaskFinish\r\n");
								;
							}
							if(Lastnode->Source_Processor_id > Global_Node_count){		//Dame, Source_Processor_id is invalid
								//printf("Dame, Source_Processor_id is invalid: 0x%lX, DTask_id: 0x%lX, DSubTask_id: 0x%lX\r\n", Lastnode->Source_Processor_id, Lastnode->DTask_id, Lastnode->DSubTask_id);
								;
							}
							pre_Lastnode = Lastnode;
							Lastnode = Lastnode->Next_TaskHandle_List;
						}
						vPortExitCritical();
					}
					else if(DDelete != NULL){
						vPortEnterCritical();
						while(DDelete != NULL){
							Distributed_TaskHandle_List_t* remove_node = DDelete;
							DDelete = DDelete->Next_TaskHandle_List;
							vPortFree(remove_node);
						}
						vPortExitCritical();
					}

					else if((BlockChangeFlag > 0) && (PublishFlag > 0)){
						if((Check_Sendable())){
							vPortEnterCritical();
							Distributed_NodeSendFreeBlock(0xffffffff, Global_Node_id);
							vPortExitCritical();
						}
					}
				}
				rec_cmd = '\0';
			}
		}
	}
}

uint32_t WaitForFlag(volatile uint32_t* Flag_Addr, uint32_t timeout_time){
	uint32_t base_tick = xTaskGetTickCount();
	uint32_t timeout_tick = base_tick + timeout_time*Timeout_Tick_Count;
	while(*(Flag_Addr) == 0){
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
	return *(Flag_Addr);
}

// UserDefine funciton
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Distributed task example, in the distributed task function
void UserDefine_Distributed_Task(void *task_info){
	Distributed_TaskHandle_List_t *data_info = Distributed_Start(task_info);
	Distributed_Data_t* array1 = Distributed_GetTragetData(data_info);

	/*
	Distributed_Data_t* array2 = Distributed_GetTragetData(data_info);
	Distributed_Data_t* array3 = Distributed_GetTragetData(data_info);
	*/
	/*
	uint32_t* malloc_addr = NULL;
	uint32_t malloc_size = array1->Data_size;
	Distributed_pvPortMalloc(malloc_addr, malloc_size*sizeof(uint32_t));
	//Distributed_vPortFree(malloc_addr);
	*/
	/*
	for(uint32_t i=0;i<array1->Data_size;i++)
		*(array1->Data_addr + i) = *(array1->Data_addr + i)*2;
	*/
	/*
	for(uint32_t i=0;i<array2->Data_size;i++)
		*(array2->Data_addr + i) = *(array2->Data_addr + i)*3;
	for(uint32_t i=0;i<array3->Data_size;i++)
		*(array3->Data_addr + i) = *(array3->Data_addr + i)*4;
	*/
	Distributed_End(data_info, array1->Data_addr, array1->Data_size);
}

void UserDefine_Distributed_Task_matrix_multiplication(void *task_info){
	Distributed_TaskHandle_List_t *data_info = Distributed_Start(task_info);
	if(data_info->DSubTask_id == 0)
		printf("mtfk\r\n");
	Distributed_Data_t* array1 = Distributed_GetTragetData(data_info);
	Distributed_Data_t* array2 = Distributed_GetTragetData(data_info);
	Distributed_Data_t* array1_column = Distributed_GetTragetData(data_info);
	Distributed_Data_t* array2_column = Distributed_GetTragetData(data_info);
	uint32_t column1 = (uint32_t)*(array1_column->Data_addr);
	uint32_t column2 = (uint32_t)*(array2_column->Data_addr);
	uint32_t row1 = array1->Data_size/column1;
	uint32_t target_size = row1*column2*sizeof(uint32_t);
	uint32_t* target_addr;
	Distributed_pvPortMalloc(target_addr, target_size);
	for(uint32_t i=0;i<row1;i++)
		for(uint32_t j=0;j<column2;j++)
			for(uint32_t k=0;k<column1;k++)
				*(target_addr+(i*column2)+j) += ((uint32_t)*(array1->Data_addr+(i*column1)+k))*((uint32_t)*(array2->Data_addr+(k*column2)+j));
	Distributed_End(data_info, target_addr, target_size);
}

void UserDefine_Distributed_Task_do_nothing(void *task_info){
	Distributed_TaskHandle_List_t *data_info = Distributed_Start(task_info);
	// test_inline();
	uint32_t tmp_get_tickcount = 0;
	if(data_info->DSubTask_id == 0)
		tmp_get_tickcount = xTaskGetTickCount();
	Distributed_Data_t* array1 = Distributed_GetTragetData(data_info);
	if(data_info->DSubTask_id == 0)
		global_record_data[5] += (xTaskGetTickCount() - tmp_get_tickcount);
	Distributed_End(data_info, array1->Data_addr, array1->Data_size);
}

void UserDefine_Distributed_Task_multiple(void *task_info){
	Distributed_TaskHandle_List_t *data_info = Distributed_Start(task_info);
	uint32_t tmp_get_tickcount = 0;
	if(data_info->DSubTask_id == 0)
		tmp_get_tickcount = xTaskGetTickCount();
	Distributed_Data_t* array1 = Distributed_GetTragetData(data_info);
	for(uint32_t i=0;i<array1->Data_size;i++)
		*(array1->Data_addr + i) = *(array1->Data_addr + i)*2;
	if(data_info->DSubTask_id == 0)
		global_record_data[5] += (xTaskGetTickCount() - tmp_get_tickcount);
	Distributed_End(data_info, array1->Data_addr, array1->Data_size);
}

void UserDefine_Distributed_Task_2d_array_convolution(void *task_info){
	Distributed_TaskHandle_List_t* data_info = Distributed_Start(task_info);

	uint32_t tmp_get_tickcount = 0;
	if(data_info->DSubTask_id == 0)
		tmp_get_tickcount = xTaskGetTickCount();

	Distributed_Data_t* array = Distributed_GetTragetData(data_info);
	Distributed_Data_t* array_column = Distributed_GetTragetData(data_info);
	Distributed_Data_t* kernel = Distributed_GetTragetData(data_info);
	Distributed_Data_t* kernel_column = Distributed_GetTragetData(data_info);
	uint32_t array_a_column = *(array_column->Data_addr);
	uint32_t kernel_a_column = *(kernel_column->Data_addr);
	uint32_t* malloc_addr = NULL;
	uint32_t malloc_size = array->Data_size;
	Distributed_pvPortMalloc(malloc_addr, malloc_size*sizeof(uint32_t));
	for(uint32_t i=0;i<array->Data_size;i++){
		int operator[] = {0, 0};
		uint32_t tmp_sum = 0;
		for(int j=-1;j<2;j++){
			for(int k=-1;k<2;k++){
				operator[0] = j;
				operator[1] = k;
				if((i<array_a_column)&&(operator[0] == -1))
					operator[0] = 1;
				if((i>=(array->Data_size-array_a_column))&&(operator[0] == 1))
					operator[0] = -1;
				if(((i%array_a_column)==0)&&(operator[1] == -1))
					operator[1] = 1;
				if((((i+1)%array_a_column)==0)&&(operator[1] == 1))
					operator[1] = -1;
				uint32_t tmp_index = (uint32_t)((int)i + operator[0]*array_a_column + operator[1]);
				tmp_sum += (*(array->Data_addr+tmp_index))*(*(kernel->Data_addr+(j+1)*kernel_a_column+(k+1)));
			}
		}
		*(malloc_addr+i) = tmp_sum;
	}

	if(data_info->DSubTask_id == 0)
		global_record_data[5] += (xTaskGetTickCount() - tmp_get_tickcount);

	Distributed_End(data_info, malloc_addr, malloc_size);
}

void UserDefine_Distributed_Task_bgr_gray_transform(void *task_info){
	Distributed_TaskHandle_List_t* data_info = Distributed_Start(task_info);

	uint32_t tmp_get_tickcount = 0;
	if(data_info->DSubTask_id == 0)
		tmp_get_tickcount = xTaskGetTickCount();

	Distributed_Data_t* array = Distributed_GetTragetData(data_info);
	uint8_t* image_addr = (uint8_t*)array->Data_addr;
	uint32_t image_size = sizeof(uint32_t)*(array->Data_size);
	uint8_t* malloc_addr = NULL;
	uint32_t malloc_size = image_size/2;
	Distributed_pvPortMalloc(malloc_addr, malloc_size);
	for(uint32_t i=0;i<malloc_size;i++){
		uint32_t B = (uint32_t)((*(image_addr+(2*i)) & 0xF8) >> 3);
		uint32_t G = ((uint32_t)((*(image_addr+(2*i)) & 0x07) << 5) | (uint32_t)((*(image_addr+(2*i)+1) & 0x0E0) >> 5));
		uint32_t R = (uint32_t)((*(image_addr+(2*i)+1) & 0x1F));
		*(malloc_addr+i) = (uint8_t)((R*299 + G*587 + B*114 + 500)/1000);
	}
	uint32_t ret_size = malloc_size/sizeof(uint32_t);
	if(data_info->DSubTask_id == 0)
		global_record_data[5] += (xTaskGetTickCount() - tmp_get_tickcount);

	Distributed_End(data_info, malloc_addr, ret_size);
}

void UserDefine_Distributed_Task_RSA(void *task_info){
	Distributed_TaskHandle_List_t *data_info = Distributed_Start(task_info);
	uint32_t tmp_get_tickcount = 0;
	if(data_info->DSubTask_id == 0)
		tmp_get_tickcount = xTaskGetTickCount();
	Distributed_Data_t* array = Distributed_GetTragetData(data_info);
	Distributed_Data_t* e_d_info = Distributed_GetTragetData(data_info);
	Distributed_Data_t* n_info = Distributed_GetTragetData(data_info);

	uint32_t e_d = *(e_d_info->Data_addr);
	uint32_t n = *(n_info->Data_addr);

	uint32_t* malloc_addr = NULL;
	uint32_t malloc_size = array->Data_size;
	Distributed_pvPortMalloc(malloc_addr, malloc_size*sizeof(uint32_t));

	for(uint32_t i=0;i<array->Data_size;i++){
		uint32_t process_data = *(array->Data_addr + i);
		uint32_t result_data = 0;
		for(uint32_t j=0;j<4;j++){
			uint32_t process_byte = ((process_data >> ((j*8))) & 0x000000FF);
			uint32_t cipher = powMod(process_byte, e_d, n);
			result_data |= (cipher << (j*8));
		}
		*(malloc_addr + i) = result_data;
	}

	if(data_info->DSubTask_id == 0)
		global_record_data[5] += (xTaskGetTickCount() - tmp_get_tickcount);
	Distributed_End(data_info, malloc_addr, malloc_size);
}

void UserDefine_Distributed_Task_bgr_gray_transform_with_2D_convolution(void *task_info){
	Distributed_TaskHandle_List_t* data_info = Distributed_Start(task_info);

	uint32_t tmp_get_tickcount = 0;
	if(data_info->DSubTask_id == 0)
		tmp_get_tickcount = xTaskGetTickCount();

	Distributed_Data_t* array = Distributed_GetTragetData(data_info);

	uint8_t* image_addr = (uint8_t*)array->Data_addr;
	uint32_t image_size = sizeof(uint32_t)*(array->Data_size);
	uint8_t* malloc_addr = NULL;
	uint32_t malloc_size = image_size/2;
	Distributed_pvPortMalloc(malloc_addr, malloc_size);
	for(uint32_t i=0;i<malloc_size;i++){
		uint32_t B = (uint32_t)((*(image_addr+(2*i)) & 0xF8) >> 3);
		uint32_t G = ((uint32_t)((*(image_addr+(2*i)) & 0x07) << 5) | (uint32_t)((*(image_addr+(2*i)+1) & 0x0E0) >> 5));
		uint32_t R = (uint32_t)((*(image_addr+(2*i)+1) & 0x1F));
		*(malloc_addr+i) = (uint8_t)((R*299 + G*587 + B*114 + 500)/1000);
	}

	Distributed_Data_t* array_column = Distributed_GetTragetData(data_info);
	Distributed_Data_t* kernel = Distributed_GetTragetData(data_info);
	Distributed_Data_t* kernel_column = Distributed_GetTragetData(data_info);
	uint32_t array_a_column = *(array_column->Data_addr);
	uint32_t kernel_a_column = *(kernel_column->Data_addr);

	uint8_t* tar_malloc_addr = NULL;
	uint32_t tar_malloc_size = malloc_size;
	Distributed_pvPortMalloc(tar_malloc_addr, tar_malloc_size);

	for(uint32_t i=0;i<tar_malloc_size;i++){
		int operator[] = {0, 0};
		uint32_t tmp_sum = 0;
		for(int j=-1;j<2;j++){
			for(int k=-1;k<2;k++){
				operator[0] = j;
				operator[1] = k;
				if((i<array_a_column)&&(operator[0] == -1))
					operator[0] = 1;
				if((i>=(tar_malloc_size-array_a_column))&&(operator[0] == 1))
					operator[0] = -1;
				if(((i%array_a_column)==0)&&(operator[1] == -1))
					operator[1] = 1;
				if((((i+1)%array_a_column)==0)&&(operator[1] == 1))
					operator[1] = -1;
				uint32_t tmp_index = (uint32_t)((int)i + operator[0]*array_a_column + operator[1]);
				tmp_sum += (*(malloc_addr+tmp_index))*(*(kernel->Data_addr+(j+1)*kernel_a_column+(k+1)));
			}
		}
		*(tar_malloc_addr+i) = tmp_sum/25;
	}
	uint32_t ret_size = tar_malloc_size/sizeof(uint32_t);
	if(data_info->DSubTask_id == 0)
		global_record_data[5] += (xTaskGetTickCount() - tmp_get_tickcount);
	Distributed_vPortFree(malloc_addr);
	Distributed_End(data_info, tar_malloc_addr, ret_size);
}

// Local task example
void UserDefine_Local_Task_2d_array_convolution(uint32_t rec_Count){
	uint32_t kernel_Data_addr[] = {2, 2, 2, 2, 2, 2, 2, 2, 2};
	uint32_t array_a_column = 128;
	uint32_t kernel_a_column = 3;
	uint32_t* array_Data_addr = (uint32_t*)0x10000000;
	uint32_t array_Data_size = 0x1000;
	for(uint32_t i=0;i<array_Data_size;i++)
		*(array_Data_addr+i) = 1;
	uint32_t tmp_get_tickcount = xTaskGetTickCount();
	uint32_t Count = 0;
	while(Count < rec_Count){
		uint32_t* malloc_addr = pvPortMalloc(array_Data_size*sizeof(uint32_t));
		for(uint32_t i=0;i<array_Data_size;i++){
			int operator[] = {0, 0};
			uint32_t tmp_sum = 0;
			for(int j=-1;j<2;j++){
				for(int k=-1;k<2;k++){
					operator[0] = j;
					operator[1] = k;
					if((i<array_a_column)&&(operator[0] == -1))
						operator[0] = 1;
					if((i>=(array_Data_size-array_a_column))&&(operator[0] == 1))
						operator[0] = -1;
					if(((i%array_a_column)==0)&&(operator[1] == -1))
						operator[1] = 1;
					if((((i+1)%array_a_column)==0)&&(operator[1] == 1))
						operator[1] = -1;
					uint32_t tmp_index = (uint32_t)((int)i + operator[0]*array_a_column + operator[1]);
					tmp_sum += (*(array_Data_addr+tmp_index))*(*(kernel_Data_addr+(j+1)*kernel_a_column+(k+1)));
				}
			}
			*(malloc_addr+i) = tmp_sum;
		}
		vPortFree(malloc_addr);
		DebugFlag = Count;
		Count++;
	}
	uint32_t Local_Task_2d_array_tick = xTaskGetTickCount() - tmp_get_tickcount;
	printf("array_Data_size: %u, Local_Task_2d_array_tick: %u\r\n", (unsigned int)array_Data_size, (unsigned int)Local_Task_2d_array_tick);
}

void UserDefine_Local_Task_RSA(uint32_t rec_Count, uint32_t e_d, uint32_t n, uint32_t array_Data_size){
	uint32_t* array_Data_addr = (uint32_t*)0x10000000;
	for(uint32_t i=0;i<array_Data_size;i++)
		*(array_Data_addr+i) = 0x075E9400;
	uint32_t* malloc_addr = NULL;
	uint32_t malloc_size = array_Data_size;
	malloc_addr = pvPortMalloc(malloc_size*sizeof(uint32_t));
	uint32_t tmp_get_tickcount = xTaskGetTickCount();
	uint32_t Count = 0;
	while(Count < rec_Count){
		for(uint32_t i=0;i<array_Data_size;i++){
			uint32_t process_data = *(array_Data_addr + i);
			uint32_t result_data = 0;
			for(uint32_t j=0;j<4;j++){
				uint32_t process_byte = ((process_data >> ((j*8))) & 0x000000FF);
				uint32_t cipher = powMod(process_byte, e_d, n);
				result_data |= (cipher << (j*8));
			}
			*(malloc_addr + i) = result_data;
		}
		Count++;
		DebugFlag = Count;
	}
	vPortFree(malloc_addr);
	uint32_t Local_Task_RSA_tick = xTaskGetTickCount() - tmp_get_tickcount;
	printf("array_Data_size: %u, Local_Task_RSA_tick: %u\r\n", (unsigned int)array_Data_size, (unsigned int)Local_Task_RSA_tick);
}

void UserDefine_Local_Task_bgr_gray_transform(uint32_t rec_Count){
	#if(SENDIMAGE == 0)
	printf("UserDefine_Local_Task_bgr_gray_transform start\r\n");
	#endif
	uint32_t tmp_get_tickcount = xTaskGetTickCount();
	uint32_t Count = 0;
	while(Count < rec_Count){
		DCMI_Start();
		while(ov_rev_ok == 0)
			;
		DCMI_Stop();
		uint8_t* image_addr = (uint8_t*)camera_buffer;
		uint32_t image_size = (PIC_WIDTH*PIC_HEIGHT*2);
		uint32_t malloc_size = image_size/2;
		uint8_t* malloc_addr = pvPortMalloc(malloc_size);
		for(uint32_t i=0;i<malloc_size;i++){
			uint32_t B = (uint32_t)((*(image_addr+(2*i)) & 0xF8) >> 3);
			uint32_t G = ((uint32_t)((*(image_addr+(2*i)) & 0x07) << 5) | (uint32_t)((*(image_addr+(2*i)+1) & 0x0E0) >> 5));
			uint32_t R = (uint32_t)((*(image_addr+(2*i)+1) & 0x1F));
			*(malloc_addr+i) = (uint8_t)((R*299 + G*587 + B*114 + 500)/1000);
		}
		vPortFree(malloc_addr);
		Count++;
	}
	#if(SENDIMAGE == 0)
	uint32_t UserDefine_Local_Task_bgr_gray_transform = xTaskGetTickCount() - tmp_get_tickcount;
	printf("rec_Count: %u, UserDefine_Local_Task_bgr_gray_transform: %u\r\n", (unsigned int)rec_Count, (unsigned int)UserDefine_Local_Task_bgr_gray_transform);
	#endif
}

void UserDefine_Local_Task_bgr_gray_transform_with_2D_convolution(uint32_t rec_Count){
	#if(SENDIMAGE == 0)
	printf("UserDefine_Local_Task_bgr_gray_transform_with_2D_convolution start\r\n");
	#endif
	uint32_t Count = 0;
	uint32_t kernel_Data_addr[] = {3, 3, 3, 3, 1, 3, 3, 3, 3};
	uint32_t array_a_column = PIC_WIDTH;
	uint32_t kernel_a_column = 3;

	uint32_t tmp_get_tickcount = xTaskGetTickCount();
	while(Count < rec_Count){
		DCMI_Start();
		while(ov_rev_ok == 0)
			;
		DCMI_Stop();
		uint8_t* image_addr = (uint8_t*)camera_buffer;
		uint32_t image_size = (PIC_WIDTH*PIC_HEIGHT*2);
		uint32_t malloc_size = image_size/2;
		uint8_t* malloc_addr = pvPortMalloc(malloc_size);
		for(uint32_t i=0;i<malloc_size;i++){
			uint32_t B = (uint32_t)((*(image_addr+(2*i)) & 0xF8) >> 3);
			uint32_t G = ((uint32_t)((*(image_addr+(2*i)) & 0x07) << 5) | (uint32_t)((*(image_addr+(2*i)+1) & 0x0E0) >> 5));
			uint32_t R = (uint32_t)((*(image_addr+(2*i)+1) & 0x1F));
			*(malloc_addr+i) = (uint8_t)((R*299 + G*587 + B*114 + 500)/1000);
		}
		uint8_t* tar_malloc_addr = pvPortMalloc(malloc_size);
		for(uint32_t i=0;i<malloc_size;i++){
			int operator[] = {0, 0};
			uint32_t tmp_sum = 0;
			for(int j=-1;j<2;j++){
				for(int k=-1;k<2;k++){
					operator[0] = j;
					operator[1] = k;
					if((i<array_a_column)&&(operator[0] == -1))
						operator[0] = 1;
					if((i>=(malloc_size-array_a_column))&&(operator[0] == 1))
						operator[0] = -1;
					if(((i%array_a_column)==0)&&(operator[1] == -1))
						operator[1] = 1;
					if((((i+1)%array_a_column)==0)&&(operator[1] == 1))
						operator[1] = -1;
					uint32_t tmp_index = (uint32_t)((int)i + operator[0]*array_a_column + operator[1]);
					tmp_sum += (*(malloc_addr+tmp_index))*(*(kernel_Data_addr+(j+1)*kernel_a_column+(k+1)));
				}
			}
			*(tar_malloc_addr+i) = tmp_sum/25;
		}

		#if(SENDIMAGE == 1)
		uint8_t* send_addr = (uint8_t*)malloc_addr;
		uint32_t send_size = malloc_size;
		char camera_char[] = {'c', 'a', 'm', 'e', 'r', 'a', '_', '0'};
		for(uint32_t i=0;i<8;i++)
			usart3_send_char(camera_char[i]);
		for(uint32_t i=0;i<send_size;i++)
			usart3_send_char(*(send_addr+i));

		send_addr = (uint8_t*)tar_malloc_addr;
		send_size = malloc_size;
		for(uint32_t i=0;i<8;i++)
			usart3_send_char(camera_char[i]);
		for(uint32_t i=0;i<send_size;i++)
			usart3_send_char(*(send_addr+i));
		#endif

		vPortFree(malloc_addr);
		vPortFree(tar_malloc_addr);
		DebugFlag = Count;
		Count++;
	}
	#if(SENDIMAGE == 0)
	uint32_t Local_Task_bgr_gray_transform_with_2D_convolution_tick = xTaskGetTickCount() - tmp_get_tickcount;
	printf("rec_Count: %u, Local_Task_bgr_gray_transform_with_2D_convolution_tick: %u\r\n", (unsigned int)rec_Count, (unsigned int)Local_Task_bgr_gray_transform_with_2D_convolution_tick);
	#endif
}

// RSA fuinction
uint32_t checkPrime(uint32_t n){
	uint32_t i;
	uint32_t m = n / 2;
	for (i = 2; i <= m; i++) {
		if (n % i == 0) {
			return 0; // Not Prime
		}
	}
	return 1; // Prime
}

uint32_t findGCD(uint32_t n1, uint32_t n2){
	uint32_t i, gcd;
	for(i = 1; i <= n1 && i <= n2; ++i) {
		if(n1 % i == 0 && n2 % i == 0)
			gcd = i;
	}
	return gcd;
}

uint32_t powMod(uint32_t a, uint32_t b, uint32_t n) {
	uint32_t x = 1, y = a;
	while (b > 0) {
		if (b % 2 == 1)
			x = (x * y) % n;
		y = (y * y) % n; // Squaring the base
		b /= 2;
	}
	return (x % n);
}

void public_key(uint32_t* e, uint32_t* d, uint32_t* n, uint32_t p, uint32_t q){
	*n = (p * q);
	uint32_t phin = (p - 1) * (q - 1);
	*e = 0;
	for ((*e) = 5; (*e) <= 100; (*e)++) {
		if (findGCD(phin, (*e)) == 1)
			break;
	}
	*d = 0;
	while(1){
		if ( (((*d) * (*e)) % phin) == 1)
			break;
		(*d)++;
	}
}

// other fuinction, not so important
void test_inline(){
	__asm (	"svc	#0x6				\n");
}
// UserDefine_Task
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Distributed task example, before and after distributed task
void UserDefine_Task(){
	#if(USE_CAMERA == 1)
		uint8_t OV7670_Init_flag = OV7670_Init();
		if(OV7670_Init_flag == 0)
			printf("OV7670_Init success\r\n");
		else
			printf("OV7670_Init fail\r\n");
	#endif
	while(1){
		if (((READ_BIT(USART3_BASE + USART_SR_OFFSET, RXNE_BIT)) || (READ_BIT(USART3_BASE + USART_SR_OFFSET, ORE_BIT))) || (exti0_flag > 0)){
			char rec_cmd = (char)REG(USART3_BASE + USART_DR_OFFSET);
			//printf("%c\r\n", rec_cmd);

			if (rec_cmd == 'd'){										//	dispatch distributed task
				checksize_count = 0;
				rec_cmd = '\0';
				for(uint32_t i=0;i<8;i++)
					global_record_fail_count[i] = 0;
				for(uint32_t i=0;i<4;i++)
					global_record_requestrelease_fail_count[i] = 0;
				for(uint32_t i=0;i<52;i++)
					global_record_time_dispatch_array[i] = 0;
				printf("Distributed task test start\r\n");
				/*
				for(uint32_t i=0;i<0x400;i++){
					*(((uint32_t*)0x10000000)+i) = 1;
					*(((uint32_t*)0x10001000)+i) = 1;
					*(((uint32_t*)0x10002000)+i) = 1;
				}
				*/
				for(uint32_t i=0;i<0x3000;i++){
					*(((uint32_t*)0x10000000)+i) = 1;
					//*(((uint32_t*)0x10000000)+i) = 0xFFB72500;
				}
				for(uint32_t i=0;i<8;i++)
					global_record_data[i] = 0;

				uint32_t Count = 0;
				for(uint32_t i=0;i<8;i++)
					SubtaskFinishArray[i] = 0;
				for(uint32_t i=0;i<16;i++)
					send_recv_data_time[i] = 0;
				send_recv_data_time_count = 8;
				uint32_t Total_base_tick = xTaskGetTickCount();

				// Set Distributed task target data and start task
				//*---------------------------------------------------------------------------------------------------------------------
				/*
				while(Count < 100){
					Distributed_Data_t* data_info = Distributed_SetTargetData((uint32_t*)0x10000000, 0x3000, 1);
					Distributed_Result* Result = Distributed_CreateTask(UserDefine_Distributed_Task, data_info, 1000, WithBarrier);
					Distributed_Data_t* Result_data = NULL;
					while(Result_data == NULL)
						Result_data = Distributed_GetResult(Result);
					Distributed_FreeResult(Result_data);
					Count++;
				}
				*/
				/*	//	UserDefine_Distributed_Task_matrix_multiplication
				uint32_t ARRAY1_COLUMN = 20, ARRAY1_ROW = 3, ARRAY2_COLUMN = 3, ARRAY2_ROW = 3;
				uint32_t* ARRAY1_ADDR = (uint32_t*)0x10000000;
				uint32_t* ARRAY2_ADDR = (uint32_t*)(0x10000000+ARRAY1_COLUMN*ARRAY1_ROW*sizeof(uint32_t));
				for(uint32_t i=0;i<(ARRAY1_COLUMN*ARRAY1_ROW);i++)
					*(ARRAY1_ADDR+i) = 1;
				for(uint32_t i=0;i<(ARRAY2_COLUMN*ARRAY2_ROW);i++)
					*(ARRAY2_ADDR+i) = 2;
				while(Count < 100){
					uint32_t tmp_global_record_data_7 = xTaskGetTickCount();
					Distributed_Data_t* data_info = Distributed_SetTargetData((uint32_t*)ARRAY1_ADDR, (ARRAY1_COLUMN*ARRAY1_ROW), ARRAY1_COLUMN);
					Distributed_AddTargetData(data_info, ARRAY2_ADDR, (ARRAY2_COLUMN*ARRAY2_ROW), 0);
					Distributed_AddTargetData(data_info, &ARRAY1_COLUMN, 1, 0);
					Distributed_AddTargetData(data_info, &ARRAY2_COLUMN, 1, 0);
					Distributed_Result* Result = Distributed_CreateTask(UserDefine_Distributed_Task_matrix_multiplication, data_info, 100, WithBarrier);
					Distributed_Data_t* Result_data = NULL;
					while(Result_data == NULL)
						Result_data = Distributed_GetResult(Result);
					Distributed_FreeResult(Result_data);
					DebugFlag = Count;
					global_record_data[7] += (xTaskGetTickCount() - tmp_global_record_data_7);
					send_recv_data_time_count = 8;
					Count++;
					printf("Count: %u", (unsigned int)Count);
				}
				*/
				/* //	UserDefine_Distributed_Task_do_nothing, less data
				while(Count < 100){
					uint32_t tmp_global_record_data_7 = xTaskGetTickCount();
					Distributed_Data_t* data_info = Distributed_SetTargetData((uint32_t*)0x10000000, 0x8, 1);
					Distributed_Result* Result = Distributed_CreateTask(UserDefine_Distributed_Task_do_nothing, data_info, 1000, WithoutBarrier);
					Distributed_Data_t* Result_data = NULL;
					while(Result_data == NULL)
						Result_data = Distributed_GetResult(Result);
					Distributed_FreeResult(Result_data);
					DebugFlag = Count;
					global_record_data[7] += (xTaskGetTickCount() - tmp_global_record_data_7);
					send_recv_data_time_count = 8;
					Count++;
				}
				*/
				/* //	UserDefine_Distributed_Task_do_nothing, large data
				while(Count < 10000){
					uint32_t tmp_global_record_data_7 = xTaskGetTickCount();
					Distributed_Data_t* data_info = Distributed_SetTargetData((uint32_t*)0x10000000, 0x1000, 1);
					Distributed_Result* Result = Distributed_CreateTask(UserDefine_Distributed_Task_do_nothing, data_info, 1000, WithoutBarrier);
					Distributed_Data_t* Result_data = NULL;
					while(Result_data == NULL)
						Result_data = Distributed_GetResult(Result);
					Distributed_FreeResult(Result_data);
					DebugFlag = Count;
					global_record_data[7] += (xTaskGetTickCount() - tmp_global_record_data_7);
					send_recv_data_time_count = 8;
					//printf("Task: %u done	=\r\n", (unsigned int)Count);
					Count++;
				}
				*/
				/* //	UserDefine_Distributed_Task_multiple, large data
				while(Count < 10000){
					uint32_t tmp_global_record_data_7 = xTaskGetTickCount();
					Distributed_Data_t* data_info = Distributed_SetTargetData((uint32_t*)0x10000000, 0x1000, 1);
					Distributed_Result* Result = Distributed_CreateTask(UserDefine_Distributed_Task_multiple, data_info, 1000, WithoutBarrier);
					Distributed_Data_t* Result_data = NULL;
					while(Result_data == NULL)
						Result_data = Distributed_GetResult(Result);
					Distributed_FreeResult(Result_data);
					DebugFlag = Count;
					//printf("Task: %u done	=\r\n", (unsigned int)Count);
					global_record_data[7] += (xTaskGetTickCount() - tmp_global_record_data_7);
					send_recv_data_time_count = 8;
					Count++;
				}
				*/
				/* //	UserDefine_Distributed_Task_2d_array_convolution
				while(Count < 1){
					uint32_t tmp_global_record_data_7 = xTaskGetTickCount();
					uint32_t array_column = 128;
					uint32_t kernel[] = {2, 2, 2, 2, 2, 2, 2, 2, 2};
					uint32_t kernel_column = 3;
					Distributed_Data_t* data_info = Distributed_SetTargetData((uint32_t*)0x10000000, 0x1000, 128);
					Distributed_AddTargetData(data_info, &array_column, 1, 0);
					Distributed_AddTargetData(data_info, kernel, 9, 0);
					Distributed_AddTargetData(data_info, &kernel_column, 1, 0);
					Distributed_Result* Result = Distributed_CreateTask(UserDefine_Distributed_Task_2d_array_convolution, data_info, 1000, WithBarrier);
					Distributed_Data_t* Result_data = NULL;
					while(Result_data == NULL)
						Result_data = Distributed_GetResult(Result);

					//uint32_t all_right = 1;
					//for(uint32_t i=0;i<Result_data->Data_size;i++){
					//	if(*(Result_data->Data_addr+i) != 0x12){
					//		all_right = 0;
					//	}
					//}
					//if(all_right == 1)
					//	//printf("Count: 0x%lX, All answer right\r\n", Count);
					//	;
					//else{
					//	printf("Count: 0x%lX, Some answer is wrong\r\n", Count);
					//}
					//printf("Result_data Data_size: 0x%lX\r\n", Result_data->Data_size);
					for(uint32_t i=0;i<Result_data->Data_size;i++){
						printf("%u, ", (unsigned int)*(Result_data->Data_addr+i));
						if((i%128) == 0)
							printf("\r\n");
					}
					Distributed_FreeResult(Result_data);
					global_record_data[7] += (xTaskGetTickCount() - tmp_global_record_data_7);
					DebugFlag = Count;
					send_recv_data_time_count = 8;
					printf("Task: %u ticks	=\r\n", (unsigned int)Count);
					Count++;
				}
				*/
				/* //	UserDefine_Local_Task_RSA
				//printf("================================\r\n");

				uint32_t e = 0;
				uint32_t d = 0;
				uint32_t n = 0;
				uint32_t p = 11;
				uint32_t q = 37;
				public_key(&e, &d, &n, p, q);
				//printf("Value of e: %u, Value of d: %u, n: %u\r\n", (unsigned int)e, (unsigned int)d, (unsigned int)n);
				uint32_t test_data_array[] = {0, 37, 183, 255};
				uint32_t test_data = 0;
				for(uint32_t i=0;i<4;i++)
					test_data |= (test_data_array[i] << (i*8));
				//printf("test_data: 0x%lX\r\n", test_data);					//	0xFFB72500
				uint32_t cipher = 0;
				for(uint32_t i=0;i<4;i++){
					uint32_t process_byte = ((test_data >> ((i*8))) & 0x000000FF);
					uint32_t result_data = powMod(process_byte, e, n);
					cipher |= (result_data << (i*8));
				}
				//printf("cipher: 0x%lX\r\n", cipher);

				uint32_t plain  = 0;
				for(uint32_t i=0;i<4;i++){
					uint32_t process_byte = ((cipher >> ((i*8))) & 0x000000FF);
					uint32_t result_data = powMod(process_byte, d, n);
					plain |= (result_data << (i*8));
				}
				//printf("plain: 0x%lX\r\n", plain);
				//printf("================================\r\n");
				//UserDefine_Local_Task_RSA(1, e, n, 0x1000);

				while(Count < 10000){
					uint32_t tmp_global_record_data_7 = xTaskGetTickCount();
					Distributed_Data_t* data_info = Distributed_SetTargetData((uint32_t*)0x10000000, 0x1000, 1);
					Distributed_AddTargetData(data_info, &e, 1, 0);
					Distributed_AddTargetData(data_info, &n, 1, 0);
					Distributed_Result* Result = Distributed_CreateTask(UserDefine_Distributed_Task_RSA, data_info, 1000, WithoutBarrier);
					Distributed_Data_t* Result_data = NULL;
					while(Result_data == NULL)
						Result_data = Distributed_GetResult(Result);

					//uint32_t all_right = 1;
					//uint32_t fail_count = 0;
					//for(uint32_t i=0;i<Result_data->Data_size;i++){
					//	if(*(Result_data->Data_addr+i) != 0X75E9400){
					//		all_right = 0;
					//		fail_count++;
					//		printf("Not 0X75E9400: 0x%lX\r\n", *(Result_data->Data_addr+i));
					//	}
					//}
					//if(all_right == 1)
					//	//printf("Count: 0x%lX, All answer right\r\n", Count);
					//	;
					//else
					//	printf("Count: 0x%lX, Some answer is wrong, fail_count: %u\r\n", Count, (unsigned int)fail_count);

					Distributed_FreeResult(Result_data);
					DebugFlag = Count;
					//printf("Task: %u done	=\r\n", (unsigned int)Count);
					global_record_data[7] += (xTaskGetTickCount() - tmp_global_record_data_7);
					send_recv_data_time_count = 8;
					Count++;
				}
				*/
				/* //	UserDefine_Local_Task_2d_array_convolution(1);*/
				/* //	UserDefine_Distributed_Task_bgr_gray_transform with camera
				#if(USE_CAMERA == 1)

				DCMI_Start();
				while(Count < 10000){
					uint32_t tmp_global_record_data_7 = xTaskGetTickCount();
					while(ov_rev_ok == 0)
						;
					DCMI_Stop();
					//printf("dame camera done\r\n");
					Distributed_Data_t* data_info = Distributed_SetTargetData((uint32_t*)camera_buffer, (32768/4), 1);
					Distributed_Result* Result = Distributed_CreateTask(UserDefine_Distributed_Task_bgr_gray_transform, data_info, 1000, WithoutBarrier);
					DCMI_Start();
					Distributed_Data_t* Result_data = NULL;
					while(Result_data == NULL)
						Result_data = Distributed_GetResult(Result);
					//printf("Result_data, Data_addr: 0x%lX, Data_size: %u\r\n", (uint32_t)Result_data->Data_addr, (unsigned int)Result_data->Data_size);
					Distributed_FreeResult(Result_data);
					DebugFlag = Count;
					Count++;
					//printf("Task: %u done	=\r\n", (unsigned int)Count);
					global_record_data[7] += (xTaskGetTickCount() - tmp_global_record_data_7);
					send_recv_data_time_count = 8;
				}
				DCMI_Stop();
				#endif
				*/
				/* //	UserDefine_Distributed_Task_bgr_gray_transform_with_2D_convolution with camera
				//printf("\r\nstart:\r\n");
				#if(USE_CAMERA == 1)
				uint32_t array_column = PIC_WIDTH;
				uint32_t kernel[] = {3, 3, 3, 3, 1, 3, 3, 3, 3};
				uint32_t kernel_column = 3;
				uint32_t camera_tmp_tick;
				uint32_t camera_tmp_duration_tick = 0;

				uint32_t camera_duration_tick = 0;
				uint32_t camera_stop_tick;
				uint32_t camera_start_tick = xTaskGetTickCount();
				DCMI_Start();
				while(Count < 10000){
					uint32_t tmp_global_record_data_7 = xTaskGetTickCount();
					while(ov_rev_ok == 0)
						;
					DCMI_Stop();
					Distributed_Data_t* data_info = Distributed_SetTargetData((uint32_t*)camera_buffer, (32768/4), 256);
					Distributed_AddTargetData(data_info, &array_column, 1, 0);
					Distributed_AddTargetData(data_info, kernel, 9, 0);
					Distributed_AddTargetData(data_info, &kernel_column, 1, 0);
					//Distributed_Result* Result = Distributed_CreateTask(UserDefine_Distributed_Task_bgr_gray_transform_with_2D_convolution, data_info, 1000, WithBarrier);
					Distributed_Result* Result = Distributed_CreateTask(UserDefine_Distributed_Task_bgr_gray_transform_with_2D_convolution, data_info, 1000, WithoutBarrier);
					DCMI_Start();
					Distributed_Data_t* Result_data = NULL;
					while(Result_data == NULL)
						Result_data = Distributed_GetResult(Result);
					//printf("Result_data, Data_addr: 0x%lX, Data_size: %u\r\n", (uint32_t)Result_data->Data_addr, (unsigned int)Result_data->Data_size);
					Distributed_FreeResult(Result_data);
					DebugFlag = Count;
					Count++;
					//printf("Task: %u done	=\r\n", (unsigned int)Count);
					global_record_data[7] += (xTaskGetTickCount() - tmp_global_record_data_7);
					send_recv_data_time_count = 8;
				}
				DCMI_Stop();
				#endif
				*/
				uint32_t duration_time = (xTaskGetTickCount() - Total_base_tick);
				printf("Duration: %u ticks	=\r\n", (unsigned int)duration_time);

				for(uint32_t i=0;i<8;i++)
					printf("Node: %u, Subtask Finish Count: %u\r\n", (unsigned int)i, (unsigned int)SubtaskFinishArray[i]);

				for(uint32_t i=0;i<8;i++)
					printf("global_record_data[%u]: %u\r\n", (unsigned int)i, (unsigned int)global_record_data[i]);
				for(uint32_t i=0;i<16;i++)
					printf("send_recv_data_time[%u]: %u\r\n", (unsigned int)i, (unsigned int)send_recv_data_time[i]);
				/*
				uint32_t duration_time = (xTaskGetTickCount() - Total_base_tick);
				printf("Duration: %u ticks	=\r\n", (unsigned int)duration_time);

				for(uint32_t i=0;i<4;i++)
					printf("Node: %u, Subtask Finish Count: %u\r\n", (unsigned int)i, (unsigned int)SubtaskFinishArray[i]);

				for(uint32_t i=0;i<8;i++)
					printf("global_record_data[%u]: %u\r\n", (unsigned int)i, (unsigned int)global_record_data[i]);
				for(uint32_t i=0;i<8;i++)
					printf("send_recv_data_time[%u]: %u\r\n", (unsigned int)i, (unsigned int)send_recv_data_time[i]);
				*/
				/*
				printf("Middleware Dispatch: %u, %u, Recycle:  %u, %u, ticks\r\n", (unsigned int)global_record_time_dispatch_array[0], (unsigned int)global_record_time_dispatch_array[1], (unsigned int)global_record_time_dispatch_array[2], (unsigned int)global_record_time_dispatch_array[3]);
				printf("Dispatch Request: %u, %u, Release: %u, %u, ticks\r\n", (unsigned int)global_record_time_dispatch_array[4], (unsigned int)global_record_time_dispatch_array[5], (unsigned int)global_record_time_dispatch_array[6], (unsigned int)global_record_time_dispatch_array[7]);
				for(uint32_t i=0;i<4;i++)
					printf("Subtask Request: %u, %u, Release: %u, %u, ticks\r\n", (unsigned int)global_record_time_dispatch_array[8+i], (unsigned int)global_record_time_dispatch_array[12+i], (unsigned int)global_record_time_dispatch_array[16+i], (unsigned int)global_record_time_dispatch_array[20+i]);
				for(uint32_t i=0;i<4;i++)
					printf("Subtask Send: %u, Done: %u, ticks\r\n", (unsigned int)global_record_time_dispatch_array[24+i], (unsigned int)global_record_time_dispatch_array[28+i]);
				printf("Recycle Request: %u, %u, Release: %u, %u, ticks\r\n", (unsigned int)global_record_time_dispatch_array[32], (unsigned int)global_record_time_dispatch_array[33], (unsigned int)global_record_time_dispatch_array[34], (unsigned int)global_record_time_dispatch_array[35]);
				for(uint32_t i=0;i<4;i++)
					printf("Subtask Send: %u, Send done: %u, ticks\r\n", (unsigned int)global_record_time_dispatch_array[36+i], (unsigned int)global_record_time_dispatch_array[40+i]);
				for(uint32_t i=0;i<4;i++)
					printf("Request Result: %u, Result done: %u, ticks\r\n", (unsigned int)global_record_time_dispatch_array[44+i], (unsigned int)global_record_time_dispatch_array[48+i]);
				*/
				/*
				//uint32_t Transmit_time = 0;
				for(uint32_t i=0;i<24;i++){
					//Transmit_time += global_record_time_dispatch_array[i];
					printf("global_record_time_dispatch_array[%d]: %u ticks\r\n", (int)i, (unsigned int)global_record_time_dispatch_array[i]);
				}
				for(uint32_t i=0;i<8;i++)
					printf("global_record_fail_count[%d]:	%u\r\n", (int)i, (unsigned int)global_record_fail_count[i]);
				for(uint32_t i=0;i<4;i++)
					printf("global_record_requestrelease_fail_count[%d]:	%u\r\n", (int)i, (unsigned int)global_record_requestrelease_fail_count[i]);
				*/
				//printf("Transmit_time: %u ticks and checksize_count: %u\r\n", (unsigned int)Transmit_time, (unsigned int)checksize_count);
				//printf("Subtask time: %u ticks\r\n", (unsigned int)(record_subtask_end_time-record_subtask_time));
				rec_cmd = '\0';
			}

			#if(USE_CAMERA == 1)
			if ((rec_cmd == 'D') || (exti0_flag > 0)){
				uint32_t tmp_get_tickcount = xTaskGetTickCount();
				uint32_t Count = 0;
				#if(DISTRIBUTED_LOCAL == 0)
				while(Count < 1){
					DCMI_Start();
					while(ov_rev_ok == 0)
						;
					DCMI_Stop();
					uint8_t* send_addr = (uint8_t*)camera_buffer;
					uint32_t send_size = 32768;
					char camera_char[] = {'c', 'a', 'm', 'e', 'r', 'a', '_', '1'};
					#if(CAMERA_BGR_GRAY == 0)
					camera_char[7] = '0';
					uint8_t* image_addr = (uint8_t*)camera_buffer;
					uint32_t malloc_size = 16384;
					uint8_t* malloc_addr = pvPortMalloc(malloc_size);
					send_size = malloc_size;
					for(uint32_t i=0;i<malloc_size;i++){
						uint32_t B = (uint32_t)((*(image_addr+(2*i)) & 0xF8) >> 3);
						uint32_t G = ((uint32_t)((*(image_addr+(2*i)) & 0x07) << 5) | (uint32_t)((*(image_addr+(2*i)+1) & 0x0E0) >> 5));
						uint32_t R = (uint32_t)((*(image_addr+(2*i)+1) & 0x1F));
						*(malloc_addr+i) = (uint8_t)((R*299 + G*587 + B*114 + 500)/1000);
					}
					send_addr = malloc_addr;
					#endif

					#if(SENDIMAGE == 1)
					for(uint32_t i=0;i<8;i++)
						usart3_send_char(camera_char[i]);
					#if(CAMERA_BGR_GRAY == 1)
					usart3_send_char('0');
					#endif
					for(uint32_t i=0;i<send_size;i++)
						usart3_send_char(*(send_addr+i));
					#endif

					#if(CAMERA_BGR_GRAY == 0)
					vPortFree(malloc_addr);
					#endif
					Count++;
				}
				#else

				DCMI_Start();
				while(ov_rev_ok == 0)
					;
				DCMI_Stop();
				#if(SENDIMAGE == 1)
				char camera_char1[] = {'c', 'a', 'm', 'e', 'r', 'a', '_', '1'};
				for(uint32_t i=0;i<8;i++)
					usart3_send_char(camera_char1[i]);
				for(uint32_t i=1;i<(PIC_WIDTH*PIC_HEIGHT*2);i++)
					usart3_send_char(*((uint8_t*)camera_buffer+i));
				usart3_send_char('\0');
				#endif
				vTaskDelay(1000/portTICK_RATE_MS);
				while(Count < 1){
					DCMI_Start();
					while(ov_rev_ok == 0)
						;
					DCMI_Stop();
					Distributed_Data_t* data_info = Distributed_SetTargetData((uint32_t*)camera_buffer, (32768/4), 1);
					Distributed_Result* Result = Distributed_CreateTask(UserDefine_Distributed_Task_bgr_gray_transform, data_info, 1000, WithBarrier);
					Distributed_Data_t* Result_data = NULL;
					while(Result_data == NULL)
						Result_data = Distributed_GetResult(Result);
					//printf("Result_data, Data_addr: 0x%lX, Data_size: %u\r\n", (uint32_t)Result_data->Data_addr, (unsigned int)Result_data->Data_size);
					#if(SENDIMAGE == 1)
					uint8_t* send_addr = (uint8_t*)Result_data->Data_addr;
					uint32_t send_size = (Result_data->Data_size)*sizeof(uint32_t);
					char camera_char[] = {'c', 'a', 'm', 'e', 'r', 'a', '_', '0'};
					for(uint32_t i=0;i<8;i++)
						usart3_send_char(camera_char[i]);
					for(uint32_t i=0;i<send_size;i++)
						usart3_send_char(*(send_addr+i));
					#endif
					Distributed_FreeResult(Result_data);
					DebugFlag = Count;
					Count++;
					send_recv_data_time_count = 8;
				}
				vTaskDelay(1000/portTICK_RATE_MS);

				Count = 0;
				uint32_t array_column = PIC_WIDTH;
				uint32_t kernel[] = {3, 3, 3, 3, 1, 3, 3, 3, 3};
				uint32_t kernel_column = 3;
				DCMI_Start();
				while(Count < 1){
					while(ov_rev_ok == 0)
						;
					DCMI_Stop();
					Distributed_Data_t* data_info = Distributed_SetTargetData((uint32_t*)camera_buffer, (32768/4), 256);
					Distributed_AddTargetData(data_info, &array_column, 1, 0);
					Distributed_AddTargetData(data_info, kernel, 9, 0);
					Distributed_AddTargetData(data_info, &kernel_column, 1, 0);
					Distributed_Result* Result = Distributed_CreateTask(UserDefine_Distributed_Task_bgr_gray_transform_with_2D_convolution, data_info, 1000, WithoutBarrier);
					DCMI_Start();
					Distributed_Data_t* Result_data = NULL;
					while(Result_data == NULL)
						Result_data = Distributed_GetResult(Result);
					#if(SENDIMAGE == 1)
					uint8_t* send_addr = (uint8_t*)Result_data->Data_addr;
					uint32_t send_size = (Result_data->Data_size)*sizeof(uint32_t);
					char camera_char[] = {'c', 'a', 'm', 'e', 'r', 'a', '_', '0'};
					for(uint32_t i=0;i<8;i++)
						usart3_send_char(camera_char[i]);
					for(uint32_t i=0;i<send_size;i++)
						usart3_send_char(*(send_addr+i));
					#endif

					Distributed_FreeResult(Result_data);
					DebugFlag = Count;
					Count++;
					send_recv_data_time_count = 8;
				}
				DCMI_Stop();

				#endif

				//UserDefine_Local_Task_bgr_gray_transform(1);
				UserDefine_Local_Task_bgr_gray_transform_with_2D_convolution(1);

				/*
				uint32_t camera_duration = xTaskGetTickCount() - tmp_get_tickcount;
				printf("camera_duration: %u\r\n", (unsigned int)camera_duration);
				*/
				vTaskDelay(1000/portTICK_RATE_MS);
				exti0_flag = 0;
				rec_cmd = '\0';
			}
			if (rec_cmd == 'C'){
				/*
				uint32_t count = 0;
				uint32_t tmp_get_tickcount = xTaskGetTickCount();
				while(count < 100){
					DCMI_Start();
					while(ov_rev_ok == 0)
						;
					DCMI_Stop();
					count++;
				}
				uint32_t camera_duration = xTaskGetTickCount() - tmp_get_tickcount;
				printf("camera_duration: %u\r\n", (unsigned int)camera_duration);
				*/
				DCMI_Start();
				while(ov_rev_ok == 0)
					;
				DCMI_Stop();

				uint8_t* send_addr = (uint8_t*)camera_buffer;
				for(uint32_t i=0;i<32768;i++)
					usart3_send_char(*(send_addr+i));
				//printf("\r\n");
				//printf("ov_rev_ok is %u\r\n", (unsigned int)ov_rev_ok);
				rec_cmd = '\0';
			}
			#endif

		}
	}
}

void LED_BLUE_TASK(){
	printf("LED_BLUE_TASK\r\n");
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

// other peripheral function, not so important
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Exti0
void init_external_interrupt(void){
	SET_BIT(RCC_BASE + RCC_AHB1ENR_OFFSET, GPIO_EN_BIT(GPIO_PORTA));
	//GPIO Configurations
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA) + GPIOx_MODER_OFFSET, MODERy_1_BIT(0));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA)+ GPIOx_MODER_OFFSET, MODERy_0_BIT(0));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA)+ GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(0));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTA)+ GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(0));

	//RCC Enable SYSCFG
	SET_BIT(SYSCFG_BASE + RCC_APB2ENR_OFFSET, SYSCFGEN_BIT);

	//SYSCFG Configurations
	WRITE_BITS(SYSCFG_BASE + SYSCFG_EXTICR1_OFFSET, EXTI0_3_BIT, EXTI0_0_BIT, 0);

	SET_BIT(EXTI_BASE + EXTI_IMR_OFFSET, 0);
	SET_BIT(EXTI_BASE + EXTI_RTSR_OFFSET, 0);
	SET_BIT(EXTI_BASE + EXTI_FTSR_OFFSET, 0);
	SET_BIT(EXTI_BASE + EXTI_PR_OFFSET, 0);
	SET_BIT(NVIC_ISER_BASE + NVIC_ISERn_OFFSET(0), 6); //IRQ6
}

void exti0_handler_c(uint32_t LR, uint32_t MSP){
	#if(SENDIMAGE == 1)
	exti0_flag = 1;
	#endif
	uint32_t *stack_frame_ptr;
	if (LR & 0x4){
		stack_frame_ptr = (uint32_t *)read_psp();
	}
	else{
		stack_frame_ptr = (uint32_t *)MSP;
	}
	uint32_t stacked_return_addr = *(stack_frame_ptr+6);
	uint32_t stacked_LR = *(stack_frame_ptr+5);
	#if(SENDIMAGE == 0)
	printf("DebugFlag: 0x%lX, SendFlag: 0x%lX, RecvFlag: 0x%lX, PublishFlag: 0x%lX, Return_addr: 0x%lX, LR: 0x%lX\r\n", DebugFlag, SendFlag, RecvFlag, PublishFlag, stacked_return_addr, stacked_LR);
	#endif
	SET_BIT(EXTI_BASE + EXTI_PR_OFFSET, 0);
}

// Usart3
void init_usart3(void){
	//RCC EN GPIO
	SET_BIT(RCC_BASE + RCC_AHB1ENR_OFFSET, GPIO_EN_BIT(GPIO_PORTD));

	//MODER => 10
	SET_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_MODER_OFFSET, MODERy_1_BIT(8));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_MODER_OFFSET, MODERy_0_BIT(8));

	SET_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_MODER_OFFSET, MODERy_1_BIT(9));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_MODER_OFFSET, MODERy_0_BIT(9));

	//OT => 0
	CLEAR_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_OTYPER_OFFSET, OTy_BIT(8));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_OTYPER_OFFSET, OTy_BIT(9));

	//OSPEEDR => 10
	SET_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(8));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(8));

	SET_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(9));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(9));

	//PUPDR = 00 => No pull-up, pull-down
	CLEAR_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(8));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(8));

	CLEAR_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(9));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTD) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(9));

	//AF sel
	WRITE_BITS(GPIO_BASE(GPIO_PORTD) + GPIOx_AFRH_OFFSET, AFRHy_3_BIT(8), AFRHy_0_BIT(8), 7);
	WRITE_BITS(GPIO_BASE(GPIO_PORTD) + GPIOx_AFRH_OFFSET, AFRHy_3_BIT(9), AFRHy_0_BIT(9), 7);

	//RCC EN USART3
	SET_BIT(RCC_BASE + RCC_APB1ENR_OFFSET, USART3EN);

	//baud rate
	const unsigned int BAUDRATE = 115200;
	const unsigned int SYSCLK_MHZ = 168;
	const double USARTDIV = SYSCLK_MHZ * 1.0e6 / 8 / 2 / BAUDRATE;

	const uint32_t DIV_MANTISSA = (uint32_t)USARTDIV;
	const uint32_t DIV_FRACTION = (uint32_t)((USARTDIV - DIV_MANTISSA) * 16);

	WRITE_BITS(USART3_BASE + USART_BRR_OFFSET, DIV_MANTISSA_11_BIT, DIV_MANTISSA_0_BIT, DIV_MANTISSA);
	WRITE_BITS(USART3_BASE + USART_BRR_OFFSET, DIV_FRACTION_3_BIT, DIV_FRACTION_0_BIT, DIV_FRACTION);

	//usart3 enable
	SET_BIT(USART3_BASE + USART_CR1_OFFSET, UE_BIT);

	//set TE
	SET_BIT(USART3_BASE + USART_CR1_OFFSET, TE_BIT);

	//set RE
	SET_BIT(USART3_BASE + USART_CR1_OFFSET, RE_BIT);

	//set RXNEIE
	SET_BIT(USART3_BASE + USART_CR1_OFFSET, RXNEIE_BIT);

	//set NVIC
	//SET_BIT(NVIC_ISER_BASE + NVIC_ISERn_OFFSET(1), 5); //IRQ71 => (m+(32*n)) | m=7, n=2
}

void usart3_send_char(const char ch){
	//wait util TXE == 1
	while (!READ_BIT(USART3_BASE + USART_SR_OFFSET, TXE_BIT))
		;
	REG(USART3_BASE + USART_DR_OFFSET) = (uint8_t)ch;
}

void usart3_handler(void){

	if (READ_BIT(USART3_BASE + USART_SR_OFFSET, ORE_BIT))
	{
		usart3_send_char('~');
	}

	else if (READ_BIT(USART3_BASE + USART_SR_OFFSET, RXNE_BIT))
	{
		char c = (char)REG(USART3_BASE + USART_DR_OFFSET);
		usart3_send_char(c);
	}
}

// main
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int main(void){
	DStart->Next_TaskHandle_List = NULL;
	init_usart3();
	led_init(LED_GREEN);
	led_init(LED_ORANGE);
	led_init(LED_RED);
	init_external_interrupt();
	REG(AIRCR_BASE) = NVIC_AIRCR_RESET_VALUE | NVIC_PRIORITYGROUP_4;
	xTaskCreate(Distributed_ManageTask, "Distributed_ManageTask", 1000, NULL, 1, &TaskHandle_1);
	xTaskCreate(UserDefine_Task, "UserDefine_Task", 1000, NULL, 1, NULL);
	xTaskCreate(LED_BLUE_TASK, "LED_BLUE_TASK", 1000, NULL, 1, &TaskHandle_3);
	vTaskStartScheduler();
	while(1)
		;
}

// stdio.h porting
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
		usart3_send_char(*ptr++);
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
