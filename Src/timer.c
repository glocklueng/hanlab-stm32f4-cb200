/*
    TIM 2,3,4,5,6,7,12,13,14    APB1 (42Mhz*2)
    TIM 1,8,9,10,11             APB2 (84Mhz*2)

    TIM 1,8             ADV TIMER (COMP)
    TIM 3,4             16bit
    TIM 2,5             32bit
    TIM 6,7             BASIC (TRIGGER ONLY, SLAVE X)
    TIM 10,11,13,14     1 channel (DMA X, MASTER TRGO X, SLAVE X)
    TIM 9,12            2 channel (DMA X)
*/

#include "main.h"

//--------------------------------

volatile unsigned int gRPM;
volatile unsigned int gPrecisionRPM;


static TIM_HandleTypeDef PwmTimHandle;
static TIM_HandleTypeDef RPMTimerHandle;
static TIM_HandleTypeDef BuzzerTimerHandle;
static TIM_HandleTypeDef BuzzerCounterTimerHandle;

//--------------------------------

int Init_PWM(void);

int Init_RPM(void);
    void TIM5_IRQHandler(void);

void Buzzer(int freq_hz, int time_ms);
int Init_BUZZER(void);
    void TIM1_BRK_TIM9_IRQHandler(void); // buzzer slave

//--------------------------------

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    static int rot_id_detect_cnt = 0;

    //FSTART();
    if ( htim->Instance == TIM5 )
    {
        if ( htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1 )
        {
            //DPUTS("DETECT: RPM_IN");
            // ĸ�絥���͸� �̿��ؼ� RPM�� ����Ѵ�.
            //WI(HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1));
            //WI(rot_id_detect_cnt);
            rot_id_detect_cnt = 0;
        }
        else if ( htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2 )
        {
            //DPUTS("DETECT: ROT_ID");
            rot_id_detect_cnt++;
            WI(HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2));
        }
    }
}

void TIM5_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&RPMTimerHandle);
}

