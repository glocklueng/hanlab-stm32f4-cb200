#include "main.h"
#include "gyrozen_protocol.h"
#include "gyrozen_motor.h"

//----------------------------------------------------------------------

void MOTOR_Run(void);
void MOTOR_SetRPM(int rpm);
void MOTOR_Stop(void);
void MOTOR_Reset(void);
void MOTOR_Free(void);

//----------------------------------------------------------------------

void MOTOR_SetRPM(int rpm)
{
    FSTART();
    GyrozenMotorSetRPM(rpm);
}

void MOTOR_Stop(void)
{
    FSTART();
    GyrozenMotorStop();
}

void MOTOR_Run(void)
{
    FSTART();
}

void MOTOR_Reset(void)
{
    FSTART();
}

void MOTOR_Free(void)
{
    FSTART();
}
