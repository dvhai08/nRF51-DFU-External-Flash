#ifndef __SYSTEM_CONFIG_H__
#define __SYSTEM_CONFIG_H__
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#define MCU_RESET_NONE					0
#define MCU_RESET_IMMEDIATELY 	1
#define MCU_RESET_AFTER_10_SEC 	2
#define MCU_RESET_AFTER_30_SEC 	3
#define MCU_RESET_IS_WAITING 		4

extern uint8_t sysResetMcuFlag;

void ResetMcuSet(uint8_t resetType);
void ResetMcuTask(void);

#endif
