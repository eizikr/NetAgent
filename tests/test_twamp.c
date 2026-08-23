#include <stdio.h>

#include "netagent/twamp.h"

static int test_valid_twamp_sender(void);
static int test_null_buffer(void);
static int test_null_packet(void);
static int test_short_buffer(void);
static int test_build_reflector_response(void);
static int test_serialize_reflector(void);
static int test_serialize_invalid_input(void);

int main(void)
{
    if (test_valid_twamp_sender() != 0)
        return 1;

    if (test_null_buffer() != 0)
        return 1;

    if (test_null_packet() != 0)
        return 1;

    if (test_short_buffer() != 0)
        return 1;

    if (test_build_reflector_response() != 0)
        return 1;

    if (test_serialize_reflector() != 0)
        return 1;

    if (test_serialize_invalid_input() != 0)
        return 1;

    puts("test_twamp: PASS");

    return 0;
}


static int test_valid_twamp_sender(void)
{
    uint8_t buffer[] = {
        /* Sequence Number */
        0x00, 0x00, 0x01, 0x30,

        /* Timestamp - Seconds */
        0xe8, 0x00, 0x00, 0x01,

        /* Timestamp - Fraction */
        0x12, 0x34, 0x56, 0x78,

        /* Error Estimate */
        0x80, 0x01
    };

    TWAMPSenderPacket packet;

    int result = parse_twamp_sender(
        buffer,
        sizeof(buffer),
        &packet
    );

    if (result != 0) {
        fprintf(stderr, "parse_twamp_sender failed\n");
        return 1;
    }

    if (packet.sequence_number != 304) {
        fprintf(stderr,
                "Sequence mismatch: expected 304, got %u\n",
                packet.sequence_number);
        return 1;
    }

    if (packet.timestamp_seconds != 0xe8000001) {
        fprintf(stderr,
                "Timestamp seconds mismatch: got 0x%08x\n",
                packet.timestamp_seconds);
        return 1;
    }

    if (packet.timestamp_fraction != 0x12345678) {
        fprintf(stderr,
                "Timestamp fraction mismatch: got 0x%08x\n",
                packet.timestamp_fraction);
        return 1;
    }

    if (packet.error_estimate != 0x8001) {
        fprintf(stderr,
                "Error estimate mismatch: got 0x%04x\n",
                packet.error_estimate);
        return 1;
    }

    return 0;
}


static int test_null_buffer(void)
{
    TWAMPSenderPacket packet;

    if (parse_twamp_sender(
            NULL,
            TWAMP_SENDER_MIN_SIZE,
            &packet) == 0) {

        fprintf(stderr, "Expected NULL buffer to fail\n");
        return 1;
    }

    return 0;
}


static int test_null_packet(void)
{
    uint8_t buffer[TWAMP_SENDER_MIN_SIZE] = {0};

    if (parse_twamp_sender(
            buffer,
            sizeof(buffer),
            NULL) == 0) {

        fprintf(stderr, "Expected NULL packet to fail\n");
        return 1;
    }

    return 0;
}


static int test_short_buffer(void)
{
    uint8_t buffer[TWAMP_SENDER_MIN_SIZE - 1] = {0};

    TWAMPSenderPacket packet;

    if (parse_twamp_sender(
            buffer,
            sizeof(buffer),
            &packet) == 0) {

        fprintf(stderr, "Expected short TWAMP buffer to fail\n");
        return 1;
    }

    return 0;
}

