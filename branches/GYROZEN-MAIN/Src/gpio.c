#include "main.h"

//--------------------------------------------------------------

static CQ_DEFINE(unsigned short, KeyBuf, 10);

static bool IgnoreKeyInput = false;
static unsigned long KeyDownStartTick = 0;
static unsigned long KeyLastRepeatTick = 0;

//--------------------------------------------------------------

void Init_GPIO(void);
    void HAL_GPIO_EXTI_Callback(uint16_t pin);

void LED(int idx, bool on);
void LEDToggle(int idx);
void RotorPower(bool on);
void SlipRingPower(bool on);
void MotorFan(bool on);
void ExternalFan(bool on);
void ImbalanceOutToExt(bool on);
void CooloerValve(int idx, bool on);
void CoolerPower(int idx, bool on);
bool CoolerPowerIsOn(int idx);
void DoorOpen(void);
bool DoorIsOpened(void);
bool DoorIsClosed(void);
void DSP_HWReset(void);

void KEY_Flush(void);
void KEY_Process(void);
    unsigned short KEY_Get(void);
    unsigned short KEY_Read(void);

//--------------------------------------------------------------

void DSP_HWReset(void)
{
    FSTART();

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, 0); // _RESET_DSP
    osDelay(100);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, 1); // _RESET_DSP
}

void DoorOpen(void)
{

}

bool DoorIsOpened(void)
{
    return true;
}

bool DoorIsClosed(void)
{
    return true;
}

void KEY_Process(void)
{
    if ( KeyLastRepeatTick == 0 ) // repeat 중이 아니라면
    {
        if ( KeyDownStartTick>0 && TICK_FROM(KeyDownStartTick)>300 ) // repeat 시작 조건 판단: 키가 눌리고서 300ms 경과
        {
            // 만약 키가 하나만 눌려있었다면
            // 만약 잠깐이라도 키가 2개 이상 동시에 눌려있었다면 그 이후로는 모든 키에서 손에 떼어질때까지 아무런 키 이벤트도
            // 발생시키지 않는다
            if ( IgnoreKeyInput == false )
            {
                // 리피트 키 이벤트를 발생시킨다
                CQ_PUT(KeyBuf, KEY_Read());
                KeyLastRepeatTick = gTick;
            }
        }
    }
    else // repeat 중이라면
    {
        if ( IgnoreKeyInput == false && KeyDownStartTick>0 ) // 키가 계속 눌려 있고 다른 키가 눌려지지 않았다면
        {
            if ( TICK_FROM(KeyLastRepeatTick) > 100 ) // 마지막 리피트 키 이벤트 발생으로부터 100ms 시간이 지났다면
            {
                // 리피트 키 이벤트를 발생시킨다
                KeyLastRepeatTick = gTick;
                CQ_PUT(KeyBuf, KEY_Read());
            }
        }
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    if ( pin & KEY_ALL ) // 만약 키 인터럽트라면
    {
        if ( pin & KEY_Read() ) // pin이 의미하는 키가 down
        {
            if ( ~pin & KEY_Read() ) // 만약 pin 키 이외의 다른 키가 눌려있는 상태라면
            {
                // 모든 키에서 손에 떼어질 때까지 어떤 키 이벤트도 발생시키지 않는다
                IgnoreKeyInput = true;
            }
            else
            {
                // 키가 눌려진 순간의 tick을 기록한다
                KeyDownStartTick = gTick;
            }
        }
        else // pin이 의미하는 키가 up
        {
            if ( IgnoreKeyInput == false )
            {
                if ( KeyLastRepeatTick==0 && KeyDownStartTick>0 && TICK_FROM(KeyDownStartTick) > 50 ) // 키가 반복이 안되고 있고 50ms 이상 계속적으로 눌려졌을때
                {
                    switch ( pin )
                    {
                        case GPIO_PIN_7:  CQ_PUT(KeyBuf, KEY_STOP); break;
                        case GPIO_PIN_8:  CQ_PUT(KeyBuf, KEY_START); break;
                        case GPIO_PIN_9:  CQ_PUT(KeyBuf, KEY_DOOR); break;
                        case GPIO_PIN_10: CQ_PUT(KeyBuf, KEY_UP); break;
                        case GPIO_PIN_11: CQ_PUT(KeyBuf, KEY_DOWN); break;
                        case GPIO_PIN_12: CQ_PUT(KeyBuf, KEY_LEFT); break;
                        case GPIO_PIN_13: CQ_PUT(KeyBuf, KEY_RIGHT); break;
                        case GPIO_PIN_14: CQ_PUT(KeyBuf, KEY_ENTER); break;
                        case GPIO_PIN_15: CQ_PUT(KeyBuf, KEY_SAVE); break;
                    }
                }
            }

            if ( KEY_Read() == 0 ) // 모든 키에서 손에 떼어진 순간 상태를 초기화시킨다
            {
                IgnoreKeyInput = false;
                KeyDownStartTick = 0;
                KeyLastRepeatTick = 0;
            }
        }
    }
    else if ( pin & (GPIO_PIN_4|GPIO_PIN_5) ) // DSP_INT, IMBAL_CHECK는 결국 같은 의미이므로 하나로 묶어서 긴급정지로 처리한다
    {
        // 긴급정지
    }

}

unsigned short KEY_Read(void)
{
    return ~(GPIOD->IDR) & KEY_ALL;
}

unsigned short KEY_Get(void)
{
    unsigned short key = KEY_NONE;

    if ( CQ_ISNOTEMPTY(KeyBuf) )
    {
        CQ_GET(KeyBuf, key);
    }

    return key;
}

void KEY_Flush(void)
{
    CQ_FLUSH(KeyBuf);
}

bool CoolerPowerIsOn(int idx)
{
    if ( idx < 0 || idx > 1 )
        return false;

    return HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_6 << idx) == 0 ? true : false;
}

