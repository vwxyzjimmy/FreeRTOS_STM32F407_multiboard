#define Distributed_End()		 			\
do {                			 			\
	__asm volatile ("svc	#0x2	\n");	\
} while (0)


typedef struct Distributed_TaskHandle_List
{
    struct Distributed_TaskHandle_List *Next_TaskHandle_List;
    uint32_t Processor_id;
    uint32_t DTask_id;
	uint32_t DSubTask_id;
    uint32_t *Instruction_addr;
	uint32_t *Instruction_addr_end;
    uint32_t *Data_addr;
    uint32_t Data_size;
    uint32_t Finish_Flag;
    TaskHandle_t TaskHandlex;
}Distributed_TaskHandle_List_t;

typedef struct A_BLOCK_LINK
{
	struct A_BLOCK_LINK *pxNextFreeBlock;
	size_t xBlockSize;
} BlockLink_t;
