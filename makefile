CROSS-COMPILER = arm-none-eabi-
RTOS = tasks.c list.c queue.c port.c heap_4.c timers.c
CAMERA = dcmi.c ov7670.c sccb.c
all: mpu.bin

mpu.bin: main.c blink.c startup.c vector_table.s asm_func.s $(RTOS) $(CAMERA)
	$(CROSS-COMPILER)gcc -std=c11 -Wall -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -nostartfiles -T stm32f4.ld main.c blink.c startup.c $(RTOS) $(CAMERA) vector_table.s asm_func.s -o mpu.elf
	$(CROSS-COMPILER)objcopy -O binary mpu.elf mpu.bin
	$(CROSS-COMPILER)objdump -D mpu.elf > mpu.txt

flash:
	st-flash --reset write mpu.bin 0x8000000

clean:
	rm -f *.o *.elf *.bin
