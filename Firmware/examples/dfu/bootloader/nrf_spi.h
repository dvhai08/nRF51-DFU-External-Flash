
#ifndef __NRF_SPI_H__
#define __NRF_SPI_H__
#include <stdint.h>
#include "boards.h"
#include "nrf_gpio.h"
#include "nrf_soc.h"
#include "nrf51_bitfields.h"
void spiInit(uint8_t spi_channel);
void spiDeInit(void);
uint8_t halSpiWriteByte(uint8_t spi_channel,uint8_t data);


#endif

