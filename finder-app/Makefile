CC      := $(CROSS_COMPILE)gcc
CFLAGS  ?= -Wall -Wextra -Werror -g
LDFLAGS ?=

TARGET  := writer
SRCS    := writer.c
OBJS    := $(SRCS:.c=.o)

.PHONY: all clean

# Default target
all: $(TARGET)

# Link
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean artifacts
clean:
	rm -f $(TARGET) $(OBJS)
