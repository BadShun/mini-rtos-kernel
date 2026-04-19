#include "os.h"

os_tick_spoke_t g_os_tick_wheel[OS_TICK_WHEEL_SIZE];

void os_tick_list_init() {
	os_tick_spoke_t *p_spoke;
	
	for (uint8_t i = 0; i < OS_TICK_WHEEL_SIZE; i++) {
		p_spoke = &g_os_tick_wheel[i];
		p_spoke->first_tcb = NULL;
		p_spoke->cnt = 0;
		p_spoke->max_cnt = 0;
	}
}

void os_tick_list_insert(os_task_ctl_block_t *p_tcb, os_tick_t time) {
	p_tcb->tick_cnt_match = os_tick_cnt + time;
	p_tcb->tick_remain = time;
	
	uint8_t spoke_index = p_tcb->tick_cnt_match % OS_TICK_WHEEL_SIZE;
	os_tick_spoke_t *p_spoke = &g_os_tick_wheel[spoke_index];
	
	if (p_spoke->cnt == 0) {
		p_tcb->next_tick_ptr = NULL;
		p_tcb->prev_tick_ptr = NULL;
		p_spoke->first_tcb   = p_tcb;
		p_spoke->cnt         = 1;
	} else {
		os_task_ctl_block_t *p_tcb_head = p_spoke->first_tcb;
		
		while (p_tcb_head != NULL) {
			p_tcb_head->tick_remain = p_tcb_head->tick_cnt_match - os_tick_cnt;
			
			if (p_tcb->tick_remain > p_tcb_head->tick_remain) {
				if (p_tcb_head->next_tick_ptr != NULL) {
					p_tcb_head = p_tcb_head->next_tick_ptr;
				}else {
					p_tcb->next_tick_ptr = NULL;
					p_tcb->prev_tick_ptr = p_tcb_head;
					p_tcb_head->next_tick_ptr = p_tcb;
					p_tcb_head = NULL;
				}
			} else {
				if (p_tcb_head->prev_tick_ptr == NULL) {
					p_spoke->first_tcb = p_tcb;
					p_tcb->prev_tick_ptr = NULL;
					p_tcb->next_tick_ptr = p_tcb_head;
					p_tcb_head->prev_tick_ptr = p_tcb;
					p_tcb_head = NULL;
				} else {
					p_tcb->next_tick_ptr = p_tcb_head;
					p_tcb->prev_tick_ptr = p_tcb_head->prev_tick_ptr;
					p_tcb_head->prev_tick_ptr->next_tick_ptr = p_tcb;
					p_tcb_head->prev_tick_ptr = p_tcb;
					p_tcb_head = NULL;
				}
			}
		}
		
		p_spoke->cnt++;
	}
	
	if (p_spoke->cnt > p_spoke->max_cnt) {
		p_spoke->max_cnt = p_spoke->cnt;
	}
	
	p_tcb->tick_spoke_ptr = p_spoke;
}

void os_tick_list_remove(os_task_ctl_block_t *p_tcb) {
	os_tick_spoke_t *p_spoke = p_tcb->tick_spoke_ptr;
	
	if (p_spoke == NULL) {
		return ;
	}
	
	p_tcb->tick_remain = 0;
		
	if (p_tcb == p_spoke->first_tcb) {
		p_spoke->first_tcb = p_spoke->first_tcb->next_tick_ptr;
		
		if (p_spoke->first_tcb != NULL) {
			p_spoke->first_tcb->prev_tick_ptr = NULL;
		}
	} else {
		os_task_ctl_block_t *p_prev_tick_tcb = p_tcb->prev_tick_ptr;
		os_task_ctl_block_t *p_next_tick_tcb = p_tcb->next_tick_ptr;
		
		p_prev_tick_tcb->next_tick_ptr = p_next_tick_tcb;
		
		if (p_next_tick_tcb != NULL) {
			p_next_tick_tcb->prev_tick_ptr = p_prev_tick_tcb;
		}
	}

	p_tcb->prev_tick_ptr  = NULL;
	p_tcb->next_tick_ptr  = NULL;
	p_tcb->tick_spoke_ptr = NULL;
	p_tcb->tick_cnt_match = 0;
	
	p_spoke->cnt--;
}

void os_tick_list_update() {
	uint32_t cpu_sr = os_cpu_sr_save();
	
	os_tick_cnt++;
	uint8_t spoke_index = os_tick_cnt % OS_TICK_WHEEL_SIZE;
	os_tick_spoke_t *p_spoke = &g_os_tick_wheel[spoke_index];
	os_task_ctl_block_t *p_tcb = p_spoke->first_tcb;
	
	while (p_tcb != NULL) {
		// 提前保存next指针，防止os_task_ready中把p_tcb删除后next变为空
		os_task_ctl_block_t *p_next_tcb = p_tcb->next_tick_ptr; 
		
		p_tcb->tick_remain = p_tcb->tick_cnt_match - os_tick_cnt;
			
		if (p_tcb->tick_cnt_match == os_tick_cnt) {
			os_task_ready(p_tcb);
		} else {
			break;
		}
		
		p_tcb = p_next_tcb;
	}
	
	os_cpu_sr_restore(cpu_sr);
}