#ifndef __MAIN_H
#define __MAIN_H

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

#define __MAIN_VERSION__     "v0.9.1"
#define __RPM_TIMER_CAPTURE__

//---------------------------------------------------------------------
// 기본 타입 정의 (다른 헤더파일들을 include 하기 전에 먼저 정의되어야 하는 것들)

#define true    (1)
#define false   (0)

#define TRUE    (1)
#define FALSE   (0)

typedef unsigned char bool;
typedef unsigned char byte;

struct GUID {
  unsigned long data1;
  unsigned short data2;
  unsigned short data3;
  unsigned char data4[8];
};

#define SWAP2(p)    { unsigned char* tempp=(unsigned char*)(p); unsigned char tempch=tempp[1]; tempp[1]=tempp[0]; tempp[0]=tempch; }
#define SWAP4(p)    { unsigned char* tempp=(unsigned char*)(p); unsigned char tempch=tempp[3]; tempp[3]=tempp[0]; tempp[0]=tempch; tempch=tempp[1]; tempp[1]=tempp[2]; tempp[2]=tempch; }

//---------------------------------------------------------------------

#include "run.h"
#include "dsp.h"
#include "rotor_comm.h"
#include "error.h"
#include "nexd_motor.h"
#include "calc_imbal.h"
#include "ansi.h"
#include "uartstdio.h"
#include "trace.h"
#include "fram.h"
#include "temp.h"
#include "debug.h"
#include "timer.h"
#include "mytask.h"
#include "myuart.h"
#include "gpio.h"
#include "can.h"
#include "cq.h"
#include "utility.h"
#include "cooler.h"
#include "mainui.h"
#include "manageui.h"
#include "old_wince_lcd.h"
#include "input.h"
#include "menu.h"
#include "myprotocol.h"
#include "display.h"
#include "gyrozen_protocol.h"
#include "gyrozen_motor.h"

//---------------------------------------------------------------------\

//extern struct GUID INTERFACENAME;

extern volatile bool gDebugMode;
extern volatile bool gCancel;

extern volatile unsigned long gSeconds;
extern volatile unsigned long gTick;

extern unsigned long gAHBClock;
extern unsigned long gAPB1Clock;
extern unsigned long gAPB2Clock;

//---------------------------------------------------------------------

#define osSemaphoreExtern(name)  extern osSemaphoreDef_t os_semaphore_def_##name; extern osSemaphoreId name
#define osSemaphoreDefine(name)  osSemaphoreDef_t os_semaphore_def_##name = { 0 }; osSemaphoreId name

#define osThreadExtern(name)  extern osThreadDef_t os_thread_def_##name; extern osThreadId name
#define osThreadDefine(name, thread, priority, instances, stacksz)  osThreadDef_t os_thread_def_##name = { #name, (thread), (priority), (instances), (stacksz) }; osThreadId name

osThreadExtern(MAIN_THREAD);
osThreadExtern(DEBUG_THREAD);
osThreadExtern(MOTOR_THREAD);
osSemaphoreExtern(UART_PRINTF_SEMAPHOR);

void BackgroundProcess(bool common_key_process);

#endif /* __MAIN_H */
