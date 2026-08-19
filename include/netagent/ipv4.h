#ifndef NETAGENT_IPV4_H
#define NETAGENT_IPV4_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t version;
    uint8_t ihl;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t total_length;
    uint32_t src_addr;
    uint32_t dst_addr;
} IPv4Header;

int parse_ipv4(const uint8_t *buffer,
               size_t length,
               IPv4Header *header);

#endif
