#ifndef __GYROZEN_MOTOR_H__
#define __GYROZEN_MOTOR_H__

void MOTOR_SetRPM(int rpm);
void MOTOR_Stop(void);

void MOTOR_Init(void);
void MOTOR_Run(void);
void MotorCommProcess(void);
void MOTOR_LoopbackTest(int option);
void MOTOR_Reset(void);
void MOTOR_WriteSetting(void);
void MOTOR_ReadSetting(void);
    void MOTOR_ReadParameter(int group, int param_start_number, int param_count);
    void MOTOR_WriteParameter(int group, int param_number, unsigned short data);


#endif
