#ifndef __DSP_COMM_H__
#define __DSP_COMM_H__

#define DSP_CMD_READY               0x1
#define DSP_CMD_MEASURE_MODE_1      0x2
#define DSP_CMD_MEASURE_MODE_2      0x4

struct DSP_INTERFACE
{
    bool alive;
    bool vib_log_on;
    int  vib_log_print_once_at_rpm;

    bool imbalance_occur;
    bool dsp_no_ack;
    bool vib_sensor_no_exist;

    bool  msg_catch;
    float vib;
    float phase;
    float rms1;
    float rms2;
    int   rpm;
    int   rpm_change;
};

extern struct DSP_INTERFACE gDSP;
extern bool gImbalanceDetectDisable;

void DSP_Start(void);
void DSP_Stop(void);
void DSP_QueryAlive(void);
void DSP_SendCommand(unsigned char cmd);
void DSP_CANRecvHandler(CanRxMsgTypeDef* msg_obj);
void DSP_Process(void);

#endif
