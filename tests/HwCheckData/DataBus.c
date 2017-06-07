#include "DataBus.h"
#include "DataBus_par.h"

static void DataBusWait(void);

void DataBusIdle(void)
{
    DDRC = 0u;  /* data bus as input */

    Dec138Set(E_DEC138_Y7); /* deactivate any signals */

    return;
}

INT8U DataBusRead(void)
{
    INT8U data = 0u;

    DataBusIdle();

    Dec138Set(DEC_138_ENUM_Y2); /* activate read signal */

    DataBusWait();

    data = PINC;

    DataBusIdle();

    return data;
}

void DataBusSet(INT8U data)
{
    DataBusIdle();

    DDRC = 0xFFu;   /* data bus as output */

    PORTC = data;

    /* external module should generate write signal and set data bus idle */

    return;
}

void DataBusWrite(void)
{
    Dec138Set(DEC_138_ENUM_Y3);     /* activate write signal */

    return;
}

static void DataBusWait(void)
{
    INT8U i = 0u;

    for (i = 0u; i < DATA_BUS_MAX_WAIT_CNT; ++i)
    {
        asm("nop");
    }

    return;
}

