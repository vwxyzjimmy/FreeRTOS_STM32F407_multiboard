#define Distributed_End(s, target_addr, target_size)		 		\
do {                			 									\
	s->Data_addr = (uint32_t*)target_addr;							\
	s->Data_number = (uint32_t)target_size;							\
	for(uint32_t tmp_COUNT=0;tmp_COUNT<1;tmp_COUNT++)				\
		;															\
	__asm volatile ("svc	#0x2	\n");							\
	Distributed_LocalSubtaskDone(s, (uint32_t*)target_addr, (uint32_t)target_size);	\
} while (0)

#define Distributed_GetTragetData(s)		 														\
({																									\
	Distributed_Data_t* tmp_array = s->Distributed_Data_List;										\
	if (s->Remain_Data_number < s->Data_number){													\
		for(uint32_t Data_number_i=0;Data_number_i<s->Remain_Data_number;Data_number_i++){			\
			tmp_array = tmp_array->Next_Distributed_Data;											\
		}																							\
		s->Remain_Data_number = s->Remain_Data_number + 1;											\
	}																								\
	tmp_array;																						\
})

#define Distributed_pvPortMalloc(malloc_addr, malloc_size)		 							\
do {																						\
	__asm (	"push 	{r0}				\n");												\
	__asm (	"mov	r0,	%0				\n"	::"r"(malloc_size));							\
	__asm (	"svc	#0x4				\n");												\
	__asm (	"mov	%0,	r0				\n"	:"=r"(malloc_addr):);							\
	__asm (	"pop	{r0}				\n");												\
} while (0)

#define Distributed_vPortFree(malloc_addr)		 											\
do {																						\
	__asm (	"push 	{r0}				\n");												\
	__asm (	"mov	r0,	%0				\n"	::"r"(malloc_addr));							\
	__asm (	"svc	#0x5				\n");												\
	__asm (	"pop	{r0}				\n");												\
} while (0)

typedef struct Distributed_Data{
    uint32_t* Data_addr;
    uint32_t Data_size;
	uint32_t Split_size;
	uint32_t Barrier;
	uint32_t* barrier_flag;
	QueueHandle_t* xQueue;
	TaskHandle_t TaskHandle;
	struct Distributed_Data* Next_Distributed_Data;
}Distributed_Data_t;

typedef struct Distributed_FreeBlock {
    uint32_t Node_id;
	uint32_t Block_number;
    uint32_t* Block_size_array;
	struct Distributed_FreeBlock* Next_Distributed_FreeBlock;
} Distributed_FreeBlock;

typedef struct Distributed_TaskHandle_List {
    struct Distributed_TaskHandle_List *Next_TaskHandle_List;
    uint32_t Source_Processor_id;
	uint32_t Destinate_Processor_id;
    uint32_t DTask_id;
	uint32_t DSubTask_id;
	uint32_t DSubTask_number;
    uint32_t* Instruction_addr;
	uint32_t* Instruction_addr_end;
    uint32_t* Data_addr;
    uint32_t* Data_size;
	uint32_t* Data_Max_size;
	uint32_t Subtask_all_size;
	uint32_t Data_number;
	uint32_t Remain_Data_number;
	uint32_t Stack_size;
    uint32_t Finish_Flag;
	uint32_t Barrier;
	uint32_t* barrier_flag;
    TaskHandle_t* TaskHandlex;
	QueueHandle_t* xQueue;
	Distributed_Data_t* Distributed_Data_List;
} Distributed_TaskHandle_List_t;

typedef struct Distributed_Result {
	QueueHandle_t* Queue;
	TaskHandle_t* TaskHandle;
	uint32_t* barrier_flag;
	Distributed_Data_t** Distributed_Data;
}Distributed_Result;

typedef struct A_BLOCK_LINK {
	struct A_BLOCK_LINK *pxNextFreeBlock;
	size_t xBlockSize;
} BlockLink_t;

typedef struct {
	volatile uint32_t   Status;
	uint32_t   ControlBufferSize;
	uint32_t   Buffer1Addr;
	uint32_t   Buffer2NextDescAddr;
} ETH_DMADESCTypeDef;

