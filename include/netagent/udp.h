#ifndef NETAGENT_UDP_H
#define NETAGENT_UDP_H

#include <stddef.h>
#include <stdint.h>

#define UDP_HEADER_SIZE 8

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} UDPHeader;

int parse_udp(const uint8_t *buffer,
               size_t length,
               UDPHeader *header);

#endif



