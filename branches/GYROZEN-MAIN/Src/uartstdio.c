#include "main.h"

//--------------------------------------------------------------------------------------------------

static CQ_DEFINE(unsigned char, TxBuf, 5000);
static CQ_DEFINE(unsigned char, RxBuf, 10);

static UART_HandleTypeDef UART2Handle;

static volatile bool UartDMATxBusy = false;
static volatile int UartDMALastSendLen = 0;

//--------------------------------------------------------------------------------------------------

int Init_DEBUGUART(void);
int DEBUGgets(char *buf, int len);
int DEBUGgetc(void);
void DEBUGputc(int ch);
void DEBUGwrite(void const *buf, int len);
void DEBUGprintf(char const *str, ...);
void DEBUGprintf_NewLine(char const *str, ...);
int UARTgetcNonBlocking(void);

//--------------------------------------------------------------------------------------------------

void DMA1_Stream6_IRQHandler(void) // USART2 TX
{
    HAL_DMA_IRQHandler(UART2Handle.hdmatx);
}

int Init_DEBUGUART(void)
{
    int result;
    static DMA_HandleTypeDef hdma1_tx;
    static DMA_HandleTypeDef hdma1_rx;
    GPIO_InitTypeDef  gpio_init;

    __USART2_CLK_ENABLE();
    __DMA1_CLK_ENABLE();
    __GPIOA_CLK_ENABLE();

    gpio_init.Pin       = GPIO_PIN_2;
    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Pull      = GPIO_NOPULL;
    gpio_init.Speed     = GPIO_SPEED_FAST;
    gpio_init.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    gpio_init.Pin = GPIO_PIN_3;
    gpio_init.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    UART2Handle.Instance        = USART2;
    UART2Handle.Init.BaudRate   = 115200;
    UART2Handle.Init.WordLength = UART_WORDLENGTH_8B;
    UART2Handle.Init.StopBits   = UART_STOPBITS_1;
    UART2Handle.Init.Parity     = UART_PARITY_NONE;
    UART2Handle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    UART2Handle.Init.Mode       = UART_MODE_TX_RX;

    if ( (result=HAL_UART_Init(&UART2Handle)) != HAL_OK )
        RETURNI(result);

    hdma1_tx.Instance                 = DMA1_Stream6;
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

    __HAL_LINKDMA(&UART2Handle, hdmatx, hdma1_tx);

    hdma1_rx.Instance                 = DMA1_Stream5;
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

    __HAL_LINKDMA(&UART2Handle, hdmarx, hdma1_rx);

    HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);

    // UART 수신의 경우 구지 인터럽트를 켜지 않더라도 버퍼를 Circular Queue로 운용할 수 있다
    // UART 수신 인터럽트를 받지 않으므로써 성능이 크게 개선되는 것을 기대할 수 있다
    // HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
    // HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);

    if ( (result=HAL_UART_Receive_DMA(&UART2Handle, (uint8_t *)RxBuf, ARRAYSIZE(RxBuf))) != HAL_OK )
        RETURNI(result);

    return 0;
}

void DEBUGwrite(void const *buf, int len)
{
    unsigned char *p = (unsigned char*) buf;

    if ( osKernelRunning() )
        osSemaphoreWait(UART_PRINTF_SEMAPHOR, 1000);

    // TX 버퍼가 가득차서 전송 못하는 데이터가 없도록 한다
    while ( CQ_FREESIZE(TxBuf) < len );

    // CQ_PUTBUF()를 하는 동안 크리티컬 섹션으로 막아서 다른 데이터가 중간에 섞이지 않도록 한다
    taskENTER_CRITICAL();
    CQ_PUTBUF(TxBuf, p, len);
    taskEXIT_CRITICAL();

    if ( UartDMATxBusy == false )
    {
        UartDMATxBusy = true;
        UartDMALastSendLen = CQ_CONTINUOUS_LEN(TxBuf);
        if ( HAL_UART_Transmit_DMA(&UART2Handle, (uint8_t*)&CQ_PEEK_DIRECT(TxBuf), UartDMALastSendLen) != HAL_OK )
        {
            osSemaphoreRelease(UART_PRINTF_SEMAPHOR);
            return;
        }
    }

    //    if ( !inHandlerMode() ) // 인터럽트 핸들러 내부가 아니라면
    //        while ( CQ_USEDSIZE(TxBuf) > 0 );

    if ( osKernelRunning() )
        osSemaphoreRelease(UART_PRINTF_SEMAPHOR);
}

void DEBUGputc(int ch)
{
    char chtemp = ch;
    DEBUGwrite(&chtemp, 1);
}

int DEBUGgets(char *buf, int max)
{
	int len = 0;

	buf[0] = '\0';
	while (1)
	{
		int ch = DEBUGgetc();
		if (ch == '\n' || ch == '\r')
		{
			buf[len] = '\0';
			printf("\r\n");
			break;
		}
		else if (ch != '\0' && ch != 0xFF && ch != EOF && len < max)
		{
			buf[len++] = ch;
			DEBUGputc(ch);
		}
	}

	return len;
}

int DEBUGgetc(void)
{
    int ch = -1;

    CQ_REAR(RxBuf) = CQ_MAX(RxBuf) - UART2Handle.hdmarx->Instance->NDTR;
    if ( CQ_USEDSIZE(RxBuf) > 0 )
    {
        CQ_GET(RxBuf, ch);
    }

    return ch;
}

int UARTgetcNonBlocking(void)
{
    return DEBUGgetc();
}

void DEBUGprintf(char const *fmt_str, ...)
{
    char buf[100];
    va_list args;
    int len;

    va_start(args, fmt_str);
    vsprintf(buf, fmt_str, args);
    va_end(args);

    len = strlen(buf);
    DEBUGwrite(buf, len);
}

void DEBUGprintf_NewLine(char const *fmt_str, ...)
{
    char buf[100];
    va_list args;
    int len;

    va_start(args, fmt_str);
    vsprintf(buf, fmt_str, args);
    va_end(args);

    len = strlen(buf);
    buf[len] = '\r';
    buf[len+1] = '\n';
    len += 2;

    DEBUGwrite(buf, len);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) // UART Tx completed callback
{
    if ( huart == &UART2Handle )
    {
        CQ_DROP(TxBuf, UartDMALastSendLen);

        if ( CQ_USEDSIZE(TxBuf) )
        {
            UartDMALastSendLen = CQ_CONTINUOUS_LEN(TxBuf);
            HAL_UART_Transmit_DMA(huart, (uint8_t*)&CQ_PEEK_DIRECT(TxBuf), UartDMALastSendLen);
            UartDMATxBusy = true;
        }
        else
        {
            UartDMALastSendLen = 0;
            UartDMATxBusy = false;
        }
    }
}
