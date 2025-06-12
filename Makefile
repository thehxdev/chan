CC ?= cc
CFLAGS = -std=gnu99 -Wall -Wextra
LDFLAGS =
LIBS = -lpthread

ifeq ($(CC), clang)
	LDFLAGS += -fuse-ld=lld
endif

ifeq ($(OPTIMIZE), 1)
	CFLAGS += -O3 -DNDEBUG
else
	CFLAGS += -Og -ggdb
	ifeq ($(CC), clang)
		# sanitizers are in conflict with valgrind
		LDFLAGS += -fsanitize=thread,undefined
	endif
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
