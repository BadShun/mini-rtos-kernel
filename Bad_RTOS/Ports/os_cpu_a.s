	IMPORT os_tcb_cur_ptr
	IMPORT os_tcb_high_ready_ptr
	IMPORT os_prio_cur
	IMPORT os_prio_high_ready
	
	EXPORT os_cpu_int_dis
	EXPORT os_cpu_int_en
	EXPORT os_cpu_sr_save
	EXPORT os_cpu_sr_restore
	EXPORT os_start_high_ready
	EXPORT PendSV_Handler
	
NVIC_INT_CTRL   EQU 0xE000ED04 ; 中断控制及状态寄存器 SCB_ICSR。
NVIC_SYSPRI14   EQU 0xE000ED22 ; 系统优先级寄存器 SCB_SHPR3：bit16~23
NVIC_PENDSV_PRI EQU 0xFF       ; PendSV 优先级的值(最低)。
NVIC_PENDSVSET  EQU 0x10000000 ; 触发PendSV异常的值 Bit28：PENDSVSET。
	
	THUMB
	REQUIRE8
	PRESERVE8
		
	AREA |.text|, CODE, READONLY, ALIGN=2
		
os_cpu_int_dis
	CPSID I
	BX LR

os_cpu_int_en
	CPSIE I
	BX LR
	
os_cpu_sr_save
	MRS R0, PRIMASK
	CPSID I
	BX LR
	
os_cpu_sr_restore
	MSR PRIMASK, R0
	BX LR

os_start_high_ready
	LDR R0, =NVIC_SYSPRI14
	LDR R1, =NVIC_PENDSV_PRI
	STRB R1, [R0]

	MOVS R0, #0
	MSR PSP, R0

	LDR R0, =NVIC_INT_CTRL
	LDR R1, =NVIC_PENDSVSET
	STR R1, [R0]

	CPSIE I

os_start_hang
	B os_start_hang
	
PendSV_Handler
	CPSID I

	MRS R0, PSP
	CBZ R0, _os_cpu_ld_first_task_stk ; 是0就跳转，第一次任务切换必然跳转

	STMDB R0!, {R4-R11}

	LDR R1, =os_tcb_cur_ptr
	LDR R1, [R1]
	STR R0, [R1]
	
_os_cpu_ld_first_task_stk ; 第一次任务切换，不保存
	LDR R0, =os_prio_cur
	LDR R1, =os_prio_high_ready
	LDR R2, [R1]
	STRB R2, [R0]
	
	LDR R0, =os_tcb_cur_ptr
	LDR R1, =os_tcb_high_ready_ptr
	LDR R2, [R1]
	STR R2, [R0]
	
	LDR R0, [R2]
	LDMIA R0!, {R4-R11}
	
	MSR PSP, R0
	ORR LR, LR, #0x04
	CPSIE I
	BX LR
	
	NOP
	
	END