CC = gcc
CFLAGS = -std=c99 -Wall -Wextra
LDFLAGS =
BUILD = build
SRC = src

SRCS = $(SRC)/game.c $(SRC)/display.c

all: minesweeper

$(BUILD)/game.o: $(SRC)/game.h $(SRC)/display.h
$(BUILD)/display.o: $(SRC)/display.h $(SRC)/game.h
$(BUILD)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

minesweeper: $(BUILD)/game.o $(BUILD)/display.o
	$(CC) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(BUILD)/*.o minesweeper $(SRC)/*~ *~

.PHONY: clean all
