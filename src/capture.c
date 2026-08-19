#define _POSIX_C_SOURCE 200809L

#include "netagent/capture.h"
#include "netagent/packet.h"

#include <arpa/inet.h>
#include <errno.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>

static volatile sig_atomic_t stop_requested = 0;

static void handle_sigint(int signo)
{
    (void)signo;
    stop_requested = 1;
}


int capture_packets(const char *interface_name) {
    uint8_t buffer[PACKET_BUFFER_SIZE];

    if (interface_name == NULL) {
        fprintf(stderr, "Interface name is NULL\n");
        return -1;
    }

    int fd = socket(
        AF_PACKET,          // use layer 2 (Ethernet) socket
        SOCK_RAW,           // work with raw frames include ethernet header
        htons(ETH_P_ALL)    // get all protocols (ethernet types)
    );

    if (fd < 0) {
        perror("socket");
        /*  about perror:
            if socket() fails, the kernel return error and libc will update errno
            perror() will make errno human readable and print it to stderr
        */
        return -1;
    }



    unsigned int ifindex = if_nametoindex(interface_name); // convert "ens33" to "2"

    if (ifindex == 0) {
        perror("if_nametoindex");
        goto error_handling;
    }

    struct sockaddr_ll addr;
    memset(&addr, 0, sizeof(addr));
    addr.sll_family = AF_PACKET;
    addr.sll_protocol = htons(ETH_P_ALL);
    addr.sll_ifindex = (int)ifindex;

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        goto error_handling;
    }
    printf("Raw socket bound to %s (ifindex=%u)\n",
           interface_name,
           ifindex);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("sigaction");
        goto error_handling;
    }

    while(!stop_requested){
        
        ssize_t received_length = recvfrom(
            /*Read N bytes into BUF through socket FD.*/
            fd,
            buffer,
            PACKET_BUFFER_SIZE,
            0,
            NULL,
            NULL
        );

        if (received_length < 0) {
            if (errno == EINTR && stop_requested) {
                break;
            }
            perror("recvfrom");
            goto error_handling;
        }

        int result = process_packet(buffer, (size_t)received_length);
        if (result < 0) {
            fprintf(stderr, "Packet processing failed: %d\n", result);
        }
    }



    puts("\nStopping NetAgent...");
    close(fd);
    return 0;

error_handling:
    close(fd);
    return -1;

}