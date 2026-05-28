#ifndef VMSIM_H
#define VMSIM_H

#include <stddef.h>
#include <stdbool.h>

typedef enum { MODE_BB, MODE_SEG } sim_mode_t;

typedef struct {
    sim_mode_t mode;
    const char *trace_path;
    const char *config_path;
    long base;   // for BB
    long limit;  // for BB
} sim_opts_t;

typedef struct {
    unsigned long accesses;
    unsigned long ok;
    unsigned long faults_bounds;
    unsigned long faults_prot;
    unsigned long faults_noseg;
} stats_t;

typedef struct {
    char name[32];
    long base;
    long limit;
    char perms[4]; // subset of r w x, up to 3 plus '\0'
    bool in_use;
    unsigned long hits; // for summary
} segment_t;

typedef struct {
    segment_t segs[16];
    size_t nsegs;
} seg_table_t;

// CLI
bool parse_args(int argc, char **argv, sim_opts_t *o);


// Run modes
int run_bb(const sim_opts_t *o, stats_t *st);
int run_seg(const sim_opts_t *o, stats_t *st);

#endif // VMSIM_H
