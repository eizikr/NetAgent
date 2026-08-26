#ifndef NETAGENT_TX_H
#define NETAGENT_TX_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "netagent/endpoint.h"


typedef struct {
    int fd;
    uint16_t local_port;
} UdpTxSocket;

int udp_tx_read_timestamp(
    const UdpTxSocket *tx,
    struct timespec *timestamp
);

int udp_tx_open(
    UdpTxSocket *tx,
    uint16_t local_port
);

int udp_tx_send(
    const UdpTxSocket *tx,
    const UdpEndpoint *destination,
    const uint8_t *payload,
    size_t payload_length
);

void udp_tx_close(
    UdpTxSocket *tx
);

#endif