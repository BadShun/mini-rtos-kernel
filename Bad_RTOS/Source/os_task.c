#include "os.h"
#include "os_cpu.h"

#define OS_IDLE_TASK_STK_SIZE 128u

cpu_stack_t os_idle_task_stk[OS_IDLE_TASK_STK_SIZE];
cpu_stack_t * const os_idle_task_stk_base_ptr = (cpu_stack_t *)&os_idle_task_stk[0];
cpu_stack_size_t const os_idle_task_stk_size = OS_IDLE_TASK_STK_SIZE;

static void _os_task_init_tcb(os_task_ctl_block_t *p_tcb) {
	p_tcb->p_stack = NULL;
	p_tcb->stack_size = 0;
	
	p_tcb->task_dly_ticks = 0;
	
	p_tcb->priority = OS_PRIORITY_INIT;
	
	p_tcb->prev_ptr = NULL;
	p_tcb->next_ptr = NULL;
}

void os_task_create(os_task_ctl_block_t *p_tcb,
				    os_task_func_ptr     p_task,
				    void                *p_args,
				    os_priority_t        priority,
			        cpu_stack_t         *p_stk_base,
				    cpu_stack_size_t       stk_size,
					os_tick_t            time_quanta) {
	_os_task_init_tcb(p_tcb);
							  
	cpu_stack_t *p_sp = os_task_stack_init(p_task, p_args, 
						   p_stk_base, stk_size);
							  
	p_tcb->priority = priority;
	p_tcb->p_stack = p_sp;
	p_tcb->stack_size = stk_size;
	p_tcb->time_quanta = time_quanta;
	p_tcb->time_quanta_cnt = time_quanta;
							  
	uint32_t cpu_sr = os_cpu_sr_save();

	os_prio_table_insert(p_tcb->priority);
	os_ready_list_insert_tail(p_tcb);
							  
	os_cpu_sr_restore(cpu_sr);
}
					
void os_task_suspend(os_task_ctl_block_t *p_tcb) {
	uint32_t cpu_sr = os_cpu_sr_save();
	
	if (p_tcb == NULL) {
		p_tcb = os_tcb_cur_ptr;
	}
	
	switch (p_tcb->task_state) {
		case OS_TASK_STATE_RDY:
			p_tcb->task_state = OS_TASK_STATE_SUSPENDED;
			p_tcb->suspend_cnt = 1;
			os_ready_list_remove(p_tcb);
			os_cpu_sr_restore(cpu_sr);
			break;
		case OS_TASK_STATE_DLY:
			p_tcb->task_state = OS_TASK_STATE_DLY_SUSPENDED;
			p_tcb->suspend_cnt = 1;
			os_cpu_sr_restore(cpu_sr);
			break;
		case OS_TASK_STATE_PEND:
			p_tcb->task_state = OS_TASK_STATE_PEND_SUSPENDED;
			p_tcb->suspend_cnt = 1;
			os_cpu_sr_restore(cpu_sr);
			break;
		case OS_TASK_STATE_PEND_TIMEOUT:
			p_tcb->task_state = OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED;
			p_tcb->suspend_cnt = 1;
			os_cpu_sr_restore(cpu_sr);
			break;
		case OS_TASK_STATE_SUSPENDED:
		case OS_TASK_STATE_DLY_SUSPENDED:
        case OS_TASK_STATE_PEND_SUSPENDED:
        case OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED:
			p_tcb->suspend_cnt++;
			os_cpu_sr_restore(cpu_sr);
			break;
		default:
			os_cpu_sr_restore(cpu_sr);
			break;
	}
	
	os_sched();
}

void os_task_resume(os_task_ctl_block_t *p_tcb) {
	uint32_t cpu_sr = os_cpu_sr_save();
	
	switch (p_tcb->task_state) {
		case OS_TASK_STATE_RDY:
		case OS_TASK_STATE_DLY:
		case OS_TASK_STATE_PEND:
		case OS_TASK_STATE_PEND_TIMEOUT:
			os_cpu_sr_restore(cpu_sr);
			break;
		case OS_TASK_STATE_SUSPENDED:
			p_tcb->suspend_cnt--;
		
			if (p_tcb->suspend_cnt == 0) {
				p_tcb->task_state = OS_TASK_STATE_RDY;
				os_task_ready(p_tcb);
			}
			
			os_cpu_sr_restore(cpu_sr);
			break;
		case OS_TASK_STATE_DLY_SUSPENDED:
			p_tcb->suspend_cnt--;
		
			if (p_tcb->suspend_cnt == 0) {
				p_tcb->task_state = OS_TASK_STATE_DLY;
			}
			
			os_cpu_sr_restore(cpu_sr);
			break;
		case OS_TASK_STATE_PEND_SUSPENDED:
			p_tcb->suspend_cnt--;
		
			if (p_tcb->suspend_cnt == 0) {
				p_tcb->task_state = OS_TASK_STATE_PEND;
			}
			
			os_cpu_sr_restore(cpu_sr);
			break;
		case OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED:
			p_tcb->suspend_cnt--;
		
			if (p_tcb->suspend_cnt == 0) {
				p_tcb->task_state = OS_TASK_STATE_PEND_TIMEOUT;
			}
			
			os_cpu_sr_restore(cpu_sr);
			break;
		default:
			os_cpu_sr_restore(cpu_sr);
			break;
	}
	
	os_sched();
}

void os_task_del(os_task_ctl_block_t *p_tcb) {
	if (p_tcb == NULL) {
		uint32_t cpu_sr = os_cpu_sr_save();
		
		p_tcb = os_tcb_cur_ptr;
		
		os_cpu_sr_restore(cpu_sr);
	}
	
	uint32_t cpu_sr = os_cpu_sr_save();
	
	switch (p_tcb->task_state) {
		case OS_TASK_STATE_RDY:
			os_ready_list_remove(p_tcb);
			break;
		case OS_TASK_STATE_SUSPENDED:
			break;
		case OS_TASK_STATE_DLY:
		case OS_TASK_STATE_DLY_SUSPENDED:
			os_tick_list_remove(p_tcb);
			break;
		default:
			os_cpu_sr_restore(cpu_sr);
			return ;
	}
	
	_os_task_init_tcb(p_tcb);
	p_tcb->task_state = OS_TASK_STATE_DEL;
	
	os_cpu_sr_restore(cpu_sr);
	
	os_sched();
}

static void _os_idle_task(void *p_args) {
	for (;;) {
		os_idle_cnt++;
	}
}

void os_idle_task_init() {
	os_idle_cnt = 0;
	
	os_task_create(&os_idle_tcb, _os_idle_task, NULL, OS_PRIORITY_MAX - 1, 
					os_idle_task_stk_base_ptr, os_idle_task_stk_size, 0);
	
}