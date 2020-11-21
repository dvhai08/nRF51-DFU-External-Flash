
#ifndef __DFU_EXT_FLASH_H__
#define __DFU_EXT_FLASH_H__
#include <stdint.h>

typedef enum
{
    DFU_UPDATE_UPDATING,
    DFU_UPDATE_APP_ERASE,
    DFU_UPDATE_APP_UPDATE,
    DFU_UPDATE_FINISH,
    DFU_UPDATE_IDLE,
} h_update_status_t;

typedef enum
{
    PSTORAGE_IDLE,
    PSTORAGE_BUSY,
} h_pstorage_status_t;

uint32_t dfu_init_remit(void);
void dfu_prepare_func_app_erase_remit(uint32_t image_size);
uint32_t dfu_data_pkt_handle_remit(uint32_t * p_data_packet,uint32_t packet_length);
uint8_t dfu_wait_app_cleared(void);


extern h_update_status_t h_dfu_update_status;
extern h_update_status_t h_dfu_update_status_store;
extern h_pstorage_status_t h_pstorage_status;
#endif //__DFU_EXT_FLASH_H__

