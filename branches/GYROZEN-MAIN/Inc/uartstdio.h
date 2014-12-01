#ifndef __UARTSTDIO_H__
#define __UARTSTDIO_H__

int Init_DEBUGUART(void);
int DEBUGgets(char *buf, int len);
int DEBUGgetc(void);
void DEBUGputc(int ch);
void DEBUGwrite(void const *buf, int len);
void DEBUGprintf(char const *str, ...);
void DEBUGprintf_NewLine(char const *str, ...);
int UARTgetcNonBlocking(void);

#endif