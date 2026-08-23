#include <stdio.h>

#include "netagent/icmp.h"

static int test_echo_request(void);
static int test_echo_reply(void);
static int test_null_buffer(void);
static int test_null_header(void);
static int test_short_buffer(void);

int main(void)
{
    if (test_echo_request() != 0)
        return 1;

    if (test_echo_reply() != 0)
        return 1;

    if (test_null_buffer() != 0)
        return 1;

    if (test_null_header() != 0)
        return 1;

    if (test_short_buffer() != 0)
        return 1;

    puts("test_icmp: PASS");
    return 0;
}

static int test_echo_request(void)
{
    uint8_t packet[] = {
        0x08, 0x00,       /* Type = Echo Request, Code = 0 */
        0x1a, 0xfb,       /* Checksum */
        0x00, 0x06,       /* Identifier = 6 */
        0x00, 0x01        /* Sequence = 1 */
    };

    ICMPHeader header;

    int result = parse_icmp(
        packet,
        sizeof(packet),
        &header
    );

    if (result != 0) {
        fprintf(stderr, "parse_icmp failed for Echo Request\n");
        return 1;
    }

    if (header.type != ICMP_ECHO_REQUEST) {
        fprintf(stderr,
                "Type mismatch: expected %u, got %u\n",
                ICMP_ECHO_REQUEST,
                header.type);
        return 1;
    }

    if (header.code != 0) {
        fprintf(stderr,
                "Code mismatch: expected 0, got %u\n",
                header.code);
        return 1;
    }

    if (header.checksum != 0x1afb) {
        fprintf(stderr,
                "Checksum mismatch: expected 0x1afb, got 0x%04x\n",
                header.checksum);
        return 1;
    }

    if (header.identifier != 6) {
        fprintf(stderr,
                "Identifier mismatch: expected 6, got %u\n",
                header.identifier);
        return 1;
    }

    if (header.sequence != 1) {
        fprintf(stderr,
                "Sequence mismatch: expected 1, got %u\n",
                header.sequence);
        return 1;
    }

    return 0;
}

static int test_echo_reply(void)
{
    uint8_t packet[] = {
        0x00, 0x00,       /* Type = Echo Reply, Code = 0 */
        0x22, 0xfb,       /* Checksum */
        0x00, 0x06,       /* Identifier = 6 */
        0x00, 0x01        /* Sequence = 1 */
    };

    ICMPHeader header;

    int result = parse_icmp(
        packet,
        sizeof(packet),
        &header
    );

    if (result != 0) {
        fprintf(stderr, "parse_icmp failed for Echo Reply\n");
        return 1;
    }

    if (header.type != ICMP_ECHO_REPLY) {
        fprintf(stderr,
                "Type mismatch: expected %u, got %u\n",
                ICMP_ECHO_REPLY,
                header.type);
        return 1;
    }

    if (header.code != 0) {
        fprintf(stderr,
                "Code mismatch: expected 0, got %u\n",
                header.code);
        return 1;
    }

    if (header.identifier != 6) {
        fprintf(stderr,
                "Identifier mismatch: expected 6, got %u\n",
                header.identifier);
        return 1;
    }

    if (header.sequence != 1) {
        fprintf(stderr,
                "Sequence mismatch: expected 1, got %u\n",
                header.sequence);
        return 1;
    }

    return 0;
}

static int test_null_buffer(void)
{
    ICMPHeader header;

    if (parse_icmp(NULL, 8, &header) == 0) {
        fprintf(stderr, "Expected NULL buffer to fail\n");
        return 1;
    }

    return 0;
}

static int test_null_header(void)
{
    uint8_t packet[8] = {0};

    if (parse_icmp(packet, sizeof(packet), NULL) == 0) {
        fprintf(stderr, "Expected NULL header to fail\n");
        return 1;
    }

    return 0;
}

static int test_short_buffer(void)
{
    uint8_t packet[7] = {0};
    ICMPHeader header;

    if (parse_icmp(packet, sizeof(packet), &header) == 0) {
        fprintf(stderr, "Expected short ICMP buffer to fail\n");
        return 1;
    }

    return 0;
}