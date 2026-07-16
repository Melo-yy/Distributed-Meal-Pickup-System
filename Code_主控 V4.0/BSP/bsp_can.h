#ifndef __BSP_CAN_H
#define __BSP_CAN_H

#include "stm32f4xx.h" 
#include "bsp_usart.h"
#include "bsp_delay.h"
#include "app_can_common.h"
#include "debug_config.h"

/**************************** CAN 配置宏定义 ****************************/

/* CAN 外设选择 */
#define CANx                       CAN1
#define CAN_CLK                    RCC_APB1Periph_CAN1

/* GPIO 配置 */
#define CAN_GPIO_PORT              GPIOA  
#define CAN_GPIO_CLK               RCC_AHB1Periph_GPIOA  
#define CAN_RX_PIN                 GPIO_Pin_11  
#define CAN_TX_PIN                 GPIO_Pin_12  
#define CAN_RX_PIN_SOURCE          GPIO_PinSource11  
#define CAN_TX_PIN_SOURCE          GPIO_PinSource12  

#define CAN_GPIO_AF                GPIO_AF_CAN1

/************************************************************************/


void 						bsp_can_Init(void);
can_status_t 		bsp_can_Transmit(uint32_t ID, uint8_t Length, uint8_t *Data);
uint8_t 				bsp_can_ReceiveFlag(void);
can_status_t  	bsp_can_Receive(uint32_t *ID, uint8_t *Length, uint8_t *Data);
void 						bsp_can_Filter_Config(uint32_t filter_id, uint32_t filter_mask);
uint32_t 				bsp_can_ReconstructID(uint8_t *id_bytes, uint8_t is_extended);
void 						bsp_can_ExtractIDBytes(uint32_t recv_id, uint8_t is_extended, uint8_t *id_bytes);
#endif
