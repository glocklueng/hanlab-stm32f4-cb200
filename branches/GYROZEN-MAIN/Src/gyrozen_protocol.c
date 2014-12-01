#include "main.h"
#include "gyrozen_protocol.h"

#define __USE_BALANCER_DEFAULT_POS__
#define __WEIGHT_MOVE_LIMIT_OVER__

//*****************************************************************************

#define STX 0x7F
#define EOX 0x7E

//*****************************************************************************

enum CMD { CMD_BALANCING_START=0x70, CMD_MOTOR_SET_RPM, CMD_VIB_INFO, CMD_ERROR, CMD_EMERGENCY, CMD_MOTOR_STOP, CMD_BALANCING_END, CMD_CALIB_START, CMD_CALIB_END };

//*****************************************************************************

unsigned char crc8(const void* buf, int len);

int ChangeOctetToInteger(int i);
int ChangeIntegerToOctet(int i);


void GyrozenProtocolProcess(void);
void GyrozenMotorSetRPM(int rpm);
void GyrozenMotorStop(void);
void GyrozenSendVibInfo(int rpm, float vib, float phase);
void GyrozenSendError(int err_code, int detail_info);
void GyrozenCalibEnd(void);
void GyrozenBalancingEnd(int try_count, float vib, float phase, int* rotor_pos);
void GyrozenAutobalancingTask(int final_rpm);
void GyrozenGetConstantTask(int test_rpm, bool adjust_phase);

//*****************************************************************************

void GyrozenProtocolProcess(void)
{
	int ch;
    static enum CMD cmd;
    static unsigned char data_len = 0;
	static unsigned char buf_len = 0;
	static enum { WAIT_STX, WAIT_CMD, WAIT_DATALEN, RECV_DATA, WAIT_CRC, WAIT_EOX } msg_state = WAIT_STX;
	static char buf[100];

	while ( (ch = UARTgetc(GYROZEN_UART)) >= 0 )
	{
		WC(ch);

		switch ( msg_state )
		{
			case WAIT_STX:
restart_wait_stx:
                if ( ch == STX )
                {
                    buf_len = 0;
                    msg_state = WAIT_CMD;
                    DPUTS("STX OK");
                }
                else
                {
                    DPUTS("STX FAIL");
                }
				break;

            case WAIT_CMD:
                if ( ch >= 0x70 && ch <= 0x7d )
                {
                    DPUTS("CMD OK");
                    cmd = ch;
                    msg_state = WAIT_DATALEN;
                }
                else
                {
                    DPUTS("CMD FAIL");
                    msg_state = WAIT_STX;
                    goto restart_wait_stx;
                }
                break;

            case WAIT_DATALEN:
                data_len = ch;
                WATCHI("DATALEN", data_len);
                buf_len = 0;
                if ( data_len > 0 )
                    msg_state = RECV_DATA;
                else
                    msg_state = WAIT_EOX;
                break;

			case RECV_DATA:
                if ( buf_len < data_len )
                {
                    buf[buf_len++] = ch;
                    if ( buf_len == data_len )
                    {
                        msg_state = WAIT_CRC;
                    }
                }
                else
                {
                    DPUTS("DATA OK");
                    msg_state = WAIT_CRC;
                }
                break;
                 
            case WAIT_CRC:
                DPUTS("CRC OK");
                msg_state = WAIT_EOX;
                break;

            case WAIT_EOX:
                msg_state = WAIT_STX;
                if ( ch == EOX )
                {
                    DPUTS("EOX OK");
                    goto process_packet;
                }
                else
                {
                    DPUTS("EOX FAIL");
                }
                break;
        }
		continue;

process_packet:
        switch ( cmd )
        {
            case CMD_BALANCING_START:
                DPUTS("CMD_BALANCING_START");
                {
                    struct BalancingStartParam {
                        int final_rpm;
                    } *param = (struct BalancingStartParam *) buf;

                    param->final_rpm = ChangeOctetToInteger(param->final_rpm);
                    
                    WI(param->final_rpm);
                    GyrozenAutobalancingTask(param->final_rpm);
                }
                break;

            case CMD_CALIB_START:
                DPUTS("CMD_CALIB_START");
                {
                    struct CalibStartParam {
                        enum { MEASURE_DEFAULT_POS=0, FULL_CALIB, SHORT_CALIB } calib_type;
                    } *param = (struct CalibStartParam*) buf;

                    WI(param->calib_type);
                    switch ( param->calib_type )
                    {
                        case MEASURE_DEFAULT_POS:
                            GyrozenGetConstantTask(1000, false);
                            break;
                        case FULL_CALIB:
                            GyrozenGetConstantTask(0, false);
                            break;
                        case SHORT_CALIB:
                            GyrozenGetConstantTask(1000, true);
                            break;
                    }
                }
                break;

            case CMD_EMERGENCY:
                DPUTS("CMD_EMERGENCY");
                RotorPower(0);
                SlipRingPower(0);
                DSP_Stop();
                break;

            case CMD_MOTOR_SET_RPM:
                DPUTS("CMD_MOTOR_SET_RPM");
                break;
                
            case CMD_MOTOR_STOP:
                DPUTS("CMD_MOTOR_STOP");
                break;
                
            case CMD_BALANCING_END:
                DPUTS("CMD_BALANCING_END");
                break;

            case CMD_CALIB_END:
                DPUTS("CMD_CALIB_END");
                break;

            case CMD_VIB_INFO:
                DPUTS("CMD_VIB_INFO");
                break;

            case CMD_ERROR:
                DPUTS("CMD_ERROR");
                break;
        }
	}
}

