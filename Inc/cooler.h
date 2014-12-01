#ifndef __COOLER_H__
#define __COOLER_H__

extern int gCoolerTemp;
extern volatile bool gPrintCoolerLog;
extern bool gCoolerAutoControl;

bool CoolerPowerStatus(void);
void CoolerControl(bool on);
void Cooler_Process(void);

#endif