static int test_build_reflector_response(void)
{
    TWAMPSenderPacket sender = {
        .sequence_number = 304,
        .timestamp_seconds = 0xe8000001,
        .timestamp_fraction = 0x12345678,
        .error_estimate = 0x8001
    };

    NtpTimestamp receive_timestamp = {0};

    TWAMPReflectorPacket response = {0};

    int result = build_twamp_reflector_response(
        &sender,
        &receive_timestamp,
        &response
    );

    if (result != 0) {
        fprintf(stderr,
                "build_twamp_reflector_response failed\n");
        return 1;
    }

    if (response.sender_sequence_number !=
        sender.sequence_number) {

        fprintf(stderr,
                "Sender sequence number mismatch\n");
        return 1;
    }

    if (response.sender_timestamp_seconds !=
        sender.timestamp_seconds) {

        fprintf(stderr,
                "Sender timestamp seconds mismatch\n");
        return 1;
    }

    if (response.sender_timestamp_fraction !=
        sender.timestamp_fraction) {

        fprintf(stderr,
                "Sender timestamp fraction mismatch\n");
        return 1;
    }

    if (response.sender_error_estimate !=
        sender.error_estimate) {

        fprintf(stderr,
                "Sender error estimate mismatch\n");
        return 1;
    }

    return 0;
}

static int test_serialize_reflector(void)
{
    TWAMPReflectorPacket packet = {
        .sequence_number = 0x11223344,

        .timestamp_seconds = 0x55667788,
        .timestamp_fraction = 0x99aabbcc,

        .error_estimate = 0xddee,

        .receive_timestamp_seconds = 0x01020304,
        .receive_timestamp_fraction = 0x05060708,

        .sender_sequence_number = 0x10203040,

        .sender_timestamp_seconds = 0x50607080,
        .sender_timestamp_fraction = 0x90a0b0c0,

        .sender_error_estimate = 0x1234,

        .sender_ttl = 64
    };

    uint8_t buffer[TWAMP_REFLECTOR_FIXED_SIZE];

    int result = serialize_twamp_reflector(
        &packet,
        buffer,
        sizeof(buffer)
    );

    if (result != 0) {
        fprintf(stderr, "serialize_twamp_reflector failed\n");
        return 1;
    }

    const uint8_t expected[TWAMP_REFLECTOR_FIXED_SIZE] = {
        /* 0-3: Reflector Sequence Number */
        0x11, 0x22, 0x33, 0x44,

        /* 4-11: T3 */
        0x55, 0x66, 0x77, 0x88,
        0x99, 0xaa, 0xbb, 0xcc,

        /* 12-13: Reflector Error Estimate */
        0xdd, 0xee,

        /* 14-15: MBZ */
        0x00, 0x00,

        /* 16-23: T2 */
        0x01, 0x02, 0x03, 0x04,
        0x05, 0x06, 0x07, 0x08,

        /* 24-27: Sender Sequence Number */
        0x10, 0x20, 0x30, 0x40,

        /* 28-35: T1 */
        0x50, 0x60, 0x70, 0x80,
        0x90, 0xa0, 0xb0, 0xc0,

        /* 36-37: Sender Error Estimate */
        0x12, 0x34,

        /* 38-39: MBZ */
        0x00, 0x00,

        /* 40: Sender TTL */
        0x40,

        /* 41-43: MBZ */
        0x00, 0x00, 0x00
    };

    for (size_t i = 0; i < TWAMP_REFLECTOR_FIXED_SIZE; i++) {
        if (buffer[i] != expected[i]) {
            fprintf(
                stderr,
                "Byte %zu mismatch: expected 0x%02x, got 0x%02x\n",
                i,
                expected[i],
                buffer[i]
            );

            return 1;
        }
    }

    return 0;
}

static int test_serialize_invalid_input(void)
{
    TWAMPReflectorPacket packet = {0};
    uint8_t buffer[TWAMP_REFLECTOR_FIXED_SIZE];

    if (serialize_twamp_reflector(
            NULL,
            buffer,
            sizeof(buffer)) == 0) {

        fprintf(stderr, "Expected NULL packet to fail\n");
        return 1;
    }

    if (serialize_twamp_reflector(
            &packet,
            NULL,
            sizeof(buffer)) == 0) {

        fprintf(stderr, "Expected NULL buffer to fail\n");
        return 1;
    }

    if (serialize_twamp_reflector(
            &packet,
            buffer,
            TWAMP_REFLECTOR_FIXED_SIZE - 1) == 0) {

        fprintf(stderr, "Expected short output buffer to fail\n");
        return 1;
    }

    return 0;
}