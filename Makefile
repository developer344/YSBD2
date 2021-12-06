CC = gcc
CCFLAGS = -no-pie
LIB = ./lib
INCLUDE = -I ./include
SRC = ./src
BIN = ./bin
BUILD = ./build

all: main

main: $(SRC)/main.c
	$(CC) $(CCFLAGS) $(INCLUDE) -o $(BIN)/main $(SRC)/main.c $(SRC)/SHT.c $(SRC)/HT.c $(LIB)/BF_64.a

clean:
	rm $(BIN)/main $(BUILD)/*