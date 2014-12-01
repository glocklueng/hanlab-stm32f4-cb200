#include "main.h"

//--------------------------------------------------------------------------------

volatile bool gDebugMode = false;
volatile bool gCancel = false;

unsigned long gAHBClock;
unsigned long gAPB1Clock;
unsigned long gAPB2Clock;
volatile unsigned long gSeconds = 0;
volatile unsigned long gTick = 0;

//--------------------------------------------------------------------------------

int main(void);
    static int SystemClock_Config(void);
    static void MainThread(void const * argument);
        void BackgroundProcess(bool common_key_process);
    static void Error_Handler(void);

void SysTick_Handler(void);
    uint32_t HAL_GetTick(void);

//--------------------------------------------------------------------------------

/*
    ���� Cube RTOS�� ������ �������� �ʾƼ� osWait() �Լ��� �̱����Ǿ� �ִ�.
    �׷��Ƿ� �켱 ������ ���� �����尡 �ڽ��� ������ �Ϸ��ϰ� ���� ���� ������ �����忡��
    ���� ������ �纸�� ����� ����. (osDelay() �Լ��� �ܼ��� �������ϻ��̴�)
    �׷��Ƿ� ��� �����忡 ���� �켱������ �־ round robin ��� �����쿡 ������ �� �ۿ� ����.
*/

osSemaphoreDefine(UART_PRINTF_SEMAPHOR);
osThreadDefine(MAIN_THREAD, MainThread, osPriorityNormal, 0, 2 * configMINIMAL_STACK_SIZE);
osThreadDefine(DEBUG_THREAD, DebugThread, osPriorityNormal, 0, 2 * configMINIMAL_STACK_SIZE);
osThreadDefine(MOTOR_THREAD, NEXDMotor_CheckMotorStatusThread, osPriorityNormal, 0, 2 * configMINIMAL_STACK_SIZE);

//--------------------------------------------------------------------------------

void BackgroundProcess(bool common_key_process)
{
    MsgRecvHandler();
    GyrozenProtocolProcess();
    RB_Process();
    KEY_Process();
    Run_Process();
    Cooler_Process();
    DSP_Process();

    // ��� ��쿡�� �����ؾ� �ϴ� �⺻Ű ó��
    if ( common_key_process )
    {
        unsigned short key;

        if ( (key = KEY_Get()) != KEY_NONE )
        {
            switch ( key )
            {
                case KEY_STOP:
                    DPUTS("USER CANCEL");
                    gCancel = true;
                    break;

                case KEY_SAVE:
                    if ( !gCoolerAutoControl )
                    {
                        if ( CoolerPowerStatus() )
                        {
                            CoolerControl(false);
                            UpdateMainUI(true);
                        }
                        else
                        {
                            CoolerControl(true);
                            UpdateMainUI(true);
                        }
                    }
                    break;
            }
        }
    }
}

static void MainThread(void const * argument)
{
    //MainUI();
    while ( 1 ) BackgroundProcess(true);
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    Init_GYROZENUART();
    Init_LCDUART();
    Init_DEBUGUART(); // ����� ����� ���� ���� ���� �ʱ�ȭ�Ѵ�
    DPUTS("==== SYSTEM INIT START ===============================================================");
    Init_GPIO();
    Init_FRAM();
    Init_TEMP();
    Init_PWM();
    Init_BUZZER();
    Init_CAN_MOTOR();
    Init_CAN_ROTOR();
#ifdef __RPM_TIMER_CAPTURE__
    Init_RPM();
#endif
    DPUTS("==== SYSTEM INIT END =================================================================");
    WI(SystemCoreClock);

    UART_PRINTF_SEMAPHOR = osSemaphoreCreate(osSemaphore(UART_PRINTF_SEMAPHOR), 1);

    MAIN_THREAD = osThreadCreate(osThread(MAIN_THREAD), NULL);
    DEBUG_THREAD = osThreadCreate(osThread(DEBUG_THREAD), NULL);
    MOTOR_THREAD = osThreadCreate(osThread(MOTOR_THREAD), NULL);

    osKernelStart(NULL, NULL);
    while ( 1 );
}

static int SystemClock_Config(void)
{
    /*
    HSE_VALUE    8000000   (���� �ܺ� ũ����Ż - ���忡 ���� �޶���)
    HSI_VALUE    16000000  (���� ũ����Ż)
    PLL_VCO =( (HSE_VALUE or HSI_VALUE) / PLL_M) * PLL_N
    USB OTG FS, SDIO and RNG Clock =  PLL_VCO / PLLQ (48Mhz)
    SYSCLK = PLL_VCO / PLL_P
    HCLK = SYSCLK / AHBCLKDivider (1)
    PB1CLK = HCLK / APB1CLKDivider (4)
    PB2CLK = HCLK / APB2CLKDivider (2)
    */

    int result;

    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_OscInitTypeDef RCC_OscInitStruct;

    __PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;

    if ( (result=HAL_RCC_OscConfig(&RCC_OscInitStruct)) != HAL_OK )
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if ( (result=HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5)) != HAL_OK )
    {
        Error_Handler();
    }

    gAHBClock = SystemCoreClock;
    gAPB1Clock = SystemCoreClock / 4;
    gAPB2Clock = SystemCoreClock / 2;

    return result;
}

static void Error_Handler(void)
{
    while(1)
    {
    }
}

uint32_t HAL_GetTick(void)
{
  return gTick;
}

void SysTick_Handler(void)
{
    static int led_toggle_tick = 0;
    static int msg_send_tick = 0;
    extern void xPortSysTickHandler(void);

    if ( ++led_toggle_tick == configTICK_RATE_HZ ) // 1��
    {
        led_toggle_tick = 0;
        LEDToggle(0);
        gSeconds++;
    }

    gTick++;

    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        xPortSysTickHandler();
    }

    if ( ++msg_send_tick == 10 )
    {
        msg_send_tick = 0;
        MsgSendHandler();
    }
}

void NMI_Handler(void)
{
}

void HardFault_Handler(void)
{
    while (1)
    {
    }
}

void MemManage_Handler(void)
{
    while (1)
    {
    }
}

void BusFault_Handler(void)
{
    while (1)
    {
    }
}

void UsageFault_Handler(void)
{
    while (1)
    {
    }
}

void DebugMon_Handler(void)
{
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line)
{
    while (1)
    {
    }
}
#endif
