#include "netagent/udp_handler.h"

#include <stdio.h>

#include "netagent/endpoint.h"
#include "netagent/twamp_reflector.h"
#include "netagent/udp.h"

int udp_handle_packet(
    uint32_t sender_ip,
    uint8_t sender_ttl,
    const uint8_t *payload,
    size_t payload_length,
    const NtpTimestamp *receive_timestamp,
    const UdpTxSocket *tx,
    NetAgentStats *stats,
    const NetAgentConfig *config)
{
    UDPHeader udp_header;

    int result = parse_udp(
        payload,
        payload_length,
        &udp_header
    );

    if (result != 0) {
        fprintf(stderr, "Failed to parse UDP header\n");
        return result;
    }

    UdpEndpoint sender = {
        .ip = sender_ip,
        .port = udp_header.src_port
    };

    printf(
        "UDP %u -> %u\n",
        udp_header.src_port,
        udp_header.dst_port
    );

    size_t udp_payload_length =
        udp_header.length - UDP_HEADER_SIZE;

    const uint8_t *udp_payload =
        payload + UDP_HEADER_SIZE;

    if (udp_header.dst_port == config->twamp_port) {
        return twamp_reflector_handle_packet(
            udp_payload,
            udp_payload_length,
            &sender,
            sender_ttl,
            receive_timestamp,
            tx,
            stats
        );
    }

    printf(
        "Generic UDP traffic on port %u\n",
        udp_header.dst_port
    );

    return 0;
}