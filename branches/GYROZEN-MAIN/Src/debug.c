#include "main.h"

//-----------------------------------------------------------

void DebugThread(void const * argument);
static void PrintDebugMenu(void);

//-----------------------------------------------------------

void PrintDebugMenu(void)
{
    DEBUGprintf("\r\n\r\n==========================================\r\n");
    DEBUGprintf("1. write fram\r\n");
    DEBUGprintf("------------------------------------------\r\n");
    DEBUGprintf("2. read fram\r\n");
    DEBUGprintf("==========================================\r\n");
    DEBUGprintf("input command:\r\n");
}

#define MSG_TYPE_REQ_CONN   0x14
#define MSG_TYPE_CONN       0x10

void DebugThread(void const * argument)
{
    char str[10];

    PrintDebugMenu();

    while ( 1 )
    {
        int ch = DEBUGgetc();

        switch ( ch )
        {
            case '1':
            {
                MOTOR_SetRPM(500);
            }
            break;

            case '2':
            {
                MOTOR_Stop();
            }

            /*
            {
                struct TDisplayRun run;
                struct PROGRAM prg;

                UI_SendDispMsg(DISP_RUN, 0, &run, &prg);
            }
            */
            break;

            case '3':
            {
                UI_SendDispMsg(DISP_TEXT, 0, NULL, NULL);
            }
            break;

            case '4':
            {
                WI(LCD_UART->ErrorCode);
                WI(LCD_UART->State);
            }
            break;

            case '7':
            {
                ManageUI();

            }
            break;

            case '8':
            {
                taskENTER_CRITICAL();

            }
            break;

            case '9':
            {
                taskEXIT_CRITICAL();
            }
            break;

            case '0':
            {
                WATCHX("DOOR PHOTO (1~6)", GPIOD->IDR & (GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15)); // DOOR_PHOTO 1-6
                WATCHX("DOOR TOPL,BOTL,TOPR,BOTR", GPIOE->IDR & (GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5)); // DOOR_LCHECK_TOP, DOOR_LCHECK_BOT, DOOR_RCHECK_TOP, DOOR_RCHECK_BOT
            }
            break;

            case '\r':
            {
                PrintDebugMenu();
            }
            break;

            case EOF:
            {
                osDelay(100);
            }
            break;
        }
    }
}
