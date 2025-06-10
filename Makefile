CC = clang
CFLAGS = -std=c99 -Wall -Wextra
LDFLAGS =
LIBS = -lpthread

ifeq ($(OPTIMIZE), 1)
	CFLAGS += -O3 -DNDEBUG
else
	CFLAGS += -Og -ggdb
endif


SRC_FILES = main.c chan.c
OBJ_FILES = $(SRC_FILES:.c=.o)

BIN = a.out

$(BIN): $(OBJ_FILES)
	$(CC) $(LDFLAGS) -o $(BIN) $(OBJ_FILES) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm *.o $(BIN)
