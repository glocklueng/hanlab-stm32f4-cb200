#include "main.h"

//------------------------------------------------------------------------------

#define NEXD_STATUS_DRIVER_ON               0x1
#define NEXD_STATUS_DRIVER_RESET            0x2
#define NEXD_STATUS_SPEED_MODE              0x4
#define NEXD_STATUS_VIB_MODE                0x8
#define NEXD_STATUS_NO_LOAD_MODE            0x10
#define NEXD_STATUS_SLOW_ACCEL_MODE         0x20

//------------------------------------------------------------------------------

static bool RotorType15000 = false;
struct NEXDMotorStatusType gNEXDMotor_Status;

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

//------------------------------------------------------------------------------

/*
void MOTOR_Reset(void)
{
    NEXDMotor_Reset();
}

void MOTOR_Free(void)
{
    NEXDMotor_Free();
}

void MOTOR_Run(void)
{
    NEXDMotor_DriverOn();
}

void MOTOR_Stop(void)
{
    NEXDMotor_SendRpm(0);
}

void MOTOR_SetRPM(int rpm)
{
    NEXDMotor_SendRpm(rpm);
}
*/

//------------------------------------------------------------------------------

int NEXDMotor_HalfRotation(void)
{
    int ret = 0;

    NEXDMotor_SetRotorTypeAngle(FALSE);

    if ( (ret = NEXDMotor_Reset()) < 0 )
        goto error;

    if ( (ret = NEXDMotor_SendRpm(5)) < 0 )
        goto error;

    if ( (ret = NEXDMotor_DriverOn()) < 0 )
        goto error;

    osDelay(500);

error:
    NEXDMotor_Free();
    NEXDMotor_SendRpm(0);

    osDelay(4000);

    while ( gPrecisionRPM != 0 )
        osDelay(10);

    return ret;
}

void NEXDMotor_EmergencyOff(void)
{
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, 0); // BLDC_RUNSTOP
}

void NEXDMotor_EmergencyOn(void)
{
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, 1); // BLDC_RUNSTOP
}

int NEXDMotor_Free(void)
{
    unsigned char param[8] = { 0, };

    FSTART();

    if ( RotorType15000 ) param[5] |= 0x4;
    param[5] = 0x0;
    if ( CAN_Send(MOTOR_CAN, 0x471, param, 8, 1000) != HAL_OK )
    {
        gNEXDMotor_Status.send_fail = TRUE;
        NEXDMotor_EmergencyOn();
        RETURN(ERROR_MOTOR_COMM);
    }

    gNEXDMotor_Status.send_fail = FALSE;
    return 0;
}

int NEXDMotor_Reset(void)
{
    unsigned char buf[8] = { 0, };

    FSTART();

    if ( RotorType15000 ) buf[5] |= 0x4;
    buf[5] |= 0x2;
    if ( CAN_Send(MOTOR_CAN, 0x471, buf, 8, 1000) != HAL_OK )
    {
        gNEXDMotor_Status.send_fail = TRUE;
        NEXDMotor_EmergencyOn();
        RETURN(ERROR_MOTOR_COMM);
    }

    gNEXDMotor_Status.error &= NEXD_ERROR_OVER_HEAT_MOTOR;
    osDelay(500);

    buf[5] &= ~0x2;
    if ( CAN_Send(MOTOR_CAN, 0x471, buf, 8, 1000) != HAL_OK )
    {
        gNEXDMotor_Status.send_fail = TRUE;
        NEXDMotor_EmergencyOn();
        RETURN(ERROR_MOTOR_COMM);
    }

    gNEXDMotor_Status.send_fail = FALSE;
    return 0;
}

int NEXDMotor_DriverOn(void)
{
    unsigned char buf[8] = { 0, };

    FSTART();

    NEXDMotor_EmergencyOff();

    if ( RotorType15000 ) buf[5] |= 0x4;
    buf[5] |= 0x1;
    if ( CAN_Send(MOTOR_CAN, 0x471, buf, 8, 1000) != HAL_OK )
    {
        gNEXDMotor_Status.send_fail = TRUE;
        NEXDMotor_EmergencyOn();
        RETURN(ERROR_MOTOR_COMM);
    }

    gNEXDMotor_Status.send_fail = FALSE;
    return 0;
}

int NEXDMotor_VibOn(void)
{
    unsigned char buf[8] = { 0, };

    FSTART();

    if ( RotorType15000 ) buf[5] |= 0x4;
    buf[5] |= 0x9;
    if ( CAN_Send(MOTOR_CAN, 0x471, buf, 8, 1000) != HAL_OK )
    {
        gNEXDMotor_Status.send_fail = TRUE;
        NEXDMotor_EmergencyOn();
        RETURN(ERROR_MOTOR_COMM);
    }

    gNEXDMotor_Status.send_fail = FALSE;
    return 0;
}

int NEXDMotor_NoLoadOn(void)
{
    unsigned char buf[8] = { 0, };
    FSTART();

    if ( RotorType15000 ) buf[5] |= 0x4;
    buf[5] |= 0x11;
    if ( CAN_Send(MOTOR_CAN, 0x471, buf, 8, 1000) != HAL_OK )
    {
        gNEXDMotor_Status.send_fail = TRUE;
        NEXDMotor_EmergencyOn();
        RETURN(ERROR_MOTOR_COMM);
    }

    gNEXDMotor_Status.send_fail = FALSE;
    return 0;
}

