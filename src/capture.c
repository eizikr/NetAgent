#include "netagent/capture.h"
#include "netagent/packet.h"
#include "netagent/twamp.h"
#include "netagent/stats.h"
#include "netagent/log.h"
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
#include <time.h>
#include <sys/uio.h>

static volatile sig_atomic_t stop_requested = 0;

static int attach_udp_port_filter(int fd, uint16_t port);

static void handle_sigint(int signo)
{
    (void)signo;
    stop_requested = 1;
}


int capture_packets(const NetAgentConfig *config) {

    uint8_t buffer[PACKET_BUFFER_SIZE];
    UdpTxSocket tx = { .fd = -1, .local_port = 0 };
    int enable_timestamp = 1;

    NetAgentStats stats;
    stats_init(&stats);

    if (config == NULL ||
        config->interface_name == NULL) {
        log_error("Invalid input to capture_packets\n");
        return -1;
    }

    const char *interface_name =
        config->interface_name;

    uint16_t port =
        config->twamp_port;

    int fd = socket(
        AF_PACKET,          // use layer 2 (Ethernet) socket
        SOCK_RAW,           // work with raw frames include ethernet header
        htons(ETH_P_ALL)    // receive all Ethernet protocols; BPF filters relevant traffic
    );

    if (fd < 0) {
        perror("socket");
        /*  about perror:
            if socket() fails, the kernel return error and libc will update errno
            perror() will make errno human readable and print it to stderr
        */
        log_error("Failed to create raw socket\n");
        return -1;
    }

    if (setsockopt(
            fd,
            SOL_SOCKET,
            SO_TIMESTAMPNS,                     // timestamp inside the packet
            &enable_timestamp,
            sizeof(enable_timestamp)) < 0) {

        perror("setsockopt(SO_TIMESTAMPNS)");
        goto error_handling;
    }

    log_info("Kernel RX timestamping enabled");

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

    log_info("Raw socket bound to %s (ifindex=%u)\n",
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
        log_error("Failed to attach BPF filter");
        goto error_handling;
    }

    if (udp_tx_open(&tx, port) != 0) {
        log_error("Failed to open UDP TX socket");
        goto error_handling;
    }

    log_info("UDP TX socket bound to port %u", tx.local_port);

    PacketSender sender = {
        .send = udp_tx_packet_sender_send,
        .context = &tx
    };

    while(!stop_requested){
        
        struct iovec iov;                   // describe where to store the received data
        iov.iov_base = buffer;              // this is the buffer
        iov.iov_len = sizeof(buffer);       // this is his size

        char control[CMSG_SPACE(sizeof(struct timespec))];
        memset(control, 0, sizeof(control));

        struct msghdr msg;
        memset(&msg, 0, sizeof(msg));

        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;

        msg.msg_control = control;
        msg.msg_controllen = sizeof(control);
        ssize_t received_length = recvmsg(
            fd,
            &msg,
            0
        );

        if (msg.msg_flags & MSG_CTRUNC) {
            log_error("Control data truncated\n");
            continue;
        }

        if (received_length < 0) {
            if (errno == EINTR && stop_requested) {
                break;
            }
            
            perror("recvmsg");
            goto error_handling;
        }

        stats.packets_received++;

        struct timespec *kernel_timestamp = NULL;

        for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
            cmsg != NULL;
            cmsg = CMSG_NXTHDR(&msg, cmsg)) {

            if (cmsg->cmsg_level == SOL_SOCKET &&
                cmsg->cmsg_type == SCM_TIMESTAMPNS) {

                kernel_timestamp =
                    (struct timespec *)CMSG_DATA(cmsg);

                break;
            }
        }

        NtpTimestamp receive_timestamp;

        if (kernel_timestamp == NULL) {
            log_error("Missing kernel RX timestamp\n");
            continue;
        }

        if (timespec_to_ntp(
                kernel_timestamp,
                &receive_timestamp) != 0) {

            log_error("Failed to convert RX timestamp to NTP\n");
            continue;
        }

        int result = process_packet(
            buffer,
            (size_t)received_length,
            &receive_timestamp,
            &sender,
            &stats,
            config
        );

        if (result < 0) {
            log_error("Packet processing failed: %d\n", result);
        }
    }


    printf("\n");
    log_info("Stopping NetAgent...\n");
    stats_print(&stats);

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
        BPF_STMT(
            BPF_LD | BPF_H | BPF_ABS,
            12
        ),
        BPF_JUMP(
            BPF_JMP | BPF_JEQ | BPF_K,
            ETH_P_IP,
            0,
            6
        ),
        BPF_STMT(
            BPF_LD | BPF_B | BPF_ABS,
            23
        ),
        BPF_JUMP(
            BPF_JMP | BPF_JEQ | BPF_K,
            IPPROTO_UDP,
            0,
            4
        ),
        BPF_STMT(
            BPF_LDX | BPF_B | BPF_MSH,
            14
        ),
        BPF_STMT(
            BPF_LD | BPF_H | BPF_IND,
            16
        ),
        BPF_JUMP(
            BPF_JMP | BPF_JEQ | BPF_K,
            port,
            0,
            1
        ),
        BPF_STMT(
            BPF_RET | BPF_K,
            0xFFFFFFFF
        ),
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


