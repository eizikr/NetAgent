#include <stdio.h>

#include "netagent/ipv4.h"

static int test_valid_ipv4(void);
static int test_null_buffer(void);
static int test_null_header(void);
static int test_short_buffer(void);
static int test_invalid_ihl(void);
static int test_ihl_exceeds_buffer(void);

int main(void)
{
    if (test_valid_ipv4() != 0)
        return 1;

    if (test_null_buffer() != 0)
        return 1;

    if (test_null_header() != 0)
        return 1;

    if (test_short_buffer() != 0)
        return 1;

    if (test_invalid_ihl() != 0)
        return 1;

    if (test_ihl_exceeds_buffer() != 0)
        return 1;

    puts("test_ipv4: PASS");

    return 0;
}

static int test_valid_ipv4(void)
{
    uint8_t packet[] = {
        0x45, 0x00, 0x00, 0x14,
        0x74, 0x8d, 0x40, 0x00,
        0x40, 0x01, 0x90, 0x9c,
        0xac, 0x11, 0xee, 0xb6,
        0xac, 0x11, 0xee, 0xa5
    };

    IPv4Header header;

    int result = parse_ipv4(
        packet,
        sizeof(packet),
        &header
    );

    if (result != 0) {
        fprintf(stderr, "parse_ipv4 failed\n");
        return 1;
    }

    if (header.version != 4) {
        fprintf(stderr,
                "Version mismatch: expected 4, got %u\n",
                header.version);
        return 1;
    }

    if (header.ihl != 5) {
        fprintf(stderr,
                "IHL mismatch: expected 5, got %u\n",
                header.ihl);
        return 1;
    }

    if (header.total_length != 20) {
        fprintf(stderr,
                "Total length mismatch: expected 20, got %u\n",
                header.total_length);
        return 1;
    }

    if (header.ttl != 64) {
        fprintf(stderr,
                "TTL mismatch: expected 64, got %u\n",
                header.ttl);
        return 1;
    }

    if (header.protocol != 1) {
        fprintf(stderr,
                "Protocol mismatch: expected 1, got %u\n",
                header.protocol);
        return 1;
    }

    if (header.src_addr != 0xac11eeb6) {
        fprintf(stderr,
                "Source address mismatch: got 0x%08x\n",
                header.src_addr);
        return 1;
    }

    if (header.dst_addr != 0xac11eea5) {
        fprintf(stderr,
                "Destination address mismatch: got 0x%08x\n",
                header.dst_addr);
        return 1;
    }

    return 0;
}

static int test_null_buffer(void)
{
    IPv4Header header;

    if (parse_ipv4(NULL, 20, &header) == 0) {
        fprintf(stderr, "Expected NULL buffer to fail\n");
        return 1;
    }

    return 0;
}

static int test_null_header(void)
{
    uint8_t packet[20] = {0};

    if (parse_ipv4(packet, sizeof(packet), NULL) == 0) {
        fprintf(stderr, "Expected NULL header to fail\n");
        return 1;
    }

    return 0;
}

static int test_short_buffer(void)
{
    uint8_t packet[10] = {0};

    IPv4Header header;

    if (parse_ipv4(packet, sizeof(packet), &header) == 0) {
        fprintf(stderr, "Expected short buffer to fail\n");
        return 1;
    }

    return 0;
}

static int test_invalid_ihl(void)
{
    uint8_t packet[20] = {
        0x44, 0x00, 0x00, 0x14
    };

    IPv4Header header;

    if (parse_ipv4(packet, sizeof(packet), &header) == 0) {
        fprintf(stderr, "Expected invalid IHL to fail\n");
        return 1;
    }

    return 0;
}

static int test_ihl_exceeds_buffer(void)
{
    uint8_t packet[20] = {
        0x46, 0x00, 0x00, 0x18
    };

    IPv4Header header;

    if (parse_ipv4(packet, sizeof(packet), &header) == 0) {
        fprintf(stderr, "Expected oversized IHL to fail\n");
        return 1;
    }

    return 0;
}