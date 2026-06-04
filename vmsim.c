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
Description: checks if a string is valid (non-negative decimal int) and converts it to long 
Parameters: s, out 
Return: 0 or 1 
*/
// avoids accepting invalid input or translating bad addresses 
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
        return 2; 
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
    line[strcspn(line, "\n")] = '\0'; // removes newline chars 
}

/*
Description:
Parameters:
Return:
*/
bool is_perms_valid(char* perms, int length) {
    for (int i = 0; i < length; i++)
    {
        perms[i] = tolower((unsigned char)perms[i]);
        if (strlen(perms) > 3 || (perms[i] != 'r' && perms[i] != 'w' && perms[i] != 'x')) // checks that operation is R or W or X
        {
            return false;
        }
    }

    return true;
}

/*
Description:
Parameters:
Return:
*/
void display_stats_summary(segment_t* segments, stats_t st, int num_entries) 
{
    printf("== stats ==\n");
    long code_hits = 0, heap_hits = 0, stack_hits = 0;
    for (int i = 0; i < num_entries; i++)
    {
        if (strcmp(segments[i].name, "code") == 0)
        {
            code_hits += segments[i].hits;
        }
        else if (strcmp(segments[i].name, "heap") == 0)
        {
            heap_hits += segments[i].hits;
        }
        else
        {
            stack_hits += segments[i].hits;
        }
    }
    printf("accesses=%ld, ok=%ld, faults.bounds=%ld\nfaults.prot=%ld, faults.noseg=%ld\n", st.accesses, st.ok, st.faults_bounds, st.faults_prot, st.faults_noseg);
    printf("seg_hits:");
    if (code_hits > 0)
    {
        printf(" code=%ld", code_hits);
    }
    if (heap_hits > 0)
    {
        printf(" heap=%ld", heap_hits);
    }
    if (stack_hits > 0)
    {
        printf(" stack=%ld", stack_hits);
    }
    printf("\n");
}