unsigned char crc8(const void* buf, int len)
{
    const static unsigned char crc8_table[256] = {  // polynomial 0x8c (mirror of 0x31)
        0x00, 0x5e, 0xbc, 0xe2, 0x61, 0x3f, 0xdd, 0x83,
        0xc2, 0x9c, 0x7e, 0x20, 0xa3, 0xfd, 0x1f, 0x41,
        0x9d, 0xc3, 0x21, 0x7f, 0xfc, 0xa2, 0x40, 0x1e,
        0x5f, 0x01, 0xe3, 0xbd, 0x3e, 0x60, 0x82, 0xdc,
        0x23, 0x7d, 0x9f, 0xc1, 0x42, 0x1c, 0xfe, 0xa0,
        0xe1, 0xbf, 0x5d, 0x03, 0x80, 0xde, 0x3c, 0x62,
        0xbe, 0xe0, 0x02, 0x5c, 0xdf, 0x81, 0x63, 0x3d,
        0x7c, 0x22, 0xc0, 0x9e, 0x1d, 0x43, 0xa1, 0xff,
        0x46, 0x18, 0xfa, 0xa4, 0x27, 0x79, 0x9b, 0xc5,
        0x84, 0xda, 0x38, 0x66, 0xe5, 0xbb, 0x59, 0x07,
        0xdb, 0x85, 0x67, 0x39, 0xba, 0xe4, 0x06, 0x58,
        0x19, 0x47, 0xa5, 0xfb, 0x78, 0x26, 0xc4, 0x9a,
        0x65, 0x3b, 0xd9, 0x87, 0x04, 0x5a, 0xb8, 0xe6,
        0xa7, 0xf9, 0x1b, 0x45, 0xc6, 0x98, 0x7a, 0x24,
        0xf8, 0xa6, 0x44, 0x1a, 0x99, 0xc7, 0x25, 0x7b,
        0x3a, 0x64, 0x86, 0xd8, 0x5b, 0x05, 0xe7, 0xb9,
        0x8c, 0xd2, 0x30, 0x6e, 0xed, 0xb3, 0x51, 0x0f,
        0x4e, 0x10, 0xf2, 0xac, 0x2f, 0x71, 0x93, 0xcd,
        0x11, 0x4f, 0xad, 0xf3, 0x70, 0x2e, 0xcc, 0x92,
        0xd3, 0x8d, 0x6f, 0x31, 0xb2, 0xec, 0x0e, 0x50,
        0xaf, 0xf1, 0x13, 0x4d, 0xce, 0x90, 0x72, 0x2c,
        0x6d, 0x33, 0xd1, 0x8f, 0x0c, 0x52, 0xb0, 0xee,
        0x32, 0x6c, 0x8e, 0xd0, 0x53, 0x0d, 0xef, 0xb1,
        0xf0, 0xae, 0x4c, 0x12, 0x91, 0xcf, 0x2d, 0x73,
        0xca, 0x94, 0x76, 0x28, 0xab, 0xf5, 0x17, 0x49,
        0x08, 0x56, 0xb4, 0xea, 0x69, 0x37, 0xd5, 0x8b,
        0x57, 0x09, 0xeb, 0xb5, 0x36, 0x68, 0x8a, 0xd4,
        0x95, 0xcb, 0x29, 0x77, 0xf4, 0xaa, 0x48, 0x16,
        0xe9, 0xb7, 0x55, 0x0b, 0x88, 0xd6, 0x34, 0x6a,
        0x2b, 0x75, 0x97, 0xc9, 0x4a, 0x14, 0xf6, 0xa8,
        0x74, 0x2a, 0xc8, 0x96, 0x15, 0x4b, 0xa9, 0xf7,
        0xb6, 0xe8, 0x0a, 0x54, 0xd7, 0x89, 0x6b, 0x35,
    };

	register int i;
    register unsigned char crc = 0;
    register unsigned char* data = (unsigned char*) buf;
    
    return 0xff;

	for ( i=0; i<len; i++ )
    {
        crc = crc8_table[crc^(*data)];
        data++;
    }

    return crc;
}

int ChangeOctetToInteger(int i)
{
    int v = 0;
    unsigned char* p = (unsigned char*) &i;
    
    v += p[0] << 12;
    v += p[1] << 8;
    v += p[2] << 4;
    v += p[3];
    
    return v;
}

int ChangeIntegerToOctet(int i)
{
    int v = 0;
    unsigned char *p = (unsigned char*) &i;    
    unsigned char *r = (unsigned char*) &v;    
    
    r[0] = (p[1] >> 4);
    r[1] = (p[1] & 0xf);
    r[2] = (p[0] >> 4);
    r[3] = (p[0] & 0xf);
    
    return v;
}

void GyrozenMotorSetRPM(int rpm)
{
    FSTART();
    
    unsigned char buf[9];
    
    rpm = ChangeIntegerToOctet(rpm);

    buf[0] = STX;
    buf[1] = CMD_MOTOR_SET_RPM;
    buf[2] = 4;
    buf[3] = ((char*)&rpm)[0];
    buf[4] = ((char*)&rpm)[1];
    buf[5] = ((char*)&rpm)[2];
    buf[6] = ((char*)&rpm)[3];
    buf[7] = crc8(buf+1, 6);
    buf[8] = EOX;
    
    for ( int i=0 ; i<9 ; i++ )
        DPRINTF(("buf[%i]: 0x%X", i, buf[i]));

    UARTwrite(GYROZEN_UART, buf, 9);
}

void GyrozenMotorStop(void)
{
    FSTART();

    unsigned char buf[5];

    buf[0] = STX;
    buf[1] = CMD_MOTOR_STOP;
    buf[2] = 0;
    buf[3] = crc8(buf+1, 2);
    buf[4] = EOX;

    for ( int i=0 ; i<5 ; i++ )
        DPRINTF(("buf[%i]: 0x%X", i, buf[i]));
    
    UARTwrite(GYROZEN_UART, buf, 5);
}

void GyrozenSendVibInfo(int rpm, float vib, float phase)
{
    FSTART();

    struct {
        int rpm;
        float vib;
        float phase;
    } param;
    unsigned char buf[sizeof(param)+5];

    param.rpm = rpm;
    param.vib = vib;
    param.phase = phase;

    buf[0] = STX;
    buf[1] = CMD_VIB_INFO;
    buf[2] = sizeof(param);

    for ( int i=0 ; i<sizeof(param) ; i++ )
        buf[3+i] = ((unsigned char*)&param)[i];

    buf[3+sizeof(param)] = crc8(buf+1, sizeof(param)+2);
    buf[4+sizeof(param)] = EOX;

    UARTwrite(GYROZEN_UART, buf, sizeof(buf));
}

void GyrozenSendError(int err_code, int detail_info)
{
    FSTART();

    struct {
        int err_code;
        int detail_info;
    } param;
    unsigned char buf[sizeof(param)+5];

    param.err_code = err_code;
    param.detail_info = detail_info;

    buf[0] = STX;
    buf[1] = CMD_ERROR;
    buf[2] = sizeof(param);

    for ( int i=0 ; i<sizeof(param) ; i++ )
        buf[3+i] = ((unsigned char*)&param)[i];

    buf[3+sizeof(param)] = crc8(buf+1, sizeof(param)+2);
    buf[4+sizeof(param)] = EOX;

    UARTwrite(GYROZEN_UART, buf, sizeof(buf));
}

