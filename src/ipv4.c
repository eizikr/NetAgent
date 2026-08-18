#include <stdio.h>
#include "netagent/ipv4.h"
#include <string.h>


int parse_ipv4(const uint8_t *buffer, size_t length, IPv4Header *header){

	if (buffer == NULL || header == NULL || length < 20) {
    		return -1;
	}

	header->version = (uint8_t)buffer[0] >> 4;
	header->ihl = buffer[0] & 0x0F;
	if (header->version != 4) {
 		return -1;
	}

	if (header->ihl < 5) {
    		return -1;
	}

	size_t header_length = (size_t)header->ihl * 4;
	if (header_length > length) {
    		return -1;
	}

	header->total_length = ((uint16_t)buffer[2] << 8) | buffer[3];
	header->ttl = buffer[8];
	header->protocol = buffer[9];
	header->src_addr =
    		((uint32_t)buffer[12] << 24) |
    		((uint32_t)buffer[13] << 16) |
    		((uint32_t)buffer[14] << 8) |
    		((uint32_t)buffer[15]);

	header->dst_addr =
                ((uint32_t)buffer[16] << 24) |
                ((uint32_t)buffer[17] << 16) |
                ((uint32_t)buffer[18] << 8) |
                ((uint32_t)buffer[19]);


	return 0;
}
