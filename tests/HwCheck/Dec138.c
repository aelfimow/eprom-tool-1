#include "Dec138.h"
#include "Dec138_common.h"
#include "Dec138_par.h"


/*  This function sets output of the TTL'138 decoder */
void Dec138Set(tDataDec138 n)
{
    const INT8U mask = (INT8U)DEC_138_PORT_MASK;
    INT8U port_data = (INT8U)DEC_138_PORT;
    INT8U val = (INT8U)(n & mask);

    port_data &= (INT8U)~mask;
    port_data |= (INT8U)val;
    DEC_138_PORT = (INT8U)port_data;
    return;
}

/* This function sets up port pins */
void Dec138SetupIO(void)
{
    const INT8U mask = (INT8U)DEC_138_PORT_MASK;

    Dec138Set(DEC_138_INIT_PORT_VAL);
    DEC_138_DD_PORT |= (INT8U)mask;
    return;
}

