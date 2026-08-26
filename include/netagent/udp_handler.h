#ifndef NETAGENT_UDP_HANDLER_H
#define NETAGENT_UDP_HANDLER_H

#include <stddef.h>
#include <stdint.h>

#include "netagent/config.h"
#include "netagent/stats.h"
#include "netagent/twamp.h"
#include "netagent/tx.h"

int udp_handle_packet(
    uint32_t sender_ip,
    uint8_t sender_ttl,
    const uint8_t *payload,
    size_t payload_length,
    const NtpTimestamp *receive_timestamp,
    const UdpTxSocket *tx,
    NetAgentStats *stats,
    const NetAgentConfig *config
);

#endif