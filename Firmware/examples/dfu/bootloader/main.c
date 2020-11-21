/* Copyright (c) 2013 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

/**@file
 *
 * @defgroup ble_sdk_app_bootloader_main main.c
 * @{
 * @ingroup dfu_bootloader_api
 * @brief Bootloader project main file.
 *
 * -# Receive start data packet. 
 * -# Based on start packet, prepare NVM area to store received data. 
 * -# Receive data packet. 
 * -# Validate data packet.
 * -# Write Data packet to NVM.
 * -# If not finished - Wait for next packet.
 * -# Receive stop data packet.
 * -# Activate Image, boot application.
 *
 */
#include "dfu_transport.h"
#include "bootloader.h"
#include "bootloader_util.h"
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_soc.h"
#include "app_error.h"
#include "nrf_gpio.h"
#include "ble.h"
#include "nrf.h"
#include "ble_hci.h"
#include "app_scheduler.h"
#include "app_timer_appsh.h"
#include "nrf_error.h"
#include "boards.h"
#include "softdevice_handler_appsh.h"
#include "pstorage_platform.h"
#include "nrf_mbr.h"
#include "nrf_log.h"
#include "nrf_delay.h"
#include "mx25l.h"
#include "dfu_ext_flash.h"

APP_TIMER_DEF(m_led_toggle_timer_id); 

#define IS_SRVC_CHANGED_CHARACT_PRESENT 1                                                       /**< Include the service_changed characteristic. For DFU this should normally be the case. */

#define BOOTLOADER_BUTTON               BSP_BUTTON_3                                            /**< Button used to enter SW update mode. */
#define UPDATE_IN_PROGRESS_LED          1                                               /**< Led used to indicate that DFU is active. */

#define APP_TIMER_PRESCALER             0                                                       /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_OP_QUEUE_SIZE         4                                                       /**< Size of timer operation queues. */

#define SCHED_MAX_EVENT_DATA_SIZE       MAX(APP_TIMER_SCHED_EVT_SIZE, 0)                        /**< Maximum size of scheduler events. */

#define SCHED_QUEUE_SIZE                20                                                      /**< Maximum number of events in the scheduler queue. */

#define FLASH_BUF_SIZE 									2048
#define USER_FLASH_START								0x08002800

#define PAGE_SIZE												(0x400)    /* 2 Kbyte */
#define FLASH_SIZE											(0x20000)  /* 256 KBytes */

uint32_t fileSize;
uint32_t fileCrc;
uint32_t flashCheckSum;
uint32_t memAddr = 0;

uint32_t firmwareFileOffSet;
uint32_t firmwareFileSize;

uint8_t buff[4096];
uint8_t flashBuff[2048];

int update_app_from_external_flash(void);
uint8_t new_application_is_valid(void);
uint8_t update_new_application_is_idle(void);
void update_new_application_init(void);
void update_new_application_start(void);
void execute_user_code(void);
/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze 
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num    Line number of the failing ASSERT call.
 * @param[in] file_name   File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(0xDEADBEEF, line_num, p_file_name);
}


/**@brief Function for initialization of LEDs.
 */
static void leds_init(void)
{
	nrf_gpio_cfg_output(1);
	nrf_gpio_cfg_output(5);
}


static void led_toggle_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    nrf_gpio_pin_toggle(5);		
}

/**@brief Function for initializing the timer handler module (app_timer).
 */
static void timers_init(void)
{
	uint32_t err_code;
	// Initialize timer module, making it use the scheduler.
	APP_TIMER_APPSH_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, true);
	
	err_code = app_timer_create(&m_led_toggle_timer_id, APP_TIMER_MODE_REPEATED,led_toggle_timeout_handler);
	APP_ERROR_CHECK(err_code);
}


/**@brief Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
 *
 * @details This function is called from the scheduler in the main loop after a BLE stack
 *          event has been received.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void sys_evt_dispatch(uint32_t event)
{
    pstorage_sys_event_handler(event);
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 *
 * @param[in] init_softdevice  true if SoftDevice should be initialized. The SoftDevice must only 
 *                             be initialized if a chip reset has occured. Soft reset from 
 *                             application must not reinitialize the SoftDevice.
 */
