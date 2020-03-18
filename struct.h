#define Distributed_End(s, target_addr, target_size)		 		\
do {                			 									\
	s->Data_addr = target_addr;										\
	s->Data_number = target_size;									\
	__asm volatile ("svc	#0x2	\n");							\
	Distributed_Local_Subtask_Done(s, target_addr, target_size);					\
} while (0)

typedef struct Distributed_Data{
    uint32_t* Data_addr;
    uint32_t Data_size;
	uint32_t Split_size;
	QueueHandle_t* xQueue;
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
    uint32_t* Instruction_addr;
	uint32_t* Instruction_addr_end;
    uint32_t* Data_addr;
    uint32_t* Data_size;
	uint32_t* Data_Max_size;
	uint32_t Data_number;
	uint32_t Remaind_Data_number;
	uint32_t Stack_size;
    uint32_t Finish_Flag;
    TaskHandle_t *TaskHandlex;
	QueueHandle_t* xQueue;
	Distributed_Data_t* Distributed_Data_List;
} Distributed_TaskHandle_List_t;

typedef struct A_BLOCK_LINK {
	struct A_BLOCK_LINK *pxNextFreeBlock;
	size_t xBlockSize;
} BlockLink_t;

#define Distributed_Get_Traget_Data(s)		 														\
({																									\
	Distributed_Data_t* tmp_array = s->Distributed_Data_List;										\
	if (s->Remaind_Data_number < s->Data_number){													\
		for(uint32_t Data_number_i=0;Data_number_i<s->Remaind_Data_number;Data_number_i++){			\
			tmp_array = tmp_array->Next_Distributed_Data;											\
		}																							\
		s->Remaind_Data_number = s->Remaind_Data_number + 1;										\
	}																								\
	tmp_array;																						\
})

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

#define DistributedNodeGetID_MSG 					0x01
#define DistributedNodeGetIDAgain_MSG 				0x02
#define DistributedNodeResponseID_MSG 				0x03
#define DistributedNodeCheck_MSG 					0x04
#define DistributedNodeCheckback_MSG 				0x05
#define DistributedNodeBackupMaster_MSG 			0x06
#define DistributedNodeInvalid_MSG 					0x07
#define DistributedNodeSendFreespace_MSG 			0x08
#define DistributedNodeSendSubtask_MSG 				0x09
#define DistributedNodeResponseSubtask_MSG 			0x0a
#define DistributedNodeDisablePublish_MSG 			0x0b
#define DistributedNodeEnablePublish_MSG 			0x0c
#define DistributedNodeResponsePublish_MSG			0x0d
#define DistributedNodeRequestKey_MSG				0x0e
#define DistributedNodeReleaseKey_MSG				0x0f
#define DistributedNodeResponseKey_MSG				0x10
#define DistributedNodeSubtaskFinish_MSG 			0x11
#define DistributedNodeResponseSubtaskFinish_MSG 	0x12
#define DistributedNodeRequestResult_MSG 			0x13
#define DistributedNodeResponseResult_MSG 			0x14
#define DistributedNodeRemoveTask_MSG 				0x15

#define Request 0x00
#define Release 0x01
