CC=gcc
CCFLAGS=-w -g -Wall
LIBDIR=headers
INCLUDE=-I headers
SRC=src
TARGET=ush

all: 
	$(CC) $(CCFLAGS) $(SRC)/parse.c $(SRC)/builtins.c $(SRC)/redirect.c $(SRC)/jobs.c $(SRC)/main.c -o $(TARGET) $(INCLUDE)



