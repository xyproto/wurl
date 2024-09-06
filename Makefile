# Simple Makefile for building and installing wurl on Arch Linux

CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = -lcurl

# Target binary name
TARGET = wurl

# Installation paths
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

# Build wurl
all: $(TARGET)

$(TARGET): wurl.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

test:
	/usr/bin/env python test.py

# Install wurl
install: $(TARGET)
	install -Dm755 $(TARGET) $(BINDIR)/$(TARGET)

# Clean build files
clean:
	rm -f $(TARGET)

# Uninstall wurl
uninstall:
	rm -f $(BINDIR)/$(TARGET)

.PHONY: all clean install test uninstall
