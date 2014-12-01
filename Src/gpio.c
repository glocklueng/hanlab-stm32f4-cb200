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
    if ( KeyLastRepeatTick == 0 ) // repeat ���� �ƴ϶��
    {
        if ( KeyDownStartTick>0 && TICK_FROM(KeyDownStartTick)>300 ) // repeat ���� ���� �Ǵ�: Ű�� ������ 300ms ���
        {
            // ���� Ű�� �ϳ��� �����־��ٸ�
            // ���� ����̶� Ű�� 2�� �̻� ���ÿ� �����־��ٸ� �� ���ķδ� ��� Ű���� �տ� ������������ �ƹ��� Ű �̺�Ʈ��
            // �߻���Ű�� �ʴ´�
            if ( IgnoreKeyInput == false )
            {
                // ����Ʈ Ű �̺�Ʈ�� �߻���Ų��
                CQ_PUT(KeyBuf, KEY_Read());
                KeyLastRepeatTick = gTick;
            }
        }
    }
    else // repeat ���̶��
    {
        if ( IgnoreKeyInput == false && KeyDownStartTick>0 ) // Ű�� ��� ���� �ְ� �ٸ� Ű�� �������� �ʾҴٸ�
        {
            if ( TICK_FROM(KeyLastRepeatTick) > 100 ) // ������ ����Ʈ Ű �̺�Ʈ �߻����κ��� 100ms �ð��� �����ٸ�
            {
                // ����Ʈ Ű �̺�Ʈ�� �߻���Ų��
                KeyLastRepeatTick = gTick;
                CQ_PUT(KeyBuf, KEY_Read());
            }
        }
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    if ( pin & KEY_ALL ) // ���� Ű ���ͷ�Ʈ���
    {
        if ( pin & KEY_Read() ) // pin�� �ǹ��ϴ� Ű�� down
        {
            if ( ~pin & KEY_Read() ) // ���� pin Ű �̿��� �ٸ� Ű�� �����ִ� ���¶��
            {
                // ��� Ű���� �տ� ������ ������ � Ű �̺�Ʈ�� �߻���Ű�� �ʴ´�
                IgnoreKeyInput = true;
            }
            else
            {
                // Ű�� ������ ������ tick�� ����Ѵ�
                KeyDownStartTick = gTick;
            }
        }
        else // pin�� �ǹ��ϴ� Ű�� up
        {
            if ( IgnoreKeyInput == false )
            {
                if ( KeyLastRepeatTick==0 && KeyDownStartTick>0 && TICK_FROM(KeyDownStartTick) > 50 ) // Ű�� �ݺ��� �ȵǰ� �ְ� 50ms �̻� ��������� ����������
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

            if ( KEY_Read() == 0 ) // ��� Ű���� �տ� ������ ���� ���¸� �ʱ�ȭ��Ų��
            {
                IgnoreKeyInput = false;
                KeyDownStartTick = 0;
                KeyLastRepeatTick = 0;
            }
        }
    }
    else if ( pin & (GPIO_PIN_4|GPIO_PIN_5) ) // DSP_INT, IMBAL_CHECK�� �ᱹ ���� �ǹ��̹Ƿ� �ϳ��� ��� ��������� ó���Ѵ�
    {
        // �������
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
    HAL_GPIO_WritePin(GPIOC, gpio_init.Pin, 1); // _RESET_DSP�� LOW_ACTIVE�̹Ƿ� HIGH�� �����ؾ� �Ѵ�

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

