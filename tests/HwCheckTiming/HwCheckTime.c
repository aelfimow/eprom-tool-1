#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include "data_types.h"
#include "Dec138_ifc.h"
#include "Dec154_ifc.h"
#include "Io8255_ifc.h"
#include "UsartDriver_ifc.h"

static void SetupIO(void);
static void Wait(void);
static void DebugOut(void);
static void DebugOutBit(INT8U val, INT8U mask);

int main(void)
{
    Dec138SetupIO();
    Dec154SetupIO();
    UsartInit();
    SetupIO();
    Io8255Reset();

    sei();

    Io8255Config();

    UsartTxStr(PSTR("Started.\r\n"));

    DebugOut();

    while (TRUE)
    {
        Dec154Set(E_DEC154_Y2);
        Wait();
        Dec154Set(E_DEC154_Y3);
        Wait();
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

static void Wait(void)
{
    INT8U i = 0u;

    for (i = 0u; i < 10u; ++i)
    {
        asm("nop");
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

