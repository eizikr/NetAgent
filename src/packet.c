#include <stdio.h>

#include "netagent/twamp_reflector.h"
#include "netagent/icmp_handler.h"
#include "netagent/udp_handler.h"
#include "netagent/ethernet.h"
#include "netagent/packet.h"
#include "netagent/twamp.h"
#include "netagent/ipv4.h"
#include "netagent/tx.h"

static int dispatch_ipv4_protocol(
    uint8_t protocol,
    uint8_t sender_ttl,
    uint32_t sender_ip,
    const NtpTimestamp *receive_timestamp,
    const uint8_t *payload,
    size_t payload_length,
    const UdpTxSocket *tx,
    NetAgentStats *stats,
    const NetAgentConfig *config
);

static int dispatch_udp_payload(
    const UdpEndpoint *sender,
    uint8_t sender_ttl,
    uint16_t dst_port,
    const uint8_t *payload,
    size_t payload_length,
    const NtpTimestamp *receive_timestamp,
    const UdpTxSocket *tx,
    NetAgentStats *stats,
    const NetAgentConfig *config
);

int process_packet(
    const uint8_t *packet,
    size_t length,
    const NtpTimestamp *receive_timestamp,
    const UdpTxSocket *tx,
    NetAgentStats *stats,
    const NetAgentConfig *config)
{
    EthernetHeader ethernet_header;
    IPv4Header ipv4_header;

    if (packet == NULL ||
        receive_timestamp == NULL ||
        tx == NULL ||
        stats == NULL) {
        return -1;
    }

    if (length < ETHERNET_HEADER_SIZE) {
        return -1;
    }

    int result = parse_ethernet(
        packet,
        length,
        &ethernet_header
    );

    if (result != ETH_OK) {
        fprintf(stderr, "Failed to parse Ethernet header\n");
        return result;
    }

    if (ethernet_header.ethertype != ETHERTYPE_IPV4) {
        return 0;
    }

    const uint8_t *ipv4_buffer =
        packet + ETHERNET_HEADER_SIZE;

    size_t ipv4_length =
        length - ETHERNET_HEADER_SIZE;

    result = parse_ipv4(
        ipv4_buffer,
        ipv4_length,
        &ipv4_header
    );

    if (result != 0) {
        fprintf(stderr, "Failed to parse IPv4 header\n");
        return result;
    }

    size_t ipv4_header_length =
        (size_t)ipv4_header.ihl * 4;

    if (ipv4_header_length > ipv4_length) {
        return -1;
    }

    const uint8_t *payload =
        ipv4_buffer + ipv4_header_length;

    size_t payload_length =
        ipv4_length - ipv4_header_length;

    return dispatch_ipv4_protocol(
        ipv4_header.protocol,
        ipv4_header.ttl,
        ipv4_header.src_addr,
        receive_timestamp,
        payload,
        payload_length,
        tx,
        stats,
        config
    );
}

static int dispatch_ipv4_protocol(
    uint8_t protocol, 
    uint8_t sender_ttl, 
    uint32_t sender_ip,
    const NtpTimestamp *receive_timestamp, 
    const uint8_t *payload, 
    size_t payload_length, 
    const UdpTxSocket *tx, 
    NetAgentStats *stats,
    const NetAgentConfig *config
){
    switch (protocol) {
        case IP_PROTO_ICMP: 
            return icmp_handle_packet(
            payload,
            payload_length
        );
        case IP_PROTO_TCP:
            /* unsupported yet */
            return 0;

        case IP_PROTO_UDP:
        return udp_handle_packet(
            sender_ip,
            sender_ttl,
            payload,
            payload_length,
            receive_timestamp,
            tx,
            stats,
            config
        );

        default:
            /* unsupported */
            return 0;
    }
}

static int dispatch_udp_payload(
    const UdpEndpoint *sender,
    uint8_t sender_ttl,
    uint16_t dst_port,
    const uint8_t *payload,
    size_t payload_length,
    const NtpTimestamp *receive_timestamp,
    const UdpTxSocket *tx,
    NetAgentStats *stats,
    const NetAgentConfig *config)
{
    if (dst_port == config->twamp_port) {
        return twamp_reflector_handle_packet(
            payload,
            payload_length,
            sender,
            sender_ttl,
            receive_timestamp,
            tx,
            stats
        );
    }

    printf("Generic UDP traffic on port %u\n", dst_port);
    return 0;
}