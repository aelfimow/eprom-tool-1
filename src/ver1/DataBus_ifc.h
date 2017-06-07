#ifndef DATA_BUS_IFC_H
#define DATA_BUS_IFC_H

extern INT8U DataBusRead(void);
extern void DataBusIdle(void);
extern void DataBusSet(INT8U data);
extern void DataBusWrite(void);
extern void DataBusPulseWrite(void);

#endif

