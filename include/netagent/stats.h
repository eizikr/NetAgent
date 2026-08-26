#ifndef NETAGENT_STATS_H
#define NETAGENT_STATS_H

#include <stdint.h>

typedef struct {
    uint64_t packets_received;

    uint64_t twamp_received;
    uint64_t twamp_sent;

    uint64_t parse_errors;
    uint64_t tx_errors;

    uint64_t sequence_gaps;
    uint64_t duplicates;
    uint64_t out_of_order;

    uint32_t last_sender_sequence;

    int has_last_sequence;
} NetAgentStats;

void stats_init(NetAgentStats *stats);
void stats_print(const NetAgentStats *stats);
void stats_track_sequence(NetAgentStats *stats, uint32_t sequence);

#endif