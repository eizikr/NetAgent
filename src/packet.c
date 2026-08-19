#include "netagent/packet.h"

#include <stdio.h>
#include <arpa/inet.h>

#include "netagent/ethernet.h"
#include "netagent/ipv4.h"
#include "netagent/icmp.h"
#include "netagent/udp.h"

static int dispatch_ipv4_protocol(
    uint8_t protocol,
    const uint8_t *payload,
    size_t payload_length
);

static int dispatch_udp_payload(
    uint16_t dst_port,
    const uint8_t *payload,
    size_t payload_length
);

int process_packet(const uint8_t *packet, size_t length){
    EthernetHeader header;
	IPv4Header ipv4_header;

    if (packet == NULL) {
        return -1;
    }

    if (length < ETHERNET_HEADER_SIZE) {
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

	if (header.ethertype != 0x0800) {
 	   fprintf(stderr, "Packet is not IPv4\n");
 	   return 1;
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

	puts("MAC Header:");
    
    printf("Destination MAC:     %02x:%02x:%02x:%02x:%02x:%02x\n",
        header.dst_mac[0],
        header.dst_mac[1],
        header.dst_mac[2],
        header.dst_mac[3],
        header.dst_mac[4],
        header.dst_mac[5]);

    printf("Source MAC:          %02x:%02x:%02x:%02x:%02x:%02x\n",
        header.src_mac[0],
        header.src_mac[1],
        header.src_mac[2],
        header.src_mac[3],
        header.src_mac[4],
        header.src_mac[5]);

    printf("EtherType:           0x%04x\n", header.ethertype);

	char src_ip[INET_ADDRSTRLEN];
	char dst_ip[INET_ADDRSTRLEN];

	uint32_t src_network = htonl(ipv4_header.src_addr);
	uint32_t dst_network = htonl(ipv4_header.dst_addr);

	inet_ntop(AF_INET, &src_network, src_ip, sizeof(src_ip));
	inet_ntop(AF_INET, &dst_network, dst_ip, sizeof(dst_ip));

	puts("\nIPv4 Header:");
	printf("Version:	     %02x\n",ipv4_header.version);
    printf("ihl:                 %02x\n",ipv4_header.ihl);
	printf("total_length:        0x%04x\n",ipv4_header.total_length);
	printf("Source IP:           %s\n", src_ip);
	printf("Destination IP:      %s\n", dst_ip);
    puts("NetAgent v0.1.0");

    
    int dispatch_result = dispatch_ipv4_protocol(ipv4_header.protocol, payload, payload_length);

    return dispatch_result;
}

static int dispatch_ipv4_protocol(uint8_t protocol, const uint8_t *payload, size_t payload_length){
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
            break;
        }
        case IP_PROTO_TCP:
            /* parse_tcp(payload, payload_length); */
            puts("TCP protocol detected");
            break;

        case IP_PROTO_UDP:
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

            size_t udp_payload_length = udp_header.length - UDP_HEADER_SIZE;
            const uint8_t *udp_payload = payload + UDP_HEADER_SIZE;

            puts("\nUDP");
            printf("Source Port:         %u\n", udp_header.src_port);
            printf("Destination Port:    %u\n", udp_header.dst_port);
            printf("Length:              %u\n", udp_header.length);
            printf("Checksum:            0x%04x\n", udp_header.checksum);

            return dispatch_udp_payload(
                udp_header.dst_port,
                udp_payload,
                udp_payload_length
            );

        default:
            /* unsupported */
            puts("Unsupported protocol detected");
            break;
    }
    return 0;

}

static int dispatch_udp_payload(
    uint16_t dst_port,
    const uint8_t *payload,
    size_t payload_length)
{
    (void)payload;
    switch (dst_port) {
    case 50000:
        printf("TWAMP traffic detected on UDP port %u\n", dst_port);
        printf("TWAMP payload length: %zu bytes\n", payload_length);
        break;

    default:
        printf("Generic UDP traffic on port %u\n", dst_port);
        break;
    }

    return 0;
}