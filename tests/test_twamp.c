#include <stdio.h>

#include "netagent/twamp.h"

static int test_valid_twamp_sender(void);
static int test_null_buffer(void);
static int test_null_packet(void);
static int test_short_buffer(void);

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