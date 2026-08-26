#include "netagent/twamp_reflector.h"

#include <stdio.h>
#include <time.h>

int twamp_reflector_handle_packet(
    const uint8_t *payload,
    size_t payload_length,
    const UdpEndpoint *sender,
    uint8_t sender_ttl,
    const NtpTimestamp *receive_timestamp,
    const UdpTxSocket *tx,
    NetAgentStats *stats)
{
    TWAMPSenderPacket sender_packet;
    TWAMPReflectorPacket response;
    uint8_t tx_buffer[TWAMP_REFLECTOR_FIXED_SIZE];

    int result = parse_twamp_sender(
        payload,
        payload_length,
        &sender_packet
    );

    if (result != 0) {
        fprintf(stderr,
                "Failed to parse TWAMP sender packet\n");

        stats->parse_errors++;
        return result;
    }

    stats->twamp_received++;

    stats_track_sequence(
        stats,
        sender_packet.sequence_number
    );

    response.sender_ttl = sender_ttl;

    result = build_twamp_reflector_response(
        &sender_packet,
        receive_timestamp,
        &response
    );

    if (result != 0) {
        fprintf(stderr,
                "Failed to build TWAMP reflector response\n");
        return result;
    }

    NtpTimestamp t3;

    if (ntp_timestamp_now(&t3) != 0) {
        return -1;
    }

    response.timestamp_seconds = t3.seconds;
    response.timestamp_fraction = t3.fraction;

    result = serialize_twamp_reflector(
        &response,
        tx_buffer,
        sizeof(tx_buffer)
    );

    if (result != 0) {
        return result;
    }

    result = udp_tx_send(
        tx,
        sender,
        tx_buffer,
        sizeof(tx_buffer)
    );

    if (result != 0) {
        stats->tx_errors++;
        return result;
    }

    stats->twamp_sent++;

    /*
     * TX (Transmit) software timestamp diagnostics.
     * This timestamp is measured after transmission and
     * is not embedded into the TWAMP response.
     */
    struct timespec kernel_tx_timestamp;

    if (udp_tx_read_timestamp(
            tx,
            &kernel_tx_timestamp) != 0) {

        fprintf(stderr,
                "Failed to get kernel TX timestamp\n");
        return -1;
    }

    NtpTimestamp kernel_tx_ntp;

    if (timespec_to_ntp(
            &kernel_tx_timestamp,
            &kernel_tx_ntp) != 0) {

        fprintf(stderr,
                "Failed to convert TX timestamp to NTP\n");
        return -1;
    }

    uint64_t embedded =
        ((uint64_t)response.timestamp_seconds << 32) |
        response.timestamp_fraction;

    uint64_t kernel_tx =
        ((uint64_t)kernel_tx_ntp.seconds << 32) |
        kernel_tx_ntp.fraction;

    uint64_t delta_ntp = kernel_tx - embedded;

    double delta_us =
        ((double)delta_ntp * 1000000.0) /
        4294967296.0;

    printf(
        "Embedded T3:  %u.%08x\n",
        response.timestamp_seconds,
        response.timestamp_fraction
    );

    printf(
        "Kernel TX T3: %u.%08x\n",
        kernel_tx_ntp.seconds,
        kernel_tx_ntp.fraction
    );

    printf(
        "T3 delta:     %.3f us\n",
        delta_us
    );

    return 0;
}