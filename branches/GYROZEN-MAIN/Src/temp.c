#include "main.h"

#define MOTOR_TEMP      0
#define CHAMBER_TEMP    1

#define __USE_TRIGGER_TRGO__
#define __USE_TIMER_TRIGGER__

//-------------------------------------------------

static ADC_HandleTypeDef AdcHandle;
    static DMA_HandleTypeDef AdcDmaHandle;
    static TIM_HandleTypeDef AdcTimHandle;

static volatile unsigned long Temp[2] = { 0, }; // MOTOR_TEMP, CHAMBER_TEMP

//-------------------------------------------------

int GetChamberTemp(void);
int GetMotorTemp(void);
int Init_TEMP(void);

//-------------------------------------------------

int GetChamberTemp(void)
{
    //  ADC REF Voltage 3.3V, 12bit AD
    int chamber_temp = ((((double)Temp[CHAMBER_TEMP] / 1241.2121) - 0.5) / 0.01) * 100;
    //WI(chamber_temp);
    return chamber_temp;
}

int GetMotorTemp(void)
{
    return (int)Temp[MOTOR_TEMP];
}

int Init_TEMP(void)
{
    int result;
    GPIO_InitTypeDef gpio_init;

    __GPIOB_CLK_ENABLE();
    __ADC1_CLK_ENABLE();
    __DMA2_CLK_ENABLE();
    __TIM2_CLK_ENABLE();

    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Pin = GPIO_PIN_0 | GPIO_PIN_1; // TEMP_1(MOTOR_TEMP), TEMP_2(CHAMBER_TEMP)
    HAL_GPIO_Init(GPIOB, &gpio_init);

    // DMA Init
    {
        AdcDmaHandle.Instance = DMA2_Stream0;
        AdcDmaHandle.Init.Channel  = DMA_CHANNEL_0;
        AdcDmaHandle.Init.Direction = DMA_PERIPH_TO_MEMORY;
        AdcDmaHandle.Init.PeriphInc = DMA_PINC_DISABLE;
        AdcDmaHandle.Init.MemInc = DMA_MINC_ENABLE;
        AdcDmaHandle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
        AdcDmaHandle.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
        AdcDmaHandle.Init.Mode = DMA_CIRCULAR;
        AdcDmaHandle.Init.Priority = DMA_PRIORITY_HIGH;
        AdcDmaHandle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        AdcDmaHandle.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
        AdcDmaHandle.Init.MemBurst = DMA_MBURST_SINGLE;
        AdcDmaHandle.Init.PeriphBurst = DMA_PBURST_SINGLE;

        HAL_DMA_Init(&AdcDmaHandle);

        __HAL_LINKDMA(&AdcHandle, DMA_Handle, AdcDmaHandle);

//        //ADC->ContinuousConvMode에서는 인터럽트를 켜면 인터럽트가 너무 많이 발생해서 시스템이 멈춘다
//        HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 3, 0);
//        HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
    }

    // ADC Init
    {
        ADC_ChannelConfTypeDef sConfig;

        AdcHandle.Instance = ADC1;
        AdcHandle.Init.ClockPrescaler = ADC_CLOCKPRESCALER_PCLK_DIV8;
        AdcHandle.Init.Resolution = ADC_RESOLUTION12b;
        AdcHandle.Init.ScanConvMode = ENABLE;
        AdcHandle.Init.DiscontinuousConvMode = DISABLE;
        AdcHandle.Init.NbrOfDiscConversion = 0;
#ifdef __USE_TIMER_TRIGGER__
        AdcHandle.Init.ContinuousConvMode = DISABLE;
#ifdef __USE_TRIGGER_TRGO__
        AdcHandle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
        AdcHandle.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_TRGO;
#else
        AdcHandle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISINGFALLING;
        AdcHandle.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_CC2;
#endif
#else
        AdcHandle.Init.ContinuousConvMode = ENABLE;
        AdcHandle.Init.ExternalTrigConvEdge = 0;
        AdcHandle.Init.ExternalTrigConv = 0;
#endif
        AdcHandle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
        AdcHandle.Init.NbrOfConversion = 2;
        AdcHandle.Init.DMAContinuousRequests = ENABLE;
        AdcHandle.Init.EOCSelection = DISABLE;

        if ( (result=HAL_ADC_Init(&AdcHandle)) != HAL_OK )
            RETURNI(result);

        sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
        sConfig.Offset = 0;

        sConfig.Channel = ADC_CHANNEL_8;
        sConfig.Rank = 1;
        if ( (result=HAL_ADC_ConfigChannel(&AdcHandle, &sConfig)) != HAL_OK )
            RETURNI(result);

        sConfig.Channel = ADC_CHANNEL_9;
        sConfig.Rank = 2;
        if ( (result=HAL_ADC_ConfigChannel(&AdcHandle, &sConfig)) != HAL_OK )
            RETURNI(result);

//        HAL_NVIC_SetPriority(ADC_IRQn, 4, 0);
//        HAL_NVIC_EnableIRQ(ADC_IRQn);
//
//        if ( (result=HAL_ADC_Start_IT(&AdcHandle)) != HAL_OK )
        if ( (result=HAL_ADC_Start_DMA(&AdcHandle, (uint32_t*)Temp, ARRAYSIZE(Temp))) != HAL_OK )
            RETURNI(result);
    }

#ifdef __USE_TIMER_TRIGGER__
    // Init TIMER
    {
        AdcTimHandle.Instance = TIM2;
        AdcTimHandle.Init.Period = 100; // 100Hz
        AdcTimHandle.Init.Prescaler = (gAPB1Clock*2) / 10000;
        AdcTimHandle.Init.ClockDivision = 0;
        AdcTimHandle.Init.CounterMode = TIM_COUNTERMODE_UP;

//        HAL_NVIC_SetPriority(TIM2_IRQn, 4, 0);
//        HAL_NVIC_EnableIRQ(TIM2_IRQn);

#ifdef __USE_TRIGGER_TRGO__
        {
            TIM_MasterConfigTypeDef master_config;

            if ( (result=HAL_TIM_Base_Init(&AdcTimHandle)) != HAL_OK )
            {
                RETURNI(result);
            }

            master_config.MasterOutputTrigger = TIM_TRGO_UPDATE;
            master_config.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
            HAL_TIMEx_MasterConfigSynchronization(&AdcTimHandle, &master_config);

            if ( (result=HAL_TIM_Base_Start_IT(&AdcTimHandle)) != HAL_OK )
            {
                RETURNI(result);
            }
        }
#else
        // ADC_EXTERNALTRIGCONV_T2_CC2
        {
            TIM_OC_InitTypeDef oc_config;

            if ( (result=HAL_TIM_OC_Init(&AdcTimHandle)) != HAL_OK )
            {
                RETURNI(result);
            }

            oc_config.OCMode = TIM_OCMODE_TOGGLE;
            oc_config.OCFastMode = TIM_OCFAST_DISABLE;
            oc_config.OCIdleState = TIM_OCNIDLESTATE_RESET;
            oc_config.OCNIdleState = TIM_OCIDLESTATE_SET;
            oc_config.OCPolarity = TIM_OCPOLARITY_HIGH;
            oc_config.OCNPolarity = TIM_OCNPOLARITY_LOW;
            oc_config.Pulse = 1;

            if((result=HAL_TIM_OC_ConfigChannel(&AdcTimHandle, &oc_config, TIM_CHANNEL_2)) != HAL_OK)
            {
                RETURNI(result);
            }

            if ( (result=HAL_TIM_OC_Start_IT(&AdcTimHandle, TIM_CHANNEL_2)) != HAL_OK )
            {
                RETURNI(result);
            }
        }
#endif
    }
#endif

    return HAL_OK;
}

void ADC_IRQHandler(void)
{
    FSTART();
    HAL_ADC_IRQHandler(&AdcHandle);
}

void TIM2_IRQHandler(void)
{
    FSTART();
    HAL_TIM_IRQHandler(&AdcTimHandle);
}

void DMA2_Stream0_IRQHandler(void)
{
    FSTART();
    HAL_DMA_IRQHandler(&AdcDmaHandle);
}
