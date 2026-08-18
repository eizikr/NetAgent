#include "netagent/icmp.h"



int parse_icmp(const uint8_t *buffer, size_t length, ICMPHeader *header){
    if (buffer == NULL || header == NULL || length < 8) {
        return -1;
    }

    header->type = buffer[0];
    header->code = buffer[1];
    header->checksum = ((uint16_t)buffer[2] << 8) | buffer[3];
    header->identifier = ((uint16_t)buffer[4] << 8) | buffer[5];
    header->sequence = ((uint16_t)buffer[6] << 8) | buffer[7];

    return 0;
}