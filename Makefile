CC = gcc
CFLAGS = -Wall -std=c11 -g 

BIN = ./bin/
SRC = ./src/
INC = ./include/

all: list parser
		
parser: $(BIN)GEDCOMparser.o
	ar rcs $(BIN)libparser.a $(BIN)GEDCOMparser.o
		
list: $(BIN)LinkedListAPI.o
	ar rcs $(BIN)liblist.a $(BIN)LinkedListAPI.o
	

LinkedListAPI.o: $(SRC)LinkedListAPI.c $(INC)LinkedListAPI.h
	$(CC) $(CFLAGS) -c -Iinclude $(SRC)LinkedListAPI.c

clean:
	rm -f $(BIN)*.o
	rm -f $(BIN)*.a
	
test: 
	$(CC) $(CFLAGS)  -Iinclude $(SRC)main.c $(BIN)libparser.a $(BIN)liblist.a
	
	
$(BIN)LinkedListAPI.o: $(SRC)LinkedListAPI.c
	$(CC) $(CLFAGS) -c $(SRC)LinkedListAPI.c -Iinclude -o $(BIN)LinkedListAPI.o
	
$(BIN)GEDCOMparser.o: $(SRC)GEDCOMparser.c
	$(CC) $(CLFAGS) -c $(SRC)GEDCOMparser.c -Iinclude -o $(BIN)GEDCOMparser.o


