#include "main.h"
#include "dsp.h"

//#define FAKE_DSP_RESPONSE

//----------------------------------------------------------------------------
// Global Valiable
//----------------------------------------------------------------------------
bool gImbalanceDetectDisable = FALSE;
struct DSP_INTERFACE gDSP = { TRUE, TRUE, 100000, FALSE, FALSE, FALSE, FALSE, 0, 0, 0, 0, 0, 0 };

static bool Running = FALSE;
static unsigned int AliveQuerySendTime = 0;
static volatile bool RecvDataCompletely = false;

static unsigned char DSPLastCmd;
static volatile unsigned int LastCatchTime;
static bool DSPReset;
static bool VibTooSmall_Count;
static int TempData[2];
static float TempVibData[4];


//----------------------------------------------------------------------------
// Function Declaration
//----------------------------------------------------------------------------
void DSP_Start(void);
void DSP_Stop(void);
void DSP_HWReset(void);
void DSP_QueryAlive(void);
void DSP_SendCommand(unsigned char cmd);
void DSP_CANRecvHandler(CanRxMsgTypeDef* msg_obj);
void DSP_Process(void);

//----------------------------------------------------------------------------
// Function Body
//----------------------------------------------------------------------------
void DSP_CANRecvHandler(CanRxMsgTypeDef* msg_obj)
{
    LastCatchTime = gSeconds;

    switch ( msg_obj->StdId )
    {
        case 0x100:
            AliveQuerySendTime = 0;
            gDSP.alive = TRUE;
            break;

        case 0x103:
            //DPUTS("IMBALANCE FROM DSP!!");
            break;

        case 0x106:
            SWAP4(&msg_obj->Data[0]);
            SWAP4(&msg_obj->Data[4]);
            TempData[0] = *((int*)&msg_obj->Data[0]); // vib
            TempData[1] = *((int*)&msg_obj->Data[4]); // phase
            RecvDataCompletely = true;
            break;

        case 0x107: // z axis
            SWAP4(&msg_obj->Data[0]);
            SWAP4(&msg_obj->Data[4]);
            TempVibData[0] = *((float*)&msg_obj->Data[0]); // vib
            TempVibData[1] = *((float*)&msg_obj->Data[4]); // phase
            break;

        case 0x109: // RMS value
            SWAP4(&msg_obj->Data[0]);
            SWAP4(&msg_obj->Data[4]);
            TempVibData[2] = *((float*)&msg_obj->Data[0]); // sensor 1 (max 2G)
            TempVibData[3] = *((float*)&msg_obj->Data[4]); // sensor 2 (max 15G)
            break;

        default:
            //WX(msg_obj->ulMsgID);
            break;
    }
}

void DSP_QueryAlive(void)
{
    CAN_Send(DSP_CAN, 0x100, "DOYOUOK?", 8, CAN_SEND_NO_WAIT);
    AliveQuerySendTime = gSeconds;
}

void DSP_SendCommand(unsigned char cmd)
{
    unsigned char send_cmd[8] = { 0, };

    if ( cmd & DSP_CMD_MEASURE_MODE_2)
        DSPLastCmd = DSP_CMD_MEASURE_MODE_2;
    else if ( cmd & DSP_CMD_MEASURE_MODE_1 )
        DSPLastCmd = DSP_CMD_MEASURE_MODE_1;
    else if ( cmd & DSP_CMD_READY )
        DSPLastCmd = DSP_CMD_READY;

    send_cmd[0] = cmd;
    CAN_Send(DSP_CAN, 0x101, send_cmd, 8, CAN_SEND_NO_WAIT);
}

void DSP_Start(void)
{
    //FSTART();

    gDSP.imbalance_occur = FALSE;
    gDSP.dsp_no_ack = FALSE;
    gDSP.vib_sensor_no_exist = FALSE;
    TempData[0] = TempData[1] = 0;
    TempVibData[0] = TempVibData[1] = TempVibData[2] = TempVibData[3] = 0;
    Running = TRUE;
    RecvDataCompletely = false;
    LastCatchTime = gSeconds;
    DSPReset = FALSE;
    VibTooSmall_Count = 0;
    DSP_SendCommand(DSP_CMD_MEASURE_MODE_1);
}

void DSP_Stop(void)
{
    //FSTART();

    Running = FALSE;
    gDSP.vib_log_print_once_at_rpm = 100000;
    DSP_SendCommand(DSP_CMD_READY);
}