void NEXDMotor_SetRotorTypeAngle(bool angle_rotor)
{
    WATCHB("FUNCTION START: NEXDMotor_SetRotorTypeAngle()", angle_rotor);
    RotorType15000 = angle_rotor;
}

int NEXDMotor_SendRpm(unsigned short rpm)
{
    unsigned char buf[8] = { 0, };

    WATCHI("FUNCTION START: NEXDMotor_SendRpm()", rpm);

    buf[1] = rpm >> 8;
    buf[0] = rpm & 0xff;
    if ( CAN_Send(MOTOR_CAN, 0x470, buf, 8, 1000) != HAL_OK )
    {
        gNEXDMotor_Status.send_fail = TRUE;
        NEXDMotor_EmergencyOn();
        RETURN(ERROR_MOTOR_COMM);
    }

    gNEXDMotor_Status.send_fail = FALSE;
    return 0;
}

void NEXDMotor_CheckMotorStatusThread(void const * argument)
{
    int old_error=0x80;
    CanRxMsgTypeDef msg;
    unsigned int last_recv_tick = 0;

    FSTART();
    memset(&gNEXDMotor_Status, 0, sizeof(gNEXDMotor_Status));

    while ( 1 )
    {
        osDelay(10);

        if ( gNEXDMotor_Status.connected && TICK_FROM(last_recv_tick)>500 )
        {
            NEXDMotor_EmergencyOn();
            gNEXDMotor_Status.connected = FALSE;
        }

        if ( CAN_Recv(MOTOR_CAN, &msg)==HAL_OK && msg.StdId==0x472 )
        {
            last_recv_tick = gTick;
            gNEXDMotor_Status.connected = TRUE;
            gNEXDMotor_Status.updated = TRUE;
            gNEXDMotor_Status.rpm = *(unsigned short*)&msg.Data[6];
            gNEXDMotor_Status.vdclink = *(unsigned short*)&msg.Data[4];
            gNEXDMotor_Status.status = msg.Data[0];
            gNEXDMotor_Status.error = msg.Data[2] | (gNEXDMotor_Status.error&NEXD_ERROR_OVER_HEAT_MOTOR);
            gNEXDMotor_Status.controller_temp = msg.Data[1];
            gNEXDMotor_Status.motor_temp = msg.Data[3];

            /*
            //RPM 신호가 들어오지 않을 경우 임시로 사용할 수 있는 코드
            gRPM = gNEXDMotor_Status.rpm;
            gPrecisionRPM = gNEXDMotor_Status.rpm;
            */

            if ( gNEXDMotor_Status.motor_temp > 130 )
                gNEXDMotor_Status.error |= NEXD_ERROR_OVER_HEAT_MOTOR;
            else
                gNEXDMotor_Status.error &= ~NEXD_ERROR_OVER_HEAT_MOTOR;

            if ( old_error != gNEXDMotor_Status.error )
            {
                if ( gNEXDMotor_Status.error&0x1 && !(old_error&NEXD_ERROR_OVER_VOLTAGE) )
                {
                    printf("DRIVER ERROR: Over Voltage\r\n");
                }
                if ( gNEXDMotor_Status.error&0x2 && !(old_error&NEXD_ERROR_OVER_CURRENT) )
                {
                    printf("DRIVER ERROR: Over Current\r\n");
                }
                if ( gNEXDMotor_Status.error&0x4 && !(old_error&NEXD_ERROR_OVER_HEAT_CONTROLLER) )
                {
                    printf("DRIVER ERROR: Over Heat (Controller)\r\n");
                }
                if ( gNEXDMotor_Status.error&0x8 && !(old_error&NEXD_ERROR_OVER_HEAT_MOTOR) )
                {
                    printf("DRIVER ERROR: Over Heat (Motor)\r\n");
                }
                if ( gNEXDMotor_Status.error&0x10 && !(old_error&NEXD_ERROR_HALL_SENSOR) )
                {
                    printf("DRIVER ERROR: Hall Sensor Error\r\n");
                }
                if ( gNEXDMotor_Status.error&0x20 && !(old_error&NEXD_ERROR_LOW_VOLTAGE) )
                {
                    printf("DRIVER ERROR: Low Voltage\r\n"); // under DC link 170V
                }
                if ( gNEXDMotor_Status.error&0x40 && !(old_error&NEXD_ERROR_OVER_VOLTAGE2) )
                {
                    printf("DRIVER ERROR: Over Voltage 2\r\n");
                }
                if ( gNEXDMotor_Status.error&0x80 && !(old_error&NEXD_ERROR_OVER_CURRENT2) )
                {
                    printf("DRIVER ERROR: Over Current 2\r\n");
                }
                if ( gNEXDMotor_Status.error==0 && old_error )
                {
                    printf("DRIVER ERROR CLEAR\r\n");
                }

                old_error = gNEXDMotor_Status.error;
            }
        }
    }
}