void GyrozenCalibEnd(void)
{
    FSTART();

    struct {
        int weight_mass[3];
        int default_pos[3];
        float vib_const[3];
        float phase_const[3];
        float limit_vib[3];
        float limit_mass[3];
    } param;
    unsigned char buf[sizeof(param)+5];

    memset(&param, sizeof(param), 0);

    param.weight_mass[0] = gRotor.weight[0];
    param.weight_mass[1] = gRotor.weight[1];
    param.weight_mass[2] = gRotor.weight[2];
    param.default_pos[0] = gRotor.default_pos[0];
    param.default_pos[1] = gRotor.default_pos[1];
    param.default_pos[2] = gRotor.default_pos[2];
    param.vib_const[0] = gSwingRotorBalancingParam.vib_const[0];
    param.vib_const[1] = gSwingRotorBalancingParam.vib_const[1];
    param.vib_const[2] = gSwingRotorBalancingParam.vib_const[2];
    param.phase_const[0] = gSwingRotorBalancingParam.vib_const[0];
    param.phase_const[1] = gSwingRotorBalancingParam.vib_const[1];
    param.phase_const[2] = gSwingRotorBalancingParam.vib_const[2];
    param.limit_vib[0] = gSwingRotorBalancingParam.limit_vib_at_rpm[0];
    param.limit_vib[1] = gSwingRotorBalancingParam.limit_vib_at_rpm[1];
    param.limit_vib[2] = gSwingRotorBalancingParam.limit_vib_at_rpm[2];
    param.limit_mass[0] = gSwingRotorBalancingParam.limit_mass_at_rpm[0];
    param.limit_mass[1] = gSwingRotorBalancingParam.limit_mass_at_rpm[1];
    param.limit_mass[2] = gSwingRotorBalancingParam.limit_mass_at_rpm[2];

    buf[0] = STX;
    buf[1] = CMD_ERROR;
    buf[2] = sizeof(param);

    for ( int i=0 ; i<sizeof(param) ; i++ )
        buf[3+i] = ((unsigned char*)&param)[i];

    buf[3+sizeof(param)] = crc8(buf+1, sizeof(param)+2);
    buf[4+sizeof(param)] = EOX;

    UARTwrite(GYROZEN_UART, buf, sizeof(buf));
}


void GyrozenBalancingEnd(int try_count, float vib, float phase, int* rotor_pos)
{
    FSTART();

    struct {
        int err_code;
        int try_count;
        float vib;
        float phase;
        int rotor_pos[3];
    } param;
    unsigned char buf[sizeof(param)+5];

    param.err_code = 0;
    param.try_count = try_count;
    param.vib = vib;
    param.phase = phase;
    param.rotor_pos[0] = rotor_pos[0];
    param.rotor_pos[1] = rotor_pos[1];
    param.rotor_pos[2] = rotor_pos[2];

    buf[0] = STX;
    buf[1] = CMD_BALANCING_END;
    buf[2] = sizeof(param);

    for ( int i=0 ; i<sizeof(param) ; i++ )
        buf[3+i] = ((unsigned char*)&param)[i];

    buf[3+sizeof(param)] = crc8(buf+1, sizeof(param)+2);
    buf[4+sizeof(param)] = EOX;

    UARTwrite(GYROZEN_UART, buf, sizeof(buf));
}


