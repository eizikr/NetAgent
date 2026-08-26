#include <stdio.h>

#include "netagent/stats.h"

static int test_first_sequence(void);
static int test_normal_sequence(void);
static int test_sequence_gap(void);
static int test_duplicate(void);
static int test_out_of_order(void);
static int test_sequence_wraparound(void);

int main(void)
{
    if (test_first_sequence() != 0)
        return 1;

    if (test_normal_sequence() != 0)
        return 1;

    if (test_sequence_gap() != 0)
        return 1;

    if (test_duplicate() != 0)
        return 1;

    if (test_out_of_order() != 0)
        return 1;

    if (test_sequence_wraparound() != 0)
        return 1;

    puts("test_stats: PASS");
    return 0;
}

static int test_sequence_wraparound(void)
{
    NetAgentStats stats;
    stats_init(&stats);

    stats_track_sequence(&stats, 0xFFFFFFFE);
    stats_track_sequence(&stats, 0xFFFFFFFF);
    stats_track_sequence(&stats, 0);
    stats_track_sequence(&stats, 1);

    if (stats.sequence_gaps != 0) {
        fprintf(stderr,
                "Wrap-around incorrectly detected a gap\n");
        return 1;
    }

    if (stats.duplicates != 0) {
        fprintf(stderr,
                "Wrap-around incorrectly detected a duplicate\n");
        return 1;
    }

    if (stats.out_of_order != 0) {
        fprintf(stderr,
                "Wrap-around incorrectly detected out-of-order\n");
        return 1;
    }

    if (stats.last_sender_sequence != 1) {
        fprintf(stderr,
                "Expected last sequence 1 after wrap-around\n");
        return 1;
    }

    return 0;
}

static int test_first_sequence(void)
{
    NetAgentStats stats;
    stats_init(&stats);

    stats_track_sequence(&stats, 100);

    if (!stats.has_last_sequence) {
        fprintf(stderr, "First sequence was not initialized\n");
        return 1;
    }

    if (stats.last_sender_sequence != 100) {
        fprintf(stderr, "Expected last sequence 100\n");
        return 1;
    }

    return 0;
}


static int test_normal_sequence(void)
{
    NetAgentStats stats;
    stats_init(&stats);

    stats_track_sequence(&stats, 100);
    stats_track_sequence(&stats, 101);
    stats_track_sequence(&stats, 102);

    if (stats.sequence_gaps != 0 ||
        stats.duplicates != 0 ||
        stats.out_of_order != 0) {

        fprintf(stderr, "Normal sequence incorrectly classified\n");
        return 1;
    }

    if (stats.last_sender_sequence != 102) {
        fprintf(stderr, "Expected last sequence 102\n");
        return 1;
    }

    return 0;
}


static int test_sequence_gap(void)
{
    NetAgentStats stats;
    stats_init(&stats);

    stats_track_sequence(&stats, 100);
    stats_track_sequence(&stats, 103);

    /*
     * Expected:
     * 100, [101, 102], 103
     *
     * Two sequence numbers are missing.
     */
    if (stats.sequence_gaps != 2) {
        fprintf(
            stderr,
            "Expected 2 missing sequences, got %llu\n",
            (unsigned long long)stats.sequence_gaps
        );
        return 1;
    }

    return 0;
}


static int test_duplicate(void)
{
    NetAgentStats stats;
    stats_init(&stats);

    stats_track_sequence(&stats, 100);
    stats_track_sequence(&stats, 100);

    if (stats.duplicates != 1) {
        fprintf(stderr, "Expected one duplicate\n");
        return 1;
    }

    return 0;
}


static int test_out_of_order(void)
{
    NetAgentStats stats;
    stats_init(&stats);

    stats_track_sequence(&stats, 100);
    stats_track_sequence(&stats, 102);
    stats_track_sequence(&stats, 101);

    if (stats.out_of_order != 1) {
        fprintf(stderr, "Expected one out-of-order packet\n");
        return 1;
    }

    /*
     * 101 arrived late, so it must not move our
     * highest observed sequence backwards.
     */
    if (stats.last_sender_sequence != 102) {
        fprintf(stderr, "Last sequence moved backwards\n");
        return 1;
    }

    return 0;
}