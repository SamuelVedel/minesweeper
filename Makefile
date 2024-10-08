CC = gcc
CFLAGS = -std=c99 -Wall -Wextra
LDFLAGS =
BUILD = build
SRC = src

SRCS = $(SRC)/game.c $(SRC)/display.c

all: demineur

$(BUILD)/game.o: $(SRC)/game.h $(SRC)/display.h
$(BUILD)/display.o: $(SRC)/display.h $(SRC)/game.h
$(BUILD)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

demineur: $(BUILD)/game.o $(BUILD)/display.o
	$(CC) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(BUILD)/*.o demineur $(SRC)/*~ *~

.PHONY: clean all
