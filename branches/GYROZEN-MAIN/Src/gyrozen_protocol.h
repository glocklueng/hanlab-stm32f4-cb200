#ifndef __GYROZEN_PROTOCOL_H__
#define __GYROZEN_PROTOCOL_H__

void GyrozenProtocolInit(void);
void GyrozenProtocolProcess(void);
void GyrozenMotorSetRPM(int rpm);
void GyrozenMotorStop(void);
void GyrozenSendVibInfo(int rpm, float vib, float phase);
void GyrozenSendError(int err_code, int detail_info);
void GyrozenCalibEnd(void);
void GyrozenBalancingEnd(int try_count, float vib, float phase, int* rotor_pos);
void GyrozenAutobalancingTask(int final_rpm);


#endif