#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "ble_hci.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "softdevice_handler.h"
#include "app_timer.h"
#include "app_button.h"
#include "ble_nus.h"
#include "app_uart.h"
#include "app_util_platform.h"
#include "bsp.h"
#include "bsp_btn_ble.h"
#include "mx25l.h"
#include "uart_config_task.h"
#include "nrf_drv_uart.h"
#include "system_config.h"
#include "sys_tick.h"

#define UART1_RX_PIN_NUMBER  15
#define UART1_TX_PIN_NUMBER  16

#define IS_SRVC_CHANGED_CHARACT_PRESENT 0                                           /**< Include the service_changed characteristic. If not enabled, the server's database cannot be changed for the lifetime of the device. */

#define CENTRAL_LINK_COUNT              0                                           /**< Number of central links used by the application. When changing this number remember to adjust the RAM settings*/
#define PERIPHERAL_LINK_COUNT           1                                           /**< Number of peripheral links used by the application. When changing this number remember to adjust the RAM settings*/

#define DEVICE_NAME                     "dfutest"                               /**< Name of device. Will be included in the advertising data. */
#define NUS_SERVICE_UUID_TYPE           BLE_UUID_TYPE_VENDOR_BEGIN                  /**< UUID type for the Nordic UART Service (vendor specific). */

#define APP_ADV_INTERVAL                64                                          /**< The advertising interval (in units of 0.625 ms. This value corresponds to 40 ms). */
#define APP_ADV_TIMEOUT_IN_SECONDS      180                                         /**< The advertising timeout (in units of seconds). */

#define APP_TIMER_PRESCALER             0                                           /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_OP_QUEUE_SIZE         4                                           /**< Size of timer operation queues. */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(20, UNIT_1_25_MS)             /**< Minimum acceptable connection interval (20 ms), Connection interval uses 1.25 ms units. */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(75, UNIT_1_25_MS)             /**< Maximum acceptable connection interval (75 ms), Connection interval uses 1.25 ms units. */
#define SLAVE_LATENCY                   0                                           /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)             /**< Connection supervisory timeout (4 seconds), Supervision Timeout uses 10 ms units. */
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER)  /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER) /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define UART_TX_BUF_SIZE                1024                                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE                1024                                         /**< UART RX buffer size. */

#define BLINKY_LED_INTERVAL     APP_TIMER_TICKS(100, APP_TIMER_PRESCALER)
#define SYSTEM_TICK_INTERVAL    APP_TIMER_TICKS(50, APP_TIMER_PRESCALER)
#define RTC_INTERVAL    				APP_TIMER_TICKS(1000, APP_TIMER_PRESCALER)

APP_TIMER_DEF(m_blinky_timer_id);
APP_TIMER_DEF(m_system_tick_timer_id);
APP_TIMER_DEF(m_rtc_timer_id);

app_uart_buffers_t uart_buffers;   
app_uart_comm_params_t h_comm_params;
static uint8_t     rx_buf[UART_RX_BUF_SIZE];
static uint8_t     tx_buf[UART_TX_BUF_SIZE];

static void uart_reinit(void);
static void uart_init(void);
/**@brief Function for assert macro callback.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyse 
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num    Line number of the failing ASSERT call.
 * @param[in] p_file_name File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}


/**@brief   Function for handling app_uart events.
 *
 * @details This function will receive a single character from the app_uart module and append it to 
 *          a string. The string will be be sent over BLE when the last character received was a 
 *          'new line' i.e '\n' (hex 0x0D) or if the string has reached a length of 
 *          @ref NUS_MAX_DATA_LENGTH.
 */
/**@snippet [Handling the data received over UART] */
void uart_event_handle(app_uart_evt_t * p_event)
{
    switch (p_event->evt_type)
    {
        case APP_UART_DATA_READY:
            break;
				case APP_UART_COMMUNICATION_ERROR:
						nrf_uart_event_clear(NRF_UART0, NRF_UART_EVENT_TXDRDY);
						nrf_uart_event_clear(NRF_UART0, NRF_UART_EVENT_RXTO);
						nrf_uart_event_clear(NRF_UART0, NRF_UART_EVENT_ERROR);  /* thanks for correction Lalit*/
						nrf_drv_uart_uninit();
						uart_reinit();
            break;

        default:
            break;
    }
}
/**@snippet [Handling the data received over UART] */


