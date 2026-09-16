CFLAGS ?= -O2 -Wall -Wextra
PREFIX ?= /usr/local

8joh: 8joh.c

install: 8joh
	install -Dm755 8joh $(DESTDIR)$(PREFIX)/bin/8joh

clean:
	rm -f 8joh

.PHONY: install clean
