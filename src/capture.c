#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include "netagent/capture.h"
#include "netagent/packet.h"
#include "netagent/twamp.h"
#include "netagent/tx.h"

#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/filter.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>

static volatile sig_atomic_t stop_requested = 0;

static int attach_udp_port_filter(int fd, uint16_t port);
static void handle_sigint(int signo)
{
    (void)signo;
    stop_requested = 1;
}


int capture_packets(const char *interface_name , uint16_t port) {

    uint8_t buffer[PACKET_BUFFER_SIZE];
    UdpTxSocket tx = { .fd = -1, .local_port = 0 };

    if (interface_name == NULL) {
        fprintf(stderr, "Interface name is NULL\n");
        return -1;
    }

    int fd = socket(
        AF_PACKET,          // use layer 2 (Ethernet) socket
        SOCK_RAW,           // work with raw frames include ethernet header
        htons(ETH_P_ALL)    // get only IPv4
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


    if (attach_udp_port_filter(fd, port) != 0) {
        puts("Failed to attach BPF filter");
        goto error_handling;
    }

    if (udp_tx_open(&tx, port) != 0) {
        fprintf(stderr, "Failed to open UDP TX socket\n");
        goto error_handling;
    }
    printf("UDP TX socket bound to port %u\n", tx.local_port);

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

        NtpTimestamp receive_timestamp;

        if (ntp_timestamp_now(&receive_timestamp) != 0) {
            fprintf(stderr, "Failed to capture T2 timestamp\n");
            continue;
        }

        int result = process_packet(buffer, (size_t)received_length, &receive_timestamp, &tx);
        if (result < 0) {
            fprintf(stderr, "Packet processing failed: %d\n", result);
        }
    }



    puts("\nStopping NetAgent...");
    udp_tx_close(&tx);
    close(fd);
    return 0;

error_handling:
    udp_tx_close(&tx);

    if (fd >= 0) {
        close(fd);
    }
    return -1;

}



static int attach_udp_port_filter(int fd, uint16_t port)
{
        struct sock_filter filter[] = {

        /* [0] Load Ethernet EtherType */
        BPF_STMT(
            BPF_LD | BPF_H | BPF_ABS,
            12
        ),

        /* [1] Not IPv4 -> DROP [8] */
        BPF_JUMP(
            BPF_JMP | BPF_JEQ | BPF_K,
            ETH_P_IP,
            0,
            6
        ),

        /* [2] Load IPv4 Protocol */
        BPF_STMT(
            BPF_LD | BPF_B | BPF_ABS,
            23
        ),

        /* [3] Not UDP -> DROP [8] */
        BPF_JUMP(
            BPF_JMP | BPF_JEQ | BPF_K,
            IPPROTO_UDP,
            0,
            4
        ),

        /* [4] X = IPv4 header length (IHL * 4) */
        BPF_STMT(
            BPF_LDX | BPF_B | BPF_MSH,
            14
        ),

        /* [5] Load UDP Destination Port */
        BPF_STMT(
            BPF_LD | BPF_H | BPF_IND,
            16
        ),

        /* [6] dst port == configured port? */
        BPF_JUMP(
            BPF_JMP | BPF_JEQ | BPF_K,
            port,
            0,
            1
        ),

        /* [7] ACCEPT */
        BPF_STMT(
            BPF_RET | BPF_K,
            0xFFFFFFFF
        ),

        /* [8] DROP */
        BPF_STMT(
            BPF_RET | BPF_K,
            0
        )
    };

    struct sock_fprog program = {
        .len = sizeof(filter) / sizeof(filter[0]),
        .filter = filter
    };

    if (setsockopt(
            fd,
            SOL_SOCKET,
            SO_ATTACH_FILTER,
            &program,
            sizeof(program)) < 0) {
        perror("setsockopt(SO_ATTACH_FILTER)");
        return -1;
    }

    return 0;
}


