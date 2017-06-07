#ifndef USART_DRIVER_IFC_H
#define USART_DRIVER_IFC_H

extern void UsartInit(void);
extern void UsartTx(INT8U data);
extern void UsartTxStr(const char *data);
extern INT8U UsartRx(void);

#endif

