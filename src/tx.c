#include "netagent/tx.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


int udp_tx_open(
    UdpTxSocket *tx,
    uint16_t local_port)
{
    if (tx == NULL) {
        return -1;
    }

    tx->fd = -1;
    tx->local_port = local_port;

    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (fd < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));

    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(local_port);
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(
            fd,
            (struct sockaddr *)&local_addr,
            sizeof(local_addr)) < 0) {

        perror("bind");
        close(fd);
        return -1;
    }

    tx->fd = fd;

    return 0;
}

int udp_tx_send(
    const UdpTxSocket *tx,
    const UdpEndpoint *destination,
    const uint8_t *payload,
    size_t payload_length)
{
    if (tx == NULL ||
        tx->fd < 0 ||
        destination == NULL ||
        payload == NULL) {
        return -1;
    }

    struct sockaddr_in destination_addr;
    memset(&destination_addr, 0, sizeof(destination_addr));

    destination_addr.sin_family = AF_INET;
    destination_addr.sin_port =
        htons(destination->port);

    destination_addr.sin_addr.s_addr =
        htonl(destination->ip);

    ssize_t sent = sendto(
        tx->fd,
        payload,
        payload_length,
        0,
        (struct sockaddr *)&destination_addr,
        sizeof(destination_addr)
    );

    if (sent < 0) {
        perror("sendto");
        return -1;
    }

    if ((size_t)sent != payload_length) {
        fprintf(stderr, "Partial UDP send\n");
        return -1;
    }

    return 0;
}

void udp_tx_close(UdpTxSocket *tx)
{
    if (tx == NULL) {
        return;
    }

    if (tx->fd >= 0) {
        close(tx->fd);
        tx->fd = -1;
    }
}