/**@brief  Function for initializing the UART module.
 */
/**@snippet [UART Initialization] */
static void uart_init(void)
{
    uint32_t                     err_code;
    const app_uart_comm_params_t comm_params =
    {
        UART1_RX_PIN_NUMBER,
        UART1_TX_PIN_NUMBER,
        RTS_PIN_NUMBER,
        CTS_PIN_NUMBER,
        APP_UART_FLOW_CONTROL_DISABLED,
        false,
        UART_BAUDRATE_BAUDRATE_Baud115200
    };   

		memcpy(&h_comm_params,&comm_params,sizeof(app_uart_comm_params_t));
			
			
		uart_buffers.rx_buf      = rx_buf;                                                              
		uart_buffers.rx_buf_size = sizeof (rx_buf);                                                     
		uart_buffers.tx_buf      = tx_buf;                                                              
		uart_buffers.tx_buf_size = sizeof (tx_buf);                                                     
		err_code = app_uart_init(&h_comm_params, &uart_buffers, uart_event_handle, APP_IRQ_PRIORITY_LOW); 
		
		APP_ERROR_CHECK(err_code);
}

/**@brief  Function for initializing the UART module.
 */
/**@snippet [UART Initialization] */
static void uart_reinit(void)
{
	uint32_t                     err_code;
	err_code = app_uart_init(&h_comm_params, &uart_buffers, uart_event_handle, APP_IRQ_PRIORITY_LOW); 
	APP_ERROR_CHECK(err_code);
}


/**@brief Function for placing the application in low power state while waiting for events.
 */
static void power_manage(void)
{
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling the Battery measurement timer timeout.
 *
 * @details This function will be called each time the battery level measurement timer expires.
 *
 * @param[in] p_context   Pointer used for passing some arbitrary information (context) from the
 *                        app_start_timer() call to the timeout handler.
 */
static void blinky_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);    
		nrf_gpio_pin_toggle(5);
}

/**@brief Function for handling the Battery measurement timer timeout.
 *
 * @details This function will be called each time the battery level measurement timer expires.
 *
 * @param[in] p_context   Pointer used for passing some arbitrary information (context) from the
 *                        app_start_timer() call to the timeout handler.
 */
static void system_tick_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    SysTick_Task(50);
		nrf_gpio_pin_toggle(6);
}

/**@brief Function for handling the Battery measurement timer timeout.
 *
 * @details This function will be called each time the battery level measurement timer expires.
 *
 * @param[in] p_context   Pointer used for passing some arbitrary information (context) from the
 *                        app_start_timer() call to the timeout handler.
 */
static void rtc_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);    
		nrf_gpio_pin_toggle(1);
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
static void timers_init(void)
{
    uint32_t err_code;

    // Initialize timer module.
    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, false);

    // Create timers.
    err_code = app_timer_create(&m_blinky_timer_id,APP_TIMER_MODE_REPEATED,blinky_timeout_handler);
    APP_ERROR_CHECK(err_code);
	
		err_code = app_timer_create(&m_system_tick_timer_id,APP_TIMER_MODE_REPEATED,system_tick_timeout_handler);
		APP_ERROR_CHECK(err_code);
	
		err_code = app_timer_create(&m_rtc_timer_id,APP_TIMER_MODE_REPEATED,rtc_timeout_handler);
		APP_ERROR_CHECK(err_code);
}

/**@brief Function for starting application timers.
 */
static void application_timers_start(void)
{
    uint32_t err_code;

    // Start application timers.
    err_code = app_timer_start(m_blinky_timer_id, BLINKY_LED_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
	
		err_code = app_timer_start(m_system_tick_timer_id, SYSTEM_TICK_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
	
		err_code = app_timer_start(m_rtc_timer_id, RTC_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
}

/**@brief Application main function.
 */
int main(void)
{
		nrf_gpio_cfg_output(1);
		nrf_gpio_cfg_output(5);
		nrf_gpio_cfg_output(6);
		UartConfigTaskInit();
    // Initialize.
    timers_init();
    uart_init();
		MX25L_Init();    
		application_timers_start();
		
    // Enter main loop.
    for (;;)
    {
			UartConfigTask();
			ResetMcuTask();
    }
}


/** 
 * @}
 */
