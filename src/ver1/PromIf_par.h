#ifndef PROM_IF_PAR_H
#define PROM_IF_PAR_H

#define MAGIC_START_A  (0xAAu)
#define MAGIC_START_B  (0x55u)

#define MAGIC_END_A     (0x33u)
#define MAGIC_END_B     (0xEEu)

#define BYTE_MASK   (0x000000FFUL)

#define MAX_DATA_LENGTH     (4u)
#define MAX_CHECK_SUM_LENGTH    (4u)

#define MAX_RX_PACKET_SIZE  (512UL)

#endif

