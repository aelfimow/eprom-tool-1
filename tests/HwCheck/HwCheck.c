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
static void SetLed(BOOLEAN on_flag);
static void DebugOut(void);
static void DebugOutBit(INT8U val, INT8U mask);

int main(void)
{
    INT8U valA = 0x55u;
    INT8U valB = 0x55u;
    INT8U valC = 0x55u;

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
        Io8255SetPortA(valA);
        Io8255SetPortB(valB);
        Io8255SetPortC(valC);

        UsartTxStr(PSTR("Written to 8255.\r\n"));

        valA = (INT8U)(~valA);
        valB = (INT8U)(~valB);
        valC = (INT8U)(~valC);

        Dec138Set(E_DEC138_Y0);
        UsartTxStr(PSTR("E_DEC138_Y0\r\n"));
        SetLed(FALSE);
        Wait();
        Dec138Set(E_DEC138_Y1);
        UsartTxStr(PSTR("E_DEC138_Y1\r\n"));
        SetLed(TRUE);
        Wait();
        Dec138Set(E_DEC138_Y2);
        UsartTxStr(PSTR("E_DEC138_Y2\r\n"));
        SetLed(FALSE);
        Wait();
        Dec138Set(E_DEC138_Y3);
        UsartTxStr(PSTR("E_DEC138_Y3\r\n"));
        SetLed(TRUE);
        Wait();
        Dec138Set(E_DEC138_Y4);
        UsartTxStr(PSTR("E_DEC138_Y4\r\n"));
        SetLed(FALSE);
        Wait();
        Dec138Set(E_DEC138_Y5);
        UsartTxStr(PSTR("E_DEC138_Y5\r\n"));
        SetLed(TRUE);
        Wait();
        Dec138Set(E_DEC138_Y6);
        UsartTxStr(PSTR("E_DEC138_Y6\r\n"));
        SetLed(FALSE);
        Wait();
        Dec138Set(E_DEC138_Y7);
        UsartTxStr(PSTR("E_DEC138_Y7\r\n"));
        SetLed(TRUE);
        Wait();
        Dec154Set(E_DEC154_Y0);
        UsartTxStr(PSTR("E_DEC154_Y0\r\n"));
        SetLed(FALSE);
        Wait();
        Dec154Set(E_DEC154_Y1);
        UsartTxStr(PSTR("E_DEC154_Y1\r\n"));
        SetLed(TRUE);
        Wait();
        Dec154Set(E_DEC154_Y2);
        UsartTxStr(PSTR("E_DEC154_Y2\r\n"));
        SetLed(FALSE);
        Wait();
        Dec154Set(E_DEC154_Y3);
        UsartTxStr(PSTR("E_DEC154_Y3\r\n"));
        SetLed(TRUE);
        Wait();
        Dec154Set(E_DEC154_Y4);
        UsartTxStr(PSTR("E_DEC154_Y4\r\n"));
        SetLed(FALSE);
        Wait();
        Dec154Set(E_DEC154_Y5);
        UsartTxStr(PSTR("E_DEC154_Y5\r\n"));
        SetLed(TRUE);
        Wait();
        Dec154Set(E_DEC154_Y6);
        UsartTxStr(PSTR("E_DEC154_Y6\r\n"));
        SetLed(FALSE);
        Wait();
        Dec154Set(E_DEC154_Y7);
        UsartTxStr(PSTR("E_DEC154_Y7\r\n"));
        SetLed(TRUE);
        Wait();
        Dec154Set(E_DEC154_Y8);
        UsartTxStr(PSTR("E_DEC154_Y8\r\n"));
        SetLed(FALSE);
        Wait();
        Dec154Set(E_DEC154_Y9);
        UsartTxStr(PSTR("E_DEC154_Y9\r\n"));
        SetLed(TRUE);
        Wait();
        Dec154Set(E_DEC154_Y10);
        UsartTxStr(PSTR("E_DEC154_Y10\r\n"));
        SetLed(FALSE);
        Wait();
        Dec154Set(E_DEC154_Y11);
        UsartTxStr(PSTR("E_DEC154_Y11\r\n"));
        SetLed(TRUE);
        Wait();
        Dec154Set(E_DEC154_Y12);
        UsartTxStr(PSTR("E_DEC154_Y12\r\n"));
        SetLed(FALSE);
        Wait();
        Dec154Set(E_DEC154_Y13);
        UsartTxStr(PSTR("E_DEC154_Y13\r\n"));
        SetLed(TRUE);
        Wait();
        Dec154Set(E_DEC154_Y14);
        UsartTxStr(PSTR("E_DEC154_Y14\r\n"));
        SetLed(FALSE);
        Wait();
        Dec154Set(E_DEC154_Y15);
        UsartTxStr(PSTR("E_DEC154_Y15\r\n"));
        SetLed(TRUE);
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

static void Wait(void)
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

