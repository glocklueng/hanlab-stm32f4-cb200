#include <math.h>
#include "main.h"
#include "run.h"

struct RUN_INTERFACE gRun = { 0, };

//------------------------------------------------------------

void Run_Process(void);
    static bool RPM_ChangeCheck(bool dir_accel, bool init);
void Run_Start(int rpm, int time);
void Run_GetVibPhase(int rpm, float limit_vib);
void Run_Stop(void);

//------------------------------------------------------------

void Run_Process(void)
{
    static int vibdata_cnt = 0;
    static int tongtong_skip_cnt = 1;
    static float vibdata[2][6];

    // RPM이 ZERO가 되는 순간 로그 출력
    {
        static int last_rpm = 0;

        if ( last_rpm != 0 && gRPM == 0 )
        {
            DPRINTF(("[%i] RPM ZERO", gSeconds));

            if ( gCoolerStopWhenRPMZero )
            {
                gCoolerStopWhenRPMZero = false;
//                CoolerControl(0);
            }
        }

        last_rpm = gRPM;
    }

    // 상태가 바뀌는 순간 내부 static 변수 초기화
    {
        static int last_status = MOTOR_STOP;

        if ( last_status != MEASURE_VIB && gRun.status == MEASURE_VIB )
        {
            vibdata_cnt = 0;
            tongtong_skip_cnt = 1;
        }

        last_status = gRun.status;
    }

    // 에러 체크 및 상태 변화
    if ( gRun.status != MOTOR_STOP && gRun.status != DECEL_SPEED && gCancel )
    {
        DPUTS("CANCEL");
        Run_Stop();
        gRun.result = ERROR_USER_CANCEL;
        return;
    }
    else if ( gRun.status == ACCEL_SPEED || gRun.status == MEASURE_VIB || gRun.status == KEEP_SPEED )
    {
        if ( TIME_FROM(gRun.run_start_time) > 10 && gRPM == 0 )
        {
            WATCHI("RPM ERROR", gRPM);
            gRun.result = ERROR_MOTOR_HALL_SENSOR;
            Run_Stop();
            return;
        }
        else if ( gProgramIdx!=19 && !gDSP.alive )
        {
            DPUTS("DSP POWER ERROR");
            gRun.result = ERROR_DSP_POWER;
            Run_Stop();
            return;
        }
        else if ( gDSP.imbalance_occur )
        {
            DPUTS("IMBALANCE OCCUR");
            gRun.result = ERROR_MOTOR_IMBALANCE;
            Run_Stop();
            return;
        }
        else if ( gProgramIdx!=19 && gDSP.dsp_no_ack )
        {
            DPUTS("DSP NOT ACK");
            gRun.result = ERROR_DSP_NOT_RESPONDING;
            Run_Stop();
            return;
        }
        else if ( gProgramIdx!=19 && gDSP.vib_sensor_no_exist )
        {
            DPUTS("DSP NOT EXIST");
            gRun.result = ERROR_DSP_NOT_EXIST_VIB_SENSOR;
            Run_Stop();
            return;
        }
        else if ( !gSetting.debug && !DoorIsClosed() )
        {
            gRun.result = ERROR_DOOR_OPEN;
            Run_Stop();
            return;
        }
        else if ( gNEXDMotor_Status.error & (NEXD_ERROR_OVER_VOLTAGE|NEXD_ERROR_OVER_VOLTAGE2) )
        {
            gRun.result = ERROR_MOTOR_OVER_VOLTAGE;
            Run_Stop();
            return;
        }
        else if ( gNEXDMotor_Status.error & NEXD_ERROR_OVER_HEAT_CONTROLLER )
        {
            gRun.result = ERROR_MOTOR_OVER_HEAT_CTRLER;
            Run_Stop();
            return;
        }
        else if ( gNEXDMotor_Status.error & NEXD_ERROR_OVER_HEAT_MOTOR )
        {
            gRun.result = ERROR_MOTOR_OVER_HEAT_MOTOR;
            Run_Stop();
            return;
        }
        else if ( gNEXDMotor_Status.error & NEXD_ERROR_LOW_VOLTAGE )
        {
            gRun.result = ERROR_MOTOR_LOW_VOLTAGE;
            Run_Stop();
            return;
        }
        else if ( gNEXDMotor_Status.error & (NEXD_ERROR_OVER_CURRENT|NEXD_ERROR_OVER_CURRENT2) )
        {
            gRun.result = ERROR_MOTOR_OVER_CURRENT;
            Run_Stop();
            return;
        }
        else if ( gProgramIdx!=19 && GetMotorTemp() > 11000 )
        {
            DPRINTF(("ERROR_MOTOR_OVER_HEAT_MOTOR at TEMP %i", GetMotorTemp()));
            gRun.result = ERROR_MOTOR_OVER_HEAT_MOTOR;
            Run_Stop();
            return;
        }
        else if ( gProgramIdx!=19 && GetChamberTemp() >= 5000 )
        {
            DPRINTF(("ERROR_TEMP_CONTROL at TEMP %i", GetChamberTemp()));
            gRun.result = ERROR_TEMP_CONTROL;
            Run_Stop();
            return;
        }

        //--------------------------------------------------------------------------

        if ( gRun.status == ACCEL_SPEED )
        {
            if ( gRPM < 1000 && RPM_ChangeCheck(true, false) == false )
            {
                gRun.result = ERROR_MOTOR_UNDER_ACCEL;
                Run_Stop();
                return;
            }
            else if ( !gRun.time_mode && gRPM >= gRun.final_rpm)
            {
                DPRINTF(("[%i] MEASURE_VIB: %i", gSeconds, gRPM));
                gRun.status = MEASURE_VIB;
                gRun.keep_start_time = gSeconds;
            }
            else if ( gRun.time_mode && gRPM >= gRun.final_rpm )
            {
                DPRINTF(("[%i] KEEP_SPEED: %i", gSeconds, gRPM));
                gRun.status = KEEP_SPEED;
                gRun.keep_start_time = gSeconds;
            }
        }
        else // KEEP_SPEED or MEASURE_VIB
        {
            if ( gRPM > gRun.final_rpm*1.5 || gRPM > gRun.final_rpm+200 )
            {
                DPRINTF(("ERROR_MOTOR_OVER_ACCEL at RPM %i", gRPM));
                gRun.result = ERROR_MOTOR_OVER_ACCEL;
                Run_Stop();
                return;
            }
            else
            if ( gRPM < gRun.final_rpm*0.8 )
            {
                DPRINTF(("ERROR_MOTOR_CANNOT_KEEP at RPM %i", gRPM));
                gRun.result = ERROR_MOTOR_CANNOT_KEEP;
                Run_Stop();
                return;
            }
            else if ( gRun.time_mode && TIME_FROM(gRun.keep_start_time) >= gRun.keep_time )
            {
                DPRINTF(("[%i] RUN_TIME_OUT, MOTOR_TEMP(%i)", gSeconds, GetChamberTemp()));
                gRun.result = SUCCESS;
                Run_Stop();
                return;
            }
            else if ( gRun.status == MEASURE_VIB )
            {
                if ( TIME_FROM(gRun.keep_start_time) > 60 )
                {
                    DPUTS("DSP TIMEOUT");
                    gRun.result = ERROR_DSP_TIMEOUT;
                    Run_Stop();
                    return;
                }
                else if ( gDSP.msg_catch )
                {
                    int rpm_limit, rpm_cnt;

                    gDSP.msg_catch = false;

                    if ( gRun.final_rpm <= 500 )
                    {
                        rpm_limit = 50;
                        rpm_cnt = 6;
                    }
                    else if ( gRun.final_rpm <= 700 )
                    {
                        rpm_limit = 35;
                        rpm_cnt = 6;
                    }
                    else if ( gRun.final_rpm <= 1200 )
                    {
                        rpm_limit = 30;
                        rpm_cnt = 6;
                    }
                    else if ( gRun.final_rpm <= 2200 )
                    {
                        rpm_limit = 40;
                        rpm_cnt = 6;
                    }
                    else
                    {
                        rpm_limit = 100;
                        rpm_cnt = 6;
                    }

                    if ( gDSP.vib == 0 || gDSP.phase == 0 )
                    {
                        // nothing to do
                    }
                    else if ( gRun.limit_vib > 0 && gDSP.vib > gRun.limit_vib )
                    {
                        DPRINTF(("%i\t%s\t%s", gRun.final_rpm, F2S(gDSP.vib), F2S(gDSP.phase)));

                        gRun.vib = gDSP.vib;
                        gRun.phase = gDSP.phase;

                        gRun.result = ERROR_VIB_TOO_MUCH;
                        Run_Stop();
                        return;
                    }
                    else if ( fabs(gDSP.rpm_change) <= rpm_limit )
                    {
                        vibdata[0][vibdata_cnt] = gDSP.vib;
                        vibdata[1][vibdata_cnt] = gDSP.phase;
                        vibdata_cnt++;
                        tongtong_skip_cnt = 2;
                    }
                    else
                    {
                        if ( tongtong_skip_cnt > 0 )
                            tongtong_skip_cnt--;
                        else
                            vibdata_cnt = 0;
                    }

                    if ( vibdata_cnt >= rpm_cnt )
                    {
                        gRun.vib = GetAverageFloat(vibdata[0], 3, false);;
                        gRun.phase = GetAverageFloat(vibdata[1], 3, true);;

                        DPRINTF(("%i\t%s\t%s", gRun.final_rpm, F2S(gRun.vib), F2S(gRun.phase)));

                        gRun.status = KEEP_SPEED;
                    }
                }
            }
        }
    }
    else if ( gRun.status == DECEL_SPEED )
    {
        if ( gRPM == 0 )
        {
            gRun.status = MOTOR_STOP;
        }
    }
}

