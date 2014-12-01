#include "main.h"

//----------------------------------------------------
static CQ_DEFINE(CanRxMsgTypeDef, MotorCANBuf, 10);

static CAN_HandleTypeDef CAN1Handle;
static CAN_HandleTypeDef CAN2Handle;

CAN_HandleTypeDef* MOTOR_CAN = &CAN1Handle;
CAN_HandleTypeDef* ROTOR_CAN = &CAN2Handle;
CAN_HandleTypeDef* DSP_CAN = &CAN2Handle;

//----------------------------------------------------

int Init_CAN_MOTOR(void); // MOTOR
    int CAN_Send(CAN_HandleTypeDef* hanlde, int msgid, void* data, int data_len, int wait_tick);
    int CAN_Recv(CAN_HandleTypeDef* handle, CanRxMsgTypeDef* data);

int Init_CAN_ROTOR(void); // ROTOR or DSP

//----------------------------------------------------

int CAN_Recv(CAN_HandleTypeDef* handle, CanRxMsgTypeDef* data)
{
    if ( handle == MOTOR_CAN )
    {
        if ( CQ_ISNOTEMPTY(MotorCANBuf) )
        {
            CQ_GET(MotorCANBuf, *data);
            return HAL_OK;
        }

        return -1;
    }

    return -2;
}


void HAL_CAN_RxCpltCallback(CAN_HandleTypeDef* hcan)
{
    if ( hcan->Instance == CAN1 ) // MOTOR
    {
        CQ_PUT(MotorCANBuf, *hcan->pRxMsg);
    }
    else if ( hcan->Instance == CAN2 ) // ROTOR or DSP
    {
        if ( hcan->pRxMsg->StdId & 0xF00 == 0x600 )
        {
            RB_CanRecvHandler(hcan->pRxMsg);
        }
        else if ( hcan->pRxMsg->StdId & 0xF00 == 0x100 )
        {
            DSP_CANRecvHandler(hcan->pRxMsg);
        }
    }
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef* hcan)
{
    DPRINTF(("[CAN%i] ErrorCode: %x", hcan->Instance==CAN1 ? 1 : 2, hcan->ErrorCode));
}


int CAN_Send(CAN_HandleTypeDef* hanlde, int msgid, void* data, int data_len, int wait_tick)
{
    memset(hanlde->pTxMsg, 0, sizeof(CanTxMsgTypeDef));

    hanlde->pTxMsg->StdId = msgid;
    hanlde->pTxMsg->DLC = data_len;
    memcpy(hanlde->pTxMsg->Data, data, data_len);

    if ( wait_tick > 0 )
        return HAL_CAN_Transmit(hanlde, wait_tick);
    else
        return HAL_CAN_Transmit_IT(hanlde);

}

int Init_CAN_MOTOR(void)
{
    int result;
    static CanTxMsgTypeDef txmsg;
    static CanRxMsgTypeDef rxmsg;
    CAN_FilterConfTypeDef filter;
    GPIO_InitTypeDef  gpio_init;

    __CAN1_CLK_ENABLE();
    __GPIOD_CLK_ENABLE();

    gpio_init.Pin       = GPIO_PIN_0;
    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Pull      = GPIO_NOPULL;
    gpio_init.Speed     = GPIO_SPEED_FAST;
    gpio_init.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOD, &gpio_init);

    gpio_init.Pin = GPIO_PIN_1;
    gpio_init.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOD, &gpio_init);

    memset(&txmsg, 0, sizeof(txmsg));
    memset(&rxmsg, 0, sizeof(rxmsg));

    CAN1Handle.Instance = CAN1;
    CAN1Handle.pRxMsg = &rxmsg;
    CAN1Handle.pTxMsg = &txmsg;
    CAN1Handle.Init.Mode = CAN_MODE_NORMAL;
    CAN1Handle.Init.ABOM = DISABLE;
    CAN1Handle.Init.AWUM = DISABLE;
    CAN1Handle.Init.NART = DISABLE;
    CAN1Handle.Init.RFLM = DISABLE;
    CAN1Handle.Init.TTCM = DISABLE;
    CAN1Handle.Init.TXFP = DISABLE;

    // CAN은 APB1 사용 (42Mhz)
    CAN1Handle.Init.SJW = CAN_SJW_1TQ;
    CAN1Handle.Init.BS1 = CAN_BS1_7TQ;
    CAN1Handle.Init.BS2 = CAN_BS2_6TQ;
    CAN1Handle.Init.Prescaler = 24;

    if ( (result = HAL_CAN_Init(&CAN1Handle)) != HAL_OK )
        RETURNI(result);

    filter.FilterNumber = 0;
    filter.BankNumber = 0;
    filter.FilterActivation = ENABLE;
    filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    filter.FilterIdHigh = 0;
    filter.FilterIdLow = 0;
    filter.FilterMaskIdHigh = 0;
    filter.FilterMaskIdLow = 0;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    HAL_CAN_ConfigFilter(&CAN1Handle, &filter);

    HAL_NVIC_SetPriority(CAN1_TX_IRQn, 5, 1);
    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 5, 1);
    HAL_NVIC_SetPriority(CAN1_RX1_IRQn, 5, 1);
    HAL_NVIC_SetPriority(CAN1_SCE_IRQn, 5, 1);

    HAL_NVIC_EnableIRQ(CAN1_TX_IRQn);
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
    HAL_NVIC_EnableIRQ(CAN1_RX1_IRQn);
    HAL_NVIC_EnableIRQ(CAN1_SCE_IRQn);

    if ( (result=HAL_CAN_Receive_IT(&CAN1Handle, CAN_FILTER_FIFO0)) != HAL_OK )
        RETURNI(result);

    return HAL_OK;
}

