CC = gcc
CFLAGS = -Wall
EXEC = fetchmail
SRCS = main.c 
OBJS = $(SRCS:.c=.o)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJS) -lm

%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f $(OBJS) $(EXEC)

format:
	clang-format -i *.c *.h