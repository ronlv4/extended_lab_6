OBJ_DIR := obj
BIN_DIR := bin
SRC_DIR := src
LIST_DIR := list

all: task3

task0:
	gcc -g -m32 -Wall -o $(BIN_DIR)/task0 $(SRC_DIR)/mypipeline.c

looper:
	gcc -g -m32 -Wall -o $(BIN_DIR)/looper $(SRC_DIR)/looper.c

task1:
	gcc -g -m32 -Wall -o $(BIN_DIR)/task1 $(SRC_DIR)/task1.c $(SRC_DIR)/lineParser.c

task3:
	gcc -g -m32 -Wall -o $(BIN_DIR)/task3 $(SRC_DIR)/task3.c $(SRC_DIR)/lineParser.c

clean:
	rm -rf $(OBJ_DIR)/* $(BIN_DIR)/* $(LIST_DIR)/*