void GyrozenAutobalancingTask(int final_rpm)
{
    int retry_cnt;
    unsigned char step;
    int min, range1, range2, range3;
    unsigned short range[3] = { 0, };
    bool skip_300rpm = FALSE;
    bool skip_1000rpm = FALSE;
    double amount[3] = { 0, };
    double amount_temp[3];
    double min_amount;
    unsigned short balancer_move_max = __BALANCER_MOVE_MAX__;
    bool without_dsp_mode = false;
#ifdef __USE_BALANCER_DEFAULT_POS__
    int balancer_default_pos[3] = { 0, };
#endif
#ifdef __WEIGHT_MOVE_LIMIT_OVER__
    const int balancer_limit_over_value = 200;
#endif

    FSTART();
    WI(final_rpm);
    return;

    FRAM_Read(100, &gSwingRotorBalancingParam, sizeof(gSwingRotorBalancingParam));

    if ( final_rpm <= 0 )
        final_rpm = 3000;

    WF(gSwingRotorBalancingParam.vib_const[0]);
    WF(gSwingRotorBalancingParam.phase_const[0]);
    WF(gSwingRotorBalancingParam.vib_const[1]);
    WF(gSwingRotorBalancingParam.phase_const[1]);
    WF(gSwingRotorBalancingParam.vib_const[2]);
    WF(gSwingRotorBalancingParam.phase_const[2]);

    if ( gRPM != 0 )
    {
        DPUTS("ERROR: still running");
        WI(gRPM);
        gBalancingTaskResult = ERROR_STILL_RUN;
        goto program_error;
    }

    if ( !gSetting.debug && !DoorIsClosed() )
    {
        DPUTS("ERROR: door is open");
        gBalancingTaskResult = ERROR_DOOR_OPEN;
        goto program_error;
    }

    gBalancingTaskResult = 0;
    gCancel = false;

    RB_PowerOnAndCheck(2);
    while ( gRotor.status != ROTOR_READY )
    {
        BackGroundProcess();
    }

    if ( gRotor.result != SUCCESS )
    {
        if ( gProgramIdx == 19 )
            goto start_auto_balancing;

        WI(gRotor.result);
        gBalancingTaskResult = gRotor.result;
        goto program_error;
    }

    RB_GetInformation();
    while ( gRotor.status != ROTOR_FINISHED )
    {
        BackGroundProcess();
    }

    if ( gRotor.result == SUCCESS )
    {
        WI(gRotor.weight[0]);
        WI(gRotor.weight[1]);
        WI(gRotor.weight[2]);

        SetBalancerWeight((float)gRotor.weight[0]/10, (float)gRotor.weight[1]/10, (float)gRotor.weight[2]/10);

        balancer_default_pos[0] = gRotor.default_pos[0];
        balancer_default_pos[1] = gRotor.default_pos[1];
        balancer_default_pos[2] = gRotor.default_pos[2];
    }
    else
    {
        if ( gProgramIdx == 19 )
        {
            SetBalancerWeight(SWING_ROTOR_BALANCER_WEIGHT, SWING_ROTOR_BALANCER_WEIGHT, SWING_ROTOR_BALANCER_WEIGHT);
            goto start_auto_balancing;
        }

        WI(gRotor.result);
        goto program_error;
    }

start_auto_balancing:
    for ( step=0 ; step<=4 ; step++ )
    {
        if ( gRPM != 0 )
        {
            DPUTS("ERROR: still running");
            WI(gRPM);
            gBalancingTaskResult = ERROR_STILL_RUN;
            goto program_error;
        }

        if ( !gSetting.debug && !DoorIsClosed() )
        {
            DPUTS("ERROR: door is open");
            gBalancingTaskResult = ERROR_DOOR_OPEN;
            goto program_error;
        }

        if ( without_dsp_mode == true )
        {
            RB_PowerOnAndCheck(2);
            while ( gRotor.status != ROTOR_READY )
            {
                BackGroundProcess();
            }

            if ( gRotor.result == SUCCESS )
            {
                RB_Move(0, balancer_default_pos[0], balancer_default_pos[1], balancer_default_pos[2]);
                while ( gRotor.status != ROTOR_FINISHED )
                {
                    BackGroundProcess();
                }
            }
        }
        else
        {
            RB_PowerOnAndCheck(2);
            while ( gRotor.status != ROTOR_READY )
            {
                BackGroundProcess();
            }

            if ( gRotor.result != SUCCESS )
            {
                WI(gRotor.result);
                gBalancingTaskResult = gRotor.result;
                goto program_error;
            }

    #ifdef __WEIGHT_MOVE_LIMIT_OVER__
            if ( skip_300rpm && !skip_1000rpm )
            {
        #ifdef __USE_BALANCER_DEFAULT_POS__
                range1 = range[0]+balancer_default_pos[0];
                range2 = range[1]+balancer_default_pos[1];
                range3 = range[2]+balancer_default_pos[2];

                min = range1>range2 ? range2 : range1;
                min = min>range3 ? range3 : min;

                range1 -= min;
                range2 -= min;
                range3 -= min;

                range1 = range1 > balancer_move_max ? balancer_move_max : range1;
                range2 = range2 > balancer_move_max ? balancer_move_max : range2;
                range3 = range3 > balancer_move_max ? balancer_move_max : range3;
        #else
                range1 = range[0] > balancer_move_max ? balancer_move_max : range[0];
                range2 = range[1] > balancer_move_max ? balancer_move_max : range[1];
                range3 = range[2] > balancer_move_max ? balancer_move_max : range[2];
        #endif
            }
            else
            {
        #ifdef __USE_BALANCER_DEFAULT_POS__
                range1 = range[0]+balancer_default_pos[0];
                range2 = range[1]+balancer_default_pos[1];
                range3 = range[2]+balancer_default_pos[2];

                min = range1>range2 ? range2 : range1;
                min = min>range3 ? range3 : min;

                range1 -= min;
                range2 -= min;
                range3 -= min;
        #else
                range1 = range[0];
                range2 = range[1];
                range3 = range[2];
        #endif
            }
    #else
        #ifdef __USE_BALANCER_DEFAULT_POS__
            range1 = range[0]+balancer_default_pos[0];
            range2 = range[1]+balancer_default_pos[1];
            range3 = range[2]+balancer_default_pos[2];
        #else
            range1 = range[0];
            range2 = range[1];
            range3 = range[2];
        #endif
    #endif

    #ifdef __USE_BALANCER_DEFAULT_POS__
            DPRINTF(("\r\nmove balancer: %i+%i (%i), %i+%i (%i), %i+%i (%i)\r\n", range[0], balancer_default_pos[0], range1, range[1], balancer_default_pos[1], range2, range[2], balancer_default_pos[2], range3));
    #else
            DPRINTF(("\r\nmove balancer: %i, %i, %i\r\n", range[0], range[1], range[2]));
    #endif

            RB_Move(0, range1, range2, range3);
            while ( gRotor.status != ROTOR_FINISHED )
            {
                BackGroundProcess();
            }

            WATCHI("Balancer1 pos", gRotor.move_result[0]);
            WATCHI("Balancer2 pos", gRotor.move_result[1]);
            WATCHI("Balancer3 pos", gRotor.move_result[2]);
            DPUTS("");

            if ( gRotor.result != SUCCESS )
            {
                WI(gRotor.result);
                gBalancingTaskResult = gRotor.result;
                goto program_error;
            }
        }

        RotorPower(0);
        SlipRingPower(0);
        DSP_Start();

        if ( without_dsp_mode==false && final_rpm>=__SWING_ROTOR_300_RPM__ && !skip_300rpm )
        {
            retry_cnt = 0;
run_retry_300:
            Run_GetVibPhase(__SWING_ROTOR_300_RPM__, 0);
            while ( gRun.status == ACCEL_SPEED || gRun.status == MEASURE_VIB )
            {
                BackGroundProcess();
            }

            if ( gRun.result == ERROR_MOTOR_UNDER_ACCEL && retry_cnt < 1 )
            {
                DPUTS("ERROR_MOTOR_UNDER_ACCEL, RETRY");

                retry_cnt++;
                while ( gRPM != 0 )
                {
                    BackGroundProcess();
                }

                goto run_retry_300;
            }

            if ( gRun.result != SUCCESS )
            {
                WI(gRun.result);
                gBalancingTaskResult = gRun.result;
                goto program_error;
            }

            if ( gRun.vib > gSwingRotorBalancingParam.limit_vib_at_rpm[0] )
            {
                SetCompensationConstant(gSwingRotorBalancingParam.vib_const[0], gSwingRotorBalancingParam.phase_const[0]);
                GetCompensationRawResult(gRun.vib, gRun.phase, &amount_temp[0], &amount_temp[1], &amount_temp[2]);

                amount[0] += amount_temp[0];
                amount[1] += amount_temp[1];
                amount[2] += amount_temp[2];

                min_amount = amount[0]>amount[1] ? amount[1] : amount[0];
                min_amount = min_amount>amount[2] ? amount[2] : min_amount;

                amount[0] -= min_amount;
                amount[1] -= min_amount;
                amount[2] -= min_amount;

                range[0] = (amount[0] / gWeight_Mass[0]) * 100;
                range[1] = (amount[1] / gWeight_Mass[1]) * 100;
                range[2] = (amount[2] / gWeight_Mass[2]) * 100;

#ifdef __USE_BALANCER_DEFAULT_POS__
                range1 = range[0]+balancer_default_pos[0];
                range2 = range[1]+balancer_default_pos[1];
                range3 = range[2]+balancer_default_pos[2];

                min = range1>range2 ? range2 : range1;
                min = min>range3 ? range3 : min;

                range1 -= min;
                range2 -= min;
                range3 -= min;
#endif

#ifdef __WEIGHT_MOVE_LIMIT_OVER__
    #ifdef __USE_BALANCER_DEFAULT_POS__
                if ( range1>balancer_move_max && range1<balancer_move_max+balancer_limit_over_value )
                    amount[0] = (float)(balancer_move_max - balancer_default_pos[0] + min)/100 * gWeight_Mass[0];

                if ( range2>balancer_move_max && range2<balancer_move_max+balancer_limit_over_value )
                    amount[1] = (float)(balancer_move_max - balancer_default_pos[1] + min)/100 * gWeight_Mass[1];

                if ( range3>balancer_move_max && range3<balancer_move_max+balancer_limit_over_value )
                    amount[2] = (float)(balancer_move_max - balancer_default_pos[2] + min)/100 * gWeight_Mass[2];

                if ( amount[0]<0 || range1>balancer_move_max+balancer_limit_over_value || amount[1]<0 || range2>balancer_move_max+balancer_limit_over_value || amount[2]<0 || range3>balancer_move_max+balancer_limit_over_value )
    #else
                if ( range[0]>balancer_move_max && range[0]<balancer_move_max+balancer_limit_over_value )
                    amount[0] = (float)balancer_move_max/100 * gWeight_Mass[0];

                if ( range[1]>balancer_move_max && range[1]<balancer_move_max+balancer_limit_over_value )
                    amount[1] = (float)balancer_move_max/100 * gWeight_Mass[1];

                if ( range[2]>balancer_move_max && range[2]<balancer_move_max+balancer_limit_over_value )
                    amount[2] = (float)balancer_move_max/100 * gWeight_Mass[2];

                if ( amount[0]<0 || range[0]>balancer_move_max+balancer_limit_over_value || amount[1]<0 || range[1]>balancer_move_max+balancer_limit_over_value || amount[2]<0 || range[2]>balancer_move_max+balancer_limit_over_value )
    #endif
                {
                    DPRINTF(("ERROR: over range"));
                    gBalancingTaskResult = ERROR_OVER_RANGE;
                    goto program_error;
                }

    #ifdef __USE_BALANCER_DEFAULT_POS__
                //if ( range1<balancer_move_max && range2<balancer_move_max && range3<balancer_move_max )
    #else
                //if ( range[0]<balancer_move_max && range[1]<balancer_move_max && range[2]<balancer_move_max )
    #endif
                    skip_300rpm = TRUE;
#else
    #ifdef __USE_BALANCER_DEFAULT_POS__
                if ( amount[0]<0 || range1>balancer_move_max || amount[1]<0 || range2>balancer_move_max || amount[2]<0 || range3>balancer_move_max )
    #else
                if ( amount[0]<0 || range[0]>balancer_move_max || amount[1]<0 || range[1]>balancer_move_max || amount[2]<0 || range[2]>balancer_move_max )
    #endif
                {
                    DPRINTF(("ERROR: over range"));
                    gBalancingTaskResult = ERROR_OVER_RANGE;
                    goto program_error;
                }

                skip_300rpm = TRUE;
#endif

#ifdef __USE_BALANCER_DEFAULT_POS__
                DPRINTF(("\r\nvib(%s) phase(%s) ==> AutoBalancing(" __SWING_ROTOR_300_RPM_STR__ "): %i, %i, %i\r\n", F2S(gRun.vib), F2S(gRun.phase), range1, range2, range3));
#else
                DPRINTF(("\r\nvib(%s) phase(%s) ==> AutoBalancing(" __SWING_ROTOR_300_RPM_STR__ "): %i, %i, %i\r\n", F2S(gRun.vib), F2S(gRun.phase), range[0], range[1], range[2]));
#endif

                Run_Stop();
                while ( gRPM != 0 )
                {
                    BackGroundProcess();
                }

                continue;
            }
        }

        if ( without_dsp_mode==false && final_rpm>=__SWING_ROTOR_1000_RPM__ && !skip_1000rpm )
        {
            retry_cnt = 0;
run_retry_1000:
            Run_GetVibPhase(__SWING_ROTOR_1000_RPM__, 0); // gSwingRotorBalancingParam.limit_vib_at_rpm[1]);
            while ( gRun.status == ACCEL_SPEED || gRun.status == MEASURE_VIB )
            {
                BackGroundProcess();
            }

            if ( gRun.result == ERROR_MOTOR_UNDER_ACCEL && retry_cnt < 1 )
            {
                DPUTS("ERROR_MOTOR_UNDER_ACCEL, RETRY");

                retry_cnt++;
                while ( gRPM != 0 )
                {
                    BackGroundProcess();
                }

                goto run_retry_1000;
            }

            if ( gRun.result != SUCCESS )
            {
                WI(gRun.result);
                gBalancingTaskResult = gRun.result;
                goto program_error;
            }

            if ( gRun.vib > gSwingRotorBalancingParam.limit_vib_at_rpm[1] )
            {
                SetCompensationConstant(gSwingRotorBalancingParam.vib_const[1], gSwingRotorBalancingParam.phase_const[1]);
                GetCompensationRawResult(gRun.vib, gRun.phase, &amount_temp[0], &amount_temp[1], &amount_temp[2]);

                amount[0] += amount_temp[0];
                amount[1] += amount_temp[1];
                amount[2] += amount_temp[2];

                min_amount = amount[0]>amount[1] ? amount[1] : amount[0];
                min_amount = min_amount>amount[2] ? amount[2] : min_amount;

                amount[0] -= min_amount;
                amount[1] -= min_amount;
                amount[2] -= min_amount;

                range[0] = (amount[0] / gWeight_Mass[0]) * 100;
                range[1] = (amount[1] / gWeight_Mass[1]) * 100;
                range[2] = (amount[2] / gWeight_Mass[2]) * 100;

#ifdef __USE_BALANCER_DEFAULT_POS__
                range1 = range[0]+balancer_default_pos[0];
                range2 = range[1]+balancer_default_pos[1];
                range3 = range[2]+balancer_default_pos[2];

                min = range1>range2 ? range2 : range1;
                min = min>range3 ? range3 : min;

                range1 -= min;
                range2 -= min;
                range3 -= min;

                if ( amount[0]<0 || range1>balancer_move_max || amount[1]<0 || range2>balancer_move_max || amount[2]<0 || range3>balancer_move_max )
#else
                if ( amount[0]<0 || range[0]>balancer_move_max || amount[1]<0 || range[1]>balancer_move_max || amount[2]<0 || range[2]>balancer_move_max )
#endif
                {
                    DPRINTF(("ERROR: over range"));
                    gBalancingTaskResult = ERROR_OVER_RANGE;
                    goto program_error;
                }

#ifdef __USE_BALANCER_DEFAULT_POS__
                DPRINTF(("\r\nvib(%s) phase(%s) ==> AutoBalancing(" __SWING_ROTOR_1000_RPM_STR__ "): %i, %i, %i\r\n", F2S(gRun.vib), F2S(gRun.phase), range1, range2, range3));
#else
                DPRINTF(("\r\nvib(%s) phase(%s) ==> AutoBalancing(" __SWING_ROTOR_1000_RPM_STR__ "): %i, %i, %i\r\n", F2S(gRun.vib), F2S(gRun.phase), range[0], range[1], range[2]));
#endif

                Run_Stop();
                while ( gRPM != 0 )
                {
                    BackGroundProcess();
                }

                skip_300rpm = TRUE;
                continue;
            }
        }

        if ( without_dsp_mode==false && final_rpm>=__SWING_ROTOR_2000_RPM__ )
        {
            retry_cnt = 0;
run_retry_2000:
            Run_GetVibPhase(__SWING_ROTOR_2000_RPM__, 0); // gSwingRotorBalancingParam.limit_vib_at_rpm[2]);
            while ( gRun.status == ACCEL_SPEED || gRun.status == MEASURE_VIB )
            {
                BackGroundProcess();
            }

            if ( gRun.result == ERROR_MOTOR_UNDER_ACCEL && retry_cnt < 1 )
            {
                DPUTS("ERROR_MOTOR_UNDER_ACCEL, RETRY");

                retry_cnt++;
                while ( gRPM != 0 )
                {
                    BackGroundProcess();
                }

                goto run_retry_2000;
            }

            if ( gRun.result != SUCCESS )
            {
                WI(gRun.result);
                gBalancingTaskResult = gRun.result;
                goto program_error;
            }

            if ( gRun.vib > gSwingRotorBalancingParam.limit_vib_at_rpm[2] )
            {
                SetCompensationConstant(gSwingRotorBalancingParam.vib_const[2], gSwingRotorBalancingParam.phase_const[2]);
                GetCompensationRawResult(gRun.vib, gRun.phase, &amount_temp[0], &amount_temp[1], &amount_temp[2]);

                amount[0] += amount_temp[0];
                amount[1] += amount_temp[1];
                amount[2] += amount_temp[2];

                min_amount = amount[0]>amount[1] ? amount[1] : amount[0];
                min_amount = min_amount>amount[2] ? amount[2] : min_amount;

                amount[0] -= min_amount;
                amount[1] -= min_amount;
                amount[2] -= min_amount;

                range[0] = (amount[0] / gWeight_Mass[0]) * 100;
                range[1] = (amount[1] / gWeight_Mass[1]) * 100;
                range[2] = (amount[2] / gWeight_Mass[2]) * 100;

#ifdef __USE_BALANCER_DEFAULT_POS__
                range1 = range[0]+balancer_default_pos[0];
                range2 = range[1]+balancer_default_pos[1];
                range3 = range[2]+balancer_default_pos[2];

                min = range1>range2 ? range2 : range1;
                min = min>range3 ? range3 : min;

                range1 -= min;
                range2 -= min;
                range3 -= min;

                if ( amount[0]<0 || range1>balancer_move_max || amount[1]<0 || range2>balancer_move_max || amount[2]<0 || range3>balancer_move_max )
#else
                if ( amount[0]<0 || range[0]>balancer_move_max || amount[1]<0 || range[1]>balancer_move_max || amount[2]<0 || range[2]>balancer_move_max )
#endif
                {
                    DPRINTF(("ERROR: over range"));
                    gBalancingTaskResult = ERROR_OVER_RANGE;
                    goto program_error;
                }

#ifdef __USE_BALANCER_DEFAULT_POS__
                DPRINTF(("\r\nvib(%s) phase(%s) ==> AutoBalancing(" __SWING_ROTOR_2000_RPM_STR__ "): %i, %i, %i\r\n", F2S(gRun.vib), F2S(gRun.phase), range1, range2, range3));
#else
                DPRINTF(("\r\nvib(%s) phase(%s) ==> AutoBalancing(" __SWING_ROTOR_2000_RPM_STR__ "): %i, %i, %i\r\n", F2S(gRun.vib), F2S(gRun.phase), range[0], range[1], range[2]));
#endif

                Run_Stop();
                while ( gRPM != 0 )
                {
                    BackGroundProcess();
                }

                skip_300rpm = TRUE;
                skip_1000rpm = TRUE;
                continue;
            }
        }

        gDSP.vib_log_print_once_at_rpm = 2200;
        retry_cnt = 0;

run_retry_final:
        GyrozenMotorSetRPM(final_rpm);

        gBalancingTaskResult = SUCCESS;
        GyrozenBalancingEnd(step, gDSP.vib, gDSP.phase, gRotor.move_result);
        return;
    }

    gBalancingTaskResult = ERROR_AUTOBALANCING;
    goto program_error;

program_error:
    GyrozenSendError(gBalancingTaskResult, 0);
    RotorPower(0);
    SlipRingPower(0);
    DSP_Stop();
    if ( gRPM > 0 ) GyrozenMotorStop();

    DPRINTF(("\r\nTEST END"));
    FEND();
}