void RotorPower(bool on)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, !on); // ROTOR_PWR
}

void SlipRingPower(bool on)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, !on); // SLIP_SOL
}

void MotorFan(bool on)
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, !on); // MOT_FAN
}

void ExternalFan(bool on)
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, !on); // EXT_FAN
}

void ImbalanceOutToExt(bool on)
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, !on); // GYRO_IMBAL
}

void CooloerValve(int idx, bool on)
{
    if ( idx < 0 || idx > 1 )
        return;

    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4 << idx, !on); // COOLER_SOL1, COOLER_SOL2
}

void CoolerPower(int idx, bool on)
{
    if ( idx < 0 || idx > 1 )
        return;

    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6 << idx, !on); // // COOLER_SSR1, COOLER_SSR2
}

void LED(int idx, bool on)
{
    if ( idx < 0 || idx > 1 )
        return;

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12 << idx, !on); // LED0_PWM, LED1_PWM
}

void LEDToggle(int idx)
{
    if ( idx < 0 || idx > 1 )
        return;

    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_12 << idx); // LED0_PWM, LED1_PWM
}

void Init_GPIO(void)
{
    GPIO_InitTypeDef  gpio_init;

    __GPIOA_CLK_ENABLE();
    __GPIOB_CLK_ENABLE();
    __GPIOC_CLK_ENABLE();
    __GPIOD_CLK_ENABLE();
    __GPIOE_CLK_ENABLE();

    //---------------------------------
    // OUTPUT

    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FAST;

    gpio_init.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7; // DOOR_ACT_RST, DOOR_ACT_DIR, DOOR_ACT_PWM
    HAL_GPIO_Init(GPIOA, &gpio_init);
    HAL_GPIO_WritePin(GPIOA, gpio_init.Pin, 0);

    gpio_init.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9; // ROTOR_PWR, SLIP_SOL, BUZZER
    HAL_GPIO_Init(GPIOB, &gpio_init);
    HAL_GPIO_WritePin(GPIOB, gpio_init.Pin, 1);

    gpio_init.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3 | GPIO_PIN_8 | GPIO_PIN_12 | GPIO_PIN_13; // MOT_FAN, EXT_FAN, _RESET_DSP, GYRO_IMBAL, LED0_PWM, LED1_PWM
    HAL_GPIO_Init(GPIOC, &gpio_init);
    HAL_GPIO_WritePin(GPIOC, gpio_init.Pin, 1); // _RESET_DSP는 LOW_ACTIVE이므로 HIGH로 유지해야 한다

    gpio_init.Pin = GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7; // BLDC_RUNSTOP, COOLER_SOL1, COOLER_SOL2, COOLER_SSR1, COOLER_SSR2
    HAL_GPIO_Init(GPIOD, &gpio_init);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, 0); // BLDC_RUNSTOP
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7, 1); // COOLER_SOL1, COOLER_SOL2, COOLER_SSR1, COOLER_SSR2

    gpio_init.Pin = GPIO_PIN_0 | GPIO_PIN_1; // DOOR_SOL1, DOOL_SOL2
    HAL_GPIO_Init(GPIOE, &gpio_init);
    HAL_GPIO_WritePin(GPIOE, gpio_init.Pin, 1);

    //---------------------------------
    // INPUT

    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = GPIO_NOPULL;

    gpio_init.Pin = GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15; // DOOR_PHOTO 1-6
    HAL_GPIO_Init(GPIOD, &gpio_init);
    //WX(GPIOD->IDR & (GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15));

    gpio_init.Pin = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5; // DOOR_LCHECK_TOP, DOOR_LCHECK_BOT, DOOR_RCHECK_TOP, DOOR_RCHECK_BOT
    HAL_GPIO_Init(GPIOE, &gpio_init);
    //WX(GPIOE->IDR & (GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5));

    //---------------------------------
    // EXTI Interrupt

    gpio_init.Mode = GPIO_MODE_IT_RISING;

#ifndef __RPM_TIMER_CAPTURE__
    gpio_init.Pin = GPIO_PIN_0 | GPIO_PIN_1; // RPM_IN, ROT_ID
    HAL_GPIO_Init(GPIOA, &gpio_init);
    HAL_NVIC_SetPriority(EXTI0_IRQn, 7, 0);
    HAL_NVIC_SetPriority(EXTI1_IRQn, 7, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);
#endif

    gpio_init.Pin = GPIO_PIN_4 | GPIO_PIN_5; // DSP_INT, IMBAL_CHECK
    HAL_GPIO_Init(GPIOC, &gpio_init);

    gpio_init.Mode = GPIO_MODE_IT_RISING_FALLING;
    gpio_init.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10; // KEY_STOP, KEY_START, KEY_DOOR, KEY_UP
    gpio_init.Pin |= GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15; // KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_ENTER, KEY_SAVE
    HAL_GPIO_Init(GPIOE, &gpio_init);

    HAL_NVIC_SetPriority(EXTI4_IRQn, 2, 0);
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 2, 0);
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0);

    HAL_NVIC_EnableIRQ(EXTI4_IRQn);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void EXTI9_5_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(EXTI->PR & 0x3E0);
}

void EXTI15_10_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(EXTI->PR & 0xFC00);
}

#ifndef __RPM_TIMER_CAPTURE__
void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

void EXTI1_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
}
#endif

void EXTI4_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
}