static void ble_stack_init(bool init_softdevice)
{
    uint32_t         err_code;
    sd_mbr_command_t com = {SD_MBR_COMMAND_INIT_SD, };
    nrf_clock_lf_cfg_t clock_lf_cfg = NRF_CLOCK_LFCLKSRC;

    if (init_softdevice)
    {
        err_code = sd_mbr_command(&com);
        APP_ERROR_CHECK(err_code);
    }
    
    err_code = sd_softdevice_vector_table_base_set(BOOTLOADER_REGION_START);
    APP_ERROR_CHECK(err_code);
   
    SOFTDEVICE_HANDLER_APPSH_INIT(&clock_lf_cfg, true);

    // Enable BLE stack.
    ble_enable_params_t ble_enable_params;
    // Only one connection as a central is used when performing dfu.
    err_code = softdevice_enable_get_default_config(1, 1, &ble_enable_params);
    APP_ERROR_CHECK(err_code);

    ble_enable_params.gatts_enable_params.service_changed = IS_SRVC_CHANGED_CHARACT_PRESENT;
    err_code = softdevice_enable(&ble_enable_params);
    APP_ERROR_CHECK(err_code);
    
    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for event scheduler initialization.
 */
static void scheduler_init(void)
{
    APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
}


/**@brief Function for bootloader main entry.
 */
int main(void)
{
    uint32_t err_code;
		uint32_t dfu_cnt = 0;
		uint32_t *u32Pt;
    bool     dfu_start = false;
    bool     app_reset = (NRF_POWER->GPREGRET == BOOTLOADER_DFU_START);

    if (app_reset)
    {
        NRF_POWER->GPREGRET = 0;
    }
    
    leds_init();
		MX25L_Init();
    // This check ensures that the defined fields in the bootloader corresponds with actual
    // setting in the chip.
    APP_ERROR_CHECK_BOOL(*((uint32_t *)NRF_UICR_BOOT_START_ADDRESS) == BOOTLOADER_REGION_START);
    APP_ERROR_CHECK_BOOL(NRF_FICR->CODEPAGESIZE == CODE_PAGE_SIZE);

    // Initialize.
    timers_init();		
		ble_stack_init(!app_reset);
		scheduler_init();
		
		err_code = app_timer_start(m_led_toggle_timer_id, APP_TIMER_TICKS(50, APP_TIMER_PRESCALER), NULL);
		APP_ERROR_CHECK(err_code);
		
		bootloader_init();
		dfu_init_remit();
		if(new_application_is_valid())
		{
			update_new_application_init();
		}
		
		memset(buff,0xAA,sizeof(buff));
		
//		dfu_prepare_func_app_erase_remit(1024);
		for (;;)
    {
        // Wait in low power state for any events.
        uint32_t err_code = sd_app_evt_wait();
        APP_ERROR_CHECK(err_code);

        // Event received. Process it from the scheduler.
        app_sched_execute();
				
				if(h_pstorage_status == PSTORAGE_IDLE)
				{
					nrf_gpio_pin_set(1);
					if(dfu_cnt == 0)dfu_cnt++;
				}
				else 
				{
					nrf_gpio_pin_clear(1);
				}
				
				
//				if(dfu_cnt == 1)
//				{
//					dfu_cnt = 2;
//					dfu_data_pkt_handle_remit((uint32_t*)buff,1024);
//				}
				
				if(update_new_application_is_idle()) break; //run application
				update_new_application_start();
    }
		// Select a bank region to use as application region.
		// @note: Only applications running from DFU_BANK_0_REGION_START is supported.
		bootloader_app_start(DFU_BANK_0_REGION_START);

    NVIC_SystemReset();
}

void execute_user_code(void)
{
	// Select a bank region to use as application region.
	// @note: Only applications running from DFU_BANK_0_REGION_START is supported.
	bootloader_app_start(DFU_BANK_0_REGION_START);	
}


int update_app_from_external_flash(void)
{
		
}

uint8_t new_application_is_valid(void)
{
	uint32_t *u32Pt = (uint32_t *)buff;
	uint32_t i;
		
	SST25_Read(FIRMWARE_INFO_ADDR + FIRMWARE_STATUS_OFFSET*4,buff,4);
	if(*u32Pt != 0x5A5A5A5A)	
	{
		return 0;
	}
	SST25_Read(FIRMWARE_INFO_ADDR + FIRMWARE_CRC_OFFSET*4,buff,4);
	fileCrc = 	*u32Pt;
	SST25_Read(FIRMWARE_INFO_ADDR + FIRMWARE_FILE_SIZE_OFFSET*4,buff,4);
	fileSize = 	*u32Pt;
	return 1;
}

uint8_t update_new_application_is_idle(void)
{
	if(h_dfu_update_status == DFU_UPDATE_IDLE) return 1;
	return 0;
}

void update_new_application_init(void)
{
	h_dfu_update_status = DFU_UPDATE_APP_ERASE;
	h_dfu_update_status_store = h_dfu_update_status;
}

void update_new_application_start(void)
{
	static uint32_t bytes_read;
	uint32_t *u32Pt = (uint32_t *)buff;
	uint32_t err_code;
	uint32_t j;
	
	switch(h_dfu_update_status)
	{
		case DFU_UPDATE_APP_ERASE:
				h_dfu_update_status_store = h_dfu_update_status;
				dfu_prepare_func_app_erase_remit(fileSize);
				h_dfu_update_status = DFU_UPDATE_UPDATING;
		
				//h_dfu_update_status = DFU_UPDATE_IDLE;
				break;
			
		case DFU_UPDATE_APP_UPDATE:
				
				if(bytes_read < fileSize)
				{
					SST25_Read(bytes_read + FIRMWARE_BASE_ADDR,buff, PAGE_SIZE);
					for(j = 0 ; j < PAGE_SIZE;j++)
					{
						if(bytes_read + j < fileSize)
							flashCheckSum += buff[j];
						else
							break;
					}
					
					u32Pt = (uint32_t *)buff;
					err_code = dfu_data_pkt_handle_remit(u32Pt,j);					
					bytes_read += PAGE_SIZE;
					h_dfu_update_status_store = h_dfu_update_status;
					h_dfu_update_status = DFU_UPDATE_UPDATING;
				}
				else
				{
					h_dfu_update_status = DFU_UPDATE_FINISH;
				}
				break;
			
			case DFU_UPDATE_FINISH:
				if(flashCheckSum == fileCrc)
				{
					
				}
				SST25_Erase(FIRMWARE_INFO_ADDR,block4k);
				nrf_gpio_pin_set(1);
				h_dfu_update_status = DFU_UPDATE_IDLE;
				break;
				
			case DFU_UPDATE_UPDATING:				
				if(h_pstorage_status == PSTORAGE_BUSY) break;
				
				if(h_dfu_update_status_store == DFU_UPDATE_APP_ERASE)
				{
					h_dfu_update_status = DFU_UPDATE_APP_UPDATE;
					bytes_read = 0;
					flashCheckSum = 0;
				}
				else if(h_dfu_update_status_store == DFU_UPDATE_APP_UPDATE)
				{
					h_dfu_update_status = DFU_UPDATE_APP_UPDATE;
				}
				break;
			
			default:
				break;
		}
}

