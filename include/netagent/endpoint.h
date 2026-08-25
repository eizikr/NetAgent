#ifndef NETAGENT_ENDPOINT_H
#define NETAGENT_ENDPOINT_H

#include <stdint.h>

typedef struct {
    uint32_t ip;
    uint16_t port;
} UdpEndpoint;


#endif