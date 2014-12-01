#include "main.h"
#include "cooler.h"

//-------------------------------------------

int gCoolerTemp = 20;
volatile bool gPrintCoolerLog = false;
bool gCoolerAutoControl = true;

//-------------------------------------------

bool CoolerPowerStatus(void);
void CoolerControl(bool on);
void Cooler_Process(void);

//-------------------------------------------

bool CoolerPowerStatus(void)
{
    return CoolerPowerIsOn(0);
}

void CoolerControl(bool on)
{
    if ( !gCoolerAutoControl )
    {
        if ( !CoolerPowerStatus() && on )
        {
            CoolerPower(0, true);
            if ( gPrintCoolerLog ) DPRINTF(("[%i] COOLER ON", gSeconds));
        }
        else if ( CoolerPowerStatus() && !on )
        {
            CoolerPower(0, false);
            if ( gPrintCoolerLog ) DPRINTF(("[%i] COOLER OFF", gSeconds));
        }
    }
    else
    {
        DPUTS("ERROR: COOLER AUTOMATIC CONTROL");
    }
}

void Cooler_Process(void)
{
    int adjust_rpm;

    if ( gCoolerAutoControl )
    {
        if ( !CoolerPowerStatus() ) // 쿨러가 꺼있을 경우
        {
            if ( gProgramRunning ) // 프로그램이 동작중
            {
                if ( gProgram[gProgramIdx].rcf_mode )
                    adjust_rpm = RCF2RPM(gProgram[gProgramIdx].rpm);
                else
                    adjust_rpm = gProgram[gProgramIdx].rpm;

                if ( GetChamberTemp() >= (gCoolerTemp*100) ) // 0 ~ 3799
                {
                    CoolerPower(0, true);
                    if ( gPrintCoolerLog ) DPRINTF(("[%i] COOLER AUTO ON", gSeconds));
                }
                else if ( adjust_rpm >= 3800 && GetChamberTemp() >= (gCoolerTemp*100)-100 ) // 3800 ~ 3999
                {
                    CoolerPower(0, true);
                    if ( gPrintCoolerLog ) DPRINTF(("[%i] COOLER AUTO ON", gSeconds));
                }
                else if ( adjust_rpm >= 4000 && GetChamberTemp() >= (gCoolerTemp*100)-200 ) // 4000 ~ 4199
                {
                    CoolerPower(0, true);
                    if ( gPrintCoolerLog ) DPRINTF(("[%i] COOLER AUTO ON", gSeconds));
                }
                else if ( adjust_rpm >= 4200 && GetChamberTemp() >= (gCoolerTemp*100)-300 ) // 4200 ~ 4299
                {
                    CoolerPower(0, true);
                    if ( gPrintCoolerLog ) DPRINTF(("[%i] COOLER AUTO ON", gSeconds));
                }
                else if ( adjust_rpm >= 4300 && GetChamberTemp() >= (gCoolerTemp*100)-400 ) // 4300 ~ 4399
                {
                    CoolerPower(0, true);
                    if ( gPrintCoolerLog ) DPRINTF(("[%i] COOLER AUTO ON", gSeconds));
                }
                else if ( adjust_rpm >= 4400 && GetChamberTemp() >= (gCoolerTemp*100)-500 ) // 4400 ~ 4499
                {
                    CoolerPower(0, true);
                    if ( gPrintCoolerLog ) DPRINTF(("[%i] COOLER AUTO ON", gSeconds));
                }
                else if ( adjust_rpm == 4500 && GetChamberTemp() >= (gCoolerTemp*100)-600 ) // 4500
                {
                    CoolerPower(0, true);
                    if ( gPrintCoolerLog ) DPRINTF(("[%i] COOLER AUTO ON", gSeconds));
                }
            }
            else if ( !gProgramRunning && GetChamberTemp() >= (gCoolerTemp*100)-200 ) // 대기중 (프리쿨링중)
            {
                CoolerPower(0, true);
                if ( gPrintCoolerLog ) DPRINTF(("[%i] COOLER AUTO ON", gSeconds));
            }
        }
        else if ( CoolerPowerStatus() ) // 쿨러가 켜있을 경우
        {
            if ( gProgram[gProgramIdx].rcf_mode )
                adjust_rpm = RCF2RPM(gProgram[gProgramIdx].rpm);
            else
                adjust_rpm = gProgram[gProgramIdx].rpm;

            if ( gProgramRunning ) // 프로그램이 동작중
            {
                if ( adjust_rpm < 3800 && GetChamberTemp() <= (gCoolerTemp*100)-100 ) // 0 ~ 3799
                {
                    CoolerPower(0, false);
                    if ( gPrintCoolerLog ) DPRINTF(("[%i] COOLER AUTO OFF", gSeconds));
                }
                else if ( adjust_rpm < 4000 && GetChamberTemp() <= (gCoolerTemp*100)-200 ) // 3800 ~ 3999
                {
                    CoolerPower(0, false);
                    if ( gPrintCoolerLog ) DPRINTF(("[%i] COOLER AUTO OFF", gSeconds));
                }
                else if ( adjust_rpm < 4200 && GetChamberTemp() <= (gCoolerTemp*100)-300 ) // 4000 ~ 4199
                {
                    CoolerPower(0, false);
                    if ( gPrintCoolerLog ) DPRINTF(("[%i] COOLER AUTO OFF", gSeconds));
                }
                else if ( adjust_rpm < 4300 && GetChamberTemp() <= (gCoolerTemp*100)-400 ) // 4200 ~ 4299
                {
                    CoolerPower(0, false);
                    if ( gPrintCoolerLog ) DPRINTF(("[%i] COOLER AUTO OFF", gSeconds));
                }
                else if ( adjust_rpm < 4400 && GetChamberTemp() <= (gCoolerTemp*100)-500 ) // 4300 ~ 4399
                {
                    CoolerPower(0, false);
                    if ( gPrintCoolerLog ) DPRINTF(("[%i] COOLER AUTO OFF", gSeconds));
                }
                else if ( adjust_rpm >= 4400 && GetChamberTemp() <= (gCoolerTemp*100)-600 ) // 4400 ~ 4499
                {
                    CoolerPower(0, false);
                    if ( gPrintCoolerLog ) DPRINTF(("[%i] COOLER AUTO OFF", gSeconds));
                }
                else if ( adjust_rpm == 4500 && GetChamberTemp() <= (gCoolerTemp*100)-700 ) // 4500
                {
                    CoolerPower(0, false);
                    if ( gPrintCoolerLog ) DPRINTF(("[%i] COOLER AUTO OFF", gSeconds));
                }
            }
            else if ( !gProgramRunning ) // 대기중 (프리쿨링중)
            {
                // 프로그램이 동작중이 아닐때는 최종 RPM에 따라 프리 쿨링을 한다
                if ( adjust_rpm < 3800 && GetChamberTemp() <= (gCoolerTemp*100)-200 ) // 0 ~ 3799
                {
                    CoolerPower(0, false);
                    if ( gPrintCoolerLog ) DPRINTF(("[%i] COOLER AUTO OFF", gSeconds));
                }
                else if ( adjust_rpm >= 3800 && GetChamberTemp() <= (gCoolerTemp*100)-300 ) // 3800 ~ 3999
                {
                    CoolerPower(0, false);
                    if ( gPrintCoolerLog ) DPRINTF(("[%i] COOLER AUTO OFF", gSeconds));
                }
                else if ( adjust_rpm >= 4000 && GetChamberTemp() <= (gCoolerTemp*100)-400 ) // 4000 ~ 4199
                {
                    CoolerPower(0, false);
                    if ( gPrintCoolerLog ) DPRINTF(("[%i] COOLER AUTO OFF", gSeconds));
                }
                else if ( adjust_rpm >= 4200 && GetChamberTemp() <= (gCoolerTemp*100)-500 ) // 4200 ~ 4399
                {
                    CoolerPower(0, false);
                    if ( gPrintCoolerLog ) DPRINTF(("[%i] COOLER AUTO OFF", gSeconds));
                }
                else if ( adjust_rpm >= 4400 && GetChamberTemp() <= (gCoolerTemp*100)-600 ) // 4400 ~ 4500
                {
                    CoolerPower(0, false);
                    if ( gPrintCoolerLog ) DPRINTF(("[%i] COOLER AUTO OFF", gSeconds));
                }
            }
        }
    }
    else
    {
        if ( GetChamberTemp() >= 3500 && !CoolerPowerStatus() )
        {
            CoolerPower(0, true);
            if ( gPrintCoolerLog ) DPRINTF(("[%i] COOLER AUTO ON", gSeconds));
        }
        else if ( GetChamberTemp() <= -400 && CoolerPowerStatus() )
        {
            CoolerPower(0, false);
            if ( gPrintCoolerLog ) DPRINTF(("[%i] COOLER AUTO OFF", gSeconds));
        }
    }
}