#include "main.h"

//-----------------------------------------------------------

#define FRAM_CMD_WREN	    0x06
#define FRAM_CMD_WRDI	    0x04
#define FRAM_CMD_RDSR	    0x05
#define FRAM_CMD_WRSR	    0x01
#define FRAM_CMD_READ	    0x03
#define FRAM_CMD_WRITE	    0x02

//-----------------------------------------------------------

static SPI_HandleTypeDef SPI2Handle;

//-----------------------------------------------------------

int Init_FRAM(void);
void SPI2_IRQHandler(void);
    static int SPI_Send(SPI_HandleTypeDef* SPIx, unsigned char send_data, int recv_cnt, unsigned char* return_data);
    static int SPI_Sends(SPI_HandleTypeDef* SPIx, int send_cnt, unsigned char* send_data, int recv_cnt, unsigned char* return_data);

int FRAM_Read(int addr, void* data, int cnt);
int FRAM_Write(int addr, void* data, int cnt);

//-----------------------------------------------------------

void SPI2_IRQHandler(void) // FRAM
{
    HAL_SPI_IRQHandler(&SPI2Handle);
}

int Init_FRAM(void)
{
    int result;
    GPIO_InitTypeDef  gpio_init;

    __GPIOB_CLK_ENABLE();
    __SPI2_CLK_ENABLE();

    //-------------------

    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FAST;

    gpio_init.Pin = GPIO_PIN_11;
    HAL_GPIO_Init(GPIOB, &gpio_init); // FRAM_WP

    gpio_init.Pin = GPIO_PIN_12;
    HAL_GPIO_Init(GPIOB, &gpio_init); // FRAM_CS

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, 0);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, 1);

    gpio_init.Pin       = GPIO_PIN_13;
    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Pull      = GPIO_PULLDOWN;
    gpio_init.Speed     = GPIO_SPEED_FAST;
    gpio_init.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &gpio_init); // FRAM_SCK

    gpio_init.Pull      = GPIO_PULLUP;
    gpio_init.Pin       = GPIO_PIN_14;
    HAL_GPIO_Init(GPIOB, &gpio_init); // FRAM_MISO

    gpio_init.Pin       = GPIO_PIN_15;
    HAL_GPIO_Init(GPIOB, &gpio_init); // FRAM_MOSI

    //-------------------

    SPI2Handle.Instance               = SPI2;
    SPI2Handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    SPI2Handle.Init.Direction         = SPI_DIRECTION_2LINES;
    SPI2Handle.Init.CLKPhase          = SPI_PHASE_1EDGE;
    SPI2Handle.Init.CLKPolarity       = SPI_POLARITY_LOW;
    SPI2Handle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLED;
    SPI2Handle.Init.CRCPolynomial     = 7;
    SPI2Handle.Init.DataSize          = SPI_DATASIZE_8BIT;
    SPI2Handle.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    SPI2Handle.Init.NSS               = SPI_NSS_SOFT;
    SPI2Handle.Init.TIMode            = SPI_TIMODE_DISABLED;
    SPI2Handle.Init.Mode              = SPI_MODE_MASTER;

    HAL_NVIC_SetPriority(SPI2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(SPI2_IRQn);

    if ( (result=HAL_SPI_Init(&SPI2Handle)) != HAL_OK )
        RETURNI(result);

    return HAL_OK;
}

int SPI_Sends(SPI_HandleTypeDef* SPIx, int send_cnt, unsigned char* send_data, int recv_cnt, unsigned char* return_data)
{
    if ( SPIx==NULL || send_cnt<=0 || send_data==NULL || (recv_cnt>10 && return_data==NULL) )
        return -1;

    if ( send_cnt > 0 )
    {
        if ( HAL_SPI_Transmit_IT(SPIx, send_data, send_cnt) != HAL_OK )
            return -2;

        while ( HAL_SPI_GetState(SPIx) != HAL_SPI_STATE_READY );
    }

    if ( recv_cnt > 0 )
    {
        unsigned char result[10];

        if ( return_data==NULL && recv_cnt<=10 )
            return_data = result;

        if ( HAL_SPI_Receive_IT(SPIx, return_data, recv_cnt) != HAL_OK )
            return -3;

        while ( HAL_SPI_GetState(SPIx) != HAL_SPI_STATE_READY );

        for ( int i=0 ; i<recv_cnt ; i++ )
        {
            WX(return_data[i]);
        }
    }

    return 0;
}

int SPI_Send(SPI_HandleTypeDef* SPIx, unsigned char send_data, int recv_cnt, unsigned char* return_data)
{
    return SPI_Sends(SPIx, 1, &send_data, recv_cnt, return_data);
}

int FRAM_Read(int addr, void* data, int cnt)
{
    int result;
    unsigned char cmd[3];

    cmd[0] = FRAM_CMD_READ;
    cmd[1] = addr >> 8;
    cmd[2] = addr & 0xff;

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET); // FRAM_CS

    result = SPI_Sends(&SPI2Handle, 3, cmd, cnt, data);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET); // FRAM_CS

    return result;
}


int FRAM_Write(int addr, void* data, int cnt)
{
    int result;
    unsigned char cmd[10];

    cmd[0] = FRAM_CMD_WRITE;
    cmd[1] = addr >> 8;
    cmd[2] = addr & 0xff;

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET); // FRAM_CS

    result = SPI_Sends(&SPI2Handle, 3, cmd, 0, NULL);
    if ( result == 0 ) result = SPI_Sends(&SPI2Handle, cnt, data, 0, NULL);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET); // FRAM_CS

    return result;
}

