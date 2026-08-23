#define _POSIX_C_SOURCE 200809L

#include "netagent/twamp.h"

#include <stdio.h>
#include <time.h>


int parse_twamp_sender(
    const uint8_t *buffer,
    size_t length,
    TWAMPSenderPacket *packet)
{

//     Offset    Size    Field
//  ────────────────────────────────────
//      0         4       Sequence Number
//      4         4       Timestamp Seconds
//      8         4       Timestamp Fraction
//      12        2       Error Estimate
//  ────────────────────────────────────
//      Total     14 bytes

    if (buffer == NULL || packet == NULL || length < TWAMP_SENDER_MIN_SIZE) {
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

int build_twamp_reflector_response(
    const TWAMPSenderPacket *sender,
    const NtpTimestamp *receive_timestamp,
    TWAMPReflectorPacket *response)
{
    if (sender == NULL ||
        receive_timestamp == NULL ||
        response == NULL) {
        puts("Invalid input to build_twamp_reflector_response");
        return -1;
    }
    response->sequence_number = sender->sequence_number;

    // T2
    response->receive_timestamp_seconds = receive_timestamp->seconds;
    response->receive_timestamp_fraction = receive_timestamp->fraction;

    // Copy original Sender information
    response->sender_sequence_number = sender->sequence_number;
    response->sender_timestamp_seconds = sender->timestamp_seconds;
    response->sender_timestamp_fraction = sender->timestamp_fraction;
    response->sender_error_estimate = sender->error_estimate;

    return 0;
}

int ntp_timestamp_now(NtpTimestamp *timestamp)
{
    if (timestamp == NULL) {
        return -1;
    }

    struct timespec ts;

    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        return -1;
    }

    timestamp->seconds =
        (uint32_t)((uint64_t)ts.tv_sec +
                   NTP_UNIX_EPOCH_OFFSET);

    timestamp->fraction =
        (uint32_t)(
            ((uint64_t)ts.tv_nsec << 32) /
            NSEC_PER_SEC
        );

    return 0;
}

int serialize_twamp_reflector(
    const TWAMPReflectorPacket *packet,
    uint8_t *buffer,
    size_t buffer_length)
{
    if (packet == NULL ||
        buffer == NULL ||
        buffer_length < TWAMP_REFLECTOR_FIXED_SIZE) {
        return -1;
    }

    // Sequence Number
    buffer[0] = (uint8_t)(packet->sequence_number >> 24);
    buffer[1] = (uint8_t)(packet->sequence_number >> 16);
    buffer[2] = (uint8_t)(packet->sequence_number >> 8);
    buffer[3] = (uint8_t)packet->sequence_number;

    // T3 Timestamp
    buffer[4] = (uint8_t)(packet->timestamp_seconds >> 24);
    buffer[5] = (uint8_t)(packet->timestamp_seconds >> 16);
    buffer[6] = (uint8_t)(packet->timestamp_seconds >> 8);
    buffer[7] = (uint8_t)packet->timestamp_seconds;

    buffer[8]  = (uint8_t)(packet->timestamp_fraction >> 24);
    buffer[9]  = (uint8_t)(packet->timestamp_fraction >> 16);
    buffer[10] = (uint8_t)(packet->timestamp_fraction >> 8);
    buffer[11] = (uint8_t)packet->timestamp_fraction;

    // Reflector Error Estimate
    buffer[12] = (uint8_t)(packet->error_estimate >> 8);
    buffer[13] = (uint8_t)packet->error_estimate;

    // MBZ
    buffer[14] = 0;
    buffer[15] = 0;

    // T2 Receive Timestamp
    buffer[16] =
        (uint8_t)(packet->receive_timestamp_seconds >> 24);
    buffer[17] =
        (uint8_t)(packet->receive_timestamp_seconds >> 16);
    buffer[18] =
        (uint8_t)(packet->receive_timestamp_seconds >> 8);
    buffer[19] =
        (uint8_t)packet->receive_timestamp_seconds;

    buffer[20] =
        (uint8_t)(packet->receive_timestamp_fraction >> 24);
    buffer[21] =
        (uint8_t)(packet->receive_timestamp_fraction >> 16);
    buffer[22] =
        (uint8_t)(packet->receive_timestamp_fraction >> 8);
    buffer[23] =
        (uint8_t)packet->receive_timestamp_fraction;

    // Sender Sequence Number
    buffer[24] =
        (uint8_t)(packet->sender_sequence_number >> 24);
    buffer[25] =
        (uint8_t)(packet->sender_sequence_number >> 16);
    buffer[26] =
        (uint8_t)(packet->sender_sequence_number >> 8);
    buffer[27] =
        (uint8_t)packet->sender_sequence_number;

    // Sender Timestamp (T1)
    buffer[28] =
        (uint8_t)(packet->sender_timestamp_seconds >> 24);
    buffer[29] =
        (uint8_t)(packet->sender_timestamp_seconds >> 16);
    buffer[30] =
        (uint8_t)(packet->sender_timestamp_seconds >> 8);
    buffer[31] =
        (uint8_t)packet->sender_timestamp_seconds;

    buffer[32] =
        (uint8_t)(packet->sender_timestamp_fraction >> 24);
    buffer[33] =
        (uint8_t)(packet->sender_timestamp_fraction >> 16);
    buffer[34] =
        (uint8_t)(packet->sender_timestamp_fraction >> 8);
    buffer[35] =
        (uint8_t)packet->sender_timestamp_fraction;

    // Sender Error Estimate
    buffer[36] =
        (uint8_t)(packet->sender_error_estimate >> 8);
    buffer[37] =
        (uint8_t)packet->sender_error_estimate;

    // MBZ
    buffer[38] = 0;
    buffer[39] = 0;

    // Sender TTL
    buffer[40] = packet->sender_ttl;

    // MBZ
    buffer[41] = 0;
    buffer[42] = 0;
    buffer[43] = 0;

    return 0;
}

int set_twamp_transmit_timestamp(
    TWAMPReflectorPacket *response
){
    NtpTimestamp T3;
    if (ntp_timestamp_now(&T3) != 0) {
        return -1;
    }

    response->timestamp_seconds = T3.seconds;
    response->timestamp_fraction = T3.fraction;

    return 0;
}