void GyrozenGetConstantTask(int test_rpm, bool adjust_phase)
{
    #define TEST_CNT            1
    #define ORG_TEST_CNT        3

    #define MOVE_AT_500RPM      1400
    #define MOVE_AT_1000RPM     1000
    #define MOVE_AT_2000RPM     100

    int retry_cnt;
    float org_vib, org_phase;
    double vib_const, phase_const;
    static int scenario[(ORG_TEST_CNT+3)*3][4]; // 384bytes
    static float result_org_vib[ORG_TEST_CNT], result_org_phase[ORG_TEST_CNT]; // 40bytes
    static float result_vib[3][3*TEST_CNT], result_phase[3][3*TEST_CNT]; // 216bytes
    char str[41];

    FSTART();
    return;

    gBalancingTaskResult = 0;
    gCancel = false;

    memset(scenario, 0, sizeof(float)*(ORG_TEST_CNT+3)*3*4);
    FRAM_Read(100, &gSwingRotorBalancingParam, sizeof(gSwingRotorBalancingParam));

    for ( int i=0 ; i<ORG_TEST_CNT+3 ; i++ )
    {
        scenario[i][0] = __SWING_ROTOR_300_RPM__;
        scenario[ORG_TEST_CNT+3+i][0] = __SWING_ROTOR_1000_RPM__;
        scenario[(ORG_TEST_CNT+3)*2+i][0] = __SWING_ROTOR_2000_RPM__;

        switch ( i )
        {
            case ORG_TEST_CNT:
                scenario[i][1] += MOVE_AT_500RPM;
                scenario[ORG_TEST_CNT+3+i][1] += MOVE_AT_1000RPM;
                scenario[(ORG_TEST_CNT+3)*2+i][1] += MOVE_AT_2000RPM;
                break;
            case ORG_TEST_CNT+1:
                scenario[i][2] += MOVE_AT_500RPM;
                scenario[ORG_TEST_CNT+3+i][2] += MOVE_AT_1000RPM;
                scenario[(ORG_TEST_CNT+3)*2+i][2] += MOVE_AT_2000RPM;
                break;
            case ORG_TEST_CNT+2:
                scenario[i][3] += MOVE_AT_500RPM;
                scenario[ORG_TEST_CNT+3+i][3] += MOVE_AT_1000RPM;
                scenario[(ORG_TEST_CNT+3)*2+i][3] += MOVE_AT_2000RPM;
                break;
        }
    }

    FSTART();
    DPRINTF(("\r\nGET CONSTANT START"));

    if ( gRPM != 0 )
    {
        DPUTS("ERROR: still running");
        WI(gRPM);
        gBalancingTaskResult = ERROR_STILL_RUN;
        goto program_error;
    }

    if ( !gSetting.debug && !DoorIsClosed() )
    {
        DPUTS("ERROR: door is open");
        gBalancingTaskResult = ERROR_DOOR_OPEN;
        goto program_error;
    }

    RB_PowerOnAndCheck(2);
    while ( gRotor.status != ROTOR_READY )
    {
        BackGroundProcess();
    }

    if ( gRotor.result != SUCCESS )
    {
        WI(gRotor.result);
        gBalancingTaskResult = gRotor.result;
        goto program_error;
    }

    RB_GetInformation();
    while ( gRotor.status != ROTOR_FINISHED )
    {
        BackGroundProcess();
    }

    if ( gRotor.result == SUCCESS )
    {
        SetBalancerWeight((float)gRotor.weight[0]/10, (float)gRotor.weight[1]/10, (float)gRotor.weight[2]/10);
    }
    else
    {
        WI(gRotor.result);
        gBalancingTaskResult = gRotor.result;
        goto program_error;
    }

    for ( int test_idx=0 ; test_idx<TEST_CNT ; test_idx++ )
    {
        DPRINTF(("\r\nTEST %i/%i", test_idx+1, TEST_CNT));

        int pos_idx = 0;
        int pos_idx_limit = (ORG_TEST_CNT+3)*3;

        switch ( test_rpm )
        {
            case __SWING_ROTOR_1000_RPM__:
                pos_idx = (ORG_TEST_CNT+3);
                pos_idx_limit = (ORG_TEST_CNT+3)*2;
                break;
            case __SWING_ROTOR_2000_RPM__:
                pos_idx = (ORG_TEST_CNT+3)*2;
                pos_idx_limit = (ORG_TEST_CNT+3)*3;
                break;
            default:
                if ( test_rpm!=0 && test_rpm==__SWING_ROTOR_300_RPM__ )
                    pos_idx_limit = (ORG_TEST_CNT+3);
                break;
        }

        for (  ; pos_idx<pos_idx_limit ; pos_idx++ )
        {
//program_retry:
            DPRINTF(("\r\nRPM %i POS (%i.%02imm, %i.%02imm, %i.%02imm)\r\n",\
                   scenario[pos_idx][0],\
                   scenario[pos_idx][1]/100, scenario[pos_idx][1]%100,\
                   scenario[pos_idx][2]/100, scenario[pos_idx][2]%100,\
                   scenario[pos_idx][3]/100, scenario[pos_idx][3]%100));

            if ( gRPM != 0 )
            {
                DPUTS("ERROR: still running");
                WI(gRPM);
                gBalancingTaskResult = ERROR_STILL_RUN;
                goto program_error;
            }

            RB_PowerOnAndCheck(2);
            while ( gRotor.status != ROTOR_READY )
            {
                BackGroundProcess();
            }

            if ( gRotor.result != SUCCESS )
            {
                WI(gRotor.result);
                gBalancingTaskResult = gRotor.result;
                goto program_error;
            }

            RB_Move(0, scenario[pos_idx][1], scenario[pos_idx][2], scenario[pos_idx][3]);
            while ( gRotor.status != ROTOR_FINISHED )
            {
                BackGroundProcess();
            }

            WATCHI("Balancer1 pos", gRotor.move_result[0]);
            WATCHI("Balancer2 pos", gRotor.move_result[1]);
            WATCHI("Balancer3 pos", gRotor.move_result[2]);
            DPUTS("");

            if ( gRotor.result != SUCCESS )
            {
                WI(gRotor.result);
                gBalancingTaskResult = gRotor.result;
                goto program_error;
            }

            RotorPower(0);
            SlipRingPower(0);
            DSP_Start();

            retry_cnt = 0;
run_retry:
            Run_GetVibPhase(scenario[pos_idx][0], 0);
            while ( gRun.status == ACCEL_SPEED || gRun.status == MEASURE_VIB )
            {
                BackGroundProcess();
            }

            if ( gRun.result == ERROR_MOTOR_UNDER_ACCEL && retry_cnt < 3 )
            {
                DPUTS("ERROR_MOTOR_UNDER_ACCEL, RETRY");

                retry_cnt++;
                while ( gRPM != 0 )
                {
                    BackGroundProcess();
                }

                goto run_retry;
            }

            if ( gRun.result != SUCCESS )
            {
                WI(gRun.result);
                gBalancingTaskResult = gRun.result;
                goto program_error;
            }

            if ( pos_idx%(ORG_TEST_CNT+3) < ORG_TEST_CNT )
            {
                result_org_vib[pos_idx%(ORG_TEST_CNT+3)] = gRun.vib;
                result_org_phase[pos_idx%(ORG_TEST_CNT+3)] = gRun.phase;

                if ( pos_idx%(ORG_TEST_CNT+3) == ORG_TEST_CNT-1 )
                {
                    org_vib = GetAverageFloat(&result_org_vib[0], ORG_TEST_CNT, FALSE);
                    org_phase = GetAverageFloat(&result_org_phase[0], ORG_TEST_CNT, TRUE);

                    DPUTS("");
                    WF(org_vib);
                    WF(org_phase);
                }
            }
            else
            {
                GetCompensationConstant((pos_idx%(ORG_TEST_CNT+3))-ORG_TEST_CNT, (scenario[pos_idx][1]+scenario[pos_idx][2]+scenario[pos_idx][3])/100,
                                        org_vib, org_phase, gRun.vib, gRun.phase, &vib_const, &phase_const);
                DPUTS("");
                DPRINTF(("result_vib[%i][%i]: %s", pos_idx/(ORG_TEST_CNT+3), (pos_idx%(ORG_TEST_CNT+3))-ORG_TEST_CNT+test_idx*3, F2S(vib_const)));
                DPRINTF(("result_phase[%i][%i]: %s", pos_idx/(ORG_TEST_CNT+3), (pos_idx%(ORG_TEST_CNT+3))-ORG_TEST_CNT+test_idx*3, F2S(phase_const)));

                result_vib[pos_idx/(ORG_TEST_CNT+3)][(pos_idx%(ORG_TEST_CNT+3))-ORG_TEST_CNT+test_idx*3] = vib_const;
                result_phase[pos_idx/(ORG_TEST_CNT+3)][(pos_idx%(ORG_TEST_CNT+3))-ORG_TEST_CNT+test_idx*3] = phase_const;

                WF(vib_const);
                WF(phase_const);
            }

            Run_Stop();
            while ( gRPM != 0 )
            {
                BackGroundProcess();
            }

            DSP_Stop();
        }
    }

    DPUTS("");
    WF(gSwingRotorBalancingParam.vib_const[0]);
    WF(gSwingRotorBalancingParam.phase_const[0]);
    WF(gSwingRotorBalancingParam.limit_vib_at_rpm[0]);
    WF(gSwingRotorBalancingParam.vib_const[1]);
    WF(gSwingRotorBalancingParam.phase_const[1]);
    WF(gSwingRotorBalancingParam.limit_vib_at_rpm[1]);
    WF(gSwingRotorBalancingParam.vib_const[2]);
    WF(gSwingRotorBalancingParam.phase_const[2]);
    WF(gSwingRotorBalancingParam.limit_vib_at_rpm[2]);

    if ( test_rpm == 0 || test_rpm == __SWING_ROTOR_300_RPM__ )
    {
        vib_const = GetAverageFloat(&result_vib[0][0], TEST_CNT*3, FALSE);
        phase_const = GetAverageFloat(&result_phase[0][0], TEST_CNT*3, TRUE);

        if ( gSwingRotorBalancingParam.limit_mass_at_rpm[0] != 0 )
            gSwingRotorBalancingParam.limit_vib_at_rpm[0] = gSwingRotorBalancingParam.limit_mass_at_rpm[0] * ROTOR_RADIUS_FOR_VIB_LIMIT * vib_const;

        gSwingRotorBalancingParam.vib_const[0] = vib_const;
        gSwingRotorBalancingParam.phase_const[0] = phase_const;

        DPUTS("");
        WATCHF("final_vib_const at "__SWING_ROTOR_300_RPM_STR__"rpm", vib_const);
        WATCHF("final_phase_const at "__SWING_ROTOR_300_RPM_STR__"rpm", phase_const);
    }

    if ( test_rpm == 0 || test_rpm == __SWING_ROTOR_1000_RPM__ )
    {
        vib_const = GetAverageFloat(&result_vib[1][0], TEST_CNT*3, FALSE);
        phase_const = GetAverageFloat(&result_phase[1][0], TEST_CNT*3, TRUE);

        if ( adjust_phase )
        {
            DPUTS("");
            WATCHF("org_phase", gSwingRotorBalancingParam.org_phase);
            WATCHF("new_phase", phase_const);
            WATCHF("new_phase - org_phase", phase_const - gSwingRotorBalancingParam.org_phase);

            gSwingRotorBalancingParam.phase_const[0] += phase_const - gSwingRotorBalancingParam.org_phase;
            gSwingRotorBalancingParam.phase_const[1] += phase_const - gSwingRotorBalancingParam.org_phase;
            gSwingRotorBalancingParam.phase_const[2] += phase_const - gSwingRotorBalancingParam.org_phase;
        }
        else
        {
            if ( gSwingRotorBalancingParam.limit_mass_at_rpm[1] != 0 )
                gSwingRotorBalancingParam.limit_vib_at_rpm[1] = gSwingRotorBalancingParam.limit_mass_at_rpm[1] * ROTOR_RADIUS_FOR_VIB_LIMIT * vib_const;

            gSwingRotorBalancingParam.vib_const[1] = vib_const;
            gSwingRotorBalancingParam.phase_const[1] = phase_const;

            DPUTS("");
            WATCHF("final_vib_const at " __SWING_ROTOR_1000_RPM_STR__ "rpm", vib_const);
            WATCHF("final_phase_const at " __SWING_ROTOR_1000_RPM_STR__ "rpm", phase_const);
        }

        gSwingRotorBalancingParam.org_phase = phase_const;
    }

    if ( test_rpm == 0 || test_rpm == __SWING_ROTOR_2000_RPM__ )
    {
        vib_const = GetAverageFloat(&result_vib[2][0], TEST_CNT*3, FALSE);
        phase_const = GetAverageFloat(&result_phase[2][0], TEST_CNT*3, TRUE);

        if ( gSwingRotorBalancingParam.limit_mass_at_rpm[2] != 0 )
            gSwingRotorBalancingParam.limit_vib_at_rpm[2] = gSwingRotorBalancingParam.limit_mass_at_rpm[2] * ROTOR_RADIUS_FOR_VIB_LIMIT * vib_const;

        gSwingRotorBalancingParam.vib_const[2] = vib_const;
        gSwingRotorBalancingParam.phase_const[2] = phase_const;

        DPUTS("");
        WATCHF("final_vib_const at " __SWING_ROTOR_2000_RPM_STR__ "rpm", vib_const);
        WATCHF("final_phase_const at " __SWING_ROTOR_2000_RPM_STR__ "rpm", phase_const);

#ifdef __USE_BALANCER_DEFAULT_POS__
        // 로터 무게추 기본 위치 계산

        double amount[3];

        SetCompensationConstant(vib_const, phase_const);
        GetCompensationRawResult(org_vib, org_phase, &amount[0], &amount[1], &amount[2]);

        gRotor.default_pos[0] = (amount[0] / gWeight_Mass[0]) * 100;
        gRotor.default_pos[1] = (amount[1] / gWeight_Mass[1]) * 100;
        gRotor.default_pos[2] = (amount[2] / gWeight_Mass[2]) * 100;

        DPUTS("");
        WATCHI("balancer defalut position 0", gRotor.default_pos[0]);
        WATCHI("balancer defalut position 1", gRotor.default_pos[1]);
        WATCHI("balancer defalut position 2", gRotor.default_pos[2]);

        RB_PowerOnAndCheck(2);
        while ( gRotor.status != ROTOR_READY )
        {
            BackGroundProcess();
        }

        if ( gRotor.result != SUCCESS )
        {
            WI(gRotor.result);
            gBalancingTaskResult = gRotor.result;
            goto program_error;
        }

        RB_SetWeightDefaultPosition(gRotor.default_pos[0], gRotor.default_pos[1], gRotor.default_pos[2]);
        while ( gRotor.status != ROTOR_FINISHED )
        {
            BackGroundProcess();
        }

        if ( gRotor.result != SUCCESS )
        {
            WI(gRotor.result);
            gBalancingTaskResult = gRotor.result;
            goto program_error;
        }

        RotorPower(0);
        SlipRingPower(0);
#endif
    }

    DPUTS("");
    WF(gSwingRotorBalancingParam.vib_const[0]);
    WF(gSwingRotorBalancingParam.phase_const[0]);
    WF(gSwingRotorBalancingParam.limit_vib_at_rpm[0]);
    WF(gSwingRotorBalancingParam.vib_const[1]);
    WF(gSwingRotorBalancingParam.phase_const[1]);
    WF(gSwingRotorBalancingParam.limit_vib_at_rpm[1]);
    WF(gSwingRotorBalancingParam.vib_const[2]);
    WF(gSwingRotorBalancingParam.phase_const[2]);
    WF(gSwingRotorBalancingParam.limit_vib_at_rpm[2]);
    DPUTS("");

    FRAM_Write(100, &gSwingRotorBalancingParam, sizeof(gSwingRotorBalancingParam));
    GyrozenCalibEnd();

    return;

program_error:
    GyrozenSendError(gBalancingTaskResult, 0);
    RotorPower(0);
    SlipRingPower(0);
    DSP_Stop();
    if ( gRPM > 0 ) GyrozenMotorStop();

    FEND();
}

