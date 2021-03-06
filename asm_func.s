.syntax unified

.global	tri_svc
tri_svc:
	svc	#0x03
	bx	lr

.global	read_sp
read_sp:
	mov	r0,	sp
	bx	lr

.global	read_msp
read_msp:
	mrs	r0,	msp
	bx	lr

.global	read_psp
read_psp:
	mrs	r0,	psp
	bx	lr

.global	read_ctrl
read_ctrl:
	mrs	r0,	control
	bx	lr

.global	read_exc_ret
read_exc_ret:
	movs	r0,	lr
	bx	lr

.global	read_lr
read_lr:
	movs	r0,	lr
	bx	lr

.global	start_user
start_user:
	movs	lr,	r0
	msr	psp,	r1
	movs	r3,	#0b10
	msr	control,	r3
	isb

	bx	lr

.global	write_ctrl
write_ctrl:
	msr	control,	r0

	bx	lr

.type read_r0_from_svc, %function
.global	read_r0_from_svc
read_r0_from_svc:
	movs	r0, r0
	bx	lr

.global	sys_call
sys_call:
	SVC	#0x0
	bx	lr
// Distributed middleware interface for user, Get LR, SP
.type Distributed_Start, %function
.global Distributed_Start
Distributed_Start:
	push	{lr}
	mov		r0,	r0											//	pass three parameter to Distributed_Dispatch_Task data_info, SP, LR
	mov		r1,	sp
	mov		r2,	lr
	blx	Distributed_DispatchTask
	mov		r0,	r0
	pop		{lr}
	bx		lr

.type svc_handler, %function
.global svc_handler
svc_handler:
	movs	r0,	lr
	mrs	r1,	msp
	b	svc_handler_c

.type hardfault_handler, %function
.global hardfault_handler
hardfault_handler:
	movs	r0,	lr
	mrs	r1,	msp
	b	hardfault_handler_c

.type exti0_handler, %function
.global exti0_handler
exti0_handler:
	movs	r0,	lr
	mrs	r1,	msp
	b	exti0_handler_c
