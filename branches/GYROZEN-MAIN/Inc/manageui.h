#ifndef __MANAGEUI_H__
#define __MANAGEUI_H__

struct SETTING
{
    bool debug;
    bool lock_program_19;
    int chamber_temp_calibration_value;
    struct {
        int motor;
        int swing_rotor;
        int bucket;
        int cooler;
    } tte;
    int temp_calibration_value;
};

extern struct SETTING gSetting;

void SettingUI(void);
void ManageUI(void);

#endif