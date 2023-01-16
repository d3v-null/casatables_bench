# create a makefile for this project
CC = g++
CFLAGS = -Wall -Wextra -std=c++11 -Wpointer-arith -Woverloaded-virtual \
	-Wwrite-strings -pedantic -Wno-long-long -fdiagnostics-color=always
LIBS = -lcasa_ms -lcasa_tables -lcasa_casa

TARGET = main
ITERS := 100

main: main.cpp
	$(CC) $(CFLAGS) -o main main.cpp $(LIBS)

all: main

debug: CFLAGS += -DDEBUG -g
debug: all

release: CFLAGS += -O2
release: all

validate: debug
validate:
	./main -T 3 -B 6 -C 5 -P 4 -V -i 0 -t columnwise -w cell
	./main -T 3 -B 6 -C 5 -P 4 -V -i 0 -t columnwise -w cells
	./main -T 3 -B 6 -C 5 -P 4 -V -i 0 -t columnwise -w column

bench: release
bench:
	./main -i $(ITERS) -t columnwise -w cell
	./main -i $(ITERS) -t columnwise -w cells
	./main -i $(ITERS) -t columnwise -w column