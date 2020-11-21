#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "boards.h"
#include "softdevice_handler.h"
#include "app_timer.h"
#include "pstorage.h"
#include "app_trace.h"
#include "boards.h"
#include "nrf_gpio.h"
#include "ble_hci.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "pstorage.h"
#include "app_uart.h"


#define CENTRAL_LINK_COUNT               0                                          /**< Number of central links used by the application. When changing this number remember to adjust the RAM settings*/
#define PERIPHERAL_LINK_COUNT            1                                          /**< Number of peripheral links used by the application. When changing this number remember to adjust the RAM settings*/

#define APP_TIMER_PRESCALER              0                                          /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_OP_QUEUE_SIZE          4                                          /**< Size of timer operation queues. */

#define DEAD_BEEF                        0xDEADBEEF                                 /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define UART_TX_BUF_SIZE                1024                                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE                16                                         /**< UART RX buffer size. */

static uint8_t pstorage_wait_flag = 0;
static pstorage_block_t pstorage_wait_handle = 0;

#define APP_LOG(...)

                                   
/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num   Line number of the failing ASSERT call.
 * @param[in] file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}


/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
static void timers_init(void)
{

    // Initialize timer module.
    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, false);

    // Create timers.

    /* YOUR_JOB: Create any timers to be used by the application.
                 Below is an example of how to create a timer.
                 For every new timer needed, increase the value of the macro APP_TIMER_MAX_TIMERS by
                 one.
    uint32_t err_code;
    err_code = app_timer_create(&m_app_timer_id, APP_TIMER_MODE_REPEATED, timer_timeout_handler);
    APP_ERROR_CHECK(err_code); */
}


/**@brief Function for dispatching a system event to interested modules.
 *
 * @details This function is called from the System event interrupt handler after a system
 *          event has been received.
 *
 * @param[in] sys_evt  System stack event.
 */
static void sys_evt_dispatch(uint32_t sys_evt)
{
    pstorage_sys_event_handler(sys_evt);
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    uint32_t err_code;
    
    nrf_clock_lf_cfg_t clock_lf_cfg = NRF_CLOCK_LFCLKSRC;
    
    // Initialize the SoftDevice handler module.
    SOFTDEVICE_HANDLER_INIT(&clock_lf_cfg, NULL);
    
    ble_enable_params_t ble_enable_params;
    err_code = softdevice_enable_get_default_config(CENTRAL_LINK_COUNT,
                                                    PERIPHERAL_LINK_COUNT,
                                                    &ble_enable_params);
    APP_ERROR_CHECK(err_code);
    
    //Check the ram settings against the used number of links
    CHECK_RAM_START_ADDR(CENTRAL_LINK_COUNT,PERIPHERAL_LINK_COUNT);
    
    // Enable BLE stack.
    err_code = softdevice_enable(&ble_enable_params);
    APP_ERROR_CHECK(err_code);
    
    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);
}

/**
 * @brief UART initialization.
 */
void uart_config(void)
{
    uint32_t                     err_code;
    const app_uart_comm_params_t comm_params =
    {
        15,
        16,
        RTS_PIN_NUMBER,
        CTS_PIN_NUMBER,
        APP_UART_FLOW_CONTROL_DISABLED,
        false,
        UART_BAUDRATE_BAUDRATE_Baud115200
    };

    APP_UART_FIFO_INIT(&comm_params,
                       UART_RX_BUF_SIZE,
                       UART_TX_BUF_SIZE,
                       NULL,
                       APP_IRQ_PRIORITY_LOW,
                       err_code);

    APP_ERROR_CHECK(err_code);
}

/**@brief Function for the Power manager.
 */
static void power_manage(void)
{
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}

