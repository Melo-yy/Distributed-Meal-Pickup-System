#ifndef __BSP_VOICE_H
#define __BSP_VOICE_H

#include "bsp_usart.h"

void bsp_voice_init(void);
void bsp_voice_send(uint8_t *Array, uint16_t Length);
void bsp_voice_send_byte(uint8_t msg_id, uint8_t send_data);
void bsp_voice_send_cmd(uint8_t msg_id);

#endif


