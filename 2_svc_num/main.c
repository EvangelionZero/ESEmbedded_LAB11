#include <stdint.h>
#include <stdio.h>
#include "reg.h"
#include "blink.h"
#include "usart.h"
#include "asm_func.h"

void user_task(void);

int main(void)
{
	extern uint32_t _msp_init;

	uint32_t *msp_init = &_msp_init;
	uint32_t *psp_init = msp_init - 8 * 1024;

	init_usart1();

	printf("[Kernel] Start in privileged thread mode.\r\n\n");

	printf("[Kernel] Control: 0x%X \r\n", (unsigned int)read_ctrl());
	printf("[Kernel] SP: 0x%X \r\n", (unsigned int)read_sp());
	printf("[Kernel] MSP: 0x%X \r\n", (unsigned int)read_msp());
	printf("[Kernel] PSP: 0x%X \r\n\n", (unsigned int)read_psp());

	printf("[Kernel] SVC 0.\r\n\n");
	printf("[Kernel] SVC return %d.\r\n\n", (int)sys_call());

	printf("[Kernel] Switch to unprivileged thread mode & start user task with psp.\r\n\n");

	//start user task
	start_user((uint32_t *)user_task, psp_init);

	while (1) //should not go here
		;
}

void user_task(void)
{
	printf("[User] Start in unprivileged thread mode.\r\n\n");

	printf("[User] Control: 0x%X \r\n", (unsigned int)read_ctrl());
	printf("[User] SP: 0x%X \r\n", (unsigned int)read_sp());
	printf("[User] MSP: 0x%X \r\n", (unsigned int)read_msp());
	printf("[User] PSP: 0x%X \r\n\n", (unsigned int)read_psp());

	printf("[User] SVC add.\r\n\n");
	printf("[User] SVC return %d.\r\n\n", (int)sys_call_add(8, 9));

	blink(LED_BLUE); //should not return
}

void svc_handler_c(uint32_t EXC_RETURN, uint32_t MSP_backup)
{
	printf("[SVC Handler] LR: 0x%X\r\n", (unsigned int)EXC_RETURN);
	printf("[SVC Handler] MSP Backup: 0x%X \r\n", (unsigned int)MSP_backup);//此msp為stackframe的初始位置
	printf("[SVC Handler] Control: 0x%X\r\n", (unsigned int)read_ctrl());
	printf("[SVC Handler] SP: 0x%X \r\n", (unsigned int)read_sp());
	printf("[SVC Handler] MSP: 0x%X \r\n", (unsigned int)read_msp());//此msp為自MSP_backup開始往下堆疊的sp,不一定與MSP_backup相同
	printf("[SVC Handler] PSP: 0x%X \r\n\n", (unsigned int)read_psp());

	uint32_t *stack_frame_ptr;
	if ((0x4) & EXC_RETURN) //Test bit 2 of EXC_RETURN
	{
		stack_frame_ptr = (uint32_t *)read_psp(); //if 1, stacking used PSP
		printf("[SVC Handler] Stacking used PSP: 0x%X \r\n\n", (unsigned int)stack_frame_ptr);
	}
	else
	{
		stack_frame_ptr = (uint32_t *)MSP_backup; //if 0, stacking used MSP
		printf("[SVC Handler] Stacking used MSP: 0x%X \r\n\n", (unsigned int)stack_frame_ptr);
	}

	uint32_t stacked_r0 = stack_frame_ptr[0];
	uint32_t stacked_r1 = stack_frame_ptr[1];
	uint32_t stacked_return_addr = stack_frame_ptr[6];

	uint16_t svc_instruction = *((uint16_t *)stacked_return_addr-1);
	uint8_t svc_num = (uint8_t) svc_instruction;

	printf("[SVC Handler] Stacked R0: 0x%X \r\n", (unsigned int)stacked_r0);
	printf("[SVC Handler] Stacked R1: 0x%X \r\n", (unsigned int)stacked_r1);
	printf("[SVC Handler] SVC number: 0x%X \r\n\n", (unsigned int)svc_num);

	if (svc_num == 0xA)
		stack_frame_ptr[0] = stacked_r0 + stacked_r1;
	else
		stack_frame_ptr[0] = 0;
}
