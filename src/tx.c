#include "netagent/tx.h"

#include <linux/net_tstamp.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <poll.h>

int udp_tx_read_timestamp(
    const UdpTxSocket *tx,
    struct timespec *timestamp)
{
    if (tx == NULL || tx->fd < 0 || timestamp == NULL) {
        return -1;
    }

    /*
     * TX timestamp arrives asynchronously through
     * the socket error queue.
     *
     * POLLERR becomes ready when error-queue data
     * (including TX timestamp metadata) is available.
     */
    struct pollfd pfd = {
        .fd = tx->fd,
        .events = POLLERR
    };

    int poll_result = poll(&pfd, 1, 1000);

    if (poll_result < 0) {
        perror("poll");
        return -1;
    }

    if (poll_result == 0) {
        fprintf(stderr, "Timed out waiting for TX timestamp\n");
        return -1;
    }

    char data[1];

    struct iovec iov = {
        .iov_base = data,
        .iov_len = sizeof(data)
    };

    char control[512];

    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control;
    msg.msg_controllen = sizeof(control);

    ssize_t result = recvmsg(
        tx->fd,
        &msg,
        MSG_ERRQUEUE
    );

    if (result < 0) {
        perror("recvmsg(MSG_ERRQUEUE)");
        return -1;
    }

    for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
         cmsg != NULL;
         cmsg = CMSG_NXTHDR(&msg, cmsg)) {

        if (cmsg->cmsg_level == SOL_SOCKET &&
            cmsg->cmsg_type == SO_TIMESTAMPING) {

            /*
             * SCM_TIMESTAMPING returns three timestamps:
             *
             * ts[0] = software timestamp
             * ts[1] = legacy transformed hardware timestamp
             * ts[2] = raw hardware timestamp
             */
            struct timespec *timestamps =
                (struct timespec *)CMSG_DATA(cmsg);

            *timestamp = timestamps[0];

            return 0;
        }
    }

    fprintf(stderr, "TX timestamp not found in error queue\n");

    return -1;
}

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


    int timestamp_flags =
    SOF_TIMESTAMPING_TX_SOFTWARE |      // Create timestamp when packet is going through the kernel stack
    SOF_TIMESTAMPING_SOFTWARE;          // return the software timestamp to us

    if (setsockopt(
            fd,
            SOL_SOCKET,
            SO_TIMESTAMPING,
            &timestamp_flags,
            sizeof(timestamp_flags)) < 0) {

        perror("setsockopt(SO_TIMESTAMPING)");
        close(fd);
        return -1;
    }

    puts("Kernel TX software timestamping enabled");

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