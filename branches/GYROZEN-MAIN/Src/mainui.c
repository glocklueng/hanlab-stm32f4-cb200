#include "main.h"

//#define __TEST_UI__

/*----------------------------------------------------------------------------*/

int gProgramIdx = 0;
struct PROGRAM gProgram[20] = { { 60, 1000, false, false, 20, 9 }, };

bool gCoolerManualOn = false;
bool DSPTestMode = false;

char gPopup[2][30] = { "READY", "" };

/*----------------------------------------------------------------------------*/

extern void SettingUI(void);

void MainUI(void);
void UpdateMainUI(bool right_now_update);

/*----------------------------------------------------------------------------*/

void UpdateMainUI(bool right_now_update)
{
    char str[15];
    static unsigned long last_ui_update_tick = 0;
    int adjust_temp, adjust_rpm;
    int program_time;

    if ( gRun.status == KEEP_SPEED )
    {
        program_time = TIME_FROM(gRun.keep_start_time);

        if ( program_time > gProgram[gProgramIdx].time )
            program_time = gProgram[gProgramIdx].time;
    }
    else if ( gRun.status == DECEL_SPEED )
    {
        program_time = gProgram[gProgramIdx].time;
    }
    else
    {
        program_time = 0;
    }

    if ( right_now_update || TICK_FROM(last_ui_update_tick) > 50 )
    {
        if ( gSetting.debug )
        {
            /*
            int graph_len = 0;

            LCD_ClearScreen();
            LCD_GraphicPrintf(0, 2, ONLY_CHAR, "RPM");
            LCD_FillRect(25, 0, 102, 9, true);
            LCD_FillRect(26, 1, 100, 7, false);
            LCD_GraphicPrintf(27, 2, ONLY_CHAR, "%i", gRPM);
            LCD_GraphicPrintf(126-24, 2, ONLY_CHAR, "%s", Int2Str(NULL, gProgram[gProgramIdx].rpm, 4, false));
            graph_len = gRPM * 100 / gProgram[gProgramIdx].rpm;
            if ( graph_len > 100 ) graph_len = 100;
            LCD_InvertRect(26, 1, graph_len, 7);

            LCD_GraphicPrintf(0, 12, ONLY_CHAR, "TIME");
            LCD_FillRect(25, 10, 102, 9, true);
            LCD_FillRect(26, 11, 100, 7, false);
            LCD_GraphicPrintf(27, 12, ONLY_CHAR, "%s:%s", Int2Str(NULL, program_time/60, 2, true), Int2Str(str, program_time%60, 2, true));
            LCD_GraphicPrintf(126-30, 12, ONLY_CHAR, "%s:%s", Int2Str(NULL, gProgram[gProgramIdx].time/60, 2, true), Int2Str(str, gProgram[gProgramIdx].time%60, 2, true));
            graph_len = program_time * 100 / gProgram[gProgramIdx].time;
            if ( graph_len > 100 ) graph_len = 100;
            LCD_InvertRect(26, 11, graph_len, 7);

            LCD_GraphicPrintf(0, 23, ONLY_CHAR, "LAST ERROR:%i", gBalancingTaskResult);

            LCD_GraphicPrintf(0, 29, ONLY_CHAR, "ROTOR:%i %i %i", gRotorResult[0], gRotorResult[1], gRotorResult[2]);

            LCD_GraphicPrintf(0, 35, ONLY_CHAR, "vib: %s", Float2Str(NULL, gDSP.vib));
            LCD_GraphicPrintf(0, 41, ONLY_CHAR, "phase: %s", Float2Str(NULL, gDSP.phase));
            LCD_GraphicPrintf(0, 47, ONLY_CHAR, "temp: C(%i) M(%i)", gTemp/100, gTemp2/100);
            LCD_GraphicPrintf(0, 53, ONLY_CHAR, "doorp: %i", gDoorPressure);

            LCD_GraphicPrintf(0, 59, ONLY_CHAR, "MAG%i LT%i LB%i RT%i RB%i",
                GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_4) ? 1 : 0,
                GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_2) ? 1 : 0,
                GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_3) ? 1 : 0,
                GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) ? 1 : 0,
                GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_5) ? 1 : 0);
            */
        }
        else
        {
            /*
            LCD_ClearScreen();

            LCD_FillRect(0, 0, 128, 16, true);
            LCD_FillRect(0, 46, 128, 18, true);

            program_time = gProgram[gProgramIdx].time - program_time;
            LCD_GraphicPrintf2(1, 0, INVERT_CHAR, 8, "%i:%s:%s", program_time/3600, Int2Str(NULL, (program_time%3600)/60, 2, true), Int2Str(str, program_time%60, 2, true));

            if ( gProgram[gProgramIdx].cooler_auto_ctrl )
            {
                if ( gTemp/100 > gProgram[gProgramIdx].temp+3 )
                    adjust_temp = gTemp/100 - 2;
                else if ( gTemp/100 > gProgram[gProgramIdx].temp+1 )
                    adjust_temp = gTemp/100 - 1;
                else if ( gTemp/100 < gProgram[gProgramIdx].temp-3 )
                    adjust_temp = gTemp/100 + 2;
                else if ( gTemp/100 < gProgram[gProgramIdx].temp-1 )
                    adjust_temp = gTemp/100 + 1;
                else
                    adjust_temp = gTemp/100;

                LCD_GraphicPrintf2(65, 0, INVERT_CHAR, 8, "%s", Int2Str(NULL, adjust_temp, 3, false));
                LCD_GraphicPrintf2(93, 0, INVERT_CHAR, 8, "C");
                LCD_GraphicPrintf(104, 2, INVERT_CHAR, "AUTO");
                LCD_GraphicPrintf(104, 9, INVERT_CHAR, "%s'C", Int2Str(NULL, gProgram[gProgramIdx].temp, 2, false));
            }
            else
            {
                LCD_GraphicPrintf2(65, 0, INVERT_CHAR, 8, "%s", Int2Str(NULL, gTemp/100, 3, false));
                LCD_GraphicPrintf2(93, 0, INVERT_CHAR, 8, "C");
                LCD_GraphicPrintf(104, 2, INVERT_CHAR, "MANU" );

                if ( GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_1) )
                    LCD_GraphicPrintf(104, 9, INVERT_CHAR, "*OFF");
                else
                    LCD_GraphicPrintf(107, 9, INVERT_CHAR, "*ON");
            }


            LCD_InvertRect(91, 2, 2, 2);

            if ( gProgram[gProgramIdx].rcf_mode )
                adjust_rpm = RPM2RCF(gRPM);
            else
                adjust_rpm = gRPM;

            if ( adjust_rpm < gProgram[gProgramIdx].rpm+50 && adjust_rpm > gProgram[gProgramIdx].rpm-50 )
                adjust_rpm = gProgram[gProgramIdx].rpm;
            else if ( adjust_rpm >= gProgram[gProgramIdx].rpm+50 )
                adjust_rpm = gRPM - 50;
            else if ( adjust_rpm <= gProgram[gProgramIdx].rpm-50 && adjust_rpm > gProgram[gProgramIdx].rpm-150 )
                adjust_rpm = gRPM + 50;

            LCD_GraphicPrintf2(10, 20, false, 12, "%s", Int2Str(NULL, adjust_rpm, 4, false));
            LCD_GraphicPrintf2(60, 28, false, 8, "/%s", Int2Str(NULL, gProgram[gProgramIdx].rpm, 4, false));
            LCD_GraphicPrintf(103, 36, ONLY_CHAR, gProgram[gProgramIdx].rcf_mode ? "RCF" : "RPM");

            if ( DSPTestMode )
            {
                LCD_GraphicPrintf(0, 49, INVERT_CHAR, "DSP TEST");
                LCD_GraphicPrintf(0, 57, INVERT_CHAR, "%f %f", gDSP.vib, gDSP.phase);
            }
            else
            {
                LCD_GraphicPrintf(0, 49, INVERT_CHAR, gPopup[0]);
                LCD_GraphicPrintf(0, 57, INVERT_CHAR, gPopup[1]);
            }
            */
        }

        //LCD_Update();
        last_ui_update_tick = gTick;
    }
}

