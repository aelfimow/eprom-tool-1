#include "Io8255.h"
#include "Io8255_par.h"

static void Io8255Wait(void);

void Io8255Reset(void)
{
    const INT8U mask = (INT8U)_BV(PD7);

    DDRD |= mask; /* as output */
    PORTD |= mask;  /* set reset-pin */

    Io8255Wait();

    PORTD &= (INT8U)~mask; /* reset reset-pin */

    return;
}

void Io8255Config(void)
{
    const INT8U AddrMask = (INT8U)(_BV(PD5) | _BV(PD6));
    const INT8U ConfAddr = (INT8U)(_BV(PD5) | _BV(PD6));
    const INT8U DataMask = 0xFFu;
    const INT8U ConfData = 0x80u;

    DDRD |= AddrMask;   /* Address bus */
    PORTD = ConfAddr;

    DDRC = DataMask;    /* Data bus */
    PORTC = ConfData;

    Io8255Wait();

    Dec154Set(E_DEC154_Y0); /* activate chip select */

    Io8255Wait();

    Dec138Set(E_DEC138_Y1); /* activat write */

    Io8255Wait();

    Dec138Set(E_DEC138_Y7); /* deactivate write */

    Io8255Wait();

    Dec154Set(E_DEC154_Y15);    /* deactivate chip select */

    DDRC = 0u;  /* Data bus as input */

    return;
}

void Io8255SetPortA(INT8U val)
{
    const INT8U AddrMask = (INT8U)(_BV(PD5) | _BV(PD6));
    const INT8U AddrPortA = 0u;
    const INT8U DataMask = 0xFFu;

    DDRD |= AddrMask;   /* Address bus */
    PORTD &= (INT8U)~AddrMask;
    PORTD |= AddrPortA;

    DDRC = DataMask;    /* Data bus */
    PORTC = val;

    Io8255Wait();

    Dec154Set(E_DEC154_Y0); /* activate chip select */

    Io8255Wait();

    Dec138Set(E_DEC138_Y1); /* activat write */

    Io8255Wait();

    Dec138Set(E_DEC138_Y7); /* deactivate write */

    Io8255Wait();

    Dec154Set(E_DEC154_Y15);    /* deactivate chip select */

    DDRC = 0u;  /* Data bus as input */

    return;
}

void Io8255SetPortB(INT8U val)
{
    const INT8U AddrMask = (INT8U)(_BV(PD5) | _BV(PD6));
    const INT8U AddrPortB = (INT8U)_BV(PD5);
    const INT8U DataMask = 0xFFu;

    DDRD |= AddrMask;   /* Address bus */
    PORTD &= (INT8U)~AddrMask;
    PORTD |= AddrPortB;

    DDRC = DataMask;    /* Data bus */
    PORTC = val;

    Io8255Wait();

    Dec154Set(E_DEC154_Y0); /* activate chip select */

    Io8255Wait();

    Dec138Set(E_DEC138_Y1); /* activat write */

    Io8255Wait();

    Dec138Set(E_DEC138_Y7); /* deactivate write */

    Io8255Wait();

    Dec154Set(E_DEC154_Y15);    /* deactivate chip select */

    DDRC = 0u;  /* Data bus as input */

    return;
}

void Io8255SetPortC(INT8U val)
{
    const INT8U AddrMask = (INT8U)(_BV(PD5) | _BV(PD6));
    const INT8U AddrPortC = (INT8U)_BV(PD6);
    const INT8U DataMask = 0xFFu;

    DDRD |= AddrMask;   /* Address bus */
    PORTD &= (INT8U)~AddrMask;
    PORTD |= AddrPortC;

    DDRC = DataMask;    /* Data bus */
    PORTC = val;

    Io8255Wait();

    Dec154Set(E_DEC154_Y0); /* activate chip select */

    Io8255Wait();

    Dec138Set(E_DEC138_Y1); /* activat write */

    Io8255Wait();

    Dec138Set(E_DEC138_Y7); /* deactivate write */

    Io8255Wait();

    Dec154Set(E_DEC154_Y15);    /* deactivate chip select */

    DDRC = 0u;  /* Data bus as input */

    return;
}

static void Io8255Wait(void)
{
    INT8U i = 0u;

    for (i = 0u; i < IO_8255_MAX_WAIT_CNT; ++i)
    {
        asm("nop");
    }

    return;
}

