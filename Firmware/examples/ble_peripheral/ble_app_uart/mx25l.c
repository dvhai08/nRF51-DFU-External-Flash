
#include "nrf_spi.h"
#include "mx25l.h"

#define SST_CS_Assert()				nrf_gpio_pin_clear(SPIM0_SS_PIN)
#define SST_CS_DeAssert()			nrf_gpio_pin_set(SPIM0_SS_PIN)

#define CMD_READ25						0x03
#define CMD_READ80						0x0B
#define CMD_ERASE_4K					0x20
#define CMD_ERASE_32K					0x52
#define CMD_ERASE_64K					0xD8
#define CMD_ERASE_ALL					0x60
#define CMD_WRITE_BYTE				0x02
#define CMD_WRITE_WORD_AAI		0xAD
#define CMD_READ_STATUS				0x05
#define CMD_WRITE_STATUS_ENABLE	0x50
#define CMD_WRITE_STATUS			0x01
#define CMD_WRITE_ENABLE			0x06
#define CMD_WRITE_DISABLE			0x04
#define CMD_READ_ID						0x90
#define CMD_READ_JEDEC_ID			0x9F
#define CMD_EBSY							0x70
#define CMD_DBSY							0x80
#define CMD_DP								0xB9
#define CMD_RDP								0xAB
	
#define SPI_Write(x)		halSpiWriteByte(0,x)
static uint8_t SST25__Status(void);
static uint8_t SST25__WriteEnable(void);
//static uint8_t SST25__WriteDisable(void);
//static uint8_t SST25__GlobalProtect(uint8_t enable);


uint8_t MX25L_Init(void)
{
	spiInit(0);	
	//MX25L__DeepPowerDown(0);	
	return 0;
}

uint8_t MX25L_DeInit(void)
{
	spiDeInit();
	return 0;
}

uint8_t SST25_Erase(uint32_t addr, enum SST25_ERASE_MODE mode)
{
	SST25__WriteEnable();

	SST_CS_Assert();
	switch(mode){
	case all:
		SPI_Write(CMD_ERASE_ALL);
		SST_CS_DeAssert();
		return 0;

	case block4k:
		SPI_Write(CMD_ERASE_4K);
		break;

	case block32k:
		SPI_Write(CMD_ERASE_32K);
		break;

	case block64k:
		SPI_Write(CMD_ERASE_64K);
		break;
	}

	SPI_Write(addr >> 16);
	SPI_Write(addr >> 8);
	SPI_Write(addr);

	SST_CS_DeAssert();

	return 0;
}



uint32_t countErrTemp = 0;
uint8_t _SST25_Write(uint32_t addr, const uint8_t *buf, uint32_t count)
{
	uint32_t  i,timeout = 1000000;
	SST25__WriteEnable();	
	SST_CS_Assert();
	SPI_Write(CMD_WRITE_BYTE);
	SPI_Write(addr >> 16);
	SPI_Write(addr >> 8);
	SPI_Write(addr);
	while(count)
	{
		SPI_Write(*buf++);
		
		count--;
	}
	SST_CS_DeAssert();
	while(timeout)
	{
		timeout--;
		i = SST25__Status();
		if((i & 0x03) == 0) break; //BUSY and WEL bit
	}
	if(timeout == 0)
	{
		countErrTemp++;
	}
	return 0;
}


uint8_t SST25_Write(uint32_t addr, const uint8_t *buf, uint32_t count)
{
	uint32_t count1 = 0;
	if((addr & 0x00000FFF) == 0)
		SST25_Erase(addr,block4k);
	
	if(((addr & 0x00000FFF) + count) > 4096)
	{
		count1 = (addr  + count) & 0x00000FFF;
		count -= count1;
	}
	_SST25_Write(addr,buf,count);
	if(count1)
	{
		SST25_Erase(addr + count,block4k);
		_SST25_Write(addr + count,buf + count,count1);
	}
	return 0;
}


uint8_t SST25_Read(uint32_t addr, uint8_t *buf, uint32_t count)
{
	uint32_t timeout = 1000000;
	while((SST25__Status() & 0x01) && timeout)
	{
		timeout--;
	}

	SST_CS_Assert();
	SPI_Write(CMD_READ80);
	SPI_Write(addr >> 16);
	SPI_Write(addr >> 8);
	SPI_Write(addr);
	SPI_Write(0); // write a dummy byte
	while(count--)
		*buf++ = SPI_Write(0);

	SST_CS_DeAssert();

	return 0;
}

static uint8_t SST25__Status(void)
{
	uint8_t res;
	SST_CS_Assert(); // assert CS
	SPI_Write(0x05); // READ status command
	res = SPI_Write(0xFF); // Read back status register content
	SST_CS_DeAssert(); // de-assert cs
	return res;
}

static uint8_t SST25__WriteEnable(void)
{
	uint8_t status;
	uint32_t timeout = 1000000;
	while((SST25__Status() & 0x01) && timeout)
	{
		timeout--;
	}

	SST_CS_Assert();
	SPI_Write(CMD_WRITE_ENABLE);
	SST_CS_DeAssert(); // deassert CS to excute command

	// loop until BUSY is cleared and WEL is set
	do{
		status = SST25__Status();
		timeout--;
	}while(((status & 0x01) || !(status & 0x02)) && timeout);

	return 0;
}

//static uint8_t SST25__WriteDisable(void)
//{
////	uint8_t status;

//// 	while(SST25__Status()&0x01);

//// 	SST_CS_Assert();
//// 	SPI_Write(CMD_WRITE_DISABLE);
//// 	SST_CS_DeAssert(); // deassert CS to excute command

//// 	// loop until BUSY is cleared and WEL is set
//// 	do{
//// 		status = SST25__Status();
//// 	}while((status & 0x01) || (status & 0x02));

//	return 0;
//}

uint8_t MX25L__DeepPowerDown(uint8_t enable)
{
	uint32_t timeout = 1000000;
	while((SST25__Status() & 0x01) && timeout)
	{
		timeout--;
	}
	SST_CS_Assert();
	if(enable)SPI_Write(CMD_DP);
	else SPI_Write(CMD_RDP);
	SST_CS_DeAssert();

	return 0;
}





