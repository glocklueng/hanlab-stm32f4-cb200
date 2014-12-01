#ifndef __TIMER_H__
#define __TIMER_H__

extern volatile unsigned int gRPM;
extern volatile unsigned int gPrecisionRPM;

int Init_PWM(void);
int Init_RPM(void);
int Init_BUZZER(void);

void Buzzer(int freq_hz, int time_ms);

#endif