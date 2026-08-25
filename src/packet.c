#include "netagent/packet.h"

#include <stdio.h>
#include <arpa/inet.h>

#include "netagent/ethernet.h"
#include "netagent/twamp.h"
#include "netagent/ipv4.h"
#include "netagent/icmp.h"
#include "netagent/udp.h"
#include "netagent/tx.h"

static int dispatch_ipv4_protocol(
    uint8_t protocol,
    uint8_t sender_ttl,
    UdpEndpoint sender,
    const NtpTimestamp *receive_timestamp,
    const uint8_t *payload,
    size_t payload_length,
    const UdpTxSocket *tx
);

static int dispatch_udp_payload(
    const UdpEndpoint *sender,
    uint8_t sender_ttl,
    uint16_t dst_port,
    const uint8_t *payload,
    size_t payload_length,
    const NtpTimestamp *receive_timestamp,
    const UdpTxSocket *tx
);

int process_packet(const uint8_t *packet, size_t length, const NtpTimestamp *receive_timestamp, const UdpTxSocket *tx){
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

    printf(
        "T2: %u.%08x\n",
        receive_timestamp->seconds,
        receive_timestamp->fraction
    );
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

    uint32_t src_ip_addr = ipv4_header.src_addr;
    uint16_t src_port = ((uint16_t)payload[0] << 8) | payload[1];
    UdpEndpoint sender = {
        .ip = src_ip_addr,
        .port = src_port
    };

    int dispatch_result = dispatch_ipv4_protocol(ipv4_header.protocol, ipv4_header.ttl, sender, receive_timestamp, payload, payload_length, tx);

    return dispatch_result;
}

static int dispatch_ipv4_protocol(uint8_t protocol, uint8_t sender_ttl, UdpEndpoint sender, const NtpTimestamp *receive_timestamp, const uint8_t *payload, size_t payload_length,   const UdpTxSocket *tx){
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
                tx
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
    const UdpTxSocket *tx)
{
    switch (dst_port) {
    case 20481:{
        TWAMPSenderPacket sender_packet;
        TWAMPReflectorPacket response;
        uint8_t tx_buffer[TWAMP_REFLECTOR_FIXED_SIZE];


        int result = parse_twamp_sender(
            payload, 
            payload_length, 
            &sender_packet
        );

        if (result != 0) {
            fprintf(stderr, "Failed to parse TWAMP sender packet\n");
            return result;
        }
        response.sender_ttl = sender_ttl;
        result = build_twamp_reflector_response(
            &sender_packet,
            receive_timestamp,
            &response
        );

        if (result != 0) {
            fprintf(stderr, "Failed to build TWAMP reflector response\n");
            return result;
        }

        NtpTimestamp t3;

        if (ntp_timestamp_now(&t3) != 0) {
            return -1;
        }

        response.timestamp_seconds = t3.seconds;
        response.timestamp_fraction = t3.fraction;

        result = serialize_twamp_reflector(
            &response,
            tx_buffer,
            sizeof(tx_buffer)
        );

        if (result != 0) {
            return result;
        }

        return udp_tx_send(
            tx,
            sender,
            tx_buffer,
            sizeof(tx_buffer)
        );

    }
    default:
        printf("Generic UDP traffic on port %u\n", dst_port);
        break;
    }

    return 0;
}