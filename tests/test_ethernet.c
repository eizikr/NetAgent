#include <stdio.h>
#include <string.h>

#include "netagent/ethernet.h"

int main(void)
{
    uint8_t packet[] = {
        0x5c, 0x60, 0xba, 0x40, 0x7a, 0x89,
        0x00, 0x0c, 0x29, 0x1b, 0x7b, 0x14,
        0x08, 0x00
    };

    EthernetHeader header;

    int result = parse_ethernet(
        packet,
        sizeof(packet),
        &header
    );

    if (result != ETH_OK) {
        fprintf(stderr, "parse_ethernet failed\n");
        return 1;
    }

    const uint8_t expected_dst[6] = {
        0x5c, 0x60, 0xba, 0x40, 0x7a, 0x89
    };

    const uint8_t expected_src[6] = {
        0x00, 0x0c, 0x29, 0x1b, 0x7b, 0x14
    };

    if (memcmp(header.dst_mac, expected_dst, 6) != 0) {
        fprintf(stderr, "Destination MAC mismatch\n");
        return 1;
    }

    if (memcmp(header.src_mac, expected_src, 6) != 0) {
        fprintf(stderr, "Source MAC mismatch\n");
        return 1;
    }

    if (header.ethertype != 0x0800) {
        fprintf(stderr, "EtherType mismatch\n");
        return 1;
    }

    puts("test_ethernet: PASS");

    return 0;
}