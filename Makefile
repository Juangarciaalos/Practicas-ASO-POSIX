#!/usr/bin/make -f

MAJOR := 0
MINOR := 1
ASOSYSINT := asosysintercept
VERSION := $(MAJOR).$(MINOR)

CFLAGS=-ggdb3 -Wall -Werror -Wno-unused -std=c11
LDFLAGS=--shared -fPIC
LDLIBS=-Wl,--no-as-needed -ldl

all: lib minigrep 

lib: lib$(ASOSYSINT).so.$(VERSION)

lib$(ASOSYSINT).so.$(VERSION): $(ASOSYSINT).o
	$(CC) $(LDFLAGS) $(LDLIBS) $^ -o $@

minigrep: minigrep.c
	$(CC) $(CFLAGS) minigrep.c -o $@

clean:
	rm -rf *~ lib$(ASOSYSINT).so.$(VERSION) $(ASOSYSINT).o  minigrep  core

.PHONY: clean
