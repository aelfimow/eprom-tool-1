#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include "data_types.h"
#include "Dec138_ifc.h"
#include "Dec154_ifc.h"
#include "Io8255_ifc.h"
#include "UsartDriver_ifc.h"
#include "AddrBus_ifc.h"
#include "DataBus_ifc.h"
#include "PromIf_ifc.h"

#define MAX_DATA_LEN    (512UL)

enum tCommand
{
    E_CMD_NOP = 0u,
    E_CMD_READ_OUT = 1u,
    E_CMD_READ_OUT_READY = 2u,
    E_CMD_WRITE = 3u,
    E_CMD_WRITE_READY = 4u
};

void Wait(void);

void SetupIO(void);
void SetLed(BOOLEAN on_flag);
void CommandCallback(INT8U *data, INT32U length);

INT8U TxData[MAX_DATA_LEN];

int main(void)
{
    INT8U RxData = 0;

    Dec138SetupIO();
    Dec154SetupIO();
    UsartInit();
    SetupIO();
    Io8255Reset();
    PromIfInit(&CommandCallback);

    SetLed(TRUE);

    sei();

    Io8255Config();

    while (TRUE)
    {
        if (FALSE == UsartRxDataAvail())
        {
            /* no data available */
        }
        else
        {
            if (0 == UsartRead(&RxData))
            {
                /* could not read data */
            }
            else
            {
                PromIfRead(&RxData, 1u);
            }
        }
    }


    return 0;
}

void SetupIO(void)
{
    DDRD |= (INT8U)_BV(PD7); /* as output */
    DDRD |= (INT8U)_BV(PD6); /* as output */
    DDRD |= (INT8U)_BV(PD5); /* as output */

    DDRC = 0u;   /* as input */

    DDRB |= (INT8U)_BV(PB0); /* as output */

    return;
}

void SetLed(BOOLEAN on_flag)
{
    const INT8U mask = 0x01u;

    if (FALSE == on_flag)
    {
        PORTB |= (INT8U)mask;   /* turn off main LED */
    }
    else
    {
        PORTB &= (INT8U)~mask;  /* turn on main LED */
    }
    return;
}

void Wait(void)
{
    INT8U i = 0;
    for (i = 0; i < 50u; ++i)
    {
        _delay_ms(50u);
    }
    return;
}

void CommandCallback(INT8U *data, INT32U length)
{
    INT32U addr = 0UL;
    INT32U offset = 0UL;
    BOOLEAN toggle = FALSE;
    enum tCommand cmd = E_CMD_NOP;
    INT8U val = 0;
    static INT32U WriteAddr = 0UL;
    INT32U i = 0UL;
    INT32U DataCnt = 0UL;

    if ((NULL == data) || (0 == length))
    {
        /* data is not valid */
    }
    else
    {
        cmd = (enum tCommand)data[0u];

        switch (cmd)
        {
            case E_CMD_NOP:
                {
                    SetLed(TRUE);
                    Wait();
                    SetLed(FALSE);
                    Wait();
                    SetLed(TRUE);
                    break;
                }

            case E_CMD_READ_OUT:
                {
                    offset = 0;

                    for (addr = 0UL; addr < 131072UL; ++addr)
                    {
                        AddrBusSet(addr);
                        AddrBusSelectProm();

                        val = DataBusRead();

                        AddrBusIdle();

                        if (0 == offset)
                        {
                            TxData[offset] = E_CMD_READ_OUT;
                            ++offset;
                            TxData[offset] = val;
                            ++offset;
                        }
                        else
                        {
                            TxData[offset] = val;
                            ++offset;

                            if (offset < MAX_DATA_LEN)
                            {
                            }
                            else
                            {
                                if (FALSE == toggle)
                                {
                                    toggle = TRUE;
                                }
                                else
                                {
                                    toggle = FALSE;
                                }

                                SetLed(toggle);

                                PromIfWrite(&TxData[0], MAX_DATA_LEN);
                                offset = 0;
                            }
                        }
                    }

                    if (0 == offset)
                    {
                        TxData[0] = E_CMD_READ_OUT_READY;
                        PromIfWrite(&TxData[0], 1UL);
                    }
                    else if (offset < MAX_DATA_LEN)
                    {
                        PromIfWrite(&TxData[0], offset);
                        offset = 0;
                        TxData[0] = E_CMD_READ_OUT_READY;
                        PromIfWrite(&TxData[0], 1UL);
                    }
                    else
                    {
                        TxData[0] = E_CMD_READ_OUT_READY;
                        PromIfWrite(&TxData[0], 1UL);
                    }

                    SetLed(TRUE);

                    break;
                }

            case E_CMD_WRITE:
                {
                    if (length > 0)
                    {
                        --length;
                    }
                    else
                    {
                    }

                    SetLed(FALSE);

                    if (0 == length)
                    {
                        WriteAddr = 0UL;
                        TxData[0] = E_CMD_WRITE_READY;
                        PromIfWrite(&TxData[0], 1UL);
                    }
                    else
                    {
                        DataCnt = 1UL;

                        for (i = 0; i < length; ++i)
                        {
                            AddrBusSet(WriteAddr);
                            AddrBusSelectProm();

                            DataBusSet(data[DataCnt]);

                            DataBusWrite();
                            _delay_ms(50u);

                            DataBusIdle();
                            AddrBusIdle();

                            ++DataCnt;
                            ++WriteAddr;
                        }

                        TxData[0] = E_CMD_WRITE_READY;
                        PromIfWrite(&TxData[0], 1UL);
                    }

                    SetLed(TRUE);

                    break;
                }

            default:
                {
                    SetLed(TRUE);
                    Wait();
                    SetLed(FALSE);
                    Wait();
                    SetLed(TRUE);
                    break;
                }
        }
    }

    return;
}

