#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include "netagent/capture.h"
#include "netagent/packet.h"

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


    puts("Classic BPF UDP filter attached");

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



static int attach_udp_port_filter(int fd, uint16_t port)
{
        struct sock_filter filter[] = {
        BPF_STMT(       //  Get the  EtherType from Ethernet header
            BPF_LD | BPF_H | BPF_ABS,
            12
        ),

        BPF_JUMP(
            BPF_JMP | BPF_JEQ | BPF_K,
            ETH_P_IP,   //  is IPv4?
            0,          // Yes - forward to next instruction
            8           // NO  - DROP (Jumps 8 steps from next instruction)
        ),

        BPF_STMT(       // Get the Protocol from IPv4 header
            BPF_LD | BPF_B | BPF_ABS,
            23
        ),

        BPF_JUMP(       
            BPF_JMP | BPF_JEQ | BPF_K,
            IPPROTO_UDP,//  is UDP?
            0,          // Yes - forward to next instruction
            6           // No  - DROP
        ),

        BPF_STMT(       // Get the IHL from IPv4 header dynamically 
            BPF_LDX | BPF_B | BPF_MSH,
            14
        ),
        BPF_STMT(       // Get the src Port from UDP header
            BPF_LD | BPF_H | BPF_IND,
            14
        ),

        BPF_JUMP(
            BPF_JMP | BPF_JEQ | BPF_K,
            port,      //  is Source Port == port?
            2,          // Yes - ACCEPT
            0           // No  - Check dst port
        ),

        BPF_STMT(       // Get the dst Port from UDP header
            BPF_LD | BPF_H | BPF_IND,
            16
        ),

        BPF_JUMP(
            BPF_JMP | BPF_JEQ | BPF_K,
            port,      //  is Destination Port == port?
            0,          // Yes - ACCEPT
            1           // No  - DROP
        ),

        BPF_STMT(       // ACCEPT   
            BPF_RET | BPF_K,
            0xFFFFFFFF
        ),

        BPF_STMT(       // DROP
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

    printf("Classic BPF filter attached for UDP port %u\n", port);

    return 0;
}