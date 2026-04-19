#include "os.h"
#include "os_cpu.h"

void os_time_tick() {
	os_tick_list_update();
	
	os_sched_round_robin(&g_os_ready_list[os_prio_cur]);
	
	os_sched();
}

void os_time_delay(os_tick_t delay_ticks) {
	uint32_t cpu_sr = os_cpu_sr_save();

	os_tick_list_insert(os_tcb_cur_ptr, delay_ticks);
	os_ready_list_remove(os_tcb_cur_ptr);
	
	os_cpu_sr_restore(cpu_sr);
	
	os_sched();
}