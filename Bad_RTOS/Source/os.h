#ifndef __OS_H
#define __OS_H

#include "os_type.h"
#include "os_cpu.h"

#ifdef OS_GLOBALS
	#define OS_EXTERN
#else
	#define OS_EXTERN extern
#endif
	
#ifndef  NVIC_INT_CTRL
#define  NVIC_INT_CTRL *((cpu_reg32_t *)0xE000ED04)   /* 中断控制及状态寄存器 SCB_ICSR */
#endif

#ifndef  NVIC_PENDSVSET
#define  NVIC_PENDSVSET 0x10000000    /* 触发PendSV异常的值 Bit28：PENDSVSET */
#endif

#define os_task_switch() NVIC_INT_CTRL = NVIC_PENDSVSET

#define OS_PRIORITY_MAX 32u
#define OS_INT_NBR_BITS 32u
#define OS_PRIO_TBL_SIZE ((OS_PRIORITY_MAX - 1u) / (OS_INT_NBR_BITS) + 1u)
#define OS_PRIORITY_INIT (OS_PRIORITY_MAX)

#define OS_TICK_WHEEL_SIZE 17u

typedef uint8_t os_state_t;
typedef uint8_t os_priority_t;
typedef uint8_t os_nesting_cnt_t;

#define OS_STATE_STOPPED (os_state_t)(0)
#define OS_STATE_RUNNING (os_state_t)(1)

#define  OS_TASK_STATE_BIT_DLY                (os_state_t)(0x01u)/*   /-------- 挂起位          */
#define  OS_TASK_STATE_BIT_PEND               (os_state_t)(0x02u)/*   | /-----  等待位          */
#define  OS_TASK_STATE_BIT_SUSPENDED          (os_state_t)(0x04u)/*   | | /---  延时/超时位      */

#define  OS_TASK_STATE_RDY                    (os_state_t)(  0u) /*   0 0 0  就绪               */
#define  OS_TASK_STATE_DLY                    (os_state_t)(  1u) /*   0 0 1  延时或者超时        */
#define  OS_TASK_STATE_PEND                   (os_state_t)(  2u) /*   0 1 0  等待               */
#define  OS_TASK_STATE_PEND_TIMEOUT           (os_state_t)(  3u) /*   0 1 1  等待+超时*/
#define  OS_TASK_STATE_SUSPENDED              (os_state_t)(  4u) /*   1 0 0  挂起               */
#define  OS_TASK_STATE_DLY_SUSPENDED          (os_state_t)(  5u) /*   1 0 1  挂起 + 延时或者超时*/
#define  OS_TASK_STATE_PEND_SUSPENDED         (os_state_t)(  6u) /*   1 1 0  挂起 + 等待         */
#define  OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED (os_state_t)(  7u) /*   1 1 1  挂起 + 等待 + 超时*/
#define  OS_TASK_STATE_DEL                    (os_state_t)(255u)

struct spoke;
	
typedef struct tcb {
	cpu_stack_t     *p_stack;
	cpu_stack_size_t stack_size;
	
	os_tick_t        task_dly_ticks;
	
	os_priority_t    priority;
	
	struct tcb      *next_ptr;
	struct tcb      *prev_ptr;
	
	struct tcb	    *next_tick_ptr;
	struct tcb      *prev_tick_ptr;
	struct spoke    *tick_spoke_ptr;
	
	os_tick_t        tick_cnt_match;
	os_tick_t        tick_remain;
	
	os_tick_t        time_quanta;
	os_tick_t        time_quanta_cnt;
	
	os_state_t       task_state;
	os_nesting_cnt_t suspend_cnt;
} os_task_ctl_block_t;

typedef struct {
	os_task_ctl_block_t *head_ptr;
	os_task_ctl_block_t *tail_ptr;
	uint16_t             task_cnt;
} os_ready_list_t;

typedef struct spoke {
	os_task_ctl_block_t *first_tcb;
	uint16_t cnt;
	uint16_t max_cnt;
} os_tick_spoke_t;

typedef void (*os_task_func_ptr)(void *p_args);

OS_EXTERN os_task_ctl_block_t *os_tcb_cur_ptr;
OS_EXTERN os_task_ctl_block_t *os_tcb_high_ready_ptr;
OS_EXTERN os_ready_list_t g_os_ready_list[OS_PRIORITY_MAX];
OS_EXTERN os_state_t os_state;

OS_EXTERN os_task_ctl_block_t os_idle_tcb;
OS_EXTERN uint32_t os_idle_cnt;

extern uint32_t os_prio_table[OS_PRIO_TBL_SIZE];
OS_EXTERN os_priority_t os_prio_cur;
OS_EXTERN os_priority_t os_prio_high_ready;
OS_EXTERN os_priority_t os_prio_saved;

extern os_tick_spoke_t g_os_tick_wheel[OS_TICK_WHEEL_SIZE];
OS_EXTERN os_tick_t os_tick_cnt;

extern cpu_stack_t * const os_idle_task_stk_base_ptr;
extern cpu_stack_size_t const os_idle_task_stk_size;

cpu_stack_t *os_task_stack_init(os_task_func_ptr p_task,
							    void            *p_args,
							    cpu_stack_t     *p_stk_base,
							    cpu_stack_size_t   stk_size);

void os_task_create(os_task_ctl_block_t *p_tcb,
				    os_task_func_ptr     p_task,
			        void                *p_args,
			    	os_priority_t        priority,
			        cpu_stack_t         *p_stk_base,
		            cpu_stack_size_t       stk_size,
		   		    os_tick_t            time_quanta);

void os_idle_task_init();
						
void os_init();
void os_start();
						
void os_prio_table_init();
void os_prio_table_insert(os_priority_t priority);
void os_prio_table_remove(os_priority_t priority);
os_priority_t os_prio_table_get_highest();
						  
void os_ready_list_insert_head(os_task_ctl_block_t *p_tcb);
void os_ready_list_insert_tail(os_task_ctl_block_t *p_tcb);
void os_ready_list_insert(os_task_ctl_block_t *p_tcb);
void os_ready_list_move_head_to_tail(os_ready_list_t *p_ready_list);
void os_ready_list_remove(os_task_ctl_block_t *p_tcb);
void os_task_ready(os_task_ctl_block_t *p_tcb);
						  
void os_tick_list_init();
void os_tick_list_insert(os_task_ctl_block_t *p_tcb, os_tick_t time);
void os_tick_list_remove(os_task_ctl_block_t *p_tcb);
void os_tick_list_update();

void os_task_suspend(os_task_ctl_block_t *p_tcb);
void os_task_resume(os_task_ctl_block_t *p_tcb);

void os_sched();
void os_sched_round_robin(os_ready_list_t *p_ready_list);

void os_cpu_int_dis();
void os_cpu_int_en();
						  
uint32_t os_cpu_sr_save();
void os_cpu_sr_restore(uint32_t cpu_sr);
					
void os_sys_tick_init(uint32_t ms);					
void os_time_tick();
void os_time_delay(os_tick_t delay_ticks);
						
void os_start_high_ready();
void PendSV_Handler();

#endif