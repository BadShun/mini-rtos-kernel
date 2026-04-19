#include "os.h"

static void _os_ready_list_init() {
	os_ready_list_t *p_ready_list;
	
	for (uint8_t i = 0; i < OS_PRIORITY_MAX; i++) {
		p_ready_list = &g_os_ready_list[i];
		p_ready_list->head_ptr = NULL;
		p_ready_list->tail_ptr = NULL;
		p_ready_list->task_cnt = 0;
	}
}

void os_ready_list_insert_head(os_task_ctl_block_t *p_tcb) {
	os_ready_list_t *p_ready_list = &g_os_ready_list[p_tcb->priority];
	
	if (p_ready_list->task_cnt == 0) {
		p_ready_list->task_cnt = 1;
		
		p_tcb->prev_ptr = NULL;
		p_tcb->next_ptr = NULL;
		
		p_ready_list->head_ptr = p_tcb;
		p_ready_list->tail_ptr = p_tcb;
	} else {
		p_ready_list->task_cnt++;
		
		p_tcb->next_ptr = p_ready_list->head_ptr;
		p_tcb->prev_ptr = NULL;
		
		p_ready_list->head_ptr->prev_ptr = p_tcb;
		p_ready_list->head_ptr = p_tcb;
	}
}

void os_ready_list_insert_tail(os_task_ctl_block_t *p_tcb) {
	os_ready_list_t *p_ready_list = &g_os_ready_list[p_tcb->priority];
	
	if (p_ready_list->task_cnt == 0) {
		p_ready_list->task_cnt = 1;
		
		p_tcb->prev_ptr = NULL;
		p_tcb->next_ptr = NULL;
		
		p_ready_list->head_ptr = p_tcb;
		p_ready_list->tail_ptr = p_tcb;
	} else {
		p_ready_list->task_cnt++;
		
		p_tcb->next_ptr = NULL;
		p_tcb->prev_ptr = p_ready_list->tail_ptr;
		
		p_ready_list->tail_ptr->next_ptr = p_tcb;
		p_ready_list->tail_ptr = p_tcb;
	}
}

void os_ready_list_insert(os_task_ctl_block_t *p_tcb) {
	os_prio_table_insert(p_tcb->priority);
	
	if (p_tcb->priority == os_prio_cur) {
		os_ready_list_insert_tail(p_tcb);
	} else {
		os_ready_list_insert_head(p_tcb);
	}
}

void os_ready_list_move_head_to_tail(os_ready_list_t *p_ready_list) {
	os_task_ctl_block_t *p_head_tcb;
	os_task_ctl_block_t *p_tail_tcb;
	
	switch (p_ready_list->task_cnt) {
		case 0:
		case 1:
			break;
		case 2: {
			p_head_tcb = p_ready_list->head_ptr;
			p_tail_tcb = p_ready_list->tail_ptr;
			
			p_head_tcb->prev_ptr = p_tail_tcb;
			p_head_tcb->next_ptr = NULL;
			
			p_tail_tcb->next_ptr = p_head_tcb;
			p_tail_tcb->prev_ptr = NULL;
			
			p_ready_list->head_ptr = p_tail_tcb;
			p_ready_list->tail_ptr = p_head_tcb;
			break;
		}
		
		default: {
			p_head_tcb = p_ready_list->head_ptr;
			p_tail_tcb = p_ready_list->tail_ptr;
			
			os_task_ctl_block_t *p_new_head_tcb = p_head_tcb->next_ptr;
			p_new_head_tcb->prev_ptr = NULL;
			
			p_head_tcb->prev_ptr = p_tail_tcb;
			p_head_tcb->next_ptr = NULL;
			
			p_tail_tcb->next_ptr = p_head_tcb;
			p_tail_tcb->prev_ptr = NULL;
			
			p_ready_list->head_ptr = p_new_head_tcb;
			p_ready_list->tail_ptr = p_head_tcb;
			
			break;
		}
	}
}

