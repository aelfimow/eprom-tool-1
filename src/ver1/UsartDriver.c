#include "UsartDriver.h"
#include "UsartDriver_par.h"

static void UsartWriteData(INT8U data);

void UsartInit(void)
{
    UBRR0H = (INT8U)((USART_DRIVER_BAUDRATE / 256u) & 0xFFu);
    UBRR0L = (INT8U)(USART_DRIVER_BAUDRATE & 0xFFu);
    UCSR0B = (INT8U)((INT8U)(1u << TXEN0) | (INT8U)(1u << RXEN0));
    UCSR0C = (INT8U)((INT8U)(3u << UCSZ00));
    return;
}

void UsartTx(INT8U data)
{
    UsartWriteData(data);
    return;
}

void UsartTxStr(const char *data)
{
    INT8U i = 0u;
    const INT8U max = 64u;
    INT8U c = 0;

    for (i = 0u; i < max; ++i)
    {
        c = pgm_read_byte(&data[i]);
        if (0u != c)
        {
            UsartWriteData(c);
        }
        else
        {
            i = max;
        }
    }

    return;
}

static void UsartWriteData(INT8U data)
{
    while (0u == (UCSR0A & (INT8U)(1u << UDRE0)))
    {
        /* do nothing */
    }

    UDR0 = data;

    return;
}

INT8U UsartRx(void)
{
    BOOLEAN done = FALSE;

    while (FALSE == done)
    {
        if (0 == (UCSR0A & (INT8U)(1u << RXC0)))
        {
            done = FALSE;
        }
        else
        {
            done = TRUE;
        }
    }

    return UDR0;
}

BOOLEAN UsartRxDataAvail(void)
{
    BOOLEAN avail = FALSE;

    if (0 == (UCSR0A & (INT8U)(1u << RXC0)))
    {
        avail = FALSE;
    }
    else
    {
        avail = TRUE;
    }

    return avail;
}

INT8U UsartRead(INT8U *data)
{
    INT8U cnt = 0;

    if (NULL == data)
    {
        cnt = 0;
    }
    else
    {
        data[0] = UDR0;
        cnt = 1u;
    }

    return cnt;
}

