#include <stdio.h>
#include <string.h>

#include "netagent/config.h"
#include "netagent/packet.h"
#include "netagent/packet_sender.h"
#include "netagent/stats.h"
#include "netagent/twamp.h"

typedef struct {
    int called;

    UdpEndpoint destination;

    uint8_t buffer[TWAMP_REFLECTOR_FIXED_SIZE];
    size_t length;
} FakeSenderContext;

static int test_wrong_udp_port_no_response(void);

static int test_truncated_packet_fails(void);

static int test_tx_failure_updates_stats(void);

static int fake_send(
    void *context,
    const UdpEndpoint *destination,
    const uint8_t *buffer,
    size_t length)
{
    if (context == NULL ||
        destination == NULL ||
        buffer == NULL) {
        return -1;
    }

    FakeSenderContext *fake =
        (FakeSenderContext *)context;

    fake->called++;
    fake->destination = *destination;
    fake->length = length;

    if (length > sizeof(fake->buffer)) {
        return -1;
    }

    memcpy(
        fake->buffer,
        buffer,
        length
    );

    return 0;
}

static int test_twamp_packet_flow(void)
{
    /*
     * Ethernet
     * IPv4
     * UDP
     * TWAMP Sender
     */
    uint8_t packet[] = {

        /* Ethernet */
        0x5c, 0x60, 0xba, 0x40, 0x7a, 0x89,
        0x00, 0x0c, 0x29, 0x1b, 0x7b, 0x14,
        0x08, 0x00,

        /* IPv4 */
        0x45, 0x00,
        0x00, 0x2a,       /* total length = 42 */
        0x00, 0x01,
        0x00, 0x00,
        0x40,              /* TTL = 64 */
        0x11,              /* UDP */
        0x00, 0x00,        /* checksum ignored by parser */

        /* Source IP: 172.17.238.165 */
        0xac, 0x11, 0xee, 0xa5,

        /* Destination IP: 172.17.238.182 */
        0xac, 0x11, 0xee, 0xb6,

        /* UDP */
        0xe0, 0xe3,        /* source port = 57571 */
        0x50, 0x01,        /* destination port = 20481 */
        0x00, 0x16,        /* UDP length = 22 */
        0x00, 0x00,

        /* TWAMP */
        0x00, 0x00, 0x01, 0x30,
        0xe8, 0x00, 0x00, 0x01,
        0x12, 0x34, 0x56, 0x78,
        0x80, 0x01
    };

    FakeSenderContext fake = {0};

    PacketSender packet_sender = {
        .send = fake_send,
        .context = &fake
    };

    NetAgentStats stats;
    stats_init(&stats);

    NetAgentConfig config = {
        .interface_name = "test0",
        .twamp_port = 20481,
        .log_level = LOG_LEVEL_ERROR
    };

    NtpTimestamp t2 = {
        .seconds = 0x01020304,
        .fraction = 0x05060708
    };

    int result = process_packet(
        packet,
        sizeof(packet),
        &t2,
        &packet_sender,
        &stats,
        &config
    );

    if (result != 0) {
        fprintf(stderr, "process_packet failed\n");
        return 1;
    }

    if (fake.called != 1) {
        fprintf(stderr, "Expected exactly one response\n");
        return 1;
    }

    if (fake.destination.ip != 0xac11eea5) {
        fprintf(stderr, "Wrong destination IP\n");
        return 1;
    }

    if (fake.destination.port != 57571) {
        fprintf(stderr, "Wrong destination port\n");
        return 1;
    }

    if (fake.length != TWAMP_REFLECTOR_FIXED_SIZE) {
        fprintf(stderr, "Wrong response length\n");
        return 1;
    }

    if (stats.twamp_received != 1) {
        fprintf(stderr, "TWAMP RX counter mismatch\n");
        return 1;
    }

    if (stats.twamp_sent != 1) {
        fprintf(stderr, "TWAMP TX counter mismatch\n");
        return 1;
    }

    if (stats.last_sender_sequence != 304) {
        fprintf(stderr, "Sequence tracking mismatch\n");
        return 1;
    }

    return 0;
}