void os_ready_list_remove(os_task_ctl_block_t *p_tcb) {
	os_ready_list_t *p_ready_list = &g_os_ready_list[p_tcb->priority];
	
	os_task_ctl_block_t *p_pre_tcb = p_tcb->prev_ptr;
	os_task_ctl_block_t *p_next_tcb = p_tcb->next_ptr;
	
	if (p_pre_tcb == NULL) {
		if (p_next_tcb == NULL) {
			p_ready_list->task_cnt = 0;
			p_ready_list->head_ptr = NULL;
			p_ready_list->tail_ptr = NULL;
			
			os_prio_table_remove(p_tcb->priority);
		} else {
			p_ready_list->task_cnt--;
			p_next_tcb->prev_ptr = NULL;
			p_ready_list->head_ptr = p_next_tcb;
		}
	} else {
		p_ready_list->task_cnt--;
		p_pre_tcb->next_ptr = p_next_tcb;
		
		if (p_next_tcb == NULL) {
			p_ready_list->tail_ptr = p_pre_tcb;
		} else {
			p_next_tcb->prev_ptr = p_pre_tcb;
		}
	}
	
	p_tcb->prev_ptr = NULL;
	p_tcb->next_ptr = NULL;
}

void os_task_ready(os_task_ctl_block_t *p_tcb) {
	os_tick_list_remove(p_tcb);
	os_ready_list_insert(p_tcb);
}

void os_init() {
	os_state = OS_STATE_STOPPED;
	
	os_tcb_cur_ptr        = NULL;
	os_tcb_high_ready_ptr = NULL;
	
	os_prio_cur        = 0;
	os_prio_high_ready = 0;
	os_prio_saved      = 0;
	
	os_prio_table_init();
	_os_ready_list_init();
	
	os_tick_cnt = 0;
	os_tick_list_init();
	
	os_idle_task_init();
}

void os_start() {
	if (os_state == OS_STATE_STOPPED) {
		os_prio_high_ready = os_prio_table_get_highest();
		os_prio_cur = os_prio_high_ready;
		
		os_tcb_high_ready_ptr = g_os_ready_list[os_prio_high_ready].head_ptr;
		os_tcb_cur_ptr = os_tcb_high_ready_ptr;
		
		os_state = OS_STATE_RUNNING;
		
		os_start_high_ready();
		
		return ;
	} else {
		return ;
	}
}

void os_sched() {
	uint32_t cpu_sr = os_cpu_sr_save();

	os_prio_high_ready = os_prio_table_get_highest();
	os_tcb_high_ready_ptr = g_os_ready_list[os_prio_high_ready].head_ptr;
	
	if (os_tcb_high_ready_ptr == os_tcb_cur_ptr) {
		os_cpu_sr_restore(cpu_sr);
		
		return ;
	}
	
	os_cpu_sr_restore(cpu_sr);
	
	os_task_switch();
}

void os_sched_round_robin(os_ready_list_t *p_ready_list) {
	uint32_t cpu_sr = os_cpu_sr_save();
	
	os_task_ctl_block_t *p_tcb = p_ready_list->head_ptr;
	
	if (p_tcb == NULL) {
		os_cpu_sr_restore(cpu_sr);
	}
	
	if (p_tcb == &os_idle_tcb) {
		os_cpu_sr_restore(cpu_sr);
	}
	
	if (p_tcb->time_quanta_cnt > 0) {
		p_tcb->time_quanta_cnt--;
	}
	
	if (p_tcb->time_quanta_cnt > 0) {
		os_cpu_sr_restore(cpu_sr);
	}
	
	if (p_ready_list->task_cnt <= 1) {
		os_cpu_sr_restore(cpu_sr);
	}
	
	os_ready_list_move_head_to_tail(p_ready_list);
	
	p_tcb = p_ready_list->head_ptr;
	p_tcb->time_quanta_cnt = p_tcb->time_quanta;
	
	os_cpu_sr_restore(cpu_sr);
}