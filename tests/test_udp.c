#include <stdio.h>

#include "netagent/udp.h"

static int test_valid_udp(void);
static int test_null_buffer(void);
static int test_null_header(void);
static int test_short_buffer(void);
static int test_invalid_udp_length(void);
static int test_udp_length_exceeds_buffer(void);

int main(void)
{
    if (test_valid_udp() != 0)
        return 1;

    if (test_null_buffer() != 0)
        return 1;

    if (test_null_header() != 0)
        return 1;

    if (test_short_buffer() != 0)
        return 1;

    if (test_invalid_udp_length() != 0)
        return 1;

    if (test_udp_length_exceeds_buffer() != 0)
        return 1;

    puts("test_udp: PASS");
    return 0;
}

static int test_null_header(void)
{
    uint8_t packet[UDP_HEADER_SIZE] = {0};

    if (parse_udp(packet, sizeof(packet), NULL) == 0) {
        fprintf(stderr, "Expected NULL header to fail\n");
        return 1;
    }

    return 0;
}

static int test_null_buffer(void)
{
    UDPHeader header;

    if (parse_udp(NULL, UDP_HEADER_SIZE, &header) == 0) {
        fprintf(stderr, "Expected NULL buffer to fail\n");
        return 1;
    }

    return 0;
}

static int test_short_buffer(void)
{
    uint8_t packet[7] = {0};
    UDPHeader header;

    if (parse_udp(packet, sizeof(packet), &header) == 0) {
        fprintf(stderr, "Expected short UDP buffer to fail\n");
        return 1;
    }

    return 0;
}

static int test_invalid_udp_length(void)
{
    uint8_t packet[] = {
        0xc3, 0x50,
        0x50, 0x01,

        0x00, 0x07,     /* Invalid: UDP Length = 7 */

        0x00, 0x00
    };

    UDPHeader header;

    if (parse_udp(packet, sizeof(packet), &header) == 0) {
        fprintf(stderr, "Expected UDP length < 8 to fail\n");
        return 1;
    }

    return 0;
}

static int test_udp_length_exceeds_buffer(void)
{
    uint8_t packet[] = {
        0xc3, 0x50,
        0x50, 0x01,

        0x00, 0x14,     /* Claims 20 bytes */

        0x00, 0x00
    };

    UDPHeader header;

    if (parse_udp(packet, sizeof(packet), &header) == 0) {
        fprintf(stderr, "Expected UDP length > buffer to fail\n");
        return 1;
    }

    return 0;
}

static int test_valid_udp(void)
{
    /*
     * UDP Header:
     *
     * Source Port:      50000 = 0xC350
     * Destination Port: 20481 = 0x5001
     * Length:           12
     * Checksum:         0
     *
     * Followed by 4 bytes payload.
     */
    uint8_t packet[] = {
        0xc3, 0x50,             /* Source Port      */
        0x50, 0x01,             /* Destination Port */
        0x00, 0x0c,             /* UDP Length       */
        0x00, 0x00,             /* Checksum         */

        0xde, 0xad, 0xbe, 0xef  /* Payload          */
    };

    UDPHeader header;

    int result = parse_udp(
        packet,
        sizeof(packet),
        &header
    );

    if (result != 0) {
        fprintf(stderr, "parse_udp failed\n");
        return 1;
    }

    if (header.src_port != 50000) {
        fprintf(stderr,
                "Source port mismatch: expected 50000, got %u\n",
                header.src_port);
        return 1;
    }

    if (header.dst_port != 20481) {
        fprintf(stderr,
                "Destination port mismatch: expected 20481, got %u\n",
                header.dst_port);
        return 1;
    }

    if (header.length != 12) {
        fprintf(stderr,
                "Length mismatch: expected 12, got %u\n",
                header.length);
        return 1;
    }

    if (header.checksum != 0) {
        fprintf(stderr,
                "Checksum mismatch: expected 0, got 0x%04x\n",
                header.checksum);
        return 1;
    }

    return 0;
}