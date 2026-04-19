#include "stm32f10x.h"                  // Device header
#include "os_cpu.h"
#include "os.h"

//#define DELAY_DURING_TIME 500000
#define DELAY_DURING_TIME 500

#define TASK1_STK_SIZE 128
#define TASK2_STK_SIZE 128
#define TASK3_STK_SIZE 128

static cpu_stack_t task1_stack[TASK1_STK_SIZE];
static cpu_stack_t task2_stack[TASK2_STK_SIZE];
static cpu_stack_t task3_stack[TASK3_STK_SIZE];

static os_task_ctl_block_t task1_tcb;
static os_task_ctl_block_t task2_tcb;
static os_task_ctl_block_t task3_tcb;

void Hardware_Prepare() {
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void Map2GPIO(uint16_t GPIO_Pin, uint32_t val) {
	if (val > 0) {
		GPIO_SetBits(GPIOA, GPIO_Pin);
	} else {
		GPIO_ResetBits(GPIOA, GPIO_Pin);
	}
}

void delay( uint32_t count ) {
    for (; count!=0; count--);
}

void task1(void *args) {
	uint32_t flag1;
	
	for (;;) {
		flag1 = 1;
		Map2GPIO(GPIO_Pin_0, flag1);
        //os_time_delay(2);
		os_task_suspend(&task1_tcb);
		
        flag1 = 0;          
		Map2GPIO(GPIO_Pin_0, flag1);
        //os_time_delay(2);
		os_task_suspend(&task1_tcb);
	}
}

void task2(void *args) {
	uint32_t flag2;
	
	for (;;) {
		flag2 = 1;
		Map2GPIO(GPIO_Pin_1, flag2);
        os_time_delay(1);
		//delay(DELAY_DURING_TIME);
		
        flag2 = 0;
		Map2GPIO(GPIO_Pin_1, flag2);
        os_time_delay(1);
		//delay(DELAY_DURING_TIME);
		os_task_resume(&task1_tcb);
	}
}

void task3(void *args) {
	uint32_t flag3;
	
	for (;;) {
		flag3 = 1;
		Map2GPIO(GPIO_Pin_2, flag3);
        os_time_delay(1);
		//delay(DELAY_DURING_TIME);
		
        flag3 = 0;
		Map2GPIO(GPIO_Pin_2, flag3);
        os_time_delay(1);
		//delay(DELAY_DURING_TIME);
	}
}

int main(void) {
	Hardware_Prepare();
	
	os_cpu_int_dis();
	
	os_sys_tick_init(10);

	os_init();
	
	os_task_create(&task1_tcb, 
				   task1, 
				   NULL,
				   1,
				   &task1_stack[0], 
				   TASK1_STK_SIZE,
				   0);
	
	os_task_create(&task2_tcb, 
				   task2, 
				   NULL, 
				   2,
				   &task2_stack[0], 
				   TASK2_STK_SIZE,
				   1);
				   
	os_task_create(&task3_tcb, 
				   task3, 
				   NULL, 
				   2,
				   &task3_stack[0], 
				   TASK3_STK_SIZE,
				   1);
	
	os_start();
	
    for (;;) {
        
    }
}