static void example_cb_handler(pstorage_handle_t  * handle,
															 uint8_t              op_code,
                               uint32_t             result,
                               uint8_t            * p_data,
                               uint32_t             data_len)
{
		if(handle->block_id == pstorage_wait_handle) { pstorage_wait_flag = 0; }  //If we are waiting for this callback, clear the wait flag.
	
		switch(op_code)
		{
			case PSTORAGE_LOAD_OP_CODE:
				 if (result == NRF_SUCCESS)
				 {
						 APP_LOG("pstorage LOAD callback received \r\n");
//						 bsp_indication_set(BSP_INDICATE_ALERT_0);				 
				 }
				 else
				 {
						 APP_LOG("pstorage LOAD ERROR callback received \r\n");
//						 bsp_indication_set(BSP_INDICATE_RCV_ERROR);
				 }
				 break;
			case PSTORAGE_STORE_OP_CODE:
				 if (result == NRF_SUCCESS)
				 {
						 APP_LOG("pstorage STORE callback received \r\n");
//						 bsp_indication_set(BSP_INDICATE_ALERT_1);
				 }
				 else
				 {
					   APP_LOG("pstorage STORE ERROR callback received \r\n");
//						 bsp_indication_set(BSP_INDICATE_RCV_ERROR);
				 }
				 break;				 
			case PSTORAGE_UPDATE_OP_CODE:
				 if (result == NRF_SUCCESS)
				 {
						 APP_LOG("pstorage UPDATE callback received \r\n");
//						 bsp_indication_set(BSP_INDICATE_ALERT_2);
				 }
				 else
				 {
						 APP_LOG("pstorage UPDATE ERROR callback received \r\n");
//						 bsp_indication_set(BSP_INDICATE_RCV_ERROR);
				 }
				 break;
			case PSTORAGE_CLEAR_OP_CODE:
				 if (result == NRF_SUCCESS)
				 {
						 APP_LOG("pstorage CLEAR callback received \r\n");
//						 bsp_indication_set(BSP_INDICATE_ALERT_3);
				 }
				 else
				 {
					   APP_LOG("pstorage CLEAR ERROR callback received \r\n");
//						 bsp_indication_set(BSP_INDICATE_RCV_ERROR);
				 }
				 break;	 
		}			
}

