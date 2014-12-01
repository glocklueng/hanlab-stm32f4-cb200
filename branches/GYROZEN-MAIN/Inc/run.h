#ifndef __RUN_H__
#define __RUN_H__

struct RUN_INTERFACE
{
    enum { MOTOR_STOP=0, ACCEL_SPEED=1, MEASURE_VIB=2, MEASURE_VIB_END=3, KEEP_SPEED=3, MOTOR_TEST=4, RUN_END=5, DECEL_SPEED=5,  } status;
    bool time_mode;    
    int keep_time; // seconds
    int final_rpm;
    int send_rpm;
    float limit_vib;
    unsigned long run_start_time;
    unsigned long keep_start_time;
    
    float vib;
    float phase;    
    int result;
};

extern struct RUN_INTERFACE gRun;

void Run_Process(void);

void Run_Start(int rpm, int time);
void Run_GetVibPhase(int rpm, float limit_vib);
void Run_Stop(void);

#endif
