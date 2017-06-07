#ifndef PROM_IF_H
#define PROM_IF_H

#include <windows.h>
#include <process.h>

enum tPromIfRxState
{
    E_RX_START = 0u,
    E_RX_LENGTH,
    E_RX_DATA,
    E_RX_CHECKSUM,
    E_RX_END
};

#endif

