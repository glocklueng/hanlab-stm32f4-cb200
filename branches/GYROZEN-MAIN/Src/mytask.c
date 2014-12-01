#include "main.h"
#include "task.h"

#define __USE_BALANCER_DEFAULT_POS__
#define __WEIGHT_MOVE_LIMIT_OVER__

//-----------------------------------------------------------------

int gBalancingTaskResult = 0;
int gUI_AutoBalStep;
int gUI_State;

bool gCoolerStopWhenRPMZero = false;
bool gFanAutoControl = true;
bool gProgramRunning = false;
bool gHaveCooler = false;
bool gHaveTempSensor = true;

struct SwingRotorBalancingParam gSwingRotorBalancingParam;

//-----------------------------------------------------------------

void RotorCheckTask(int test_mode);
void GetConstantTask(int test_rpm, bool adjust_phase);
void AutoBalancingTask(int final_rpm, int keep_time);
void MotorTestTask(int rpm, int time, int rotor_pos1, int rotor_pos2, int rotor_pos3);
    static char* ErrorString(int err);

//-----------------------------------------------------------------

void RotorCheckTask(int test_mode)
{
    FSTART();

    switch ( test_mode )
    {
        case 0:
            strcpy(gPopup[0], "ROTOR TEST");
            break;

        case 1:
            strcpy(gPopup[0], "ROTOR RETURN ORG POS");
            break;
    }

    gPopup[1][0] = NULL;
    UpdateMainUI(true);

    gBalancingTaskResult = 0;
    gCancel = false;

    RB_PowerOnAndCheck(0);
    while ( gRotor.status != ROTOR_READY ) BackgroundProcess(true);

    if ( gRotor.result != SUCCESS )
    {
        WI(gRotor.result);
        gBalancingTaskResult = gRotor.result;
        goto error;
    }

    RB_GetInformation();
    while ( gRotor.status != ROTOR_FINISHED ) BackgroundProcess(true);

    if ( gRotor.result != SUCCESS )
    {
        WI(gRotor.result);
        gBalancingTaskResult = gRotor.result;
        goto error;
    }

    switch ( test_mode )
    {
        case 0:
            RB_Move(0, __BALANCER_MOVE_MAX__, __BALANCER_MOVE_MAX__, __BALANCER_MOVE_MAX__);
            break;
        case 1:
            RB_Move(0, 0, 0, 0);
            break;
    }
    while ( gRotor.status != ROTOR_FINISHED ) BackgroundProcess(true);

    if ( gRotor.result != SUCCESS )
    {
        WI(gRotor.result);
        gBalancingTaskResult = gRotor.result;
        goto error;
    }

error:
    RotorPower(0);
    SlipRingPower(0);

    if ( gBalancingTaskResult < 0 )
        sprintf(gPopup[1], "ERR:%s", ErrorString(gBalancingTaskResult));
    else
        sprintf(gPopup[1], " %i %i %i", gRotor.move_result[0], gRotor.move_result[1], gRotor.move_result[2]);
    UpdateMainUI(true);

    FEND();
}

void MotorTestTask(int rpm, int time, int rotor_pos1, int rotor_pos2, int rotor_pos3)
{
    FSTART();

    gFanAutoControl = true;
    gBalancingTaskResult = 0;
    gCancel = false;
    gProgramRunning = true;

    if ( rotor_pos1 >= 0 && rotor_pos2 >= 0 && rotor_pos3 >= 0 )
    {
        RB_PowerOnAndCheck(2);
        while ( gRotor.status != ROTOR_READY ) BackgroundProcess(true);

        if ( gRotor.result != SUCCESS )
        {
            WI(gRotor.result);
            gBalancingTaskResult = gRotor.result;
            goto error;
        }

        RB_GetInformation();
        while ( gRotor.status != ROTOR_FINISHED ) BackgroundProcess(true);

        if ( gRotor.result != SUCCESS )
        {
            WI(gRotor.result);
            gBalancingTaskResult = gRotor.result;
            goto error;
        }

        RB_Move(0, rotor_pos1, rotor_pos2, rotor_pos3);
        while ( gRotor.status != ROTOR_FINISHED ) BackgroundProcess(true);

        if ( gRotor.result != SUCCESS )
        {
            WI(gRotor.result);
            gBalancingTaskResult = gRotor.result;
            goto error;
        }

        RotorPower(0);
        SlipRingPower(0);
    }

    DSP_Start();

    Run_Start(rpm, time);
    while ( gRun.status == ACCEL_SPEED || gRun.status == KEEP_SPEED || gRun.status == MOTOR_TEST )
    {
        BackgroundProcess(true);
        UpdateMainUI(false);
    }

    if ( gRun.result != SUCCESS )
    {
        WI(gRun.result);
        gBalancingTaskResult = gRun.result;
        goto error;
    }

    while ( gRPM != 0 )
    {
        BackgroundProcess(true);
        UpdateMainUI(false);
    }

error:
    gProgramRunning = false;
    RotorPower(0);
    SlipRingPower(0);
    DSP_Stop();
    if ( gRPM > 0 ) Run_Stop();
    FEND();
}


