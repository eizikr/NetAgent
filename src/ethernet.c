#include <stdio.h>
#include <string.h>

#include "netagent/ethernet.h"
#include "netagent/log.h"


int parse_ethernet(const uint8_t *buffer, size_t length, EthernetHeader *header)
{
    if (buffer == NULL){
        log_error("ERROR: buffer argument is NULL");
        return ERR_BUFFER_NULL;
    }

    if (header == NULL){
	    log_error("ERROR: header argument is NULL");
        return ERR_HEADER_NULL;
    }

    if (length < 14){
        log_error("ERROR: buffer length is short");
        return ERR_ARGS;
    }

    memcpy(header->dst_mac  , buffer, 6);
    memcpy(header->src_mac  , buffer + 6, 6);
    header->ethertype =
	((uint16_t)buffer[12] << 8) |
	buffer[13];

    return ETH_OK;
}
