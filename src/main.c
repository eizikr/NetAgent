#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "netagent/capture.h"
#include "netagent/config.h"
#include "netagent/log.h"

int main(int argc, char *argv[])
{
    LogLevel log_level = LOG_LEVEL_INFO;

    if (argc != 3 && argc != 4) {
        log_error(
            "Usage: %s <interface> <udp-port> [--debug]",
            argv[0]
        );
        return 1;
    }

    if (argc == 4) {
        if (strcmp(argv[3], "--debug") != 0) {
            log_error("Unknown option: %s", argv[3]);
            return 1;
        }

        log_level = LOG_LEVEL_DEBUG;
    }

    const char *interface_name = argv[1];

    unsigned long port_value =
        strtoul(argv[2], NULL, 10);

    if (port_value > 65535) {
        log_error("Invalid UDP port");
        return 1;
    }

    uint16_t port = (uint16_t)port_value;

    NetAgentConfig config = {
        .interface_name = interface_name,
        .twamp_port = port,
        .log_level = log_level
    };

    log_set_level(config.log_level);

    log_info("NetAgent v0.1.0");
    log_info(
        "Listening on %s, UDP port %u",
        interface_name,
        port
    );

    return capture_packets(&config);
}