int Init_CAN_ROTOR(void)
{
    int result;
    static CanTxMsgTypeDef txmsg;
    static CanRxMsgTypeDef rxmsg;
    CAN_FilterConfTypeDef filter;
    GPIO_InitTypeDef  gpio_init;

    __CAN2_CLK_ENABLE();
    __GPIOB_CLK_ENABLE();

    gpio_init.Pin       = GPIO_PIN_5;
    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Pull      = GPIO_NOPULL;
    gpio_init.Speed     = GPIO_SPEED_FAST;
    gpio_init.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOB, &gpio_init);

    gpio_init.Pin = GPIO_PIN_6;
    gpio_init.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOB, &gpio_init);

    memset(&txmsg, 0, sizeof(txmsg));
    memset(&rxmsg, 0, sizeof(rxmsg));

    CAN2Handle.Instance = CAN2;
    CAN2Handle.pRxMsg = &rxmsg;
    CAN2Handle.pTxMsg = &txmsg;
    CAN2Handle.Init.Mode = CAN_MODE_NORMAL;
    CAN2Handle.Init.ABOM = DISABLE;
    CAN2Handle.Init.AWUM = DISABLE;
    CAN2Handle.Init.NART = DISABLE;
    CAN2Handle.Init.RFLM = DISABLE;
    CAN1Handle.Init.TTCM = DISABLE;
    CAN2Handle.Init.TXFP = DISABLE;

    // CAN은 APB1 사용 (42Mhz)
    CAN2Handle.Init.SJW = CAN_SJW_1TQ;
    CAN2Handle.Init.BS1 = CAN_BS1_7TQ;
    CAN2Handle.Init.BS2 = CAN_BS2_6TQ;
    CAN2Handle.Init.Prescaler = 24;

    if ( (result = HAL_CAN_Init(&CAN2Handle)) != HAL_OK )
        RETURNI(result);

    filter.FilterNumber = 0;
    filter.BankNumber = 0;
    filter.FilterActivation = ENABLE;
    filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    filter.FilterIdHigh = 0;
    filter.FilterIdLow = 0;
    filter.FilterMaskIdHigh = 0;
    filter.FilterMaskIdLow = 0;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    HAL_CAN_ConfigFilter(&CAN1Handle, &filter);

    HAL_NVIC_SetPriority(CAN2_TX_IRQn, 6, 1);
    HAL_NVIC_SetPriority(CAN2_RX0_IRQn, 6, 1);
    HAL_NVIC_SetPriority(CAN2_RX1_IRQn, 6, 1);
    HAL_NVIC_SetPriority(CAN2_SCE_IRQn, 6, 1);

    HAL_NVIC_EnableIRQ(CAN2_TX_IRQn);
    HAL_NVIC_EnableIRQ(CAN2_RX0_IRQn);
    HAL_NVIC_EnableIRQ(CAN2_RX1_IRQn);
    HAL_NVIC_EnableIRQ(CAN2_SCE_IRQn);

    if ( (result=HAL_CAN_Receive_IT(&CAN2Handle, CAN_FILTER_FIFO0)) != HAL_OK )
        RETURNI(result);

    return HAL_OK;
}

void CAN1_TX_IRQHandler(void)
{
    HAL_CAN_IRQHandler(&CAN1Handle);
}

void CAN1_RX0_IRQHandler(void)
{
    HAL_CAN_IRQHandler(&CAN1Handle);
    HAL_CAN_Receive_IT(&CAN1Handle, CAN_FILTER_FIFO0);
}

void CAN1_RX1_IRQHandler(void)
{
    HAL_CAN_IRQHandler(&CAN1Handle);
    HAL_CAN_Receive_IT(&CAN1Handle, CAN_FILTER_FIFO1);
}

void CAN1_SCE_IRQHandler(void)
{
    HAL_CAN_IRQHandler(&CAN1Handle);
}

void CAN2_TX_IRQHandler(void)
{
    HAL_CAN_IRQHandler(&CAN2Handle);
}

void CAN2_RX0_IRQHandler(void)
{
    HAL_CAN_IRQHandler(&CAN2Handle);
    HAL_CAN_Receive_IT(&CAN2Handle, CAN_FILTER_FIFO0);
}

void CAN2_RX1_IRQHandler(void)
{
    HAL_CAN_IRQHandler(&CAN2Handle);
    HAL_CAN_Receive_IT(&CAN2Handle, CAN_FILTER_FIFO1);
}

void CAN2_SCE_IRQHandler(void)
{
    HAL_CAN_IRQHandler(&CAN2Handle);

}
