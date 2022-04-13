include config.mk

# VPATH		= src
DAEMON_OBJ	= common.o desktop-application.o pademelon-daemon.o pademelon-config.o tools.o signals.o desktop-files.o
TOOLS_OBJ	= pademelon-tools.o tools.o common.o signals.o desktop-application.o pademelon-config.o cliparse.o desktop-files.o

ifdef X11_SUPPORT
DAEMON_OBJ 	+= x11-utils.o
TOOLS_OBJ 	+= x11-utils.o
endif # X11_SUPPORT


all: pademelon-daemon pademelon-tools

%.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

pademelon-%.1: doc/%.md
	go-md2man -in $< -out $@

pademelon.1: README.md
	go-md2man -in $< -out $@

common.o: src/common.c src/common.h src/signals.h
cliparse.o: src/cliparse.c src/cliparse.h
desktop-application.o: src/desktop-application.c src/desktop-application.h src/common.h src/signals.h src/desktop-files.h
desktop-files.o: src/desktop-files.c src/desktop-files.h
pademelon-daemon.o: src/pademelon-daemon.c src/pademelon-config.h src/common.h src/tools.h src/signals.h
pademelon-config.o: src/pademelon-config.c src/common.h src/desktop-application.h
pademelon-tools.o: src/pademelon-tools.c src/tools.h src/x11-utils.h src/cliparse.h
signals.o: src/signals.c src/signals.h src/common.h src/desktop-application.h
tools.o: src/tools.c src/common.h src/x11-utils.h src/desktop-application.h src/desktop-files.h

x11-utils.o: src/x11-utils.c src/x11-utils.h src/common.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

pademelon-daemon: $(DAEMON_OBJ)
	$(CC) $(LDFLAGS) $(LIBS) -o $@ $^

pademelon-tools: $(TOOLS_OBJ)
	$(CC) $(LDFLAGS) $(LIBS) -o $@ $^

clean:
	rm -f *.o
	rm -f *.1
	rm -f pademelon-daemon pademelon-tools

install: pademelon-daemon pademelon-tools
	install -Dm755 pademelon-daemon -t ${DESTDIR}${PREFIX}/bin
	install -Dm755 pademelon-tools -t ${DESTDIR}${PREFIX}/bin
	install -Dm755 src/pademelon-settings -t ${DESTDIR}${PREFIX}/bin
	install -Dm755 src/xdg-xmenu -t ${DESTDIR}${PREFIX}/bin
	install -Dm755 pademelon-settings.desktop pademelon-wallpaper.desktop -t ${DESTDIR}${PREFIX}/share/applications
	mkdir -p ${DESTDIR}${PREFIX}/share/xsessions/
	sed "s/PREFIX/$(shell echo "${PREFIX}" | sed 's/\//\\\//g')/g" < pademelon.desktop > ${DESTDIR}${PREFIX}/share/xsessions/pademelon.desktop
	sed "s/PREFIX/$(shell echo "${PREFIX}" | sed 's/\//\\\//g')/g" < pademelon-setup.desktop > ${DESTDIR}${PREFIX}/share/xsessions/pademelon-setup.desktop

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/pademelon-daemon ${DESTDIR}${PREFIX}/bin/pademelon-settings \
		${DESTDIR}${PREFIX}/bin/xdg-xmenu ${DESTDIR}${PREFIX}/bin/pademelon-tools
	rm -f ${DESTDIR}${PREFIX}/share/applications/pademelon-settings.desktop
	rm -f ${DESTDIR}${PREFIX}/share/applications/pademelon-wallpaper.desktop

install-applications:
	install -Dm644 desktop-files/* -t ${DESTDIR}${PREFIX}/share/pademelon/applications

uninstall-applications:
	rm -rf ${DESTDIR}${PREFIX}/share/pademelon/applications

install-docs: pademelon.1 pademelon-config.1 pademelon-desktop-files.1
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" < pademelon.1 > ${DESTDIR}${MANPREFIX}/man1/pademelon.1
	chmod 644 ${DESTDIR}${MANPREFIX}/man1/pademelon.1
	install -Dm644 pademelon-config.1 pademelon-desktop-files.1 -t ${DESTDIR}${MANPREFIX}/man1/

uninstall-docs:
	rm -f ${DESTDIR}${MANPREFIX}/man1/pademelon.1
	rm -f ${DESTDIR}${MANPREFIX}/man1/pademelon-config.1
	rm -f ${DESTDIR}${MANPREFIX}/man1/pademelon-desktop-applications.1

install-all: install install-applications install-docs

uninstall-all: uninstall uninstall-applications install-docs

.PHONY: all clean install uninstall install-daemons uninstall-daemons install-docs uninstall-docs \
	install-all uninstall-all
.NOTPARALLEL: clean
