CC ?= gcc
CFLAGS ?= -Wall -O2
LDFLAGS = -lcurl

TARGET = wget

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

all: $(TARGET)

$(TARGET): wurl.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

install: $(TARGET)
	install -Dm755 $(TARGET) $(BINDIR)/$(TARGET)

test:
	python test.py

clean:
	rm -f $(TARGET)

uninstall:
	rm -f $(BINDIR)/$(TARGET)

.PHONY: all clean install test uninstall
