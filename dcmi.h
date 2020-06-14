#ifndef _DCMI_H
#define _DCMI_H
#include "reg.h"
#include <stdio.h>
void DCMI_DMA_Init(uint32_t DMA_Memory0BaseAddr,uint16_t DMA_BufferSize);
void dma2_stream1_handler(void);
void My_DCMI_Init(void);
void DCMI_Start(void);
void DCMI_Stop(void);
void dcmi_handler(void);
void DCMI_Set_Window(uint16_t sx,uint16_t sy,uint16_t width,uint16_t height);
void DMA2_Steam1_DeInit();
uint8_t DMA2_Steam1_GetCmdStatus();
void DMA2_Steam1_Init(uint32_t DMA_Memory0BaseAddr,uint16_t DMA_BufferSize);
void DCMI_DeInit();
void DCMI_Init();

#endif
