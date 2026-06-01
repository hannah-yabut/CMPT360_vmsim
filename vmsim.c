/* 
 Student Name: Hannah Yabut and Ismael Robleh 
 Student ID: 3131432 & 3149556
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
    if (s == NULL || *s == '\0')  // checks if string is misisng or empty 
    {
        return 0;
    }
    for (int i = 0; s[i] != '\0'; i++) // loops through each char in str till null char 
    {
        if (!isdigit((unsigned char) s[i])) 
        {
            return 0; // if chars are not digits return false 
        }
    }
    char *end = NULL; // points to end of strtol 
    errno = 0; // resets errno before converting 
    long val = strtol(s, &end, 10); // str -> long 

    if (errno != 0 || *end != '\0' || val < 0) // checks for errors or negatives
    {
        return 0; 
    }
    *out = val; // stores converted val to output variable 
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
    if (hash) // if comment symbol present
    {
        *hash = '\0';// cuts off line at the comment 
    }
    line[strcspn(line, "\n")] = '\0'; // removes newline chars from fgets 
}

// CLI
/*
Description: 
Parameters: 
Return: 
*/
bool parse_args(int argc, char **argv, sim_opts_t *o) 
{
    if (o == NULL)  // checks opt struct exists 
    {
        return 0;
    }
    memset(o, 0, sizeof(*o)); // initalize all fields 

    // set flags to false first 
    bool seen_mode = 0; 
    bool seen_base = 0; 
    bool seen_limit = 0; 
    bool seen_trace = 0; 
    bool seen_config = 0; 

    for (int i = 1; i < argc; i++) 
    {
        if (strncmp(argv[i], "--mode=", 7) == 0) // check for mode 
        {
            const char *val = argv[i] + 7; // skips "--mode" to get actual val 
            if (strcmp(val, "bb") == 0 && seen_mode != 1) // check for bb mode 
            {
                o->mode = MODE_BB; // store bb mode 
            }
            else if (strcmp(val, "seg") == 0 && seen_mode != 1) // checl for seg mode 
            {
                o->mode = MODE_SEG; 
            }
            else
            {
                fprintf(stderr, "Error: mode must be bb or seg\n"); // if not bb or seg, prints error message 
                return 0; 
            }
            seen_mode = 1; // set flag to true after 
        }
        else if (strncmp(argv[i], "--base=", 7)== 0)
        {
            if (!parse_uint(argv[i] + 7, &o->base)) // converts value after "--base"
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
            o->trace_path = argv[i] + 8; // store trace file path 
            seen_trace = 1; // record that trace is found 
         }
         else if (strncmp(argv[i], "--config=", 9)== 0)
         {
            o->config_path = argv[i] + 9;  // store config path 
            seen_config = 1; 
         }
         else
         {
            fprintf(stderr, "Error: invalid option %s\n", argv[i]);
            return 0; 
         }
    }

    if (!seen_mode || !seen_trace) // if doesn't have mode and trace print error message 
    {
        fprintf(stderr, "Error: missing --mode or --trace\n");
        return 0; 
    }
    if (o->mode == MODE_BB)
    {
        if (!seen_base || !seen_limit)  // if bb doesn't have  base or limit 
        {
            fprintf(stderr, "Error: bb mode missing --base and --limit\n");
            return 0; 
        }
        if (seen_config) // if bb mode has config 
        {
            fprintf(stderr, "Error: bb mode does not use --config\n");
            return 0;
        }
    }
    if (o->mode == MODE_SEG)
    {
        if (!seen_config) // if doesn't have config 
        {
            fprintf(stderr, "Error: seg mode missing --config\n");
            return 0;
        }
        if (seen_base || seen_limit) // if seg mode has base or limit 
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
    FILE *file = fopen(o->trace_path, "r"); // read trace file 
    if (file == NULL) // if file invalid return error message 
    {
        fprintf(stderr, "Error cannot open file %s\n", o->trace_path);
        return 1; 
    }
    char line[256]; // store lines from trace file 
    int num_line = -1; // tracks line num 
    while (fgets(line, sizeof(line), file)!= NULL)
    {
        num_line++; // increment line number 
        clean_line(line); // remove comments and newline chars 
        char op_str[32]; // store operation str (R or W)
        char addr[32]; // store visual address as str 
        char extra[32]; // store extra input if line has too many 

        int count = sscanf(line, "%31s %31s %31s", op_str, addr, extra);

        if (count == EOF || count == 0) 
        {
            continue; // skips blank line or comment line 
        }
        if (count != 2) // if line has more than operation and adress 
        {
            fprintf(stderr, "trace: %s:%d: malformed: expected \"OP ADDR\"\n", o->trace_path, num_line);
            continue; // skip malformed line 
        }
        if (strlen(op_str) != 1 || (op_str[0] != 'R' && op_str[0]!= 'W')) // checks that operation is R or W 
        {
            fprintf(stderr, "trace: %s:%d: malformed: op must be R/W, got \"%s\"\n", o->trace_path, num_line, op_str);
            continue; // skip invalid operation 
        }
        long va; // store conerted address 
        if (!parse_uint(addr, &va))  // address str to num, if not num 
        {
            fprintf(stderr, "trace: %s:%d: bad address \"%s\" (not decimal)\n", o->trace_path, num_line, addr);
            continue; // skip invalid visual address 
        }
        st->accesses++;  // counts valid memory accesses 
        if (va >= 0 && va < o->limit) // checks if visual addr is within bounds 
        {
            long pa = o->base + va; // visual addr to physical addr 
            st->ok++; // increment successful accesses 
            printf("%c %ld -> PA %ld ; ok\n", op_str[0], va, pa);
        }
        else // if va outside limit 
        {
            st->faults_bounds++; // increment bounds fault 
            printf("%c %ld -> fault: BOUNDS\n", op_str[0], va); // print bounds fault 
        }
    }
    fclose(file); 
    if (num_line == 0)
    {
        fprintf("No entries in file for BB implementation!");
        return 1;
    }
    printf("== stats == \n");
    printf("accesses=%lu, ok=%lu, faults.bounds=%lu\n", st->accesses, st->ok, st->faults_bounds);  // prints final simulation 
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
