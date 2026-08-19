#ifndef NETAGENT_ETHERNET_H
#define NETAGENT_ETHERNET_H

#include <stddef.h>
#include <stdint.h>

#define ETHERNET_HEADER_SIZE 14

typedef enum {
	ETH_OK=0,
	ERR_ARGS,
	ERR_BUFFER_NULL,
	ERR_HEADER_NULL
	} ETH_ERROR;

typedef struct{
	uint8_t dst_mac[6];
        uint8_t src_mac[6];
        uint16_t ethertype;


} EthernetHeader;


int parse_ethernet(const uint8_t *buffer, size_t length, EthernetHeader *header);

#endif
