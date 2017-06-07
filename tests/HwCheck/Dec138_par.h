#ifndef DEC_138_PAR_H
#define DEC_138_PAR_H

#define DEC_138_ENUM_Y0     (0)
#define DEC_138_ENUM_Y1     (_BV(PA2))
#define DEC_138_ENUM_Y2     (_BV(PA1))
#define DEC_138_ENUM_Y3     ((_BV(PA1) | _BV(PA2)))
#define DEC_138_ENUM_Y4     (_BV(PA0))
#define DEC_138_ENUM_Y5     ((_BV(PA0) | _BV(PA2)))
#define DEC_138_ENUM_Y6     ((_BV(PA0) | _BV(PA1)))
#define DEC_138_ENUM_Y7     ((_BV(PA0) | _BV(PA1) | _BV(PA2)))

#define DEC_138_PORT_MASK   (_BV(PA0) | _BV(PA1) | _BV(PA2))

#define DEC_138_INIT_PORT_VAL   DEC_138_ENUM_Y7

#define DEC_138_PORT        PORTA
#define DEC_138_DD_PORT     DDRA

#endif

