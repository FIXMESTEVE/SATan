CC = g++
C = gcc
OBJ = obj

SRC_DIR = src

.PHONY: project_code

project_code:
	$(CC) -std=c++11 -c $(SRC_DIR)/Algorithmic.cpp $(SRC_DIR)/Utility.cpp $(SRC_DIR)/main.cpp;
	$(C) -c $(SRC_DIR)/gengraph.c;
	mv *.o obj;
	make main;
	make gengraph;

main: $(OBJ)/main.o $(OBJ)/Algorithmic.o $(OBJ)/Utility.o
	$(CC) -std=c++11 -o main $(OBJ)/main.o $(OBJ)/Algorithmic.o $(OBJ)/Utility.o

gengraph: $(OBJ)/gengraph.o
	$(C) -lm -o gengraph $(OBJ)/gengraph.o

clean:
	rm -rf $(OBJ)/*.o;
	rm -rf gengraph main;