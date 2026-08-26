#ifndef NETAGENT_CAPTURE_H
#define NETAGENT_CAPTURE_H
#include "netagent/config.h"

#define PACKET_BUFFER_SIZE 65536
int capture_packets(const NetAgentConfig *config);

#endif
