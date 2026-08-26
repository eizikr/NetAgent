#ifndef NETAGENT_ICMP_HANDLER_H
#define NETAGENT_ICMP_HANDLER_H

#include <stddef.h>
#include <stdint.h>

int icmp_handle_packet(
    const uint8_t *payload,
    size_t payload_length
);

#endif