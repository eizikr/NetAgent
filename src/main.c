#include <stdio.h>
#include "netagent/ethernet.h"
#include "netagent/ipv4.h"

typedef struct {
    uint8_t a;
    uint32_t b;
    uint8_t c;
} AlignmentTest;

int main(void){

	EthernetHeader header;
	IPv4Header ipv4_header;

	uint8_t packet[] = {
	    0x5c, 0x60, 0xba, 0x40, 0x7a, 0x89,
	    0x00, 0x0c, 0x29, 0x1b, 0x7b, 0x14,
	    0x08, 0x00,
	    0x45, 0x00, 0x00, 0x54,
	    0x74, 0x8d, 0x40, 0x00,
	    0x40, 0x01, 0x90, 0x9c,
	    0xac, 0x11, 0xee, 0xb6,
	    0xac, 0x11, 0xee, 0xa5
	};

	//	PARSE MAC HEADER
	int result = parse_ethernet(
		packet,
		sizeof(packet),
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
	size_t ipv4_length = sizeof(packet) - ETHERNET_HEADER_SIZE;
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


	puts("\nIPv4 Header:");
	printf("Version:	     %02x\n",ipv4_header.version);
        printf("ihl:                 %02x\n",ipv4_header.ihl);
	printf("total_length:        0x%04x\n",ipv4_header.total_length);
	printf("Source IP raw:       0x%08x\n", ipv4_header.src_addr);
	printf("Destination IP raw:  0x%08x\n", ipv4_header.dst_addr);

	puts("NetAgent v0.1.0");
	return result;

}
