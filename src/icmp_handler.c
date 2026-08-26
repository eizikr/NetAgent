#include "netagent/icmp_handler.h"

#include <stdio.h>

#include "netagent/icmp.h"

int icmp_handle_packet(
    const uint8_t *payload,
    size_t payload_length)
{
    ICMPHeader icmp_header;

    int result = parse_icmp(
        payload,
        payload_length,
        &icmp_header
    );

    if (result != 0) {
        fprintf(stderr, "Failed to parse ICMP header\n");
        return result;
    }

    switch (icmp_header.type) {

    case ICMP_ECHO_REQUEST:
        puts("\nICMP Echo Request");
        break;

    case ICMP_ECHO_REPLY:
        puts("\nICMP Echo Reply");
        break;

    default:
        printf("\nICMP Type: %u\n", icmp_header.type);
        break;
    }

    printf("Type:                %u\n", icmp_header.type);
    printf("Code:                %u\n", icmp_header.code);
    printf("Identifier:          %u\n", icmp_header.identifier);
    printf("Sequence:            %u\n", icmp_header.sequence);

    return 0;
}