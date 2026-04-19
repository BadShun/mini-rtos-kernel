#include "os.h"

uint32_t os_prio_table[OS_PRIO_TBL_SIZE];

void os_prio_table_init() {
	for (uint32_t i = 0; i< OS_PRIO_TBL_SIZE; i++) {
		os_prio_table[i] = 0u;
	}
}

void os_prio_table_insert(os_priority_t priority) {
	uint8_t prio_index = priority / OS_INT_NBR_BITS;
	uint32_t bit_nbr = (uint32_t)priority & (OS_INT_NBR_BITS - 1u);
	uint32_t bit = 1u;
	bit <<= (OS_INT_NBR_BITS - 1u) - bit_nbr;
	
	os_prio_table[prio_index] |= bit;
}

void os_prio_table_remove(os_priority_t priority) {
	uint8_t prio_index = priority / OS_INT_NBR_BITS;
	uint32_t bit_nbr = (uint32_t)priority & (OS_INT_NBR_BITS - 1u);
	uint32_t bit = 1u;
	bit <<= (OS_INT_NBR_BITS - 1u) - bit_nbr;
	
	os_prio_table[prio_index] &= ~bit;
}

static __asm uint32_t _cnt_lead_zeros(uint32_t val) {
	PRESERVE8

	CLZ R0, R0
	BX LR
}

static __asm uint32_t _cnt_trail_zeros(uint32_t val) {
	PRESERVE8
	
	RBIT R0, R0
	CLZ R0, R0
	BX LR
}

os_priority_t os_prio_table_get_highest() {
	uint32_t *p_table = &os_prio_table[0];
	os_priority_t priority = 0;
	
	while (*p_table == 0) {
		priority += OS_INT_NBR_BITS;
		p_table++;
	}
	
	priority += (os_priority_t)_cnt_lead_zeros(*p_table);
	
	return priority;
}