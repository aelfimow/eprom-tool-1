#ifndef PROM_IF_PAR_H
#define PROM_IF_PAR_H

#define MAX_RX_PACKETS      (32u)
#define MAX_RX_PACKET_SIZE  (1024u)

#define MAX_TX_PACKET_SIZE  (1024u)

#define MAGIC_START_A   (0xAAu)
#define MAGIC_START_B   (0x55u)

#define MAGIC_END_A     (0x33u)
#define MAGIC_END_B     (0xEEu)

#define MAX_CHECKSUM_LENGTH     (4u)

#define LENGTH_MASK     (0x000000FFUL)

#endif

