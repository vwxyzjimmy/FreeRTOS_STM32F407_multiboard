.syntax unified

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

.global	sys_call
sys_call:
	SVC	#0x0
	bx	lr

.type Distributed_Start, %function
.global Distributed_Start
Distributed_Start:
	push	{lr}
	mov	r0,	r0
	mov	r1,	r1
	mov	r2,	sp
	mov	r3,	lr
	blx	distributed_manager_task
	mov	r0,	r0
	pop	{lr}
	bx	lr


.type svc_handler, %function
.global svc_handler
svc_handler:
	movs	r0,	lr
	mrs	r1,	msp
	b	svc_handler_c
