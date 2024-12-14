ifeq ($(origin CC), default)
CC = gcc
endif

TARGET ?= ../lab8

CLIBS ?= -lpq
CFLAGS ?= -Wall -Werror -Wextra 

SRC = main.c
OBJ = $(SRC:.c=.o)

.PHONY: main
main: $(TARGET)/main

$(TARGET)/%: $(OBJ)
	$(CC) $^ -o $@ $(CLIBS)

.PHONY: clean
clean:
	rm -rf $(OBJ) $(TARGET)/main
