#include "main.h"

//--------------------------------------------

static CQ_DEFINE(unsigned char, UART6RxBuf, 100);
static CQ_DEFINE(unsigned char, UART3RxBuf, 100);

static UART_HandleTypeDef UART6Handle;
static UART_HandleTypeDef UART3Handle;

UART_HandleTypeDef* GYROZEN_UART = &UART6Handle;
UART_HandleTypeDef* LCD_UART = &UART3Handle;

//--------------------------------------------

int Init_LCDUART(void);
    void USRT3_IRQHandler(void);

int Init_GYROZENUART(void);
    void USART6_IRQHandler(void);

int UARTgets(UART_HandleTypeDef* handle, char *buf, int len);
int UARTgetc(UART_HandleTypeDef* handle);
void UARTputc(UART_HandleTypeDef* handle, int ch);
void UARTprintf(UART_HandleTypeDef* handle, char const *str, ...);
void UARTwrite(UART_HandleTypeDef* handle, void const *buf, int len);

//--------------------------------------------

void UARTwrite(UART_HandleTypeDef* handle, void const *buf, int len)
{
    unsigned char* p = (unsigned char*) buf;

    for ( int i=0 ; i<len ; i++, p++ )
        UARTputc(handle, *p);
}

void UARTputc(UART_HandleTypeDef* handle, int ch)
{
    while ( __HAL_UART_GET_FLAG(handle, UART_FLAG_TXE) == RESET );
    handle->Instance->DR = ch & 0xff;
}

int UARTgets(UART_HandleTypeDef* handle, char *buf, int max)
{
	int len = 0;

	buf[0] = '\0';
	while (1)
	{
		int ch = UARTgetc(handle);
		if (ch == '\n' || ch == '\r')
		{
			buf[len] = '\0';
			printf("\r\n");
			break;
		}
		else if (ch != '\0' && ch != 0xFF && ch != EOF && len < max)
		{
			buf[len++] = ch;
			UARTputc(handle, ch);
		}
	}

	return len;
}

int UARTgetc(UART_HandleTypeDef* handle)
{
    int ch = -1;

    if ( handle->Instance == USART3 )
    {
        CQ_REAR(UART3RxBuf) = CQ_MAX(UART3RxBuf) - UART3Handle.hdmarx->Instance->NDTR;
        if ( CQ_USEDSIZE(UART3RxBuf) > 0 )
        {
            CQ_GET(UART3RxBuf, ch);
        }
    }
    else if ( handle->Instance == USART6 )
    {
        if ( CQ_USEDSIZE(UART6RxBuf) > 0 )
        {
            CQ_GET(UART6RxBuf, ch);
        }
    }

    return ch;
}

void UARTprintf(UART_HandleTypeDef* handle, char const *fmt_str, ...)
{
    char buf[100];
    va_list args;
    int len;

    va_start(args, fmt_str);
    vsprintf(buf, fmt_str, args);
    va_end(args);

    len = strlen(buf);
    UARTwrite(handle, buf, len);
}