// CLI
/*
Description: validates CLI inputs 
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
            else if (strcmp(val, "seg") == 0 && seen_mode != 1) // check for seg mode 
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
    int num_line = 0; // tracks line num 
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
        long va; // store converted address 
        if (!parse_uint(addr, &va))  // converts address str to num, if not num 
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
        printf("No entries in file for BB implementation!");
        return 1;
    }
    printf("== stats == \n");
    printf("accesses=%lu, ok=%lu, faults.bounds=%lu\n", st->accesses, st->ok, st->faults_bounds);  // prints final simulation 
    return 0; 
}

//temp seg for milestone 1, reads and echoes config and trace files 
/*
Description: 
Parameters: 
Return: 
*/
int run_seg(const sim_opts_t *o, stats_t *st) 
{
   (void)st;

    FILE *config = fopen(o->config_path, "r"); // read config file 
    if (config == NULL)
    {
        fprintf(stderr, "Error: cannot open config file %s\n", o->config_path);
        return 1;
    }

    char line[256]; // store each line read 
    int num_line = 0; // tracks line num 
    int capacity = 16;
    int num_entries = 0;

    segment_t* segments = (segment_t*)malloc(capacity * sizeof(segment_t));
    if (segments == NULL)
    {
        fprintf(stderr, "Memory allocation failed in seg!");
        fclose(config);
        return 1;
    }

    while (fgets(line, sizeof(line), config) != NULL)
    {
        num_line++;
        clean_line(line); // remove nl and anything after #

        char segment_name[32];
        char base[32];
        char limit[32];
        char perms[4]; 
        char extra[32]; // store extra input if line has too many 

        int count = sscanf(line, "%31s %31s %31s %3s %31s", segment_name, base, limit, perms, extra);

        if (count == EOF || count == 0 || line[0] == '\0')
        {
            continue; // skips blank line or comment line 
        }

        if (count != 4)
        {
            fprintf(stderr, "config: %s:%d: wrong shape: expected \"SEG BASE LIMIT PERMS\"\n", o->config_path, num_line);
            continue;
        }

        if (strcmp(segment_name, "code") != 0 && strcmp(segment_name, "heap") != 0 && strcmp(segment_name, "stack") != 0)
        {
            fprintf(stderr, "config: %s:%d: incorrect segment names: expected \"code or heap or stack\"\n", o->config_path, num_line);
            continue;
        }

        long _base;
        if (!parse_uint(base, &_base))
        {
            fprintf(stderr, "config: %s:%d: bad base \"%s\" (not decimal)\n", o->config_path, num_line, base);
            continue; // skip invalid visual address 
        }

        long _limit;
        if (!parse_uint(limit, &_limit))
        {
            fprintf(stderr, "config: %s:%d: bad limit \"%s\" (not decimal)\n", o->config_path, num_line, limit);
            continue; // skip invalid visual address 
        }
        int length = strlen(perms);
        if (!is_perms_valid(perms, length))
        {
            fprintf(stderr, "config: %s:%d: invalid perms: perms must be a combination of R/W/X, got \"%s\"\n", o->config_path, num_line, perms);
            continue;
        }

        if (num_entries > capacity)
        {
            capacity *= 2;
            // used size of pointer instead of struct 
            segment_t* new_segments = (segment_t*)realloc(segments, capacity * sizeof(segment_t));

            if (new_segments == NULL)
            {
                fprintf(stderr, "Memory reallocation failed in RUN_SEG!");
                free(segments);
                fclose(config);
                return 1;
            }
            segments = new_segments;
        }

        //populate config entries into segment struct
        strcpy(segments[num_entries].name, segment_name);
        segments[num_entries].base = _base;
        segments[num_entries].limit = _limit;
        strcpy(segments[num_entries].perms, perms);
        segments->hits = 0;
        num_entries++;
    }

    fclose(config);
    FILE *trace = fopen(o->trace_path, "r"); // open trace file 
    if (trace == NULL)
    {
        fprintf(stderr, "Error: cannot open trace file %s\n", o->trace_path);
        return 1;
    }

    num_line = 0;
    while (fgets(line, sizeof(line), trace) != NULL) // read trace file 
    {
        num_line++;
        clean_line(line); // remove nl and anything after #

        char op_str[4];
        char segment_name[32];
        char offset[32];
        char extra[32]; // store extra input if line has too many 

        int count = sscanf(line, "%3s %31s %31s %31s", op_str, segment_name, offset, extra);

        if (count == EOF || count == 0 || line[0] == '\0')
        {
            continue; // skips blank line or comment line 
        }

        if (count != 3)
        {
            fprintf(stderr, "trace: %s:%d: wrong shape: expected \"OP SEG PERMS\"\n", o->trace_path, num_line);
            continue;
        }

        if (strlen(op_str) != 1 || (op_str[0] != 'R' && op_str[0] != 'W' && op_str[0] != 'X')) // checks that operation is R or W or X
        {
            fprintf(stderr, "trace: %s:%d: malformed: op must be R/W/X, got \"%s\"\n", o->trace_path, num_line, op_str);
            continue; // skip invalid operation 
        }

        if (strcmp(segment_name, "code") != 0 && strcmp(segment_name, "heap") != 0 && strcmp(segment_name, "stack") != 0)
        {
            fprintf(stderr, "trace: %s:%d: incorrect segment names: expected \"code or heap or stack\"\n", o->trace_path, num_line);
            continue;
        }

        long _offset;
        int parse_uint_return = parse_uint(offset, &_offset);
        if (parse_uint_return == 0)
        {
            fprintf(stderr, "trace: %s:%d: bad offset \"%s\" (not decimal)\n", o->trace_path, num_line, offset);
            continue;
        }
        else if (parse_uint_return == 2)
        {
            fprintf(stderr, "trace: %s:%d: bad offset \"%s\" (non-negative raw offset)\n", o->trace_path, num_line, offset);
            continue;
        }

        bool no_seg = true;
        for (int i = 0; i < num_entries; i++)
        {
            if (strcmp(segment_name, segments[i].name) == 0)
            {
                st->accesses++;
                no_seg = false;
                bool has_perms = false;
                for (int j = 0; j < 3; j++)
                {
                    if (op_str[0] == toupper(segments[i].perms[j]))
                    {
                        has_perms = true;
                        break;
                    }
                }
                if (strcmp(segment_name, "stack") == 0)
                {
                    long offset_signed = _offset - segments[i].limit;
                    long physical_address = segments[i].base + offset_signed;
                    if ((offset_signed < 0 && offset_signed >= -segments[i].limit) && has_perms)
                    {
                        segments[i].hits++;
                        st->ok++;
                        printf("%-10s -> PA %ld ; ok\n", line, physical_address);
                    }
                    else if (!has_perms)
                    {
                        st->faults_prot++;
                        printf("%-10s -> fault: PROTECTION (needed '%c', have '%s')\n", line, tolower(op_str[0]), segments[i].perms); //will need to show what perms was needed
                    }
                    else if (offset_signed >= 0 || offset_signed < -segments[i].limit)
                    {
                        st->faults_bounds++;
                        printf("%-10s -> fault: BOUNDS\n", line);
                    }
                }
                else
                {
                    long physical_address = segments[i].base + _offset;
                    if (_offset < segments[i].limit && has_perms)
                    {
                        segments[i].hits++;
                        st->ok++;
                        printf("%-10s -> PA %ld ; ok\n", line, physical_address);
                    }
                    else if (!has_perms)
                    {
                        st->faults_prot++;
                        printf("%-10s -> fault: PROTECTION (needed '%c', have '%s')\n", line, tolower(op_str[0]), segments[i].perms); //will need to show what perms was needed
                    }
                    else if (_offset >= segments[i].limit)
                    {
                        st->faults_bounds++;
                        printf("%-10s -> fault: BOUNDS\n", line);
                    }
                }
            }
        }
        if (no_seg)
        {
            st->faults_noseg++;
            printf("%-10s -> fault: NOSEG\n", line);
        }
    }
    display_stats_summary(segments, *st, num_entries);
    free(segments);
    fclose(trace); // close trace file 

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

