#ifndef DEC_154_PAR_H
#define DEC_154_PAR_H

#define DEC_154_ENUM_Y0     (0)
#define DEC_154_ENUM_Y1     (_BV(PA3))
#define DEC_154_ENUM_Y2     (_BV(PA4))
#define DEC_154_ENUM_Y3     ((_BV(PA3) | _BV(PA4)))
#define DEC_154_ENUM_Y4     (_BV(PA5))
#define DEC_154_ENUM_Y5     ((_BV(PA3) | _BV(PA5)))
#define DEC_154_ENUM_Y6     ((_BV(PA4) | _BV(PA5)))
#define DEC_154_ENUM_Y7     ((_BV(PA3) | _BV(PA4) | _BV(PA5)))
#define DEC_154_ENUM_Y8     (_BV(PA6))
#define DEC_154_ENUM_Y9     ((_BV(PA3) | _BV(PA6)))
#define DEC_154_ENUM_Y10     ((_BV(PA4) | _BV(PA6)))
#define DEC_154_ENUM_Y11     ((_BV(PA3) | _BV(PA4) | _BV(PA6)))
#define DEC_154_ENUM_Y12     ((_BV(PA5) | _BV(PA6)))
#define DEC_154_ENUM_Y13     ((_BV(PA3) | _BV(PA5) | _BV(PA6)))
#define DEC_154_ENUM_Y14     ((_BV(PA4) | _BV(PA5) | _BV(PA6)))
#define DEC_154_ENUM_Y15     ((_BV(PA3) | _BV(PA4) | _BV(PA5) | _BV(PA6)))

#define DEC_154_PORT_MASK   (_BV(PA3) | _BV(PA4) | _BV(PA5) | _BV(PA6))

#define DEC_154_INIT_PORT_VAL   E_DEC154_Y15

#define DEC_154_PORT        PORTA

#define DEC_154_DD_PORT     DDRA

#endif