void DSP_Process(void)
{
    int rpm_range;
    static bool skip_first = TRUE;

    if ( gRPM==0 && skip_first==FALSE )
        skip_first = TRUE;

    if ( AliveQuerySendTime > 0 && TIME_FROM(AliveQuerySendTime) > 1 )
    {
        AliveQuerySendTime = 0;
        gDSP.alive = FALSE;
    }

    if ( !Running )
    {
        skip_first = TRUE;
        gDSP.vib = 0;
        gDSP.phase = 0;
    }
    else
    {
        const static unsigned char wait_time_limit[] = { 20, 15, 10, 10 };

        if ( gRPM<550 ) rpm_range = 0;
        else if ( gRPM>=550 && gRPM<1100 ) rpm_range = 1;
        else if ( gRPM>=1100 && gRPM<2100 ) rpm_range = 2;
        else rpm_range = 3;

        if ( gRPM < 200 )
        {
            LastCatchTime = gSeconds;
        }
        else if ( gRPM>=200 && TIME_FROM(LastCatchTime)>wait_time_limit[rpm_range] )
        {
#ifndef FAKE_DSP_RESPONSE
            if ( DSPReset == FALSE )
            {
                DSP_HWReset();
                Delay_ms(100);
                DSP_SendCommand(DSPLastCmd);
                DSPReset = TRUE;
                TempData[0] = TempData[1] = 0;
                TempVibData[0] = TempVibData[1] = TempVibData[2] = TempVibData[3] = 0;
                LastCatchTime = gSeconds;
            }
            else
            {
                gDSP.dsp_no_ack = TRUE;
            }
#else // 진동센서가 미동작시에 가짜로 동작하는 것처럼 만든다
            gDSP.rpm = gRPM*100;
            gDSP.rpm_change = 0;
            gDSP.vib = 0.1;
            gDSP.phase = 0.1;
            gDSP.msg_catch = TRUE;
            gDSP.rms1 = 0.1;
            gDSP.rms2 = 0.1;
            LastCatchTime = gTime;

            if ( gRPM > gDSP.vib_log_print_once_at_rpm )
            {
                gDSP.vib_log_print_once_at_rpm += 200;
                DPRINTF(("\t%i\t%i\t%s\t%s\t%s\t%s", gDSP.rpm, gDSP.rpm_change, F2S(gDSP.vib), F2S(gDSP.phase), F2S(gDSP.rms1), F2S(gDSP.rms2)));
            }
            else if ( gDSP.vib_log_on )
            {
                DPRINTF(("\t%i\t%i\t%s\t%s\t%s\t%s", gDSP.rpm, gDSP.rpm_change, F2S(gDSP.vib), F2S(gDSP.phase), F2S(gDSP.rms1), F2S(gDSP.rms2)));
            }
#endif
        }

        if ( RecvDataCompletely )
        {
            RecvDataCompletely = false;

            if ( gRPM > gDSP.vib_log_print_once_at_rpm )
            {
                gDSP.vib_log_print_once_at_rpm += 200;
                DPRINTF(("\t%i\t%i\t%s\t%s\t%s\t%s", TempData[0], TempData[1], F2S(TempVibData[0]), F2S(TempVibData[1]), F2S(TempVibData[2]), F2S(TempVibData[3])));
            }
            else if ( gDSP.vib_log_on )
            {
                DPRINTF(("\t%i\t%i\t%s\t%s\t%s\t%s", TempData[0], TempData[1], F2S(TempVibData[0]), F2S(TempVibData[1]), F2S(TempVibData[2]), F2S(TempVibData[3])));
            }

            //if ( TempVibData[0]!=0 && TempVibData[1]!=0 ) // && TempVibData[2]!=0 && TempVibData[3]!=0 )
            {
                gDSP.rpm = TempData[0];
                gDSP.rpm_change = TempData[1];
                gDSP.vib = TempVibData[0];
                gDSP.phase = TempVibData[1];
                gDSP.msg_catch = TRUE;
                gDSP.rms1 = TempVibData[2];
                gDSP.rms2 = TempVibData[3];
            }

            if ( skip_first )
            {
                skip_first = FALSE;
                return;
            }

            if ( gDSP.vib_sensor_no_exist == FALSE && gDSP.rpm > 50000 && gDSP.vib < 0.007 )
            {
                ++VibTooSmall_Count;

                WI(gDSP.rpm);
                WF(gDSP.vib);
                WI(VibTooSmall_Count);

                if ( VibTooSmall_Count >= 3 )
                {
                    gDSP.vib_sensor_no_exist = TRUE;
                    WB(gDSP.vib_sensor_no_exist);
                    return;
                }
            }

            if ( gImbalanceDetectDisable == false )
            {
                const static unsigned char imbal_limit_swingrotor[] = { 1, 10, 50, 120, 60 };

                if ( gDSP.rpm<30000 ) rpm_range = 0;
                else if ( gDSP.rpm>=30000 && gDSP.rpm<70000 ) rpm_range = 1;
                else if ( gDSP.rpm>=70000 && gDSP.rpm<110000 ) rpm_range = 2;
                else if ( gDSP.rpm>=110000 && gDSP.rpm<160000 ) rpm_range = 3;
                else rpm_range = 4;

                if ( TempVibData[0] > imbal_limit_swingrotor[rpm_range] )
                {
                    gDSP.imbalance_occur = TRUE;
                    WI(gDSP.rpm);
                    WF(gDSP.vib);
                    MOTOR_Stop();
                    return;
                }
            }
        }
    }
}