void GetConstantTask(int test_rpm, bool adjust_phase)
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

    if ( adjust_phase )
        strcpy(gPopup[0], "ROTOR PHASE ADJUST");
    else
        strcpy(gPopup[0], "GET CONSTANT");
    gPopup[1][0] = NULL;

    gFanAutoControl = true;
    gBalancingTaskResult = 0;
    gCancel = false;
    gProgramRunning = true;
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
        BackgroundProcess(true);
        UpdateMainUI(false);
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
        BackgroundProcess(true);
        UpdateMainUI(false);
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
                BackgroundProcess(true);
                UpdateMainUI(false);
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
                BackgroundProcess(true);
                UpdateMainUI(false);
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
                BackgroundProcess(true);
                UpdateMainUI(false);
            }

            if ( gRun.result == ERROR_MOTOR_UNDER_ACCEL && retry_cnt < 3 )
            {
                DPUTS("ERROR_MOTOR_UNDER_ACCEL, RETRY");

                retry_cnt++;
                while ( gRPM != 0 )
                {
                    BackgroundProcess(true);
                    UpdateMainUI(false);
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
                BackgroundProcess(true);
                UpdateMainUI(false);
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
            BackgroundProcess(true);
            UpdateMainUI(false);
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
            BackgroundProcess(true);
            UpdateMainUI(false);
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

    strcpy(gPopup[1], "SUCCESS");

    goto program_stop;

program_error:
    if ( gBalancingTaskResult > 0 )
    {
        DPUTS("TEST CANCEL: by user input, wait door-open");
        strcpy(gPopup[1], "USER CANCEL");
    }
    else
    {
        sprintf(gPopup[1], "ERR:%s", ErrorString(gBalancingTaskResult));
    }

program_stop:
    gProgramRunning = false;
    RotorPower(0);
    SlipRingPower(0);
    DSP_Stop();
    if ( gRPM > 0 ) Run_Stop();
    FEND();
}


void AutoBalancingTask(int final_rpm, int keep_time)
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
#ifdef __USE_BALANCER_DEFAULT_POS__
    int balancer_default_pos[3] = { 0, };
#endif
#ifdef __WEIGHT_MOVE_LIMIT_OVER__
    const int balancer_limit_over_value = 200;
#endif

    FSTART();
    DPUTS("{");
    WI(final_rpm);
    WI(keep_time);

    if ( gSetting.lock_program_19 && gProgramIdx==19 )
    {
        DPUTS("ERROR: program #19 is locked");
        gBalancingTaskResult = ERROR_PROGRAM_19_LOCK;
        goto program_error;
    }

    if ( gSetting.tte.motor < 100 )
    {
        DPUTS("ERROR: motor tte expired");
        gBalancingTaskResult = ERROR_TTE_MOTOR_EXPIRED;
        goto program_error;
    }
    else if ( gSetting.tte.swing_rotor < 100 )
    {
        DPUTS("ERROR: swing rotor tte expired");
        gBalancingTaskResult = ERROR_TTE_ROTOR_EXPIRED;
        goto program_error;
    }
    else if ( gSetting.tte.bucket < 100 )
    {
        DPUTS("ERROR: bucket tte expired");
        gBalancingTaskResult = ERROR_TTE_BUCKET_EXPIRED;
        goto program_error;
    }

    FRAM_Read(100, &gSwingRotorBalancingParam, sizeof(gSwingRotorBalancingParam));

    if ( keep_time <= 0 )
        keep_time = 300;
    if ( final_rpm <= 0 )
        final_rpm = 3000;

    WF(gSwingRotorBalancingParam.vib_const[0]);
    WF(gSwingRotorBalancingParam.phase_const[0]);
    WF(gSwingRotorBalancingParam.vib_const[1]);
    WF(gSwingRotorBalancingParam.phase_const[1]);
    WF(gSwingRotorBalancingParam.vib_const[2]);
    WF(gSwingRotorBalancingParam.phase_const[2]);

    if ( gProgramIdx != 19 &&
         gSwingRotorBalancingParam.vib_const[0]==0 || gSwingRotorBalancingParam.vib_const[1]==0 || gSwingRotorBalancingParam.vib_const[2]==0 ||
         gSwingRotorBalancingParam.phase_const[0]==0 || gSwingRotorBalancingParam.phase_const[1]==0 || gSwingRotorBalancingParam.phase_const[2]==0 ||
         gSwingRotorBalancingParam.limit_vib_at_rpm[0]==0 || gSwingRotorBalancingParam.limit_vib_at_rpm[1]==0 || gSwingRotorBalancingParam.limit_vib_at_rpm[2]==0 )
    {
        DPUTS("ERROR: configuration not inited");
        gBalancingTaskResult = ERROR_PARAMETER;
        goto program_error;
    }

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

    if ( gCoolerAutoControl )
    {
        if ( GetChamberTemp() >= (gCoolerTemp*100)+200 )
        {
            DPUTS("ERROR: need pre-cooling");
            gBalancingTaskResult = ERROR_NOT_PRE_COOLING;
            goto program_error;
        }
    }

    strcpy(gPopup[0], "PROGRAM RUNNING");
    gPopup[1][0] = NULL;

    gFanAutoControl = true;
    gBalancingTaskResult = 0;
    gCancel = false;
    gProgramRunning = true;

    RB_PowerOnAndCheck(2);
    while ( gRotor.status != ROTOR_READY )
    {
        BackgroundProcess(true);
        UpdateMainUI(false);
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
        BackgroundProcess(true);
        UpdateMainUI(false);
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
    gUI_AutoBalStep = 0;
    gUI_State = UI_AUTOBALANCE;

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

        if ( gProgramIdx == 19 )
        {
            RB_PowerOnAndCheck(2);
            while ( gRotor.status != ROTOR_READY )
            {
                BackgroundProcess(true);
                UpdateMainUI(false);
            }

            if ( gRotor.result == SUCCESS )
            {
                RB_Move(0, balancer_default_pos[0], balancer_default_pos[1], balancer_default_pos[2]);
                while ( gRotor.status != ROTOR_FINISHED )
                {
                    BackgroundProcess(true);
                    UpdateMainUI(false);
                }
            }
        }
        else
        {
            RB_PowerOnAndCheck(2);
            while ( gRotor.status != ROTOR_READY )
            {
                BackgroundProcess(true);
                UpdateMainUI(false);
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
                BackgroundProcess(true);
                UpdateMainUI(false);
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

        if ( step == 1 ) gUI_AutoBalStep = 2;
        else if ( step > 1 ) gUI_AutoBalStep = 4;

        RotorPower(0);
        SlipRingPower(0);
        DSP_Start();

        if ( gProgramIdx!=19 && final_rpm>=__SWING_ROTOR_300_RPM__ && !skip_300rpm )
        {
            retry_cnt = 0;
run_retry_300:
            Run_GetVibPhase(__SWING_ROTOR_300_RPM__, 0); // gSwingRotorBalancingParam.limit_vib_at_rpm[0]);
            while ( gRun.status == ACCEL_SPEED || gRun.status == MEASURE_VIB )
            {
                BackgroundProcess(true);
                UpdateMainUI(false);
            }

            if ( gRun.result == ERROR_MOTOR_UNDER_ACCEL && retry_cnt < 1 )
            {
                DPUTS("ERROR_MOTOR_UNDER_ACCEL, RETRY");

                retry_cnt++;
                while ( gRPM != 0 )
                {
                    BackgroundProcess(true);
                    UpdateMainUI(false);
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

                if ( step == 0 ) gUI_AutoBalStep = 1;
                else if ( step <= 3 ) gUI_AutoBalStep = 3;

                Run_Stop();
                while ( gRPM != 0 )
                {
                    BackgroundProcess(true);
                    UpdateMainUI(false);
                }

                continue;
            }
        }

        if ( gProgramIdx!=19 && final_rpm>=__SWING_ROTOR_1000_RPM__ && !skip_1000rpm )
        {
            retry_cnt = 0;
run_retry_1000:
            Run_GetVibPhase(__SWING_ROTOR_1000_RPM__, 0); // gSwingRotorBalancingParam.limit_vib_at_rpm[1]);
            while ( gRun.status == ACCEL_SPEED || gRun.status == MEASURE_VIB )
            {
                BackgroundProcess(true);
                UpdateMainUI(false);
            }

            if ( gRun.result == ERROR_MOTOR_UNDER_ACCEL && retry_cnt < 1 )
            {
                DPUTS("ERROR_MOTOR_UNDER_ACCEL, RETRY");

                retry_cnt++;
                while ( gRPM != 0 )
                {
                    BackgroundProcess(true);
                    UpdateMainUI(false);
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

                if ( step == 0 ) gUI_AutoBalStep = 1;
                else if ( step <= 3 ) gUI_AutoBalStep = 3;

                Run_Stop();
                while ( gRPM != 0 )
                {
                    BackgroundProcess(true);
                    UpdateMainUI(false);
                }

                skip_300rpm = TRUE;
                continue;
            }
        }

        if ( gProgramIdx!=19 && final_rpm>=__SWING_ROTOR_2000_RPM__ )
        {
            retry_cnt = 0;
run_retry_2000:
            Run_GetVibPhase(__SWING_ROTOR_2000_RPM__, 0); // gSwingRotorBalancingParam.limit_vib_at_rpm[2]);
            while ( gRun.status == ACCEL_SPEED || gRun.status == MEASURE_VIB )
            {
                BackgroundProcess(true);
                UpdateMainUI(false);
            }

            if ( gRun.result == ERROR_MOTOR_UNDER_ACCEL && retry_cnt < 1 )
            {
                DPUTS("ERROR_MOTOR_UNDER_ACCEL, RETRY");

                retry_cnt++;
                while ( gRPM != 0 )
                {
                    BackgroundProcess(true);
                    UpdateMainUI(false);
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

                if ( step == 0 ) gUI_AutoBalStep = 1;
                else if ( step <= 3 ) gUI_AutoBalStep = 3;

                Run_Stop();
                while ( gRPM != 0 )
                {
                    BackgroundProcess(true);
                    UpdateMainUI(false);
                }

                skip_300rpm = TRUE;
                skip_1000rpm = TRUE;
                continue;
            }
        }

        gDSP.vib_log_print_once_at_rpm = 2200;

        retry_cnt = 0;
run_retry_final:
        Run_Start(final_rpm, keep_time);
        while ( gRun.status == ACCEL_SPEED || gRun.status == KEEP_SPEED || gRun.status == MOTOR_TEST )
        {
            BackgroundProcess(true);
            UpdateMainUI(false);
        }

        if ( gRun.result == ERROR_MOTOR_UNDER_ACCEL && retry_cnt < 1 )
        {
            DPUTS("ERROR_MOTOR_UNDER_ACCEL, RETRY");

            retry_cnt++;
            while ( gRPM != 0 )
            {
                BackgroundProcess(true);
                UpdateMainUI(false);
            }

            goto run_retry_final;
        }

        if ( gRun.result != SUCCESS )
        {
            WI(gRun.result);
            gBalancingTaskResult = gRun.result;
            goto program_error;
        }

        // TTE 계산
        {
            if ( final_rpm <= 3400 )
                gSetting.tte.swing_rotor -= keep_time * 22 / 52;
            else if ( final_rpm <= 3900 )
                gSetting.tte.swing_rotor -= keep_time * 22 / 50;
            else if ( final_rpm <= 4100 )
                gSetting.tte.swing_rotor -= keep_time * 22 / 40;
            else if ( final_rpm <= 4300 )
                gSetting.tte.swing_rotor -= keep_time * 22 / 30;
            else if ( final_rpm <= 4400 )
                gSetting.tte.swing_rotor -= keep_time * 22 / 25;
            else
                gSetting.tte.swing_rotor -= keep_time;

            if ( final_rpm <= 2900 )
                gSetting.tte.bucket -= keep_time * 22 / 52;
            else if ( final_rpm <= 3400 )
                gSetting.tte.bucket -= keep_time * 22 / 45;
            else if ( final_rpm <= 3900 )
                gSetting.tte.bucket -= keep_time * 22 / 34;
            else if ( final_rpm <= 4100 )
                gSetting.tte.bucket -= keep_time * 22 / 29;
            else if ( final_rpm <= 4300 )
                gSetting.tte.bucket -= keep_time * 22 / 25;
            else if ( final_rpm <= 4400 )
                gSetting.tte.bucket -= keep_time * 22 / 23;
            else
                gSetting.tte.bucket -= keep_time;

            gSetting.tte.motor -= keep_time;

            if ( gSetting.tte.motor < 0 ) gSetting.tte.motor = 0;
            if ( gSetting.tte.bucket < 0 ) gSetting.tte.bucket = 0;
            if ( gSetting.tte.swing_rotor < 0 ) gSetting.tte.swing_rotor = 0;
        }


        gUI_State = UI_PROGRAM_SUCCESS;
        gBalancingTaskResult = SUCCESS;

        strcpy(gPopup[1], "SUCCESS");

        while ( gRPM != 0 )
        {
            BackgroundProcess(true);
            UpdateMainUI(false);
        }

        goto program_stop;
    }

    gBalancingTaskResult = ERROR_AUTOBALANCING;
    goto program_error;

program_error:
    if ( gBalancingTaskResult > 0 )
    {
        DPUTS("TEST CANCEL: by user input");
        strcpy(gPopup[1], "USER CANCEL");
    }
    else
    {
        sprintf(gPopup[1], "ERR:%s", ErrorString(gBalancingTaskResult));
    }

program_stop:
    /*
    if ( gCoolerAutoControl )
        gCoolerStopWhenRPMZero = true;
    */
    gProgramRunning = false;
    RotorPower(0);
    SlipRingPower(0);
    DSP_Stop();
    if ( gRPM > 0 ) Run_Stop();

    DPRINTF(("\r\nTEST END"));
    DPUTS("}");
    FEND();
}

char* ErrorString(int err)
{
    /*
    const char error_string[] = {
        "ERROR_UNKNOWN", // -1
        "ERROR_PARAMETER", // -2
        "ERROR_STILL_RUN", // -3
        "ERROR_OVER_RANGE", // -4
        "ERROR_AUTOBALANCING", // -5
        "ERROR_NOT_DEFINED", // -6
        "ERROR_NOT_DEFINED", // -7
        "ERROR_NOT_DEFINED", // -8
        "ERROR_NOT_DEFINED", // -9
        "ERROR_MAINBOARD_CAN", // -10
        "ERROR_FRAM_CLEAR", // -11
        "ERROR_FRAM_CANNOT_WRITE", // -12
        "ERROR_ROTOR_TYPE", // -13
        "ERROR_ROTOR_POWER", // -14
        "ERROR_ROTOR_NOT_RESPONDING", // -15
        "ERROR_ROTOR_RET_ORG", // -16
        "ERROR_ROTOR_JAM", // -17
        "ERROR_ROTOR_TIMEOUT", // -18
        "ERROR_DSP_POWER", // -20
        "ERROR_DSP_NOT_RESPONDING", // -21
        "ERROR_DSP_TIMEOUT", // -22
        "ERROR_MOTOR_COMM", // -23
        "ERROR_MOTOR_IMBALANCE", // -25
        "ERROR_MOTOR_OVER_VOLTAGE", // -26
        "ERROR_MOTOR_OVER_CURRENT", // -27
        "ERROR_MOTOR_OVER_HEAT_CTRLER", // -28
        "ERROR_MOTOR_OVER_HEAT_MOTOR", // -29
        "ERROR_MOTOR_HALL_SENSOR", // -30
        "ERROR_MOTOR_LOW_VOLTAGE", // -31
        "ERROR_MOTOR_OVER_ACCEL", // -32
        "ERROR_MOTOR_UNDER_ACCEL", // -33
        "ERROR_MOTOR_CANNOT_KEEP", // -34
        "ERROR_MOTOR_FREERUN", // -35
        "ERROR_MOTOR_CANNOT_DECEL", // -36
        "ERROR_MOTOR_CANNOT_STOP", // -37
        "ERROR_DOOR_OPEN", // -38
        "ERROR_NO_RPM", // -39
        "ERROR_VIB_TOO_MUCH", // -40
        "ERROR_VIB_UNSTABLE", // -41
        "ERROR_TTE_MOTOR_EXPIRED", // -42
        "ERROR_TTE_ROTOR_EXPIRED", // -43
        "ERROR_TTE_BUCKET_EXPIRED", // -44
        "ERROR_TTE_COOLER_EXPIRED", // -45
        "ERROR_TEMP_CONTROL", // -46
        "ERROR_FRAM_READ", // -47
        "ERROR_FRAM_WRITE", // -48
        "ERROR_FRAM_CRC", // -49
        "ERROR_ROTOR_PAIR", // -50
        "ERROR_FRAM_COMPARE", // -51
        "ERROR_PROGRAM_19_LOCK", // -52
        "ERROR_DSP_NOT_EXIST_VIB_SENSOR", // -53
        "ERROR_NOT_PRE_COOLING", // -54
    };
    */

    /*
    "Configuration cleared",
    "FRAM Init. Error",
    "Temp. Sensor Error",
    "Configuration not inited",
    "DSP Power Error",
    "DSP not responding",
    "Vib Measuring Error",
    "Imbalance deteced",
    "Balancing Range Error",
    "Balancing failed",
    "NO RPM Signal",
    "Rotor Pair Error",
    "Rotor Power Error",
    "Rotor Size Error",
    "Balancer Return Error",
    "Balancer is jammed",
    "Rotor not responding",
    "Balancer timeout",
    "Temp. Control Error",
    "Motor Comm. Error",
    "Motor Accel. Error",
    "Motor Error",
    "Motor Speed Error",
    "Motor Decel. Error",
    "Motor Stop Error",
    "Door is open",
    */

    const char* error_string[] = {
        "SUCCESS", // 0
        "unknown", // -1
        "setting cleared", // -2
        "rotor still run", // -3
        "balancing range", // -4
        "balancing failed", // -5
        "", // -6
        "", // -7
        "", // -8
        "", // -9
        "CAN transmit fail", // -10
        "setting cleared", // -11
        "FRAM init.", // -12
        "rotor type", // -13
        "rotor power", // -14
        "rotor no response", // -15
        "balancer return", // -16
        "balancer jammed", // -17
        "balancer timeout", // -18
        "", // -19
        "DSP power", // -20
        "DSP no response", // -21
        "VIB measuring", // -22
        "motor comm error", // -23
        "", // -24
        "Imbalance deteced", // -25
        "Motor Error (OV)", // -26
        "Motor Error (OC)", // -27
        "Motor Error (OHC)", // -28
        "Motor Error (OHM)", // -29
        "Motor Error (HS)", // -30
        "Motor Error (LV)", // -31
        "Motor Accel Err 1", // -32
        "Motor Accel Err 2", // -33
        "Motor Speed Error", // -34
        "Motor Freerun", // -35
        "Motor Decel Error", // -36
        "Motor Stop Error", // -37
        "Door is open", // -38
        "NO RPM Signal", // -39
        "vib is too much", // -40
        "vib unstable", // -41
        "TTE_MOTOR_EXPIRE", // -42
        "TTE_ROTOR_EXPIRE", // -43
        "TTE_BUCKET_EXPIRE", // -44
        "TTE_COOLER_EXPIRE", // -45
        "Temp Ctrl Error", // -46
        "FRAM_READ", // -47
        "FRAM_WRITE", // -48
        "FRAM_CRC", // -49
        "Rotor Pair Error", // -50
        "FRAM_COMPARE", // -51
        "PROGRAM_19_LOCK", // -52
        "NOT_EXIST_VSENSOR", // -53
        "NEED PRE-COOLING", // -54
    };

    if ( err <= 0 && err >= -54 )
    {
        err = -err;
        WI(err);
        WS(error_string[err]);
        return (char*) error_string[err];
    }
    else
    {
        return "NO ERROR";
    }
}

