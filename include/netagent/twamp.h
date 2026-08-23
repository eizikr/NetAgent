#ifndef NETAGENT_TWAMP_H
#define NETAGENT_TWAMP_H

#define _POSIX_C_SOURCE 200809L

#include <stddef.h>
#include <stdint.h>

#define TWAMP_SENDER_MIN_SIZE 14
#define TWAMP_REFLECTOR_FIXED_SIZE 44
#define NTP_UNIX_EPOCH_OFFSET 2208988800ULL
#define NSEC_PER_SEC          1000000000ULL

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

typedef struct {
    uint32_t sequence_number;

    /* T3 */
    uint32_t timestamp_seconds;
    uint32_t timestamp_fraction;

    uint16_t error_estimate;

    /* T2 */
    uint32_t receive_timestamp_seconds;
    uint32_t receive_timestamp_fraction;

    uint32_t sender_sequence_number;

    /* T1 */
    uint32_t sender_timestamp_seconds;
    uint32_t sender_timestamp_fraction;

    uint16_t sender_error_estimate;

    uint8_t sender_ttl;
} TWAMPReflectorPacket;

typedef struct {
    uint32_t seconds;
    uint32_t fraction;
} NtpTimestamp;

int build_twamp_reflector_response(
    const TWAMPSenderPacket *sender,
    const NtpTimestamp *receive_timestamp,
    TWAMPReflectorPacket *response
);

int ntp_timestamp_now(NtpTimestamp *timestamp);

int serialize_twamp_reflector(
    const TWAMPReflectorPacket *packet,
    uint8_t *buffer,
    size_t buffer_length
);

int set_twamp_transmit_timestamp(
    TWAMPReflectorPacket *response
);

#endif



