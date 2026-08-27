#include <stdio.h>
#include <time.h>

#include "netagent/twamp_reflector.h"
#include "netagent/log.h"

int twamp_reflector_handle_packet(
    const uint8_t *payload,
    size_t payload_length,
    const UdpEndpoint *sender,
    const PacketSender *packet_sender,
    uint8_t sender_ttl,
    const NtpTimestamp *receive_timestamp,
    NetAgentStats *stats)
{
    if (sender == NULL ||
        packet_sender == NULL ||
        packet_sender->send == NULL ||
        stats == NULL) {
        log_error("Invalid input to twamp_reflector_handle_packet");
        return -1;
    }

    uint8_t tx_buffer[TWAMP_REFLECTOR_FIXED_SIZE];
    uint32_t sender_sequence;

    int result = twamp_reflector_build_response(
        payload,
        payload_length,
        sender_ttl,
        receive_timestamp,
        tx_buffer,
        sizeof(tx_buffer),
        &sender_sequence
    );

    if (result != 0) {
        stats->parse_errors++;
        return result;
    }

    stats->twamp_received++;

    stats_track_sequence(
        stats,
        sender_sequence
    );

    result = packet_sender->send(
        packet_sender->context,
        sender,
        tx_buffer,
        sizeof(tx_buffer)
    );

    if (result != 0) {
        stats->tx_errors++;
        return result;
    }

    stats->twamp_sent++;

    return 0;
}


int twamp_reflector_build_response(
    const uint8_t *payload,
    size_t payload_length,
    uint8_t sender_ttl,
    const NtpTimestamp *receive_timestamp,
    uint8_t *tx_buffer,
    size_t tx_buffer_length,
    uint32_t *sender_sequence)
{
    if (payload == NULL ||
        receive_timestamp == NULL ||
        tx_buffer == NULL ||
        sender_sequence == NULL) {

        log_error("Invalid input to twamp_reflector_build_response");
        return -1;
    }

    TWAMPSenderPacket sender_packet;
    TWAMPReflectorPacket response;

    int result = parse_twamp_sender(
        payload,
        payload_length,
        &sender_packet
    );

    if (result != 0) {
        return result;
    }

    *sender_sequence =
        sender_packet.sequence_number;

    response.sender_ttl =
        sender_ttl;

    result = build_twamp_reflector_response(
        &sender_packet,
        receive_timestamp,
        &response
    );

    if (result != 0) {
        return result;
    }

    NtpTimestamp t3;

    if (ntp_timestamp_now(&t3) != 0) {
        log_error("Failed to get current NTP timestamp");
        return -1;
    }

    response.timestamp_seconds =
        t3.seconds;

    response.timestamp_fraction =
        t3.fraction;

    return serialize_twamp_reflector(
        &response,
        tx_buffer,
        tx_buffer_length
    );
}