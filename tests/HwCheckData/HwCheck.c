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

void Wait(void);

static void SetupIO(void);
static void SetLed(BOOLEAN on_flag);
static void DebugOut(void);
static void DebugOutBit(INT8U val, INT8U mask);
static void DebugOutInt32u(INT32U val);
static void DebugOutInt8u(INT8U val);

int main(void)
{
    INT32U Addr = 0UL;
    INT8U data = 0u;

    Dec138SetupIO();
    Dec154SetupIO();
    UsartInit();
    SetupIO();
    Io8255Reset();
    SetLed(FALSE);

    sei();

    Io8255Config();

    UsartTxStr(PSTR("Started.\r\n"));

    DebugOut();

    while (TRUE)
    {
        for (Addr = 0u; Addr < 131072UL; ++Addr)
        {
            AddrBusSet(Addr);

            Dec154Set(E_DEC154_Y1); /* enable "chip enable" of the external unit */

            data = DataBusRead();

            if (0 == data)
            {
                /* do not print */
            }
            else
            {
#if 0
                DebugOutInt32u(Addr);
                DebugOutInt8u(data);
#endif

                if ((data >= 32u) && (data <= 127u))
                {
                    UsartTx(data);
                }
                else
                {
                }
            }

            Dec154Set(E_DEC154_Y15); /* disable any chip enable signals */
        }

        UsartTxStr(PSTR("\r\n"));

        data = 0u;

        while (TRUE)
        {
            SetLed(TRUE);

            DataBusSet(data);

            DataBusWrite();

            DebugOutInt8u(data);

            while (0u == UsartRx())
            {
                /* do nothing */
            }

            ++data;
        }
    }

    return 0;
}

static void SetupIO(void)
{
    DDRD |= (INT8U)_BV(PD7); /* as output */
    DDRD |= (INT8U)_BV(PD6); /* as output */
    DDRD |= (INT8U)_BV(PD5); /* as output */

    DDRC = 0u;   /* as input */

    DDRB |= (INT8U)_BV(PB0); /* as output */

    return;
}

static void SetLed(BOOLEAN on_flag)
{
    const INT8U mask = 0x01u;

    if (FALSE == on_flag)
    {
        PORTB &= (INT8U)~mask;
        UsartTxStr(PSTR("LED an.\r\n"));
    }
    else
    {
        PORTB |= (INT8U)mask;
        UsartTxStr(PSTR("LED aus.\r\n"));
    }
    return;
}

void Wait(void)
{
    INT8U i = 0;
    for (i = 0; i < 200u; ++i)
    {
        _delay_ms(200u);
    }
    return;
}

static void DebugOut(void)
{
    INT8U reg = SREG;

    UsartTxStr(PSTR("Global Interrupt Enable: "));
    DebugOutBit(reg, 128u);
    UsartTxStr(PSTR("Bit Copy Storage: "));
    DebugOutBit(reg, 64u);
    UsartTxStr(PSTR("Half Carry Flag: "));
    DebugOutBit(reg, 32u);
    UsartTxStr(PSTR("Sign Bit: "));
    DebugOutBit(reg, 16u);
    UsartTxStr(PSTR("Two's Complement Overflow Flag: "));
    DebugOutBit(reg, 8u);
    UsartTxStr(PSTR("Negative Flag: "));
    DebugOutBit(reg, 4u);
    UsartTxStr(PSTR("Zero Flag: "));
    DebugOutBit(reg, 2u);
    UsartTxStr(PSTR("Carry Flag: "));
    DebugOutBit(reg, 1u);

    return;
}

static void DebugOutBit(INT8U val, INT8U mask)
{
    if (0u != (val & mask))
    {
        UsartTxStr(PSTR("1\r\n"));
    }
    else
    {
        UsartTxStr(PSTR("0\r\n"));
    }

    return;
}

static void DebugOutInt32u(INT32U val)
{
    INT32U mask = 0x80000000UL;
    INT8U i = 0u;

    for (i = 0u; i < 32u; ++i)
    {
        if (0u == (val & mask))
        {
            UsartTxStr(PSTR("0"));
        }
        else
        {
            UsartTxStr(PSTR("1"));
        }

        mask /= 2UL;
    }

    UsartTxStr(PSTR("\r\n"));

    return;
}

static void DebugOutInt8u(INT8U val)
{
    INT8U mask = 0x80u;
    INT8U i = 0u;

    for (i = 0u; i < 8u; ++i)
    {
        if (0u == (val & mask))
        {
            UsartTxStr(PSTR("0"));
        }
        else
        {
            UsartTxStr(PSTR("1"));
        }

        mask /= 2u;
    }

    UsartTxStr(PSTR("\r\n"));

    return;
}

