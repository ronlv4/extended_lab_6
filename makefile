OBJ_DIR := obj
BIN_DIR := bin
SRC_DIR := src
LIST_DIR := list

all: looper task1 task2 task3

looper:
	gcc -g -m32 -Wall -o $(BIN_DIR)/looper $(SRC_DIR)/looper.c

task1:
	gcc -g -m32 -Wall -o $(BIN_DIR)/task1 $(SRC_DIR)/task1.c $(SRC_DIR)/lineParser.c

task2:
	gcc -g -m32 -Wall -o $(BIN_DIR)/task2 $(SRC_DIR)/task2.c $(SRC_DIR)/lineParser.c

task3:
	gcc -g -m32 -Wall -o $(BIN_DIR)/task3 $(SRC_DIR)/task3.c $(SRC_DIR)/lineParser.c

clean:
	rm -rf $(OBJ_DIR)/* $(BIN_DIR)/* $(LIST_DIR)/*
