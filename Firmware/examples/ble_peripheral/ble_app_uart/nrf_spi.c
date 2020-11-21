
#include <nrf.h>
#include "nrf_spi.h"

//***********************************************************************************


//***********************************************************************************
// Function prototypes
void spiWriteByte(uint8_t write);
void spiReadByte(uint8_t *read, uint8_t write);


void spiInit(uint8_t spi_channel)
{
	if(spi_channel == 0)
	{
    //Configure GPIO
		nrf_gpio_pin_set(SPIM0_SS_PIN);
		nrf_gpio_cfg_output(SPIM0_SS_PIN);
		nrf_gpio_pin_set(SPIM0_SS_PIN);
	
    nrf_gpio_cfg_output(SPIM0_SCK_PIN);
    nrf_gpio_cfg_output(SPIM0_MOSI_PIN);
    nrf_gpio_cfg_input(SPIM0_MISO_PIN, NRF_GPIO_PIN_NOPULL);
		//mapping
		NRF_SPI0->PSELSCK = SPIM0_SCK_PIN;
		NRF_SPI0->PSELMOSI = SPIM0_MOSI_PIN;
		NRF_SPI0->PSELMISO = SPIM0_MISO_PIN;
		NRF_SPI0->FREQUENCY = SPI_FREQUENCY_FREQUENCY_M8;
		NRF_SPI0->CONFIG = (SPI_CONFIG_ORDER_MsbFirst << SPI_CONFIG_ORDER_Pos)//MSB first
											| (SPI_CONFIG_CPHA_Leading << SPI_CONFIG_CPHA_Pos)
											| (SPI_CONFIG_CPOL_ActiveHigh << SPI_CONFIG_CPOL_Pos);
		/* Clear waiting interrupts and events */
		NRF_SPI0->EVENTS_READY = 0;
		NRF_SPI0->INTENSET = 0; //disable interrupt
		/* Enable SPI hardware */
		NRF_SPI0->ENABLE = (SPI_ENABLE_ENABLE_Enabled << SPI_ENABLE_ENABLE_Pos);
	}
	else if(spi_channel == 1)
	{
		//Configure GPIO
		nrf_gpio_pin_set(SPIM1_SS_PIN);
		nrf_gpio_cfg_output(SPIM1_SS_PIN);
		nrf_gpio_pin_set(SPIM1_SS_PIN);
	
    nrf_gpio_cfg_output(SPIM1_SCK_PIN);
    nrf_gpio_cfg_output(SPIM1_MOSI_PIN);
    nrf_gpio_cfg_input(SPIM1_MISO_PIN, NRF_GPIO_PIN_NOPULL);
		//mapping
		NRF_SPI1->PSELSCK = SPIM1_SCK_PIN;
		NRF_SPI1->PSELMOSI = SPIM1_MOSI_PIN;
		NRF_SPI1->PSELMISO = SPIM1_MISO_PIN;
		NRF_SPI1->FREQUENCY = SPI_FREQUENCY_FREQUENCY_M8;
		NRF_SPI1->CONFIG = (SPI_CONFIG_ORDER_MsbFirst << SPI_CONFIG_ORDER_Pos)//MSB first
											| (SPI_CONFIG_CPHA_Leading << SPI_CONFIG_CPHA_Pos)
											| (SPI_CONFIG_CPOL_ActiveHigh << SPI_CONFIG_CPOL_Pos);
		/* Clear waiting interrupts and events */
		NRF_SPI1->EVENTS_READY = 0;
		NRF_SPI1->INTENSET = 0; //disable interrupt
		/* Enable SPI hardware */
		NRF_SPI1->ENABLE = (SPI_ENABLE_ENABLE_Enabled << SPI_ENABLE_ENABLE_Pos);
	}
}

void spiDeInit(void)
{

}



uint8_t halSpiWriteByte(uint8_t spi_channel,uint8_t data)
{
	uint8_t rc;
	uint32_t timeout = 1000;
	if(spi_channel == 0)
	{
    while (NRF_SPI0->EVENTS_READY && timeout--);
		timeout = 1000;
		NRF_SPI0->EVENTS_READY = 0;
		NRF_SPI0->TXD = data;
    while (!NRF_SPI0->EVENTS_READY && timeout--);
		NRF_SPI0->EVENTS_READY = 0;
    rc = NRF_SPI0->RXD;
	}
	else if(spi_channel == 1)
	{
		while (NRF_SPI1->EVENTS_READY && timeout--);
		timeout = 1000;
		NRF_SPI1->EVENTS_READY = 0;
		NRF_SPI1->TXD = data;
    while (!NRF_SPI1->EVENTS_READY && timeout--);
		NRF_SPI1->EVENTS_READY = 0;
    rc = NRF_SPI1->RXD;
	}	
	return rc;
}


