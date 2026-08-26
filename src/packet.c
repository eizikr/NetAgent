#include <stdio.h>
#include <arpa/inet.h>

#include "netagent/twamp_reflector.h"
#include "netagent/ethernet.h"
#include "netagent/packet.h"
#include "netagent/twamp.h"
#include "netagent/ipv4.h"
#include "netagent/icmp.h"
#include "netagent/udp.h"
#include "netagent/tx.h"

static int dispatch_ipv4_protocol(
    uint8_t protocol,
    uint8_t sender_ttl,
    uint32_t sender_ip,
    const NtpTimestamp *receive_timestamp,
    const uint8_t *payload,
    size_t payload_length,
    const UdpTxSocket *tx,
    NetAgentStats *stats
);

static int dispatch_udp_payload(
    const UdpEndpoint *sender,
    uint8_t sender_ttl,
    uint16_t dst_port,
    const uint8_t *payload,
    size_t payload_length,
    const NtpTimestamp *receive_timestamp,
    const UdpTxSocket *tx,
    NetAgentStats *stats
);

int process_packet(const uint8_t *packet, size_t length, const NtpTimestamp *receive_timestamp, const UdpTxSocket *tx, NetAgentStats *stats) {
    EthernetHeader header;
	IPv4Header ipv4_header;

    if (packet == NULL) {
        return -1;
    }

    if (length < ETHERNET_HEADER_SIZE) {
        return -1;
    }

    if (receive_timestamp == NULL) {
        return -1;
    }

	//	PARSE MAC HEADER
	int result = parse_ethernet(
		packet,
		length,
		&header
	);

	if (result != ETH_OK) {
    		fprintf(stderr, "Failed to parse Ethernet header\n");
    		return result;
	}

    switch (header.ethertype) {
    case 0x0800:    // IPv4
        break;

    case 0x0806:    // ARP
        return 0;

    case 0x86DD:    // IPv6
        return 0;

    default:
        return 0;
    }


	//	PARSE IPv4 HEADER

	size_t ipv4_length = length - ETHERNET_HEADER_SIZE;
	const uint8_t *ipv4_buffer = packet + ETHERNET_HEADER_SIZE;

        int ipv4_result = parse_ipv4(
		ipv4_buffer,
		ipv4_length,
		&ipv4_header
	);

	if (ipv4_result != 0) {
 		fprintf(stderr, "Failed to parse IPv4 header\n");
    		return ipv4_result;
	}

    size_t ipv4_header_length = (size_t)ipv4_header.ihl * 4;
    if (ipv4_header_length > ipv4_length) {
        return -1;
    } 
    
    const uint8_t   *payload =          ipv4_buffer + ipv4_header_length;
    size_t           payload_length =   ipv4_length - ipv4_header_length;

	char src_ip[INET_ADDRSTRLEN];
	char dst_ip[INET_ADDRSTRLEN];

	uint32_t src_network = htonl(ipv4_header.src_addr);
	uint32_t dst_network = htonl(ipv4_header.dst_addr);

	inet_ntop(AF_INET, &src_network, src_ip, sizeof(src_ip));
	inet_ntop(AF_INET, &dst_network, dst_ip, sizeof(dst_ip));


    int dispatch_result = dispatch_ipv4_protocol(
        ipv4_header.protocol,
        ipv4_header.ttl,
        ipv4_header.src_addr,
        receive_timestamp,
        payload,
        payload_length,
        tx,
        stats
    );
    return dispatch_result;
}

static int dispatch_ipv4_protocol(
    uint8_t protocol, 
    uint8_t sender_ttl, 
    uint32_t sender_ip,
    const NtpTimestamp *receive_timestamp, 
    const uint8_t *payload, 
    size_t payload_length, 
    const UdpTxSocket *tx, 
    NetAgentStats *stats
){
    switch (protocol) {
        case IP_PROTO_ICMP: {
            ICMPHeader icmp_header;
            int result = parse_icmp(payload, payload_length, &icmp_header);
            if (result != 0) {
                fprintf(stderr, "Failed to parse ICMP header\n");
                return result;
            }
            switch (icmp_header.type) {
                case ICMP_ECHO_REQUEST:
                    puts("\nICMP Echo Request");
                    break;

                case ICMP_ECHO_REPLY:
                    puts("\nICMP Echo Reply");
                    break;

                default:
                    printf("\nICMP Type: %u\n", icmp_header.type);
                    break;
            }
            printf("Type:                %u\n", icmp_header.type);
            printf("Code:                %u\n", icmp_header.code);
            printf("Identifier:          %u\n", icmp_header.identifier);
            printf("Sequence:            %u\n", icmp_header.sequence);
            return 0;
        }
        case IP_PROTO_TCP:
            /* unsupported yet */
            return 0;

        case IP_PROTO_UDP:{
            UDPHeader udp_header;

            int result = parse_udp(payload, payload_length, &udp_header);

            if (result != 0) {
                fprintf(stderr, "Failed to parse UDP header\n");
                return result;
            }
            if (udp_header.length < UDP_HEADER_SIZE) {
                return -1;
            }
            if (udp_header.length > payload_length) {
                return -1;
            }

            UdpEndpoint sender = {
                .ip = sender_ip,
                .port = udp_header.src_port
            };
            
            printf("UDP %u -> %u\n",
                udp_header.src_port,
                udp_header.dst_port
            );
            
            size_t udp_payload_length = udp_header.length - UDP_HEADER_SIZE;
            const uint8_t *udp_payload = payload + UDP_HEADER_SIZE;
            
            return dispatch_udp_payload(
                &sender,
                sender_ttl,
                udp_header.dst_port,
                udp_payload,
                udp_payload_length,
                receive_timestamp,
                tx,
                stats
            );
        }

        default:
            /* unsupported */
            return 0;
    }
    return 0;

}

static int dispatch_udp_payload(
    const UdpEndpoint *sender,
    uint8_t sender_ttl,
    uint16_t dst_port,
    const uint8_t *payload,
    size_t payload_length,
    const NtpTimestamp *receive_timestamp,
    const UdpTxSocket *tx,
    NetAgentStats *stats)
{
    switch (dst_port) {
    case 20481:
        return twamp_reflector_handle_packet(
            payload,
            payload_length,
            sender,
            sender_ttl,
            receive_timestamp,
            tx,
            stats
        );
    default:
        printf("Generic UDP traffic on port %u\n", dst_port);
        break;
    }

    return 0;
}