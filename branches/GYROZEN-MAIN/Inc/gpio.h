#ifndef __GPIO_H__
#define __GPIO_H__

#define KEY_NONE    0
#define KEY_STOP    GPIO_PIN_7
#define KEY_START   GPIO_PIN_8
#define KEY_DOOR    GPIO_PIN_9
#define KEY_UP      GPIO_PIN_10
#define KEY_DOWN    GPIO_PIN_11
#define KEY_LEFT    GPIO_PIN_12
#define KEY_RIGHT   GPIO_PIN_13
#define KEY_ENTER   GPIO_PIN_14
#define KEY_SAVE    GPIO_PIN_15

#define KEY_ALL     0xFF80

void Init_GPIO(void);

void LED(int idx, bool on);
void LEDToggle(int idx);
void RotorPower(bool on);
void SlipRingPower(bool on);
void MotorFan(bool on);
void ExternalFan(bool on);
void ImbalanceOutToExt(bool on);
void CooloerValve(int idx, bool on);
void CoolerPower(int idx, bool on);
bool CoolerPowerIsOn(int idx);
void DoorOpen(void);
bool DoorIsOpened(void);
bool DoorIsClosed(void);
void DSP_HWReset(void);

void KEY_Flush(void);
void KEY_Process(void);
    unsigned short KEY_Get(void);
    unsigned short KEY_Read(void);

#endif