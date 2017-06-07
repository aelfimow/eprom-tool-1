#ifndef PROM_IF_IFC_H
#define PROM_IF_IFC_H

extern void PromIfInit(void (*Callback)(INT8U *, INT32U));
extern void PromIfWrite(const INT8U *data, const INT32U length);
extern void PromIfRead(const INT8U *data, const INT32U length);

#endif

