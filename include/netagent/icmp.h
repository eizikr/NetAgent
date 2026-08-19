#ifndef NETAGENT_ICMP_H
#define NETAGENT_ICMP_H

#include <stddef.h>
#include <stdint.h>

#define ICMP_ECHO_REPLY    0
#define ICMP_ECHO_REQUEST  8

typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t sequence;
} ICMPHeader;

int parse_icmp(const uint8_t *buffer,
               size_t length,
               ICMPHeader *header);

#endif