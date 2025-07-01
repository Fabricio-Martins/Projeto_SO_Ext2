CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
INCLUDES = -I. -Icmd

SRCS = 	main-shell.c \
		utils/utils.c \
		cmd/info/info.c \
		cmd/cat/cat.c \

OBJS = $(SRCS:.c=.o)
TARGET = main-shell

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