int Init_RPM(void)
{
    int result = 0;

    TIM_IC_InitTypeDef ic_config;
    TIM_SlaveConfigTypeDef slave_config;
    GPIO_InitTypeDef  gpio_init;

    __TIM5_CLK_ENABLE();

    gpio_init.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_HIGH;
    gpio_init.Alternate = GPIO_AF2_TIM5;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    RPMTimerHandle.Instance = TIM5;
    RPMTimerHandle.Init.Period = 10000;
    RPMTimerHandle.Init.Prescaler = SystemCoreClock / 10000;
    RPMTimerHandle.Init.ClockDivision = 0;
    RPMTimerHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
    HAL_TIM_IC_Init(&RPMTimerHandle);

    ic_config.ICPrescaler = TIM_ICPSC_DIV1;
    ic_config.ICFilter = 0;
    ic_config.ICPolarity = TIM_ICPOLARITY_RISING;
    ic_config.ICSelection = TIM_ICSELECTION_DIRECTTI;
    HAL_TIM_IC_ConfigChannel(&RPMTimerHandle, &ic_config, TIM_CHANNEL_1);
    HAL_TIM_IC_ConfigChannel(&RPMTimerHandle, &ic_config, TIM_CHANNEL_2);

    // RPM(CH1)�� �ԷµǸ� Ÿ�̸Ӹ� �����ؼ� �ٽ� �����Ѵ�
    slave_config.SlaveMode     = TIM_SLAVEMODE_RESET;
    slave_config.InputTrigger  = TIM_TS_TI1FP1;
    HAL_TIM_SlaveConfigSynchronization(&RPMTimerHandle, &slave_config);

    HAL_NVIC_SetPriority(TIM5_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(TIM5_IRQn);

    HAL_TIM_IC_Start_IT(&RPMTimerHandle, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(&RPMTimerHandle, TIM_CHANNEL_2);

    return result;
}

int Init_PWM(void)
{
    int result = 0;

    TIM_OC_InitTypeDef oc_config;
    GPIO_InitTypeDef  gpio_init;

    __TIM3_CLK_ENABLE();
    __GPIOA_CLK_ENABLE();

    gpio_init.Pin = GPIO_PIN_7; // DOOR_ACT_PWM
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_HIGH;
    gpio_init.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    PwmTimHandle.Instance = TIM3;
    PwmTimHandle.Init.Period = 100; // 10000 / 100 = 100Hz
    PwmTimHandle.Init.Prescaler = (gAPB1Clock*2) / 10000; // Ÿ�̸� Ŭ���� APB ���� Ŭ���� �ι��̴�. (HCLK�� �ѱ� �� ����)
    PwmTimHandle.Init.ClockDivision = 0;
    PwmTimHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
    HAL_TIM_PWM_Init(&PwmTimHandle);

    oc_config.OCMode = TIM_OCMODE_PWM1;
    oc_config.OCFastMode = TIM_OCFAST_ENABLE;
    oc_config.OCIdleState = TIM_OCNIDLESTATE_RESET;
    oc_config.OCNIdleState = TIM_OCIDLESTATE_SET;
    oc_config.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc_config.OCNPolarity = TIM_OCNPOLARITY_LOW;
    oc_config.Pulse = PwmTimHandle.Init.Period / 2; // duty 50%
    HAL_TIM_PWM_ConfigChannel(&PwmTimHandle, &oc_config, TIM_CHANNEL_2);

    //HAL_TIM_PWM_Start(&PwmTimHandle, TIM_CHANNEL_2);

    return result;
}

int Init_BUZZER(void)
{
    int result = 0;

    TIM_OC_InitTypeDef oc_config;
    GPIO_InitTypeDef  gpio_init;

    __TIM11_CLK_ENABLE(); // master
    __TIM9_CLK_ENABLE(); // slave
    __GPIOB_CLK_ENABLE();

    gpio_init.Pin = GPIO_PIN_9; // BUZZER
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_HIGH;
    gpio_init.Alternate = GPIO_AF3_TIM11;
    HAL_GPIO_Init(GPIOB, &gpio_init);

    BuzzerTimerHandle.Instance = TIM11;
    BuzzerTimerHandle.Init.Period = 100; // 100000 / 100 = 1000Hz
    BuzzerTimerHandle.Init.Prescaler = (gAPB2Clock*2) / 100000; // Ÿ�̸� Ŭ���� APB ���� Ŭ���� �ι��̴�. (HCLK�� �ѱ� �� ����)
    BuzzerTimerHandle.Init.ClockDivision = 0;
    BuzzerTimerHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
    HAL_TIM_PWM_Init(&BuzzerTimerHandle);

    oc_config.OCMode = TIM_OCMODE_PWM1;
    oc_config.OCFastMode = TIM_OCFAST_ENABLE;
    oc_config.OCIdleState = TIM_OCNIDLESTATE_RESET;
    oc_config.OCNIdleState = TIM_OCIDLESTATE_SET;
    oc_config.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc_config.OCNPolarity = TIM_OCNPOLARITY_LOW;
    oc_config.Pulse = 100;
    HAL_TIM_PWM_ConfigChannel(&BuzzerTimerHandle, &oc_config, TIM_CHANNEL_1);

    // ������ ������ �ð� ���� �︮�� �ڵ����� ���߰� �ϱ� ���ؼ� Slave Timer�� ����Ѵ�
    {
        TIM_MasterConfigTypeDef master_config;
        TIM_SlaveConfigTypeDef slave_config;

        BuzzerCounterTimerHandle.Instance = TIM9;
        BuzzerCounterTimerHandle.Init.Period = 1000;
        BuzzerCounterTimerHandle.Init.Prescaler = 0;
        BuzzerCounterTimerHandle.Init.ClockDivision = 0;
        BuzzerCounterTimerHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
        HAL_TIM_Base_Init(&BuzzerCounterTimerHandle);

        master_config.MasterOutputTrigger = TIM_TRGO_OC1REF;
        master_config.MasterSlaveMode = TIM_MASTERSLAVEMODE_ENABLE;
        HAL_TIMEx_MasterConfigSynchronization(&BuzzerTimerHandle, &master_config);

        slave_config.SlaveMode = TIM_SLAVEMODE_EXTERNAL1;
        slave_config.InputTrigger = TIM_TS_ITR3;
        slave_config.TriggerFilter = 0;
        slave_config.TriggerPolarity = TIM_TRIGGERPOLARITY_RISING;
        slave_config.TriggerPrescaler = TIM_ETRPRESCALER_DIV1;
        HAL_TIM_SlaveConfigSynchronization(&BuzzerCounterTimerHandle, &slave_config);
    }

    HAL_NVIC_SetPriority(TIM1_BRK_TIM9_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(TIM1_BRK_TIM9_IRQn);

    //HAL_TIM_Base_Start_IT(&BuzzerCounterTimerHandle);
    //HAL_TIM_PWM_Start(&BuzzerTimerHandle, TIM_CHANNEL_1);

    return result;
}

void TIM1_BRK_TIM9_IRQHandler(void) // buzzer slave
{
    // slave timer�� ī��Ʈ�� ARR�� �����ؼ� UpdateEvent�� �߻��ϸ� �������� �����Ѵ�
    // ���� �̰��� ������ ��ε��� ������ ���������� �� �� �ְ� �ؼ� ��ε� ���ָ� �����ϰ� �Ѵ�.

    HAL_TIM_IRQHandler(&BuzzerCounterTimerHandle);
    HAL_TIM_PWM_Stop(&BuzzerTimerHandle, TIM_CHANNEL_1);
}

void Buzzer(int freq_hz, int time_ms)
{
    HAL_TIM_Base_Stop(&BuzzerCounterTimerHandle);
    HAL_TIM_PWM_Stop(&BuzzerTimerHandle, TIM_CHANNEL_1);

    BuzzerTimerHandle.Instance = TIM11;
    BuzzerTimerHandle.Init.Period = 100000 / freq_hz;
    BuzzerTimerHandle.Init.Prescaler = (gAPB2Clock*2) / 100000; // Ÿ�̸� Ŭ���� APB ���� Ŭ���� �ι��̴�. (HCLK�� �ѱ� �� ����)
    BuzzerTimerHandle.Init.ClockDivision = 0;
    BuzzerTimerHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
    HAL_TIM_PWM_Init(&BuzzerTimerHandle);

    BuzzerCounterTimerHandle.Instance = TIM9;
    BuzzerCounterTimerHandle.Init.Period = (freq_hz * time_ms) / 1000;
    BuzzerCounterTimerHandle.Init.Prescaler = 0;
    BuzzerCounterTimerHandle.Init.ClockDivision = 0;
    BuzzerCounterTimerHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
    HAL_TIM_Base_Init(&BuzzerCounterTimerHandle);

    HAL_TIM_Base_Start_IT(&BuzzerCounterTimerHandle);
    HAL_TIM_PWM_Start(&BuzzerTimerHandle, TIM_CHANNEL_1);
}