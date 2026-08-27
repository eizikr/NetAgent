#ifndef NETAGENT_PACKET_SENDER_H
#define NETAGENT_PACKET_SENDER_H

#include <stddef.h>
#include <stdint.h>

#include "netagent/endpoint.h"

typedef int (*PacketSendFunction)(
    void *context,
    const UdpEndpoint *destination,
    const uint8_t *buffer,
    size_t length
);

typedef struct {
    PacketSendFunction send;
    void *context;
} PacketSender;

#endif