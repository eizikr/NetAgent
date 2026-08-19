#ifndef NETAGENT_TWAMP_H
#define NETAGENT_TWAMP_H

#include <stddef.h>
#include <stdint.h>

#define TWAMP_HEADER_SIZE 14

typedef struct { // 64-bit NTP timestamp ( 4 bytes seconds + 4 bytes fraction )
    uint32_t sequence_number;       // 4 bytes
    uint32_t timestamp_seconds;     // 4 bytes
    uint32_t timestamp_fraction;    // 4 bytes
    uint16_t error_estimate;        // 2 bytes
} TWAMPSenderPacket;

int parse_twamp_sender(
    const uint8_t *buffer,
    size_t length,
    TWAMPSenderPacket *packet
);

#endif