static void pstorage_test_store_and_update(void)
{
		pstorage_handle_t       handle;
		pstorage_handle_t				block_0_handle;
		pstorage_handle_t				block_1_handle;
		pstorage_handle_t				block_2_handle;
		pstorage_module_param_t param;
		uint8_t                 source_data_0[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
		uint8_t                 source_data_1[16] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};
//		uint8_t                 source_data_2[16] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F};
		uint8_t                 source_data_9[16] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', '\0'};
		uint8_t                 dest_data_0[16];
		uint8_t                 dest_data_1[16];
		uint8_t                 dest_data_2[16];
		uint32_t                retval;
		
		retval = pstorage_init();
//		if(retval != NRF_SUCCESS)
//		{
//				bsp_indication_set(BSP_INDICATE_FATAL_ERROR);
//		}
	     
		param.block_size  = 16;                   //Select block size of 16 bytes
		param.block_count = 4;                   //Select 10 blocks, total of 160 bytes
		param.cb          = example_cb_handler;   //Set the pstorage callback handler
			
		APP_LOG("\r\n\r\npstorage initializing ... \r\n");
		retval = pstorage_register(&param, &handle);
//		if (retval != NRF_SUCCESS)
//		{
//				bsp_indication_set(BSP_INDICATE_FATAL_ERROR);
//		}
		
//		//Get block identifiers
		pstorage_block_identifier_get(&handle, 0, &block_0_handle);
		pstorage_block_identifier_get(&handle, 1, &block_1_handle);
		pstorage_block_identifier_get(&handle, 2, &block_2_handle);
		
		//Clear 100 bytes, i.e. one block
		APP_LOG("pstorage clear 48 bytes from block 0 \r\n");
		retval = pstorage_clear(&block_0_handle, 48);                       
//		if(retval != NRF_SUCCESS)  { bsp_indication_set(BSP_INDICATE_RCV_ERROR); }
		
		
		//Store data to three blocks. Wait for the last store operation to finish before reading out the data.
		APP_LOG("pstorage store into block 0 \r\n");
		pstorage_store(&block_0_handle, source_data_0, 16, 0);     //Write to flash, only one block is allowed for each pstorage_store command
//		if(retval != NRF_SUCCESS)
//		{
//				bsp_indication_set(BSP_INDICATE_RCV_ERROR);
//		}
		
		APP_LOG("pstorage store into block 1 \r\n");
		pstorage_store(&block_1_handle, source_data_1, 16, 0);     //Write to flash, only one block is allowed for each pstorage_store command		
		pstorage_wait_handle = block_2_handle.block_id;            //Specify which pstorage handle to wait for
		pstorage_wait_flag = 1;                                    //Set the wait flag. Cleared in the example_cb_handler
		APP_LOG("pstorage store into block 2\r\n    Data written into block 2: %s\r\n", source_data_9);
		pstorage_store(&block_2_handle, source_data_9, 16, 0);     //Write to flash
		printf("pstorage wait for block 2 store to complete ... \r\n");
		while(pstorage_wait_flag) { power_manage(); }              //Sleep until store operation is finished.
		
		printf("pstorage load blocks 0 \r\n");
//		pstorage_load(dest_data_0, &block_0_handle, 16, 0);				 //Read from flash, only one block is allowed for each pstorage_load command
//		printf("pstorage load blocks 1 \r\n");
//		pstorage_load(dest_data_1, &block_1_handle, 16, 0);				 //Read from flash
//		printf("pstorage load blocks 2\r\n");
//		pstorage_load(dest_data_2, &block_2_handle, 16, 0);			   //Read from flash
//		printf("    Data loaded from block 2: %s\r\n", dest_data_2);
		
//		printf("pstorage clear 32 bytes from block 0 \r\n");
//		pstorage_wait_handle = block_0_handle.block_id;            //Specify which pstorage handle to wait for
//		pstorage_wait_flag = 1;                                    //Set the wait flag. Cleared in the example_cb_handler
//		pstorage_clear(&block_0_handle, 32);                       //Clear 32 bytes
//		printf("pstorage wait for clear to complete ... \r\n");
//		while(pstorage_wait_flag) { power_manage(); }              //Sleep until store operation is finished.
		
//		printf("pstorage load blocks 0 \r\n");
//		pstorage_load(dest_data_0, &block_0_handle, 16, 0);				 //Read from flash, only one block is allowed for each pstorage_load command
//		printf("pstorage load blocks 1 \r\n");
//		pstorage_load(dest_data_1, &block_1_handle, 16, 0);				 //Read from flash
//		printf("pstorage load blocks 2 \r\n");
//		pstorage_load(dest_data_2, &block_2_handle, 16, 0);			   //Read from flash
		
//		printf("pstorage update block 0 \r\n");
//		pstorage_wait_handle = block_0_handle.block_id;            //Specify which pstorage handle to wait for 
//		pstorage_wait_flag = 1;                                    //Set the wait flag. Cleared in the example_cb_handler
//		pstorage_update(&block_0_handle, source_data_9, 16, 0);    //update flash block 0
//		printf("pstorage wait for update to complete ... \r\n");
//		while(pstorage_wait_flag) { power_manage(); }              //Sleep until update operation is finished.
		
//		printf("pstorage load blocks 0  \r\n");
//		pstorage_load(dest_data_0, &block_0_handle, 16, 0);				 //Read from flash, only one block is allowed for each pstorage_load command
//		printf("pstorage load blocks 1 \r\n");
//		pstorage_load(dest_data_1, &block_1_handle, 16, 0);				 //Read from flash
//		printf("pstorage load blocks 2 \r\n");
//		pstorage_load(dest_data_2, &block_2_handle, 16, 0);			   //Read from flash
}

/**@brief Function for application main entry.
 */
int main(void)
{
    // Initialize.
    timers_init();
		uart_config();
    ble_stack_init();
	
		pstorage_test_store_and_update();

    // Enter main loop.
    for (;;)
    {
        power_manage();
    }
}

/**
 * @}
 */
