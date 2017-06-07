#include "Dec154.h"
#include "Dec154_common.h"
#include "Dec154_par.h"


void Dec154Set(tDataDec154 n)
{
    const INT8U mask = (INT8U)DEC_154_PORT_MASK;
    INT8U port_data = DEC_154_PORT;
    INT8U val = (INT8U)(n & mask);

    port_data &= (INT8U)~mask;
    port_data |= (INT8U)val;
    DEC_154_PORT = (INT8U)port_data;
    return;
}

void Dec154SetupIO(void)
{
    const INT8U mask = (INT8U)DEC_154_PORT_MASK;

    Dec154Set(DEC_154_INIT_PORT_VAL);
    DEC_154_DD_PORT |= (INT8U)mask;
    return;
}


