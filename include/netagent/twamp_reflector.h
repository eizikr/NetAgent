#ifndef NETAGENT_TWAMP_REFLECTOR_H
#define NETAGENT_TWAMP_REFLECTOR_H

#include <stddef.h>
#include <stdint.h>

#include "netagent/endpoint.h"
#include "netagent/stats.h"
#include "netagent/twamp.h"
#include "netagent/tx.h"

int twamp_reflector_handle_packet(
    const uint8_t *payload,
    size_t payload_length,
    const UdpEndpoint *sender,
    uint8_t sender_ttl,
    const NtpTimestamp *receive_timestamp,
    const UdpTxSocket *tx,
    NetAgentStats *stats
);

#endif