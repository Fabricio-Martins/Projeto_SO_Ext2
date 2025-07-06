CC      := 	gcc
CFLAGS  := 	-Wall -g -Iinclude

OPS_DIR := 	ops
OBJ_DIR := 	.exec

SRCS    :=	main.c \
			utils.c \
			$(OPS_DIR)/commands.c

OBJS    := 	$(patsubst %.c,$(OBJ_DIR)/%.o,$(notdir $(SRCS)))

TARGET  := 	main
IMG     := 	myext2image.img

.PHONY: all clean shell

all: $(TARGET)

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(OPS_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

shell: all
	./$(TARGET) $(IMG)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
