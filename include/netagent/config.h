#ifndef NETAGENT_CONFIG_H
#define NETAGENT_CONFIG_H

#include <stdint.h>

#include "netagent/log.h"

typedef struct {
    const char *interface_name;
    uint16_t twamp_port;
    LogLevel log_level;
} NetAgentConfig;

#endif