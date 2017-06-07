#include "UsartDriver.h"
#include "UsartDriver_par.h"

static void UsartWriteData(INT8U data);

void UsartInit(void)
{
    UBRR0H = (INT8U)((USART_DRIVER_BAUDRATE / 256u) & 0xFFu);
    UBRR0L = (INT8U)(USART_DRIVER_BAUDRATE & 0xFFu);
    UCSR0B = (INT8U)(1u << TXEN0);
    UCSR0C = (INT8U)((1u << USBS0) | (3 << UCSZ00));
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
    while (0u == (UCSR0A & (1u << UDRE0)))
    {
        asm ("nop");
    }
    UDR0 = data;

    return;
}