void MainUI(void)
{
restart:
    UpdateMainUI(true);

    while ( 1 )
    {
        int key = KEY_Read();

        UpdateMainUI(false);
        BackgroundProcess(false);

        if ( key == (KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT) || key == (KEY_STOP|KEY_ENTER) )
        {
            KEY_Flush();
            ManageUI();
        }
        else if ( key == (KEY_STOP|KEY_DOWN) )
        {
            KEY_Flush();
            RotorCheckTask(0);
        }
        else if ( key == (KEY_STOP|KEY_RIGHT) )
        {
            KEY_Flush();
            RotorCheckTask(1);
        }
        else if ( key == (KEY_STOP|KEY_UP) )
        {
            KEY_Flush();
            if ( DSPTestMode )
            {
                DSPTestMode = false;
                DSP_Stop();
            }
            else
            {
                DSPTestMode = true;
                DSP_Start();
            }
        }
        else if ( key == (KEY_STOP|KEY_SAVE) )
        {
            KEY_Flush();
            GetConstantTask(1000, true);
        }

        if ( (key = KEY_Get()) != KEY_NONE )
        {
            switch ( key )
            {
                case KEY_START:
                    AutoBalancingTask(gProgram[gProgramIdx].rpm, gProgram[gProgramIdx].time);
                    break;

                case KEY_STOP:
                    DPUTS("MAINUI: KEY_STOP");
                    gCancel = true;
                    MOTOR_Stop();

                    if ( gRPM == 0 )
                    {
                        strcpy(gPopup[0], "READY");
                        gPopup[1][0] = NULL;
                    }
                    break;

                case KEY_UP:
                    if ( gProgramIdx < 19 )
                        gProgramIdx += 1;
                    else
                        gProgramIdx = 0;
                    UpdateMainUI(true);
                    break;

                case KEY_DOWN:
                    if ( gProgramIdx > 0 )
                        gProgramIdx -= 1;
                    else
                        gProgramIdx = 19;
                    UpdateMainUI(true);
                    break;

                case KEY_DOOR:
                    DoorOpen();
                    break;

                case KEY_ENTER:
                    SettingUI();
                    goto restart;

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

