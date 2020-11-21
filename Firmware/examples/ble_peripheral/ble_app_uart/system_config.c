#include "system_config.h"
#include "sys_tick.h"
#include "nrf51.h"

uint8_t sysResetMcuFlag = MCU_RESET_NONE;

void ResetMcuSet(uint8_t resetType)
{
	sysResetMcuFlag = resetType;
}

void ResetMcuTask(void)
{
		static Timeout_Type tMcuResetTimeout;
		switch(sysResetMcuFlag)
		{
			case MCU_RESET_NONE:
				break;
			
			case MCU_RESET_IMMEDIATELY:
				NVIC_SystemReset();
				break;
			
			case MCU_RESET_AFTER_10_SEC:
				InitTimeout(&tMcuResetTimeout,SYSTICK_TIME_SEC(10));
				sysResetMcuFlag = MCU_RESET_IS_WAITING;
				break; 
			
			case MCU_RESET_AFTER_30_SEC:
				InitTimeout(&tMcuResetTimeout,SYSTICK_TIME_SEC(30));	
				sysResetMcuFlag = MCU_RESET_IS_WAITING;
				break;
			
			case MCU_RESET_IS_WAITING:
				if(CheckTimeout(&tMcuResetTimeout) == SYSTICK_TIMEOUT)
				{
					NVIC_SystemReset();
				}
				break;
				
			default:
				NVIC_SystemReset();
				break;
		}
}
