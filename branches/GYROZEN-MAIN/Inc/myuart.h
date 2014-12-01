#ifndef __MYUART_H__
#define __MYUART_H__

extern UART_HandleTypeDef* GYROZEN_UART;
extern UART_HandleTypeDef* LCD_UART;

int Init_LCDUART(void);
int Init_GYROZENUART(void);

int UARTgets(UART_HandleTypeDef* handle, char *buf, int len);
int UARTgetc(UART_HandleTypeDef* handle);
void UARTputc(UART_HandleTypeDef* handle, int ch);
void UARTprintf(UART_HandleTypeDef* handle, char const *str, ...);
void UARTwrite(UART_HandleTypeDef* handle, void const *buf, int len);

#endif