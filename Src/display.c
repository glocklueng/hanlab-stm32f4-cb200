#include "main.h"
#include "display.h"

#define CB200R_SWING_RADIUS 190

//---------------------------------------------------------------------

volatile int gRunStatus = 0;
volatile bool gDisplayRCF = FALSE;
volatile struct DEBUG_DATA gDebugData = { 0, };

//---------------------------------------------------------------------

void UI_CustomPopup(int ui_state, char* title, char* text, ...)
{
    char buf[200];
    va_list args;
    unsigned char err;

    if ( gUI_State != UI_MANAGE )
    {
        FSTART();

        va_start(args, text);
        vsprintf(buf, text, args);
        va_end(args);

        gUI_State = ui_state;
        UI_SendDispMsg(DISP_POPUP_TITLE_START, 0, title, 0);
        UI_SendDispMsg(DISP_POPUP_TEXT_START, 0, buf, 0);
    }
}

void UI_SendDispMsg(unsigned char cmd, unsigned char option, void* data, struct PROGRAM* cur_prg)
{
    unsigned char buf[DATA_MSG_MAX_LEN];
    static int rpm[5] = { 0, };
    static int rpm_idx = 0;
    unsigned char err;

    memset(buf, 0, DATA_MSG_MAX_LEN);
    buf[0] = cmd;

    switch ( cmd )
    {
        case DISP_POPUP_TITLE_START:
            {
                int send_len=0, len = strlen(data);
                while ( len > DATA_MSG_MAX_LEN-2 )
                {
                    if ( send_len > 0 ) buf[0] = DISP_POPUP_TITLE_CONTINUE;
                    buf[1] = DATA_MSG_MAX_LEN-2;
                    memcpy(&buf[2], ((unsigned char*)data)+send_len, DATA_MSG_MAX_LEN-2);
                    send_len += DATA_MSG_MAX_LEN-2;
                    len -= DATA_MSG_MAX_LEN-2;
                    MsgSend(MSG_TYPE_DISPLAY, buf, DATA_MSG_MAX_LEN);
                }
                if ( send_len > 0 ) buf[0] = DISP_POPUP_TITLE_CONTINUE;
                buf[1] = len;
                memcpy(&buf[2], ((unsigned char*)data)+send_len, len);
                MsgSend(MSG_TYPE_DISPLAY, buf, DATA_MSG_MAX_LEN);
            }
            return;

        case DISP_POPUP_TEXT_START:
            {
                int send_len=0, len = strlen(data);
                while ( len > DATA_MSG_MAX_LEN-2 )
                {
                    if ( send_len > 0 ) buf[0] = DISP_POPUP_TEXT_CONTINUE;
                    buf[1] = DATA_MSG_MAX_LEN-2;
                    memcpy(&buf[2], ((unsigned char*)data)+send_len, DATA_MSG_MAX_LEN-2);
                    send_len += DATA_MSG_MAX_LEN-2;
                    len -= DATA_MSG_MAX_LEN-2;
                    MsgSend(MSG_TYPE_DISPLAY, buf, DATA_MSG_MAX_LEN);
                }
                if ( send_len > 0 ) buf[0] = DISP_POPUP_TEXT_CONTINUE;
                buf[1] = len;
                memcpy(&buf[2], (unsigned char*)data+send_len, len);
                MsgSend(MSG_TYPE_DISPLAY, buf, DATA_MSG_MAX_LEN);
            }
            return;

        case DISP_NETWORK_SETTING:
            memcpy(&buf[1], data, sizeof(struct TDisplayNetworkSetting));
            break;

        case DISP_BOOT:
            {
                struct TDisplayBoot param;
                memset(&param, 0, sizeof(param));

                if ( data )
                    memcpy(&param, data, sizeof(param));

                param.popup = option;

                if ( gHaveCooler ) param.disp_option |= DISP_OPTION_HAVE_COOLER;

                memcpy(&buf[1], &param, sizeof(param));
            }
            break;

        case DISP_RUN:
            {
                struct TDisplayRun param;
                memset(&param, 0, sizeof(param));

                if ( data )
                    memcpy(&param, data, sizeof(param));

                if ( cur_prg )
                {
                    param.status_and_rotor_type |= (cur_prg->rotor_type+1) << 5;
                    param.accel_and_decel = (cur_prg->accel+1) << 4 | (cur_prg->decel+1);

                    if ( cur_prg->rcf_mode != gDisplayRCF )
                    {
                        if ( cur_prg->rcf_mode )
                        {
                            param.end_rpm = RCF2RPM(cur_prg->rpm);
                        }
                        else
                        {
                            param.end_rpm = RPM2RCF(cur_prg->rpm);
                        }
                    }
                    else param.end_rpm = cur_prg->rpm;
                    param.total_time_secs = cur_prg->time;
                    if ( cur_prg->cooler_auto_ctrl )
                        param.limit_temp = cur_prg->temp;
                    else
                        param.limit_temp = -100;
                }

                if ( gDisplayRCF ) param.disp_option |= DISP_OPTION_RCF_MODE_AT_RUN;

                // 현재 드라이버가 5000모드에서는 0.4%정도 RPM이 높아지는 문제가 있다
                // 이를 수정하기 위해서 땜빵으로 넣은 코드이므로 나중엔 반드시 제거해야 한다
                /*
                if ( !(gNEXDMotor_Status.status & NEXD_STATUS_SPEED_MODE) )
                {
                    gRPM = gRPM * 0.996;
                    gPrecisionRPM = gPrecisionRPM * 0.996;
                }
                */

                if ( gDisplayRCF )
                {
                    param.cur_rpm = RPM2RCF(gRPM);

                    if ( param.cur_rpm < param.end_rpm+5 && param.cur_rpm > param.end_rpm-5 )
                        param.cur_rpm = param.end_rpm;
                }
                else
                {
                    rpm[rpm_idx++] = gRPM;
                    if ( rpm_idx >= 5 ) rpm_idx = 0;

                    if ( gPrecisionRPM > 200 )
                    {
                        param.cur_rpm = gRPM;
                        //param.cur_rpm = (rpm[0]+rpm[1]+rpm[2]+rpm[3]+rpm[4]) / 5;
                    }
                    else
                    {
                        param.cur_rpm = gPrecisionRPM;
                    }

                    if ( param.cur_rpm < param.end_rpm+5 && param.cur_rpm > param.end_rpm-5 )
                        param.cur_rpm = param.end_rpm;
                }

                param.cur_temp = GetChamberTemp() / 10000;
                param.popup = option;
                param.status_and_rotor_type |= gRunStatus;

                if ( param.popup != POPUP_MSG_AUTOBAL_STEP_START )
                    param.autobal_step_and_program_no |= (gProgramIdx+1) << 3;

                if ( gHaveTempSensor )
                {
                    gDebugData.motor_temp = gNEXDMotor_Status.motor_temp;
                    gDebugData.motor_driver_temp = gNEXDMotor_Status.controller_temp;
                }

                // 온도가 설정범위로부터 너무 많이 벗어나게 디스플레이되는 것을 방지하기 위해서
                // 적절하게 보정해서 출력한다
                if ( gHaveCooler && gHaveTempSensor && gProgram[gProgramIdx].cooler_auto_ctrl )
                {
                    if ( gUI_State==UI_AUTOBALANCE || gUI_State==UI_COOLING )
                    {
                        if ( param.cur_temp < gProgram[gProgramIdx].temp-2 )
                        {
                            param.cur_temp += 1;
                        }
                    }
                    else if ( gUI_State == UI_ACCEL )
                    {
                        if ( param.cur_temp < gProgram[gProgramIdx].temp-2 )
                        {
                            if ( param.cur_temp < gProgram[gProgramIdx].temp-3 )
                                param.cur_temp += 2;
                            else
                                param.cur_temp += 1;
                        }
                    }
                    else if ( gUI_State == UI_RUNNING )
                    {
                        if ( param.cur_temp > gProgram[gProgramIdx].temp+2 )
                        {
                            if ( TIME_FROM(gRun.keep_start_time)>60 && param.cur_temp>gProgram[gProgramIdx].temp+3 )
                            {
                                if ( TIME_FROM(gRun.keep_start_time)>120 && param.cur_temp>gProgram[gProgramIdx].temp+5 )
                                    param.cur_temp -= 3;
                                else
                                    param.cur_temp -= 2;
                            }
                            else
                                param.cur_temp -= 1;
                        }
                        else if ( param.cur_temp < gProgram[gProgramIdx].temp-2 )
                        {
                            if ( param.cur_temp < gProgram[gProgramIdx].temp-3 )
                            {
                                if ( param.cur_temp < gProgram[gProgramIdx].temp-5 )
                                    param.cur_temp += 3;
                                else
                                    param.cur_temp += 2;
                            }
                            else
                                param.cur_temp += 1;
                        }
                    }
                    else if ( (gUI_State>=UI_PROGRAM_SUCCESS && gUI_State<=UI_PROGRAM_CANCEL+0xff) || gUI_State==UI_CUSTOM_ERR || gUI_State==UI_CUSTOM_ERR_TEXT )
                    {
                        if ( gRunStatus == STATUS_DECEL )
                        {
                            if ( param.cur_temp > gProgram[gProgramIdx].temp+2 )
                            {
                                if ( TIME_FROM(gRun.keep_start_time)>60 && param.cur_temp>gProgram[gProgramIdx].temp+3 )
                                    param.cur_temp -= 2;
                                else
                                    param.cur_temp -= 1;
                            }
                            else if ( param.cur_temp < gProgram[gProgramIdx].temp-2 )
                            {
                                if ( param.cur_temp < gProgram[gProgramIdx].temp-3 )
                                    param.cur_temp += 2;
                                else
                                    param.cur_temp += 1;
                            }
                        }
                        else
                        {
                            if ( param.cur_temp > gProgram[gProgramIdx].temp+2 )
                            {
                                param.cur_temp -= 1;
                            }
                            else if ( param.cur_temp < gProgram[gProgramIdx].temp-2 )
                            {
                                param.cur_temp += 1;
                            }
                        }
                    }
                    else if ( gUI_State==UI_READY && gRunStatus==STATUS_DECEL )
                    {
                        if ( param.cur_temp > gProgram[gProgramIdx].temp+2 )
                        {
                            param.cur_temp -= 1;
                        }
                        else if ( param.cur_temp < gProgram[gProgramIdx].temp-2 )
                        {
                            param.cur_temp += 1;
                        }
                    }
                }

                if ( gHaveCooler ) param.disp_option |= DISP_OPTION_HAVE_COOLER;
                if ( !gHaveTempSensor ) param.disp_option |= DISP_OPTION_TEMP_DISP_OFF_AT_RUN;

                if ( DoorIsOpened() ) param.status_and_rotor_type |= STATUS_DOOROPEN;
                else param.status_and_rotor_type &= ~STATUS_DOOROPEN;

                if ( CoolerPowerIsOn(0) ) param.status_and_rotor_type |= STATUS_COOLING;
                else param.status_and_rotor_type &= ~STATUS_COOLING;

                if ( gDebugData.disp_option & DISP_OPTION_DEBUG_DISP_ON_AT_RUN )
                {
                    param.disp_option |= gDebugData.disp_option;
                    memcpy((void*)&param.dsp, (void*)&gDebugData.dsp, sizeof(gDebugData.dsp));

                    if ( gDebugData.disp_option & DISP_OPTION_DEBUG_VIB_MODE_AT_RUN )
                    {
                        memcpy((void*)&param.dsp, (void*)&gDebugData.dsp, sizeof(gDebugData.dsp));
                    }
                    else
                    {
                        memcpy((void*)&param.debug, (void*)&gDebugData.debug, sizeof(gDebugData.debug));
                    }
                }

                param.motor_temp = gDebugData.motor_temp;
                param.motor_driver_temp = gDebugData.motor_driver_temp;

                memcpy(&buf[1], &param, sizeof(param));
            }
            break;

        case DISP_SETTING:
            {
                struct TDisplaySetting param;
                memset(&param, 0, sizeof(param));

                if ( data )
                    memcpy(&param, data, sizeof(param));

                if ( cur_prg )
                {
                    param.rcf_mode = cur_prg->rcf_mode;
                    param.rpm = cur_prg->rpm;
                    param.time = cur_prg->time;
                    param.rotor_type = cur_prg->rotor_type;
                    param.temp = cur_prg->temp;
                    param.cooler_control_auto = cur_prg->cooler_auto_ctrl;
                    param.accel = cur_prg->accel;
                    param.decel = cur_prg->decel;
                }

                if ( gHaveCooler ) param.disp_option |= DISP_OPTION_HAVE_COOLER;

                memcpy(&buf[1], &param, sizeof(param));
            }
            break;
    }

    MsgSend(MSG_TYPE_DISPLAY, buf, DATA_MSG_MAX_LEN);
}