void Run_Start(int rpm, int time)
{
    //FSTART();

    if ( gNEXDMotor_Status.error )
        MOTOR_Reset();

    gImbalanceDetectDisable = false;

    gRun.result = SUCCESS;
    gRun.final_rpm = rpm;
    gRun.run_start_time = gSeconds;
    gRun.keep_start_time = gSeconds;
    gRun.time_mode = true;
    gRun.keep_time = time;
    gRun.limit_vib = 0;
    gRun.send_rpm = rpm;

    RPM_ChangeCheck(true, true);

    WI(gRun.send_rpm);

    if ( gRPM >= rpm )
    {
        gRun.status = KEEP_SPEED;
        gRun.keep_start_time = gSeconds;

        MOTOR_SetRPM(gRun.send_rpm);
    }
    else
    {
        gRun.status = ACCEL_SPEED;

        if ( gRPM == 0 )
        {
            MOTOR_SetRPM(0);
            MOTOR_Run();
        }

        MOTOR_SetRPM(gRun.send_rpm);
    }
}

void Run_GetVibPhase(int rpm, float limit_vib)
{
    //FSTART();

    if ( gNEXDMotor_Status.error )
        MOTOR_Reset();

    gImbalanceDetectDisable = false;

    gRun.result = SUCCESS;
    gRun.final_rpm = rpm;
    gRun.run_start_time = gSeconds;
    gRun.keep_start_time = gSeconds;
    gRun.time_mode = false;
    gRun.keep_time = 0;
    gRun.limit_vib = limit_vib;

    RPM_ChangeCheck(true, true);

    if ( gRPM >= rpm )
    {
        gRun.status = MEASURE_VIB;
        MOTOR_SetRPM(rpm);
    }
    else
    {
        gRun.status = ACCEL_SPEED;

        if ( gRPM == 0 )
        {
            MOTOR_SetRPM(0);
            MOTOR_Run();
        }

        MOTOR_SetRPM(rpm);
    }

    gDSP.rpm_change = 9999;
}

