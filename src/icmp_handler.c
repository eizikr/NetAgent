#include "netagent/icmp_handler.h"

#include <stdio.h>

#include "netagent/icmp.h"
#include "netagent/log.h"

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
        log_error("Failed to parse ICMP header\n");
        return result;
    }

    switch (icmp_header.type) {

    case ICMP_ECHO_REQUEST:
        log_info("ICMP Echo Request");
        break;

    case ICMP_ECHO_REPLY:
        log_info("ICMP Echo Reply");
        break;

    default:
        log_info("ICMP Type: %u\n", icmp_header.type);
        break;
    }

    log_info("Type:                %u\n", icmp_header.type);
    log_info("Code:                %u\n", icmp_header.code);
    log_info("Identifier:          %u\n", icmp_header.identifier);
    log_info("Sequence:            %u\n", icmp_header.sequence);

    return 0;
}