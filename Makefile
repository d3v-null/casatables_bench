# create a makefile for this project
CC = g++
CFLAGS = -Wall -Wextra -std=c++11 -Wpointer-arith -Woverloaded-virtual \
	-Wwrite-strings -pedantic -Wno-long-long -fdiagnostics-color=always
LIBS := -lcasa_tables -lcasa_casa

TARGET = main

ARGS := ""

main: main.cpp
	$(CC) $(CFLAGS) -o main main.cpp $(LIBS)

all: main

debug: CFLAGS += -DDEBUG -g
debug: all

release: CFLAGS += -O2 -DNDEBUG
release: all

validate: debug
validate:
	./main $(ARGS) -V -i 0 -t columnwise -w cell
	./main $(ARGS) -V -i 0 -t columnwise -w cells
	./main $(ARGS) -V -i 0 -t columnwise -w column
	./main $(ARGS) -V -i 0 -t rowwise -w cell
	./main $(ARGS) -V -i 0 -t rowwise -w cells

bench: release
bench:
	./main $(ARGS) -s -t columnwise -w cell
	./main $(ARGS) -s -t columnwise -w cells
	./main $(ARGS) -s -t rowwise -w cell
	./main $(ARGS) -s -t rowwise -w cells
	./main $(ARGS) -t columnwise -w cell
	./main $(ARGS) -t columnwise -w cells
	./main $(ARGS) -t columnwise -w column
	./main $(ARGS) -t rowwise -w cell
	./main $(ARGS) -t rowwise -w cells