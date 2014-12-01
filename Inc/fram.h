#ifndef __SPI_H__
#define __SPI_H__

int Init_FRAM(void);
int FRAM_Read(int addr, void* data, int cnt);
int FRAM_Write(int addr, void* data, int cnt);


#endif
