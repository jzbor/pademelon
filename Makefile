CC 			= gcc
EXTRAFLAGS 	= -g -Wshadow -Wformat=2 -Wconversion -Wextra
CPPFLAGS	=
CFLAGS 		= -std=c11 -pedantic -Wall -Werror -Wno-error=unused-function -Wno-error=unused-label \
		 	  -Wno-error=unused-value -Wno-error=unused-variable -D_XOPEN_SOURCE=700
LDFLAGS		= -linih
PREFIX		= /usr/local
# VPATH		= src

all: pademelon-daemon

%.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -g -c $< -o $@

common.o: src/common.c src/common.h
desktop-daemon.o: src/desktop-daemon.c src/desktop-daemon.h src/common.h
pademelon-daemon.o: src/pademelon-daemon.c src/common.h
pademelon-daemon-config.o: src/pademelon-daemon-config.c src/common.h

pademelon-daemon: common.o desktop-daemon.o pademelon-daemon.o pademelon-daemon-config.o
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f common.o
	rm -f desktop-daemon.o
	rm -f pademelon-daemon
	rm -f pademelon-daemon.o
	rm -f pademelon-daemon-config.o

install:
	install -Dm755 pademelon-daemon -t ${DESTDIR}${PREFIX}/bin

install-daemons:
	install -Dm644 daemons/* -t ${DESTDIR}${PREFIX}/share/pademelon/daemons

install-all: install install-daemons

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/pademelon-daemon

uninstall-daemons:
	rm -rf ${DESTDIR}${PREFIX}/share/pademelon/daemons

uninstall-all: uninstall uninstall-daemons

.PHONY: all clean install install-daemons install-all uninstall uninstall-daemons uninstall-all
