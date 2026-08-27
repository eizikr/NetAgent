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
    const PacketSender *packet_sender,
    uint8_t sender_ttl,
    const NtpTimestamp *receive_timestamp,
    NetAgentStats *stats
);

int twamp_reflector_build_response(
    const uint8_t *payload,
    size_t payload_length,
    uint8_t sender_ttl,
    const NtpTimestamp *receive_timestamp,
    uint8_t *tx_buffer,
    size_t tx_buffer_length,
    uint32_t *sender_sequence
);

#endif