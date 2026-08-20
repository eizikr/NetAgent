#ifndef NETAGENT_CAPTURE_H
#define NETAGENT_CAPTURE_H
#include <stdint.h>

#define PACKET_BUFFER_SIZE 65536
int capture_packets(const char *interface_name, uint16_t port);

#endif
