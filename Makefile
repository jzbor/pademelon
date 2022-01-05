include config.mk

# VPATH		= src

all: pademelon-daemon pademelon-tools

%.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -g -c $< -o $@

common.o: src/common.c src/common.h
desktop-daemon.o: src/desktop-daemon.c src/desktop-daemon.h src/common.h src/signals.h
pademelon-daemon.o: src/pademelon-daemon.c src/common.h src/tools.h src/signals.h
pademelon-daemon-config.o: src/pademelon-daemon-config.c src/common.h
pademelon-tools.o: src/pademelon-tools.c src/tools.h src/x11-utils.h
signals.o: src/signals.c src/signals.h src/common.h
tools.o: src/tools.c src/common.h src/x11-utils.h

x11-utils.o: src/x11-utils.c src/x11-utils.h src/common.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(X11INC) -g -c $< -o $@

pademelon-daemon: common.o desktop-daemon.o pademelon-daemon.o pademelon-daemon-config.o x11-utils.o tools.o signals.o
	$(CC) $(LDFLAGS) $(INIHLIB) $(X11LIB) $(IMLIB2LIB) -o $@ $^

pademelon-tools: pademelon-tools.o tools.o common.o x11-utils.o
	$(CC) $(LDFLAGS) $(X11LIB) $(IMLIB2LIB) -o $@ $^

clean:
	rm -f common.o
	rm -f desktop-daemon.o
	rm -f pademelon-daemon
	rm -f pademelon-daemon-config.o
	rm -f pademelon-daemon.o
	rm -f pademelon-tools
	rm -f pademelon-tools.o
	rm -f tools.o
	rm -f x11-utils.o

install: pademelon-daemon pademelon-tools
	install -Dm755 pademelon-daemon -t ${DESTDIR}${PREFIX}/bin
	install -Dm755 pademelon-tools -t ${DESTDIR}${PREFIX}/bin
	install -Dm755 src/pademelon-settings -t ${DESTDIR}${PREFIX}/bin
	install -Dm755 pademelon-settings.desktop -t ${DESTDIR}${PREFIX}/share/applications
	sed "s/PREFIX/$(shell echo "${PREFIX}" | sed 's/\//\\\//g')/g" < pademelon.desktop > ${DESTDIR}${PREFIX}/share/xsessions/pademelon.desktop

install-daemons:
	install -Dm644 daemons/* -t ${DESTDIR}${PREFIX}/share/pademelon/daemons

install-all: install install-daemons

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/pademelon-daemon ${DESTDIR}${PREFIX}/bin/pademelon-settings

uninstall-daemons:
	rm -rf ${DESTDIR}${PREFIX}/share/pademelon/daemons

uninstall-all: uninstall uninstall-daemons

.PHONY: all clean install install-daemons install-all uninstall uninstall-daemons uninstall-all
