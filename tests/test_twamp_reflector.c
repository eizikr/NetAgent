#include <stdio.h>

#include "netagent/twamp_reflector.h"

static int test_build_valid_response(void);
static int test_short_twamp_payload(void);
static int test_null_receive_timestamp(void);

int main(void)
{
    if (test_build_valid_response() != 0)
        return 1;

    if (test_short_twamp_payload() != 0)
        return 1;

    if (test_null_receive_timestamp() != 0)
        return 1;

    puts("test_twamp_reflector: PASS");
    return 0;
}

static int test_build_valid_response(void)
{
    uint8_t payload[] = {
        0x00, 0x00, 0x01, 0x30,
        0xe8, 0x00, 0x00, 0x01,
        0x12, 0x34, 0x56, 0x78,
        0x80, 0x01
    };

    NtpTimestamp t2 = {
        .seconds = 0x01020304,
        .fraction = 0x05060708
    };

    uint8_t response[TWAMP_REFLECTOR_FIXED_SIZE];
    uint32_t sender_sequence = 0;

    int result = twamp_reflector_build_response(
        payload,
        sizeof(payload),
        64,
        &t2,
        response,
        sizeof(response),
        &sender_sequence
    );

    if (result != 0) {
        fprintf(stderr, "Failed to build TWAMP response\n");
        return 1;
    }

    if (sender_sequence != 304) {
        fprintf(stderr, "Unexpected sender sequence\n");
        return 1;
    }

    if (response[16] != 0x01 ||
        response[17] != 0x02 ||
        response[18] != 0x03 ||
        response[19] != 0x04) {

        fprintf(stderr, "T2 seconds not serialized correctly\n");
        return 1;
    }

    if (response[20] != 0x05 ||
        response[21] != 0x06 ||
        response[22] != 0x07 ||
        response[23] != 0x08) {

        fprintf(stderr, "T2 fraction not serialized correctly\n");
        return 1;
    }

    if (response[40] != 64) {
        fprintf(stderr, "Sender TTL mismatch\n");
        return 1;
    }

    return 0;
}

static int test_short_twamp_payload(void)
{
    uint8_t payload[10] = {0};
    uint8_t response[TWAMP_REFLECTOR_FIXED_SIZE];
    uint32_t sender_sequence = 0;

    NtpTimestamp t2 = {
        .seconds = 1,
        .fraction = 0
    };

    int result = twamp_reflector_build_response(
        payload,
        sizeof(payload),
        64,
        &t2,
        response,
        sizeof(response),
        &sender_sequence
    );

    if (result == 0) {
        fprintf(stderr,
                "Expected short TWAMP payload to fail\n");
        return 1;
    }

    return 0;
}

static int test_null_receive_timestamp(void)
{
    uint8_t payload[] = {
        0x00, 0x00, 0x01, 0x30,
        0xe8, 0x00, 0x00, 0x01,
        0x12, 0x34, 0x56, 0x78,
        0x80, 0x01
    };

    uint8_t response[TWAMP_REFLECTOR_FIXED_SIZE];
    uint32_t sender_sequence = 0;

    int result = twamp_reflector_build_response(
        payload,
        sizeof(payload),
        64,
        NULL,
        response,
        sizeof(response),
        &sender_sequence
    );

    if (result == 0) {
        fprintf(stderr,
                "Expected NULL receive timestamp to fail\n");
        return 1;
    }

    return 0;
}