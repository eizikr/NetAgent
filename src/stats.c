#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "netagent/stats.h"
#include "netagent/log.h"

void stats_init(NetAgentStats *stats)
{
    if (stats == NULL) {
        log_error("Invalid input to stats_init\n");
        return;
    }

    memset(stats, 0, sizeof(*stats));
}

void stats_print(const NetAgentStats *stats)
{
    if (stats == NULL) {
        return;
    }

    log_info("NetAgent Statistics");
    log_info("-------------------");

    log_info("Packets received:       %" PRIu64 "\n",
           stats->packets_received);

    log_info("TWAMP received:         %" PRIu64 "\n",
           stats->twamp_received);

    log_info("TWAMP responses sent:   %" PRIu64 "\n",
           stats->twamp_sent);

    log_info("Parse errors:           %" PRIu64 "\n",
           stats->parse_errors);

    log_info("TX errors:              %" PRIu64 "\n",
           stats->tx_errors);

    log_info("Last sender sequence:   %" PRIu32 "\n",
           stats->last_sender_sequence);

    log_info("Sequence gaps:          %" PRIu64 "\n",
        stats->sequence_gaps);

    log_info("Duplicates:             %" PRIu64 "\n",
        stats->duplicates);

    log_info("Out of order:           %" PRIu64 "\n",
        stats->out_of_order);
}

void stats_track_sequence(
    NetAgentStats *stats,
    uint32_t sequence)
{
    if (stats == NULL) {
        log_error("Invalid input to stats_track_sequence\n");
        return;
    }

    if (!stats->has_last_sequence) {
        stats->last_sender_sequence = sequence;
        stats->has_last_sequence = 1;
        return;
    }

    uint32_t last = stats->last_sender_sequence;
    uint32_t expected = last + 1;

    if (sequence == expected) {
        /*
         * Normal next packet.
         *
         * uint32_t arithmetic intentionally handles:
         * 0xFFFFFFFF + 1 -> 0
         */
        stats->last_sender_sequence = sequence;
        return;
    }

    if (sequence == last) {
        stats->duplicates++;
        return;
    }

    /*
     * Signed modular difference.
     *
     * Positive  -> sequence is ahead of last.
     * Negative  -> sequence is behind last.
     */
    int32_t difference =
        (int32_t)(sequence - last);

    if (difference > 0) {
        stats->sequence_gaps +=
            (uint64_t)(difference - 1);

        stats->last_sender_sequence = sequence;
    }
    else {
        stats->out_of_order++;
    }
}