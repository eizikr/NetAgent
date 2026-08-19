#ifndef NETAGENT_PACKET_H
#define NETAGENT_PACKET_H
#include <stddef.h>
#include <stdint.h>

#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP  6
#define IP_PROTO_UDP  17

int process_packet(const uint8_t *packet, size_t length);
#endif