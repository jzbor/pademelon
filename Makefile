CC 			= gcc
EXTRAFLAGS 	= -g -Wshadow -Wformat=2 -Wconversion -Wextra
CPPFLAGS	=
CFLAGS 		= -std=c11 -pedantic -Wall -Werror -Wno-error=unused-function -Wno-error=unused-label \
		 	  -Wno-error=unused-value -Wno-error=unused-variable -D_XOPEN_SOURCE=700
LDFLAGS		= -linih
# VPATH		= src

all: pademelon-daemon

%.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -g -c $< -o $@

common.o: src/common.c src/common.h
desktop-daemon.o: src/desktop-daemon.c src/desktop-daemon.h src/common.h
pademelon-daemon.o: src/pademelon-daemon.c src/common.h

pademelon-daemon: common.o desktop-daemon.o pademelon-daemon.o
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f pademelon-daemon
	rm -f common.o
	rm -f desktop-daemon.o
	rm -f pademelon-daemon.o

.PHONY: all clean
