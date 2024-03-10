CC := g++
CFLAGS := -Wall -Wextra -std=c++11

SRC := main.cpp
OBJ := main.o

all: program

program: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $<

clean:
	rm -rf program $(OBJ)
