#ifndef __MY_TASK_H__
#define __MY_TASK_H__

#define __BALANCER_MOVE_MAX__       2400 // 1500

#define __SWING_ROTOR_300_RPM__     300
#define __SWING_ROTOR_1000_RPM__    1000
#define __SWING_ROTOR_2000_RPM__    2000

#define __SWING_ROTOR_300_RPM_STR__     "300"
#define __SWING_ROTOR_1000_RPM_STR__    "1000"
#define __SWING_ROTOR_2000_RPM_STR__    "2000"

#define ROTOR_RADIUS                175.0
#define ROTOR_RADIUS_FOR_VIB_LIMIT  111.15

struct SwingRotorBalancingParam
{
    float weight_mass[3];
    double vib_const[3];
    double phase_const[3];
    float limit_mass_at_rpm[3];
    float limit_vib_at_rpm[3];
    double org_phase;
};

enum
{
    UI_BOOTING          = 0x0000,
    UI_READY            = 0x0100,
    UI_DOOR_OPEN        = 0x0200,
    UI_ERROR            = 0x0300,
    UI_PRE_COOLING      = 0x0400,
    UI_INSERT_SAMPLE    = 0x0500,
    UI_PREPARE          = 0x0600,
    UI_COOLING          = 0x0700,
    UI_AUTOBALANCE      = 0x0800,
    UI_ACCEL            = 0x0900,
    UI_RUNNING          = 0x0A00,
    UI_ROTOR_ADJUST     = 0x0B00,
    UI_PROGRAM_SUCCESS  = 0x0C00,
    UI_PROGRAM_FAIL     = 0x0D00,
    UI_PROGRAM_CANCEL   = 0x0E00,
    UI_SETTING          = 0x0F00,
    UI_MANAGE           = 0x1000,
    UI_CHK_WEIGHT_0     = 0x1100,
    UI_CHK_ROTOR        = 0x1200,
    UI_PROGRAM_INFO     = 0x1300,

    UI_CUSTOM_FLAT      = 0xF600,
    UI_CUSTOM_FLAT_TEXT = 0xF700,
    UI_CUSTOM_MSG       = 0xF800,
    UI_CUSTOM_MSG_TEXT  = 0xF900,
    UI_CUSTOM_WARN      = 0xFA00,
    UI_CUSTOM_WARN_TEXT = 0xFB00,
    UI_CUSTOM_ERR       = 0xFC00,
    UI_CUSTOM_ERR_TEXT  = 0xFD00,
    UI_CUSTOM_QUES      = 0xFE00,
    UI_CUSTOM_QUES_TEXT = 0xFF00,
};

extern int gBalancingTaskResult;
extern int gUI_AutoBalStep;
extern int gUI_State;

extern bool gProgramRunning;
extern bool gFanAutoControl;
extern bool gHaveCooler;
extern bool gHaveTempSensor;

extern struct SwingRotorBalancingParam gSwingRotorBalancingParam;
extern int gBalancingTaskResult;
extern bool gCoolerStopWhenRPMZero;

void MotorTestTask(int rpm, int time, int rotor_pos1, int rotor_pos2, int rotor_pos3);
void RotorCheckTask(int test_mode);
void GetConstantTask(int test_rpm, bool adjust_phase);
void AutoBalancingTask(int final_rpm, int keep_time);
void NoBalancingTask(int final_rpm, int keep_time);

#endif