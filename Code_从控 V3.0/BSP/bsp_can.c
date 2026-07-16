#include "bsp_can.h"                  // Device header

/**************************************************************************
Function: CAN controller initialization
Input   : none
Output  : none
函数功能：CAN控制器初始化
入口参数：无
返回  值：无
**************************************************************************/
void bsp_can_Init(void)
{
   GPIO_InitTypeDef GPIO_InitStructure;
    CAN_InitTypeDef CAN_InitStructure;
    CAN_FilterInitTypeDef CAN_FilterInitStructure;

    /* 使能时钟 ------------------------------------------------*/
    RCC_APB2PeriphClockCmd(CAN_GPIO_CLK, ENABLE);  // 开启GPIOA时钟
    RCC_APB1PeriphClockCmd(CAN_CLK, ENABLE);       // 开启CAN1时钟

    /* 配置GPIOA11、A12为CAN RX/TX ----------------------------*/
    GPIO_InitStructure.GPIO_Pin = CAN_RX_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;     // RX 上拉输入
    GPIO_Init(CAN_GPIO_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = CAN_TX_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;   // TX 复用推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(CAN_GPIO_PORT, &GPIO_InitStructure);

    /* CAN 初始化 --------------------------------------------*/
    CAN_DeInit(CANx);
    CAN_StructInit(&CAN_InitStructure);

    CAN_InitStructure.CAN_TTCM = DISABLE;     // 时间触发通信模式
    CAN_InitStructure.CAN_ABOM = ENABLE;      // 自动离线管理
    CAN_InitStructure.CAN_AWUM = DISABLE;     // 自动唤醒
    CAN_InitStructure.CAN_NART = DISABLE;     // 自动重传
    CAN_InitStructure.CAN_RFLM = DISABLE;     // FIFO 锁定
    CAN_InitStructure.CAN_TXFP = DISABLE;     // 优先级按标识符
    CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;  // 正常模式

		CAN_InitStructure.CAN_Prescaler = 48;		//波特率 = 36M / 48 / (1 + 2 + 3) = 125K
		CAN_InitStructure.CAN_BS1 = CAN_BS1_2tq;
		CAN_InitStructure.CAN_BS2 = CAN_BS2_3tq;
		CAN_InitStructure.CAN_SJW = CAN_SJW_2tq;
		
    CAN_Init(CANx, &CAN_InitStructure);

    /* CAN 过滤器配置 -----------------------------------------*/
    CAN_FilterInitStructure.CAN_FilterNumber = 0;
    CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
    CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
    CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;
    CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;
    CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;
    CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;
    CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_Filter_FIFO0;
    CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
    CAN_FilterInit(&CAN_FilterInitStructure);
}	

/**************************************************************************
Function: Configure CAN filter (exact match mode)
Input   : filter_id - Standard ID to be filtered (e.g. 0x001)
          filter_mask - Mask (0x7FF for exact match)
Output  : none
函数功能：CAN过滤器配置（精确匹配模式）
入口参数：filter_id——要过滤的标准ID
          filter_mask——掩码（0x7FF表示精确匹配）
返回  值：无
**************************************************************************/
void bsp_can_Filter_Config(uint32_t filter_id, uint32_t filter_mask)
{
    CAN_FilterInitTypeDef filter;
    
    // 标准库过滤器配置：标准帧ID必须左移21位
    uint32_t id = filter_id << 21;        // 左移21位到正确位置
    uint32_t mask = filter_mask << 21;    // 掩码同样左移
    
    filter.CAN_FilterNumber = 0;          // 使用过滤器组0
    filter.CAN_FilterMode = CAN_FilterMode_IdMask;
    filter.CAN_FilterScale = CAN_FilterScale_32bit;
    filter.CAN_FilterIdHigh = id >> 16;
    filter.CAN_FilterIdLow = id & 0xFFFF;
    filter.CAN_FilterMaskIdHigh = mask >> 16;
    filter.CAN_FilterMaskIdLow = mask & 0xFFFF;
    filter.CAN_FilterFIFOAssignment = CAN_Filter_FIFO0;
    filter.CAN_FilterActivation = ENABLE;
    
    CAN_FilterInit(&filter);
    
    DBG_INFO("BSP_CAN: Filter configured - ID: 0x%03X, Mask: 0x%03X\r\n", filter_id, filter_mask);
	
}

/**************************************************************************
Function: CAN data transmission with retry and error classification
Input   : ID     - 11-bit standard CAN identifier  
          Length - number of data bytes (0-8)  
          Data   - pointer to data buffer
Output  : can_status_t - result code
函数功能：带重试与错误分类的CAN数据发送
入口参数：ID     - 11位标准CAN标识符
          Length - 数据长度（0-8）
          Data   - 数据缓冲区指针
返回  值：can_status_t 枚举值
          CAN_STATUS_OK         发送成功
          CAN_STATUS_TIMEOUT    等待发送完成超时
          CAN_STATUS_SEND_FAIL  无空闲邮箱或普通发送失败
          CAN_STATUS_HW_ERROR   总线Bus-off等硬件故障
**************************************************************************/
can_status_t bsp_can_Transmit(uint32_t ID, uint8_t Length, uint8_t *Data)
{
    CanTxMsg TxMessage;
	
     // 自动检测使用标准帧还是扩展帧
    if (ID <= 0x7FF) {
        // 标准帧
        TxMessage.StdId = ID;
        TxMessage.ExtId = 0;
        TxMessage.IDE = CAN_Id_Standard;
    } else if (ID <= 0x1FFFFFFF) {
        // 扩展帧
        TxMessage.ExtId = ID;
        TxMessage.StdId = 0;
        TxMessage.IDE = CAN_Id_Extended;
    } else {
        DBG_ERROR("CAN_ERROR: ID 0x%08X exceeds both standard and extended limits\r\n", ID);
        return CAN_STATUS_INVALID_ID;
    }
	
    TxMessage.RTR = CAN_RTR_Data;
    TxMessage.DLC = Length;
    for (uint8_t i = 0; i < Length; i++) {
        TxMessage.Data[i] = Data[i];
    }

    uint8_t retry_count = 0;
    const uint8_t max_retries = 3;
    can_status_t final_status = CAN_STATUS_SEND_FAIL; // 初始化最终状态为发送失败

    while (retry_count < max_retries) {
        // 尝试发送消息
        uint8_t TransmitMailbox = CAN_Transmit(CAN1, &TxMessage);
        
        // 检查是否有空闲邮箱
        if (TransmitMailbox == CAN_NO_MB) {
            DBG_WARN("No free mailbox! Retry %d/%d\r\n", retry_count + 1, max_retries);
            final_status = CAN_STATUS_SEND_FAIL; // 记录本次失败原因
            retry_count++;
            vTaskDelay(pdMS_TO_TICKS(10)); 
            continue;
        }

        // 等待发送完成，使用基于时间的超时（1s）
        uint32_t startTime = getSysTickCnt();
        uint32_t timeoutMs = 1000;
        can_status_t attempt_status = CAN_STATUS_SEND_FAIL; // 本次尝试的状态

        while (CAN_TransmitStatus(CAN1, TransmitMailbox) != CAN_TxStatus_Ok) {
            if (getSysTickCnt() - startTime > timeoutMs) {
                DBG_WARN("Timeout!!! Retry %d/%d\r\n", retry_count + 1, max_retries);
                
                // 读取错误状态寄存器
                uint32_t esr = CAN1->ESR;
                DBG_ERROR("CAN ESR: 0x%08lX\r\n", esr);
                
                // 解析错误状态，并决定本次尝试的失败原因
                if (esr & CAN_ESR_BOFF) {
                    DBG_ERROR("Bus-off state detected\r\n");
                    attempt_status = CAN_STATUS_HW_ERROR; // 记录为硬件错误
                } 
                else if (esr & CAN_ESR_EPVF) {
                    DBG_ERROR("Error passive state detected\r\n");
                    attempt_status = CAN_STATUS_SEND_FAIL; // 仍归类为发送失败
                }
                else if (esr & CAN_ESR_EWGF) {
                    DBG_ERROR("Error warning state detected\r\n");
                    attempt_status = CAN_STATUS_SEND_FAIL; // 仍归类为发送失败
                }
                else {
                    // 普通超时，无特殊错误标志
                    attempt_status = CAN_STATUS_SEND_FAIL;
                }
                break; // 跳出内部等待循环
            }
        }
        
        // 检查发送是否成功
        if (CAN_TransmitStatus(CAN1, TransmitMailbox) == CAN_TxStatus_Ok) {
            DBG_INFO("Transmit successful on attempt %d\r\n", retry_count + 1);
            return CAN_STATUS_OK;
        } else {
            // 本次尝试失败，更新最终状态（HW_ERROR的优先级最高）
            if (attempt_status == CAN_STATUS_HW_ERROR) {
                final_status = CAN_STATUS_HW_ERROR;
            }
            // 如果最终状态还不是HW_ERROR，但本次是SEND_FAIL，则覆盖。
            // 这确保了如果多次重试中有一次是HW_ERROR，最终就返回HW_ERROR。
            else if (final_status != CAN_STATUS_HW_ERROR) {
                final_status = attempt_status;
            }
            DBG_WARN("Attempt %d failed with status: %d\r\n", retry_count + 1, attempt_status);
        }
        
        retry_count++;
        vTaskDelay(pdMS_TO_TICKS(50)); // 重试前延迟
    }
    
    // 循环结束，所有重试均失败
    DBG_ERROR("Transmit failed after %d attempts. Final error: %d\r\n", max_retries, final_status);
    return final_status;
}


/**************************************************************************
Function: CAN receive flag check
Input   : none
Output  : uint8_t - receive flag (1: data available, 0: no data)
函数功能：CAN接收标志检查
入口参数：无
返回  值：uint8_t——接收标志 (1: 有数据, 0: 无数据)
**************************************************************************/
uint8_t bsp_can_ReceiveFlag(void)
{
	return (CAN_MessagePending(CAN1, CAN_FIFO0) > 0) ? 1 : 0;
}


/**************************************************************************
Function: CAN message reception with error handling
Input   : ID     - pointer to store 11-bit standard CAN identifier
          Length - pointer to store received data length (0-8)
          Data   - pointer to store data buffer
Output  : can_status_t - result code
函数功能：带错误处理的CAN报文接收
入口参数：ID     - 用于保存11位标准CAN标识符的指针
          Length - 用于保存数据长度的指针
          Data   - 用于保存数据的缓冲区指针
返回  值：can_status_t 枚举值
          CAN_STATUS_OK         接收成功
          CAN_STATUS_NO_DATA    FIFO无数据
          CAN_STATUS_HW_ERROR   FIFO溢出等硬件故障
          CAN_STATUS_RESP_ERROR 收到远程帧或其他格式错误
**************************************************************************/
can_status_t  bsp_can_Receive(uint32_t *ID, uint8_t *Length, uint8_t *Data)
{
	
	  // 优先检查硬件错误（FIFO溢出）
    if (CAN_GetFlagStatus(CAN1, CAN_FLAG_FOV0) != RESET)
    {
        DBG_ERROR("FIFO0 overflow detected!\r\n");
        CAN_ClearFlag(CAN1, CAN_FLAG_FOV0);
				return CAN_STATUS_HW_ERROR;
    }
		
    // 检查 FIFO0 是否有待处理报文 
    if (CAN_MessagePending(CAN1, CAN_FIFO0) == 0)
    {
        *Length = 0;
				return CAN_STATUS_NO_DATA;
    }

    // 正式接收报文 
    CanRxMsg RxMessage;
    CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);

    // 提取标识符 
     *ID = (RxMessage.IDE == CAN_Id_Standard) ? RxMessage.StdId : RxMessage.ExtId;

    // 数据帧 or 远程帧 
    if (RxMessage.RTR == CAN_RTR_Data)
    {
        *Length = RxMessage.DLC;

        // 拷贝数据 
         memcpy(Data, RxMessage.Data, *Length);

        // 调试打印 
        DBG_INFO("Receive: Data Frame received (ID: 0x%03lX) Data: ", (unsigned long)(*ID));
        for (uint8_t i = 0; i < *Length; i++)
        {
            DBG_INFO("%02X ", Data[i]);
        }
        DBG_INFO("\r\n");
				
				//返回成功
				return CAN_STATUS_OK;
    }
    else
    {
        // 远程帧 
        *Length = 0;
        DBG_INFO("Receive: Remote Frame received (ID: 0x%03lX)\r\n", (unsigned long)(*ID));
				return CAN_STATUS_RESP_ERROR;
    }
}