void Run_Stop()
{
    //FSTART();

    gImbalanceDetectDisable = true;

    if ( gRPM > 0 )
    {
        gRun.status = DECEL_SPEED;
    }
    else
    {
        gRun.status = MOTOR_STOP;
    }

    MOTOR_Stop();
}

bool RPM_ChangeCheck(bool dir_accel, bool init)
{
    static short rpm[8];
    static unsigned long last_time;

    if ( init )
    {
        last_time = gSeconds;
        if ( dir_accel == TRUE )
            rpm[7] = rpm[6] = rpm[5] = rpm[4] = rpm[3] = rpm[2] = rpm[1] = rpm[0] = -1;
        else
            rpm[7] = rpm[6] = rpm[5] = rpm[4] = rpm[3] = rpm[2] = rpm[1] = rpm[0] = 32000;
    }
    else
    {
        /*
        if ( (dir_accel && (int)gRPM<=rpm[0]) || (!dir_accel && (int)gRPM>rpm[0]) )
        {
            DPUTS("RPM_CHANGE_ERROR");
            WB(dir_accel);
            WI(rpm[0]);
            WI(gRPM);
            RETURN(false);
        }
        */

        int err_cnt = 0;
        if ( dir_accel && (int)gRPM <= rpm[0] )
        {
            for ( int i=0 ; i<7 ; i++ )
            {
                if ( (int)gRPM <= rpm[i] )
                    err_cnt++;
            }

            if ( err_cnt >= 3 )
            {
                DPUTS("RPM_CHANGE_ERROR");
                WB(dir_accel);
                WI(rpm[0]);
                WI(gRPM);
                RETURN(false);
            }
        }
        else if ( !dir_accel && (int)gRPM >= rpm[0] )
        {
            for ( int i=0 ; i<7 ; i++ )
            {
                if ( (int)gRPM >= rpm[i] )
                    err_cnt++;
            }

            if ( err_cnt >= 3 )
            {
                DPUTS("RPM_CHANGE_ERROR");
                WB(dir_accel);
                WI(rpm[0]);
                WI(gRPM);
                RETURN(false);
            }
        }

        if ( last_time != gSeconds )
        {
            last_time = gSeconds;

            rpm[0] = rpm[1];
            rpm[1] = rpm[2];
            rpm[2] = rpm[3];
            rpm[3] = rpm[4];
            rpm[4] = rpm[5];
            rpm[5] = rpm[6];
            rpm[6] = rpm[7];
            rpm[7] = gRPM;
        }
    }

    return true;
}
