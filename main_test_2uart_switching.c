/* Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
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

/** @file
 * @defgroup uart_example_main main.c
 * @{
 * @ingroup uart_example
 * @brief UART Example Application main file.
 *
 * This file contains the source code for a sample application using UART.
 * 
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "app_uart.h"
#include "app_error.h"
#include "nrf_delay.h"
#include "nrf.h"
#include "bsp.h"

//#define ENABLE_LOOPBACK_TEST  /**< if defined, then this example will be a loopback test, which means that TX should be connected to RX to get data loopback. */

#define MAX_TEST_DATA_BYTES     (15U)                	/**< max number of test bytes to be used for tx and rx. */
#define UART_TX_BUF_SIZE 256                         	/**< UART TX buffer size. */
#define UART_RX_BUF_SIZE 256													/**< UART RX buffer size. */

#define UART_1_RX_PIN			6		
#define UART_1_TX_PIN			7

#define UART_2_RX_PIN			8	
#define UART_2_TX_PIN			9	


app_uart_buffers_t buffers;                 
static uint8_t     rx_buf[UART_RX_BUF_SIZE];
static uint8_t     tx_buf[UART_TX_BUF_SIZE];


void uart_error_handle(app_uart_evt_t * p_event)
{
    if (p_event->evt_type == APP_UART_COMMUNICATION_ERROR)
    {
        APP_ERROR_HANDLER(p_event->data.error_communication);
    }
    else if (p_event->evt_type == APP_UART_FIFO_ERROR)
    {
        APP_ERROR_HANDLER(p_event->data.error_code);
    }
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
    //static uint8_t data_array[BLE_NUS_MAX_DATA_LEN];
    //static uint8_t index = 0;
		uint8_t data;
    //uint32_t       err_code;

    switch (p_event->evt_type)
    {
        case APP_UART_DATA_READY:
            UNUSED_VARIABLE(app_uart_get(&data));
						UNUSED_VARIABLE(app_uart_put(data));
            //index++;
            break;

        case APP_UART_COMMUNICATION_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_communication);
            break;

        case APP_UART_FIFO_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_code);
            break;

        default:
            break;
    }
}


#ifdef ENABLE_LOOPBACK_TEST
/** @brief Function for setting the @ref ERROR_PIN high, and then enter an infinite loop.
 */
static void show_error(void)
{
    
    LEDS_ON(LEDS_MASK);
    while(true)
    {
        // Do nothing.
    }
}


/** @brief Function for testing UART loop back. 
 *  @details Transmitts one character at a time to check if the data received from the loopback is same as the transmitted data.
 *  @note  @ref TX_PIN_NUMBER must be connected to @ref RX_PIN_NUMBER)
 */
static void uart_loopback_test()
{
    uint8_t * tx_data = (uint8_t *)("\n\rLOOPBACK_TEST\n\r");
    uint8_t   rx_data;

    // Start sending one byte and see if you get the same
    for (uint32_t i = 0; i < MAX_TEST_DATA_BYTES; i++)
    {
        uint32_t err_code;
        while(app_uart_put(tx_data[i]) != NRF_SUCCESS);

        nrf_delay_ms(10);
        err_code = app_uart_get(&rx_data);

        if ((rx_data != tx_data[i]) || (err_code != NRF_SUCCESS))
        {
            show_error();
        }
    }
    return;
}

#endif


/**
 * @brief Function for main application entry.
 */
int main(void)
{
    LEDS_CONFIGURE(LEDS_MASK);
    LEDS_OFF(LEDS_MASK);
    uint32_t err_code;
		uint8_t uart_channel = 1;
		app_uart_comm_params_t h_comm_params;
    const app_uart_comm_params_t comm_params =
      {
          UART_1_RX_PIN,
          UART_1_TX_PIN,
          RTS_PIN_NUMBER,
          CTS_PIN_NUMBER,
          APP_UART_FLOW_CONTROL_DISABLED,
          false,
          UART_BAUDRATE_BAUDRATE_Baud115200
      };

		memcpy(&h_comm_params,&comm_params,sizeof(app_uart_comm_params_t));
			
			
		buffers.rx_buf      = rx_buf;                                                              
		buffers.rx_buf_size = sizeof (rx_buf);                                                     
		buffers.tx_buf      = tx_buf;                                                              
		buffers.tx_buf_size = sizeof (tx_buf);                                                     
		err_code = app_uart_init(&h_comm_params, &buffers, uart_event_handle, APP_IRQ_PRIORITY_LOW); 			

    APP_ERROR_CHECK(err_code);

		nrf_gpio_cfg_output(14);
		nrf_gpio_pin_set(14);
#ifndef ENABLE_LOOPBACK_TEST
    printf("\n\rUART 1 Start: \n\r");

    while (true)
    {
        //uint8_t cr;
        //while(app_uart_get(&cr) != NRF_SUCCESS);
        //while(app_uart_put(cr) != NRF_SUCCESS);       
			nrf_delay_ms(1000);
			nrf_gpio_pin_toggle(14);
			if(uart_channel == 1)
			{
				uart_channel = 2;
				app_uart_close();
				h_comm_params.rx_pin_no = UART_2_RX_PIN;
				h_comm_params.tx_pin_no = UART_2_TX_PIN;
				h_comm_params.baud_rate = UART_BAUDRATE_BAUDRATE_Baud9600;
				err_code = app_uart_init(&h_comm_params, &buffers, uart_event_handle, APP_IRQ_PRIORITY_LOW); 
				printf("\n\rUART 2 Start: \n\r");
			}
			else
			{
				uart_channel = 1;
				app_uart_close();
				h_comm_params.rx_pin_no = UART_1_RX_PIN;
				h_comm_params.tx_pin_no = UART_1_TX_PIN;
				h_comm_params.baud_rate = UART_BAUDRATE_BAUDRATE_Baud115200;
				err_code = app_uart_init(&h_comm_params, &buffers, uart_event_handle, APP_IRQ_PRIORITY_LOW);
				printf("\n\rUART 1 Start: \n\r");
			}			
    }
#else

    // This part of the example is just for testing the loopback .
    while (true)
    {
        uart_loopback_test();
    }
#endif
}


/** @} */
