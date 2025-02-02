CC = gcc
SRCDIR = ./source
BINDIR = ./binaries

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(BINDIR)/%.o,$(SOURCES))

CFLAGS = -Wall -Wextra
DEBUG_FLAGS = -g -DDEBUG
PROD_FLAGS = -O3 -DNDEBUG

all: setup debug

setup:
	@mkdir -p $(BINDIR)

debug: CFLAGS += $(DEBUG_FLAGS)
debug: setup $(BINDIR)/server_d

$(BINDIR)/server_d: $(OBJECTS)
	$(CC) $(OBJECTS) -o $@

prod: CFLAGS += $(PROD_FLAGS)
prod: setup $(BINDIR)/server

$(BINDIR)/server: $(OBJECTS)
	$(CC) $(OBJECTS) -o $@

$(BINDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BINDIR)

.PHONY: all setup debug prod clean