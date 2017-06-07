#ifndef IO_8255_IFC_H
#define IO_8255_IFC_H

#include "data_types.h"

extern void Io8255Reset(void);
extern void Io8255Config(void);
extern void Io8255SetPortA(INT8U val);
extern void Io8255SetPortB(INT8U val);
extern void Io8255SetPortC(INT8U val);

#endif

