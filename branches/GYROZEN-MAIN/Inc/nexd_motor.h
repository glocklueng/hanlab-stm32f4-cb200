#ifndef __RS485_MOTOR_H__
#define __RS485_MOTOR_H__

#define NEXD_ERROR_OVER_VOLTAGE             0x1
#define NEXD_ERROR_OVER_CURRENT             0x2
#define NEXD_ERROR_OVER_HEAT_CONTROLLER     0x4
#define NEXD_ERROR_OVER_HEAT_MOTOR          0x8     // 실제로 드라이버에서 발생시키지 않고 있음, 메인보드에서 체크해야 함
#define NEXD_ERROR_HALL_SENSOR              0x10
#define NEXD_ERROR_LOW_VOLTAGE              0x20    // under DC link 170V
#define NEXD_ERROR_OVER_CURRENT2            0x40
#define NEXD_ERROR_OVER_VOLTAGE2            0x80

//------------------------------------------------------------------------------

struct NEXDMotorStatusType
{
    volatile bool send_fail;
    volatile bool connected;
    volatile bool updated;
    volatile unsigned short rpm;
    volatile unsigned short vdclink;
    volatile unsigned char status;
    volatile unsigned char error;
    volatile unsigned char controller_temp;
    volatile unsigned char motor_temp;
};

extern struct NEXDMotorStatusType gNEXDMotor_Status;

//------------------------------------------------------------------------------

void MOTOR_Run(void);
void MOTOR_SetRPM(int rpm);
void MOTOR_Stop(void);
void MOTOR_Reset(void);
void MOTOR_Free(void);

int NEXDMotor_Free(void);
int NEXDMotor_Reset(void);
int NEXDMotor_DriverOn(void);
int NEXDMotor_VibOn(void);
int NEXDMotor_NoLoadOn(void);
int NEXDMotor_SendRpm(unsigned short new_rpm);
int NEXDMotor_HalfRotation(void);
void NEXDMotor_EmergencyOn(void);
void NEXDMotor_EmergencyOff(void);
void NEXDMotor_SetRotorTypeAngle(bool angle_rotor);
void NEXDMotor_CheckMotorStatusThread(void const * argument);

#endif
