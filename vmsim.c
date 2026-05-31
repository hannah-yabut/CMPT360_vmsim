/* 
 Student Name: Hannah Yabut and Ismael Robleh 
 Student ID: 3131432 
 Submission Date: June 7, 2026 
 File: vmsim.c 
*/

#include "vmsim.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

// usage
/*
Description: 
Parameters: 
Return: 
*/
static void usage(const char *prog) {
    fprintf(stderr,
        "Usage:\n"
        "  %s --mode=bb  --base=N --limit=N --trace=FILE \n"
        "  %s --mode=seg --config=FILE --trace=FILE \n",
        prog, prog);
}

/*
Description: checks if a string is valid (non-negative decimal int) and converts it to int 
Parameters: s, out 
Return: 0 or 1 
*/
static int parse_uint(const char *s, long *out)
{
    if (s == NULL || *s == '\0')
    {
        return 0;
    }
    for (int i = 0; s[i] != '\0'; i++)
    {
        if (!isdigit((unsigned char) s[i])) 
        {
            return 0; 
        }
    }
    char *end = NULL; 
    errno = 0; 
    long val = strtol(s, &end, 10); 

    if (errno != 0 || *end != '\0' || val < 0) 
    {
        return 0; 
    }
    *out = val; 
    return 1; 
}

/*
Description: 
Parameters: 
Return: 
*/
static void clean_line(char *line) 
{
    char *hash = strchr(line, '#');
    if (hash) 
    {
        *hash = '\0';
    }
    line[strcspn(line, "\n")] = '\0'; 
}

// CLI
/*
Description: 
Parameters: 
Return: 
*/
bool parse_args(int argc, char **argv, sim_opts_t *o) 
{
    if (o == NULL)
    {
        return 0;
    }
    memset(o, 0, sizeof(*o)); 

    // set flags to false first 
    bool seen_mode = 0; 
    bool seen_base = 0; 
    bool seen_limit = 0; 
    bool seen_trace = 0; 
    bool seen_config = 0; 

    for (int i = 1; i < argc; i++) 
    {
        if (strncmp(argv[i], "--mode=", 7) == 0) 
        {
            const char *val = argv[i] + 7; 
            if (strcmp(val, "bb") == 0 && seen_mode != 1)
            {
                o->mode = MODE_BB;
            }
            else if (strcmp(val, "seg") == 0 && seen_mode != 1)
            {
                o->mode = MODE_SEG; 
            }
            else
            {
                fprintf(stderr, "Error: mode must be bb or seg\n");
                return 0; 
            }
            seen_mode = 1; // set flag to true after 
        }
        else if (strncmp(argv[i], "--base=", 7)== 0)
        {
            if (!parse_uint(argv[i] + 7, &o->base))
            {
                fprintf(stderr, "Error: base must be a decimal\n");
                return 0; 
            }
            seen_base = 1; 
        }
        else if (strncmp(argv[i], "--limit=", 8)== 0)
        {
            if (!parse_uint(argv[i] + 8, &o->limit))
            {
                fprintf(stderr, "Error: base must be a decimal\n");
                return 0; 
            }
            seen_limit = 1; 
        }
         else if (strncmp(argv[i], "--trace=", 8)== 0)
         {
            o->trace_path = argv[i] + 8; 
            seen_trace = 1;
         }
         else if (strncmp(argv[i], "--config=", 9)== 0)
         {
            o->config_path = argv[i] + 9; 
            seen_config = 1; 
         }
         else
         {
            fprintf(stderr, "Error: invalid option %s\n", argv[i]);
            return 0; 
         }
    }

    if (!seen_mode || !seen_trace) 
    {
        fprintf(stderr, "Error: missing --mode or --trace\n");
        return 0; 
    }
    if (o->mode == MODE_BB)
    {
        if (!seen_base || !seen_limit)
        {
            fprintf(stderr, "Error: bb mode missing --base and --limit\n");
            return 0; 
        }
        if (seen_config)
        {
            fprintf(stderr, "Error: bb mode does not use --config\n");
            return 0;
        }
    }
    if (o->mode == MODE_SEG)
    {
        if (!seen_config)
        {
            fprintf(stderr, "Error: seg mode missing --config\n");
            return 0;
        }
        if (seen_base || seen_limit)
        {
            fprintf(stderr, "Error: seg mode does not use --base or --limit\n");
            return 0;
        }
    }
    return 1; 
}

//bb
/*
Description: 
Parameters: 
Return: 
*/
int run_bb(const sim_opts_t *o, stats_t *st) 
{
    FILE *file = fopen(o->trace_path, "r");
    if (file == NULL)
    {
        fprintf(stderr, "Error cannot open file %s\n", o->trace_path);
        return 1; 
    }
    char line[256];
    int num_line = -1; 
    while (fgets(line, sizeof(line), file)!= NULL)
    {
        num_line++; 
        clean_line(line);
        char op_str[32];
        char addr[32];
        char extra[32];
        int count = sscanf(line, "%31s %31s %31s", op_str, addr, extra);

        if (count == EOF || count == 0) 
        {
            continue; // blank line or comment line 
        }
        if (count != 2)
        {
            fprintf(stderr, "trace: %s:%d: malformed: expected \"OP ADDR\"\n", o->trace_path, num_line);
            continue;
        }
        if (strlen(op_str) != 1 || (op_str[0] != 'R' && op_str[0]!= 'W'))
        {
            fprintf(stderr, "trace: %s:%d: malformed: op must be R/W, got \"%s\"\n", o->trace_path, num_line, op_str);
            continue; 
        }
        long va; 
        if (!parse_uint(addr, &va))
        {
            fprintf(stderr, "trace: %s:%d: bad address \"%s\" (not decimal)\n", o->trace_path, num_line, addr);
            continue;
        }
        st->accesses++; // increment accesses count 
        if (va >= 0 && va < o->limit)
        {
            long pa = o->base + va; 
            st->ok++; 
            printf("%c %ld -> PA %ld ; ok\n", op_str[0], va, pa);
        }
        else 
        {
            st->faults_bounds++; 
            printf("%c %ld -> fault: BOUNDS\n", op_str[0], va);
        }
    }
    fclose(file); 
    printf("== stats == \n");
    printf("accesses=%lu, ok=%lu, faults.bounds=%lu\n", st->accesses, st->ok, st->faults_bounds); 
    return 0; 
}

//seg
/*
Description: 
Parameters: 
Return: 
*/
int run_seg(const sim_opts_t *o, stats_t *st) {
    (void)o; (void)st;
    fprintf(stderr, "TODO: run_seg()\n");
    return 0;
}

//main()
/*
Description: 
Parameters: 
Return: 
*/
int main(int argc, char **argv) {
    sim_opts_t opts;
    if (!parse_args(argc, argv, &opts)) { usage(argv[0]); return 1; }
    stats_t st = (stats_t){0};
    if (opts.mode == MODE_BB) return run_bb(&opts, &st);
    else return run_seg(&opts, &st);
}
