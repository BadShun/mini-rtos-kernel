#include "stm32f10x.h"                  // Device header
#include "os_cpu.h"
#include "os.h"

cpu_stack_t *os_task_stack_init(os_task_func_ptr p_task,
							 void            *p_args,
							 cpu_stack_t     *p_stk_base,
							 cpu_stack_size_t   stk_size) {
	cpu_stack_t *p_stk;
	p_stk = &p_stk_base[stk_size];
	/* 异常发生时自动保存的寄存器 */
    *--p_stk = (cpu_stack_t)0x01000000u;    /* xPSR的bit24必须置1，用于指定指令集，0：ARM，1：Thumb  */
    *--p_stk = (cpu_stack_t)p_task;         /* R15 (PC)任务的入口地址 */
    *--p_stk = (cpu_stack_t)0x14141414u;    /* R14 (LR)            */
    *--p_stk = (cpu_stack_t)0x12121212u;    /* R12                 */
    *--p_stk = (cpu_stack_t)0x03030303u;    /* R3                  */
    *--p_stk = (cpu_stack_t)0x02020202u;    /* R2                  */
    *--p_stk = (cpu_stack_t)0x01010101u;    /* R1                  */
    *--p_stk = (cpu_stack_t)p_args;         /* R0 : 任务形参*/
    /* 异常发生时需手动保存的寄存器 */
    *--p_stk = (cpu_stack_t)0x11111111u;    /* R11                 */
    *--p_stk = (cpu_stack_t)0x10101010u;    /* R10                 */
    *--p_stk = (cpu_stack_t)0x09090909u;    /* R9                  */
    *--p_stk = (cpu_stack_t)0x08080808u;    /* R8                  */
    *--p_stk = (cpu_stack_t)0x07070707u;    /* R7                  */
    *--p_stk = (cpu_stack_t)0x06060606u;    /* R6                  */
    *--p_stk = (cpu_stack_t)0x05050505u;    /* R5                  */
    *--p_stk = (cpu_stack_t)0x04040404u;    /* R4                  */
	
	return p_stk;
}
							 
void os_sys_tick_init(uint32_t ms) {
	NVIC_SetPriority(SysTick_IRQn, (1 << __NVIC_PRIO_BITS) - 1);
	
	SysTick->LOAD = ms * SystemCoreClock / 1000 - 1;
	SysTick->VAL = 0;
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | 
					SysTick_CTRL_TICKINT_Msk |
					SysTick_CTRL_ENABLE_Msk;
}

void SysTick_Handler() {
	os_time_tick();
}

