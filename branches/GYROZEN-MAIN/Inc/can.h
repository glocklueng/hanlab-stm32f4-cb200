#ifndef __CAN_H__
#define __CAN_H__

#define CAN_SEND_NO_WAIT     0

extern CAN_HandleTypeDef* MOTOR_CAN;
extern CAN_HandleTypeDef* ROTOR_CAN;
extern CAN_HandleTypeDef* DSP_CAN;

int Init_CAN_MOTOR(void);
int Init_CAN_ROTOR(void);
int CAN_Send(CAN_HandleTypeDef* hanlde, int msgid, void* data, int data_len, int wait_tick);
int CAN_Recv(CAN_HandleTypeDef* handle, CanRxMsgTypeDef* data);

#endif