
#include "uart_config_task.h"
#include "ringbuf.h"
#include "app_uart.h"
#include "nrf_log.h"

void CfgUARTSendData(CFG_PROTOCOL_TYPE *data);


CFG_PROTOCOL_TYPE UART_ProtoRecv;
CFG_PROTOCOL_TYPE UART_ProtoSend;
uint8_t UART_DataBuff[UART_PACKET_SIZE];
PARSER_PACKET_TYPE UART_parserPacket;
Timeout_Type checkUart;
uint32_t uart1DataHead;


void UartConfigTaskInit(void)
{
	AppConfigTaskInit();
	InitTimeout(&checkUart,SYSTICK_TIME_MS(10));
	UART_ProtoRecv.dataPt = UART_DataBuff;
	UART_parserPacket.state = CFG_CMD_WAITING_SATRT_CODE;
	UART_parserPacket.lenMax = UART_PACKET_SIZE;	
}


void UartConfigTask(void)
{
	uint8_t c;		
	if(app_uart_get(&c) == NRF_SUCCESS)
	{
		NRF_LOG_PRINTF("%02x",c);
		if(CfgParserPacket(&UART_parserPacket,&UART_ProtoRecv,c) == 0)
		{
			CfgProcessData(&UART_ProtoRecv,&UART_ProtoSend,UART_DataBuff,UART_parserPacket.lenMax - 4,1);
			CfgUARTSendData(&UART_ProtoSend);
		}
	}
}


void CfgUARTSendData(CFG_PROTOCOL_TYPE *data)
{
	uint16_t i;
	if(data->length)
	{
		app_uart_put(*(uint8_t *)&data->start);
		app_uart_put(((uint8_t *)&data->length)[0]);
		app_uart_put(((uint8_t *)&data->length)[1]);
		app_uart_put(*(uint8_t *)&data->opcode);
		for(i = 0;i < data->length;i++)
			app_uart_put(*(uint8_t *)&data->dataPt[i]);
		app_uart_put(*(uint8_t *)&data->crc);
		data->length = 0;
	}
}

