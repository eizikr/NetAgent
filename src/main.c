#include <stdio.h>
#include "netagent/ethernet.h"

typedef struct {
    uint8_t a;
    uint32_t b;
    uint8_t c;
} AlignmentTest;

int main(void){

	EthernetHeader header;

	uint8_t packet[] = {
    		0x5c, 0x60, 0xba, 0x40, 0x7a, 0x89,
    		0x00, 0x0c, 0x29, 0x1b, 0x7b, 0x14,
    		0x08, 0x00
	};

	int result = parse_ethernet(
		packet,
		sizeof(packet),
		&header
	);

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


	puts("NetAgent v0.1.0");


AlignmentTest test;

printf("sizeof(AlignmentTest) = %zu\n", sizeof(AlignmentTest));

printf("&test   = %p\n", (void *)&test);
printf("&test.a = %p\n", (void *)&test.a);
printf("&test.b = %p\n", (void *)&test.b);
printf("&test.c = %p\n", (void *)&test.c);

	return result;

}
