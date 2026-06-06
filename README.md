# CMPT 360, Assignment 3 - Address Translation in Practice
# Hannah Yabut and Ismael Robleh 
# Date: June 7, 2026 


# Academic Integrity 
" I certify that this submission represents entirely my own work."

# Overview 
This assignment simulates Base and Bounds check for address translations by treating each process as one contiguous virtual space and ensuring the virtual address is within range. As well as Segmentation by splitting address spaces into named regions (Code, Heap, and Stack) and ensure valid access for each access given their parameters (base, limit, and perms). Finally displaying a detailed summary of all of each access reporting if successful or a failure.

# Build 
To compile the script use: "make all" 

# Run 
To check memory leaks use: "make valgrind" or "make vg" 
Example Runs: 
BB: ./vmsim --mode=bb --base=4096 --limit=64 --trace=tests/bb/t1.txt
Segmentation: ./vmsim --mode=seg --config=tests/seg/three-seg.ini --trace=tests/seg/t2.txt

# Solution Logic 
- Followed the skeleton outline of the file vmsim.c for the main functions used in the program 
- Parses commands given through terminal to ensure correctness
For BB implementation:
- Read workload file and validate each header (perms and virtual address)
- Ignores comments and empty lines
- Check whether virtual address is within bounds
- Display summary of valid accesses, invalid accesses, and out of bounds faults
For Segmentation implementation:
- Read workload file for config table and validate each header (segment name, base, limit, perms)
- Ignore comments and empty line
- Dynamically resizes arrays using realloc() 
- Store all segment entries in a table for later use
- Read workload file for trace table and validate each header (operation, seg offset)
- Ignores comments and empty lines 
- Iterate through all of the config segment entries and check if the trace segments are valid (correct perms and within bounds) given the corresponding segment type (stack vs non-stack)
- Display summary of valid accesses, missing segments, fault protections, and out of bounds

# Status
The program works as intended throughout all the different test cases 

# Assumptions / Notes 
- The BB and Segmentation files entries should not exceed 255 lines

