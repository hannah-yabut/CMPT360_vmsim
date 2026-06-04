#Name: Hannah Yabut & Ismael Robleh
#Student ID: 3131432 & 3149556
#May 10, 2026 
#Compile, run, and test vsim program for memory leaks or any potential errors

#Compiler 
CC := gcc-13

#Compiler flags
CFLAGS := -std=c11 -Wall -Wextra -pedantic -O2 -g

#Valgrind command
VALGRIND ?= valgrind

#Valgrind flags for detailed and thorough memory leak detection
VGFLAGS  ?= --leak-check=full --show-leak-kinds=all --track-origins=yes --errors-for-leak-kinds=all --error-exitcode=1

#Declare the following targets as makefile commands instead of file targets
.PHONY: all clean valgrind_BB valgrind_SEG vg run_BB run_SEG

#Build userclean executable
all: vmsim

#Link object file to into executable
vmsim: vmsim.o 
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

#Compile source file into object file
vmsim.o: vmsim.c vmsim.h
	$(CC) $(CFLAGS) -c $<

MODE ?= BB
TRACE	?= tests/bb/t1.txt

#BB mode variables
BASE	?= 4096
LIMIT	?= 64

#SEG mode variables
CONFIG	?= tests/seg/three-seg.ini

#Run executable automatically
run_BB: vmsim
	./vmsim --mode=bb --base=$(BASE) --limit=$(LIMIT) --trace=tests/bb/$(TRACE)

run_SEG: vmsim
	./vmsim --mode=seg --config=tests/seg/$(CONFIG) --trace=tests/seg/$(TRACE)

#Run memory leak and error detection with Valgrind by using workload.txt file as input
valgrind_BB: vmsim
	$(VALGRIND) $(VGFLAGS) ./vmsim --mode=bb --base=$(BASE) --limit=$(LIMIT) --trace=$(TRACE)

valgrind_SEG: vmsim
	$(VALGRIND) $(VGFLAGS) ./vmsim --mode=seg --config=$(CONFIG) --trace=$(TRACE)

#Alias for Valgrind
vg: valgrind

#Remove all files from directory (specifically executable and object file)
clean:
	rm -f vmsim vmsim.o