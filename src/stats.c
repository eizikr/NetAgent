#include "netagent/stats.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

void stats_init(NetAgentStats *stats)
{
    if (stats == NULL) {
        return;
    }

    memset(stats, 0, sizeof(*stats));
}

void stats_print(const NetAgentStats *stats)
{
    if (stats == NULL) {
        return;
    }

    puts("\nNetAgent Statistics");
    puts("-------------------");

    printf("Packets received:       %" PRIu64 "\n",
           stats->packets_received);

    printf("TWAMP received:         %" PRIu64 "\n",
           stats->twamp_received);

    printf("TWAMP responses sent:   %" PRIu64 "\n",
           stats->twamp_sent);

    printf("Parse errors:           %" PRIu64 "\n",
           stats->parse_errors);

    printf("TX errors:              %" PRIu64 "\n",
           stats->tx_errors);

    printf("Last sender sequence:   %" PRIu32 "\n",
           stats->last_sender_sequence);

    printf("Sequence gaps:          %" PRIu64 "\n",
        stats->sequence_gaps);

    printf("Duplicates:             %" PRIu64 "\n",
        stats->duplicates);

    printf("Out of order:           %" PRIu64 "\n",
        stats->out_of_order);
}

void stats_track_sequence(
    NetAgentStats *stats,
    uint32_t sequence)
{
    if (stats == NULL) {
        return;
    }

    if (!stats->has_last_sequence) {
        stats->last_sender_sequence = sequence;
        stats->has_last_sequence = 1;
        return;
    }

    uint32_t expected =
        stats->last_sender_sequence + 1;

    if (sequence == expected) {
        /* normal packet */
    }
    else if (sequence == stats->last_sender_sequence) {
        stats->duplicates++;
    }
    else if (sequence > expected) {
        stats->sequence_gaps +=
            (uint64_t)(sequence - expected);
    }
    else {
        stats->out_of_order++;
    }

    /*
     * Only advance the last sequence if this packet
     * is newer than the previous one.
     */
    if (sequence > stats->last_sender_sequence) {
        stats->last_sender_sequence = sequence;
    }
}