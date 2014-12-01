#ifndef __MAINUI_H__
#define __MAINUI_H__

struct PROGRAM {
    int time;
    int rpm;
    bool rcf_mode;
    bool cooler_auto_ctrl;
    int temp;
    int accel;
    int decel;
    unsigned char rotor_type;
};

extern int gProgramIdx;
extern struct PROGRAM gProgram[20];

extern char gPopup[2][30];
extern bool gCoolerManualOn;

void MainUI(void);
void UpdateMainUI(bool right_now_update);

#endif
