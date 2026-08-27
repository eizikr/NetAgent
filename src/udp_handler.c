
#include <stdio.h>

#include "netagent/twamp_reflector.h"
#include "netagent/udp_handler.h"
#include "netagent/endpoint.h"
#include "netagent/udp.h"
#include "netagent/log.h"

int udp_handle_packet(
    uint32_t sender_ip,
    uint8_t sender_ttl,
    const uint8_t *payload,
    size_t payload_length,
    const NtpTimestamp *receive_timestamp,
    const PacketSender *packet_sender,
    NetAgentStats *stats,
    const NetAgentConfig *config
)
{
    UDPHeader udp_header;

    int result = parse_udp(
        payload,
        payload_length,
        &udp_header
    );

    if (result != 0) {
        log_error("Failed to parse UDP header\n");
        return result;
    }

    UdpEndpoint sender_endpoint = {
        .ip = sender_ip,
        .port = udp_header.src_port
    };

    log_debug(
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
            &sender_endpoint,
            packet_sender,
            sender_ttl,
            receive_timestamp,
            stats
        );
    }

    log_info(
        "Generic UDP traffic on port %u\n",
        udp_header.dst_port
    );

    return 0;
}