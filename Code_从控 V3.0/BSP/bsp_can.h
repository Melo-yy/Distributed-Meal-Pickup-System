#ifndef __BSP_CAN_H
#define __BSP_CAN_H

#include "main.h" 
#include "app_can_common.h"

/**************************** CAN 配置宏定义 ****************************/

/* CAN 外设选择 */
#define CANx                       CAN1
#define CAN_CLK                    RCC_APB1Periph_CAN1

/* GPIO 配置 */
#define CAN_GPIO_PORT              GPIOA  
#define CAN_GPIO_CLK               RCC_APB2Periph_GPIOA  
#define CAN_RX_PIN                 GPIO_Pin_11  
#define CAN_TX_PIN                 GPIO_Pin_12  

/************************************************************************/

/********************************************函数声明********************************************/

void 						bsp_can_Init(void);
can_status_t 		bsp_can_Transmit(uint32_t ID, uint8_t Length, uint8_t *Data);
uint8_t 				bsp_can_ReceiveFlag(void);
can_status_t  	bsp_can_Receive(uint32_t *ID, uint8_t *Length, uint8_t *Data);
void 						bsp_can_Filter_Config(uint32_t filter_id, uint32_t filter_mask);

#endif