static int test_wrong_udp_port_no_response(void)
{
    uint8_t packet[] = {

        /* Ethernet */
        0x5c, 0x60, 0xba, 0x40, 0x7a, 0x89,
        0x00, 0x0c, 0x29, 0x1b, 0x7b, 0x14,
        0x08, 0x00,

        /* IPv4 */
        0x45, 0x00,
        0x00, 0x2a,
        0x00, 0x01,
        0x00, 0x00,
        0x40,
        0x11,
        0x00, 0x00,

        0xac, 0x11, 0xee, 0xa5,
        0xac, 0x11, 0xee, 0xb6,

        /* UDP */
        0xe0, 0xe3,

        /* destination port = 50000, not 20481 */
        0xc3, 0x50,

        0x00, 0x16,
        0x00, 0x00,

        /* payload */
        0x00, 0x00, 0x01, 0x30,
        0xe8, 0x00, 0x00, 0x01,
        0x12, 0x34, 0x56, 0x78,
        0x80, 0x01
    };

    FakeSenderContext fake = {0};

    PacketSender packet_sender = {
        .send = fake_send,
        .context = &fake
    };

    NetAgentStats stats;
    stats_init(&stats);

    NetAgentConfig config = {
        .interface_name = "test0",
        .twamp_port = 20481,
        .log_level = LOG_LEVEL_ERROR
    };

    NtpTimestamp t2 = {
        .seconds = 1,
        .fraction = 0
    };

    int result = process_packet(
        packet,
        sizeof(packet),
        &t2,
        &packet_sender,
        &stats,
        &config
    );

    if (result != 0) {
        fprintf(stderr, "Unexpected process_packet failure\n");
        return 1;
    }

    if (fake.called != 0) {
        fprintf(stderr, "Response sent for wrong UDP port\n");
        return 1;
    }

    if (stats.twamp_received != 0 ||
        stats.twamp_sent != 0) {
        fprintf(stderr, "TWAMP stats changed for wrong UDP port\n");
        return 1;
    }

    return 0;
}

static int test_truncated_packet_fails(void)
{
    uint8_t packet[] = {
        0x5c, 0x60, 0xba
    };

    FakeSenderContext fake = {0};

    PacketSender packet_sender = {
        .send = fake_send,
        .context = &fake
    };

    NetAgentStats stats;
    stats_init(&stats);

    NetAgentConfig config = {
        .interface_name = "test0",
        .twamp_port = 20481,
        .log_level = LOG_LEVEL_ERROR
    };

    NtpTimestamp t2 = {
        .seconds = 1,
        .fraction = 0
    };

    int result = process_packet(
        packet,
        sizeof(packet),
        &t2,
        &packet_sender,
        &stats,
        &config
    );

    if (result == 0) {
        fprintf(stderr, "Expected truncated packet to fail\n");
        return 1;
    }

    if (fake.called != 0) {
        fprintf(stderr, "Response sent for truncated packet\n");
        return 1;
    }

    return 0;
}

static int fake_send_fail(
    void *context,
    const UdpEndpoint *destination,
    const uint8_t *buffer,
    size_t length)
{
    (void)context;
    (void)destination;
    (void)buffer;
    (void)length;

    return -1;
}

static int test_tx_failure_updates_stats(void)
{
    uint8_t packet[] = {

        /* Ethernet */
        0x5c, 0x60, 0xba, 0x40, 0x7a, 0x89,
        0x00, 0x0c, 0x29, 0x1b, 0x7b, 0x14,
        0x08, 0x00,

        /* IPv4 */
        0x45, 0x00,
        0x00, 0x2a,
        0x00, 0x01,
        0x00, 0x00,
        0x40,
        0x11,
        0x00, 0x00,

        0xac, 0x11, 0xee, 0xa5,
        0xac, 0x11, 0xee, 0xb6,

        /* UDP */
        0xe0, 0xe3,
        0x50, 0x01,
        0x00, 0x16,
        0x00, 0x00,

        /* TWAMP */
        0x00, 0x00, 0x01, 0x30,
        0xe8, 0x00, 0x00, 0x01,
        0x12, 0x34, 0x56, 0x78,
        0x80, 0x01
    };

    PacketSender packet_sender = {
        .send = fake_send_fail,
        .context = NULL
    };

    NetAgentStats stats;
    stats_init(&stats);

    NetAgentConfig config = {
        .interface_name = "test0",
        .twamp_port = 20481,
        .log_level = LOG_LEVEL_ERROR
    };

    NtpTimestamp t2 = {
        .seconds = 1,
        .fraction = 0
    };

    int result = process_packet(
        packet,
        sizeof(packet),
        &t2,
        &packet_sender,
        &stats,
        &config
    );

    if (result == 0) {
        fprintf(stderr, "Expected TX failure\n");
        return 1;
    }

    if (stats.twamp_received != 1) {
        fprintf(stderr, "Expected TWAMP RX counter to be 1\n");
        return 1;
    }

    if (stats.twamp_sent != 0) {
        fprintf(stderr, "TWAMP TX counter should remain 0\n");
        return 1;
    }

    if (stats.tx_errors != 1) {
        fprintf(stderr, "Expected TX error counter to be 1\n");
        return 1;
    }

    return 0;
}

int main(void)
{
    if (test_twamp_packet_flow() != 0)
        return 1;

    if (test_wrong_udp_port_no_response() != 0)
        return 1;
    
    if (test_truncated_packet_fails() != 0)
        return 1;

    if (test_tx_failure_updates_stats() != 0)
        return 1;
    
    puts("test_packet_flow: PASS");
    return 0;
}