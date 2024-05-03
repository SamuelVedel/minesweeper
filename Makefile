CC = gcc
CFLAGS = -std=c99 -Wall -Wextra
LDFLAGS =
BUILD = build
SRC = src

all: demineur

$(BUILD)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) $< -o $@

demineur: $(SRC)/demineur.o
	$(CC) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(BUILD)/*.o demineur

.PHONY: clean all
