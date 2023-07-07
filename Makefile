CC = gcc
VERSION = -std=c2x
DEBUG = -g

CFLAGS = -pedantic -Wall -Wextra $(VERSION) $(DEBUG)
LFLAGS = -Wall $(DEBUG) $(VERSION)

INCS = -I.
SRCS = $(wildcard *.c)

OBJS = $(SRCS:.c=.o)
EXEC = test.out

all: $(SRCS) $(EXEC)

%.o:%.c $(INCS)
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

$(EXEC): $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o $@

test: all
	./$(EXEC)

clean:
	\rm -f *.o $(EXEC)
	@echo clean done