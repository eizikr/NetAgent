#include "netagent/udp.h"
#include <stdio.h>



int parse_udp(const uint8_t *buffer, size_t length, UDPHeader *header){
    // Validation checks
    if (buffer == NULL || header == NULL || length < UDP_HEADER_SIZE) {
        puts("Invalid input to parse_udp");
        return -1;
    }

    // Parse UDP header
    header->src_port = ((uint16_t)buffer[0] << 8) | buffer[1];
    header->dst_port = ((uint16_t)buffer[2] << 8) | buffer[3];
    header->length = ((uint16_t)buffer[4] << 8) | buffer[5];
    header->checksum = ((uint16_t)buffer[6] << 8) | buffer[7];

    return 0;
}