typedef struct {
  volatile ETH_DMADESCTypeDef *FS_Rx_Desc;          /*!< First Segment Rx Desc */
  volatile ETH_DMADESCTypeDef *LS_Rx_Desc;          /*!< Last Segment Rx Desc */
  volatile uint32_t  Seg_Count;                     /*!< Segment count */
} ETH_DMA_Rx_Frame_infos;

typedef struct {
  uint32_t length;
  uint32_t buffer;
  volatile ETH_DMADESCTypeDef *descriptor;
} FrameTypeDef;

#define ETH_TXBUFNB   2
#define ETH_RXBUFNB   2
#define ETH_TX_BUF_SIZE 1524
#define ETH_RX_BUF_SIZE 1524
ETH_DMADESCTypeDef  DMARxDscrTab[ETH_RXBUFNB] __attribute__((aligned (4)));/* Ethernet Rx MA Descriptor */
ETH_DMADESCTypeDef  DMATxDscrTab[ETH_TXBUFNB] __attribute__((aligned (4)));/* Ethernet Tx DMA Descriptor */
uint8_t Rx_Buff[ETH_RXBUFNB][ETH_RX_BUF_SIZE] __attribute__((aligned (4))); /* Ethernet Receive Buffer */
uint8_t Tx_Buff[ETH_TXBUFNB][ETH_TX_BUF_SIZE] __attribute__((aligned (4))); /* Ethernet Transmit Buffer */

volatile ETH_DMADESCTypeDef  *DMATxDescToSet;
volatile ETH_DMADESCTypeDef  *DMARxDescToGet;
ETH_DMA_Rx_Frame_infos RX_Frame_Descriptor;
volatile ETH_DMA_Rx_Frame_infos *DMA_RX_FRAME_infos;

#define     DP83848_PHY_ADDRESS     0x01
#define 	ETH_SPEED_10M   0x00000000
#define 	ETH_SPEED_100M   0x00004000
#define 	ETH_MODE_FULLDUPLEX   0x00000800
#define 	ETH_MODE_HALFDUPLEX   0x00000000

#define Distributed_NodeGetID_MSG 					0x01
#define Distributed_NodeGetIDAgain_MSG 				0x02
#define Distributed_NodeResponseID_MSG 				0x03
#define Distributed_NodeCheck_MSG 					0x04
#define Distributed_NodeCheckback_MSG 				0x05
#define Distributed_NodeBackupMaster_MSG 			0x06
#define Distributed_NodeInvalid_MSG 				0x07
#define Distributed_NodeSendFreeBlock_MSG 			0x08
#define Distributed_NodeSendSubtask_MSG 			0x09
#define Distributed_NodeSendRemainSubtask_MSG		0x0a
#define Distributed_NodeRecvSubtask_MSG 			0x0b
#define Distributed_NodeRecvRemainSubtask_MSG		0x0c
#define Distributed_NodeDisablePublish_MSG 			0x0d
#define Distributed_NodeEnablePublish_MSG 			0x0e
#define Distributed_NodeResponsePublish_MSG			0x0f
#define Distributed_NodeRequestKey_MSG				0x10
#define Distributed_NodeReleaseKey_MSG				0x11
#define Distributed_NodeResponseKey_MSG				0x12
#define Distributed_NodeSubtaskFinish_MSG 			0x13
#define Distributed_NodeResponseSubtaskFinish_MSG 	0x14
#define Distributed_NodeRequestResult_MSG 			0x15
#define Distributed_NodeRequestRemainResult_MSG		0x16
#define Distributed_NodeResponseResult_MSG 			0x17
#define Distributed_NodeResponseRemainResult_MSG 	0x18
#define Distributed_NodeRemoveTask_MSG 				0x19
#define Distributed_NodeSendComplete_MSG			0x1a

#define Request 0x00
#define Release 0x01
#define WithBarrier 0x0
#define WithoutBarrier 0x01
#define ETH_FRAM_SIZE 1500

#define Broadcast_Node_id 0xFFFFFFFF

#define Compaction 1
