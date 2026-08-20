#include "netagent/twamp.h"
#include <stdio.h>


int parse_twamp_sender(
    const uint8_t *buffer,
    size_t length,
    TWAMPSenderPacket *packet)
{
    if (buffer == NULL || packet == NULL || length < TWAMP_HEADER_SIZE) {
        puts("Invalid input to parse_twamp_sender");
        return -1;
    }


    packet->sequence_number = ((uint32_t)buffer[0] << 24) |
                                 ((uint32_t)buffer[1] << 16) |
                                 ((uint32_t)buffer[2] << 8) |
                                 buffer[3];


    packet->timestamp_seconds = ((uint32_t)buffer[4] << 24) |
                                ((uint32_t)buffer[5] << 16) |
                                ((uint32_t)buffer[6] << 8) |
                                buffer[7];

    packet->timestamp_fraction = ((uint32_t)buffer[8] << 24) |
                                 ((uint32_t)buffer[9] << 16) |
                                 ((uint32_t)buffer[10] << 8) |
                                 buffer[11];

    packet->error_estimate = ((uint16_t)buffer[12] << 8) |
                             buffer[13];

    return 0;
}