int Init_LCDUART(void)
{
    int result;
    GPIO_InitTypeDef  gpio_init;

    __USART3_CLK_ENABLE();
    __GPIOD_CLK_ENABLE();
    __DMA1_CLK_ENABLE();

    gpio_init.Pin       = GPIO_PIN_8|GPIO_PIN_9;
    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Pull      = GPIO_NOPULL;
    gpio_init.Speed     = GPIO_SPEED_FAST;
    gpio_init.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOD, &gpio_init);

    UART3Handle.Instance        = USART3;
    UART3Handle.Init.BaudRate   = 115200;
    UART3Handle.Init.WordLength = UART_WORDLENGTH_8B;
    UART3Handle.Init.StopBits   = UART_STOPBITS_1;
    UART3Handle.Init.Parity     = UART_PARITY_NONE;
    UART3Handle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    UART3Handle.Init.Mode       = UART_MODE_TX_RX;

    if ( (result=HAL_UART_Init(&UART3Handle)) != HAL_OK )
        RETURNI(result);

    // TX는 DMA로 처리한다
    {
        static DMA_HandleTypeDef hdma1_tx;
        static DMA_HandleTypeDef hdma1_rx;

        hdma1_tx.Instance                 = DMA1_Stream3;
        hdma1_tx.Init.Channel             = DMA_CHANNEL_4;
        hdma1_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
        hdma1_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
        hdma1_tx.Init.MemInc              = DMA_MINC_ENABLE;
        hdma1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma1_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
        hdma1_tx.Init.Mode                = DMA_NORMAL;
        hdma1_tx.Init.Priority            = DMA_PRIORITY_LOW;
        hdma1_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
        hdma1_tx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
        hdma1_tx.Init.MemBurst            = DMA_MBURST_INC4;
        hdma1_tx.Init.PeriphBurst         = DMA_PBURST_INC4;

        if ( (result=HAL_DMA_Init(&hdma1_tx)) != HAL_OK )
            RETURNI(result);

        __HAL_LINKDMA(&UART3Handle, hdmatx, hdma1_tx);

        hdma1_rx.Instance                 = DMA1_Stream1;
        hdma1_rx.Init.Channel             = DMA_CHANNEL_4;
        hdma1_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
        hdma1_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
        hdma1_rx.Init.MemInc              = DMA_MINC_ENABLE;
        hdma1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma1_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
        hdma1_rx.Init.Mode                = DMA_CIRCULAR;
        hdma1_rx.Init.Priority            = DMA_PRIORITY_HIGH;
        hdma1_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
        hdma1_rx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
        hdma1_rx.Init.MemBurst            = DMA_MBURST_INC4;
        hdma1_rx.Init.PeriphBurst         = DMA_PBURST_INC4;

        if ( (result=HAL_DMA_Init(&hdma1_rx)) != HAL_OK )
            RETURNI(result);

        __HAL_LINKDMA(&UART3Handle, hdmarx, hdma1_rx);

        HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 6, 1);
        HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
    }


    if ( (result=HAL_UART_Receive_DMA(&UART3Handle, (uint8_t *)UART3RxBuf, ARRAYSIZE(UART3RxBuf))) != HAL_OK )
        RETURNI(result);

    /*
    __HAL_UART_ENABLE_IT(&UART3Handle, UART_IT_RXNE);

    HAL_NVIC_SetPriority(USART3_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
    */

    return 0;
}

void USART3_IRQHandler(void)
{
    CQ_PUT(UART3RxBuf, UART3Handle.Instance->DR);
}

void DMA1_Stream3_IRQHandler(void) // USART3 TX
{
    int lisr = DMA1->LISR;

    HAL_DMA_IRQHandler(UART3Handle.hdmatx);

    if ( lisr & DMA_FLAG_TCIF3_7 )
        MsgSendIrqHandler();
}

int Init_GYROZENUART(void)
{
    int result;
    GPIO_InitTypeDef  gpio_init;

    __USART6_CLK_ENABLE();
    __GPIOC_CLK_ENABLE();

    gpio_init.Pin       = GPIO_PIN_6|GPIO_PIN_7;
    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Pull      = GPIO_NOPULL;
    gpio_init.Speed     = GPIO_SPEED_FAST;
    gpio_init.Alternate = GPIO_AF8_USART6;
    HAL_GPIO_Init(GPIOC, &gpio_init);

    UART6Handle.Instance        = USART6;
    UART6Handle.Init.BaudRate   = 9600;
    UART6Handle.Init.WordLength = UART_WORDLENGTH_8B;
    UART6Handle.Init.StopBits   = UART_STOPBITS_1;
    UART6Handle.Init.Parity     = UART_PARITY_NONE;
    UART6Handle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    UART6Handle.Init.Mode       = UART_MODE_TX_RX;

    if ( (result=HAL_UART_Init(&UART6Handle)) != HAL_OK )
        RETURNI(result);

    __HAL_UART_ENABLE_IT(&UART6Handle, UART_IT_RXNE);

    HAL_NVIC_SetPriority(USART6_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(USART6_IRQn);

    return 0;
}

void USART6_IRQHandler(void)
{
    CQ_PUT(UART6RxBuf, UART6Handle.Instance->DR);
}