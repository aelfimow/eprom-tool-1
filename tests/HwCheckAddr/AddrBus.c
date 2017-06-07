#include "AddrBus.h"
#include "AddrBus_par.h"

void AddrBusSet(INT32U AddrVal)
{
    INT8U a = (INT8U)(AddrVal & ADDR_BUS_LSB_MASK);
    INT8U b = (INT8U)((INT32U)(AddrVal / 256UL) & ADDR_BUS_LSB_MASK);
    INT8U c = (INT8U)((INT32U)(AddrVal / 65536UL) & ADDR_BUS_LSB_MASK);

    Io8255SetPortA(a);  /* set a0..a7 */

    Io8255SetPortB(b);  /* set a8..a15 */

    Io8255SetPortC(c);  /* set a16..a23 */

    return;
}

