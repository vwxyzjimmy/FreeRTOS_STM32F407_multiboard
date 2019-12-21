#define Distributed_End(s, addr, size)		 						\
do {                			 									\
	s->Finish_Flag = 1;												\
	__asm volatile ("svc	#0x2	\n");							\
	Distributed_Check(s, addr, size);								\
} while (0)

/*
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
	uint32_t Data_Max_size;
    uint32_t Finish_Flag;
    TaskHandle_t *TaskHandlex;
	QueueHandle_t* xQueue;
}Distributed_TaskHandle_List_t;
*/
typedef struct Distributed_Data
{
    uint32_t* Data_addr;
    uint32_t Data_size;
	QueueHandle_t* xQueue;
	struct Distributed_Data* Next_Distributed_Data
}Distributed_Data_t;

typedef struct Distributed_TaskHandle_List
{
    struct Distributed_TaskHandle_List *Next_TaskHandle_List;
    uint32_t Processor_id;
    uint32_t DTask_id;
	uint32_t DSubTask_id;
    uint32_t* Instruction_addr;
	uint32_t* Instruction_addr_end;
    uint32_t* Data_addr;
    uint32_t* Data_size;
	uint32_t* Data_Max_size;
	uint32_t Data_number;
	uint32_t Remaind_Data_number;
    uint32_t Finish_Flag;
    TaskHandle_t *TaskHandlex;
	QueueHandle_t* xQueue;
	Distributed_Data_t* Distributed_Data_List;
}Distributed_TaskHandle_List_t;

typedef struct A_BLOCK_LINK
{
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
