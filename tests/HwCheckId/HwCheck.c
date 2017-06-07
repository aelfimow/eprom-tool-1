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
void DebugOut(void);

void SetupIO(void);
void SetLed(BOOLEAN on_flag);
void DebugOutBit(INT8U val, INT8U mask);
void DebugOutInt32u(INT32U val);
void DebugOutInt8u(INT8U val);

int main(void)
{
    INT32U addr = 0UL;
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

    while (TRUE)
    {
        for (addr = 0UL; addr < 6UL; ++addr)
        {
            AddrBusSet(addr);
            Dec154Set(E_DEC154_Y1);

            data = DataBusRead();

            AddrBusIdle();

            DebugOutInt8u(data);
        }

        AddrBusSet(0x00005555UL);
        Dec154Set(E_DEC154_Y1);

        DataBusSet(0xAAu);
        DataBusPulseWrite();

        AddrBusIdle();

        AddrBusSet(0x00002AAAUL);
        Dec154Set(E_DEC154_Y1);

        DataBusSet(0x55u);
        DataBusPulseWrite();

        AddrBusIdle();

        AddrBusSet(0x00005555UL);
        Dec154Set(E_DEC154_Y1);

        DataBusSet(0x90u);
        DataBusPulseWrite();

        AddrBusIdle();

        Wait();

        UsartTxStr(PSTR("\r\n"));

        for (addr = 0UL; addr < 6UL; ++addr)
        {
            AddrBusSet(addr);
            Dec154Set(E_DEC154_Y1);

            data = DataBusRead();

            AddrBusIdle();

            DebugOutInt8u(data);
        }

        while (TRUE)
        {
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

void DebugOut(void)
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

void DebugOutBit(INT8U val, INT8U mask)
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

void DebugOutInt32u(INT32U val)
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

void DebugOutInt8u(INT8U val)
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

