include config.mk

# VPATH		= src
DAEMON_OBJ	= common.o desktop-application.o pademelon-daemon.o pademelon-config.o tools.o signals.o
TOOLS_OBJ	= pademelon-tools.o tools.o common.o signals.o desktop-application.o pademelon-config.o cliparse.o

ifdef X11_SUPPORT
DAEMON_OBJ 	+= x11-utils.o
TOOLS_OBJ 	+= x11-utils.o
endif # X11_SUPPORT



all: pademelon-daemon pademelon-tools

%.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

common.o: src/common.c src/common.h src/signals.h
cliparse.o: src/cliparse.c src/cliparse.h
desktop-application.o: src/desktop-application.c src/desktop-application.h src/common.h src/signals.h src/pademelon-config.h
pademelon-daemon.o: src/pademelon-daemon.c src/pademelon-config.h src/common.h src/tools.h src/signals.h
pademelon-config.o: src/pademelon-config.c src/common.h
pademelon-tools.o: src/pademelon-tools.c src/tools.h src/x11-utils.h src/cliparse.h
signals.o: src/signals.c src/signals.h src/common.h src/desktop-application.h
tools.o: src/tools.c src/common.h src/x11-utils.h src/desktop-application.h

x11-utils.o: src/x11-utils.c src/x11-utils.h src/common.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

pademelon-daemon: $(DAEMON_OBJ)
	$(CC) $(LDFLAGS) $(LIBS) -o $@ $^

pademelon-tools: $(TOOLS_OBJ)
	$(CC) $(LDFLAGS) $(LIBS) -o $@ $^

clean:
	rm -f cliparse.o
	rm -f common.o
	rm -f desktop-application.o
	rm -f desktop-daemon.o
	rm -f pademelon-config.o
	rm -f pademelon-daemon
	rm -f pademelon-daemon.o
	rm -f pademelon-tools
	rm -f pademelon-tools.o
	rm -f signals.o
	rm -f tools.o
	rm -f x11-utils.o

install: pademelon-daemon pademelon-tools
	install -Dm755 pademelon-daemon -t ${DESTDIR}${PREFIX}/bin
	install -Dm755 pademelon-tools -t ${DESTDIR}${PREFIX}/bin
	install -Dm755 src/pademelon-settings -t ${DESTDIR}${PREFIX}/bin
	install -Dm755 pademelon-settings.desktop pademelon-wallpaper.desktop -t ${DESTDIR}${PREFIX}/share/applications
	mkdir -p ${DESTDIR}${PREFIX}/share/xsessions/
	sed "s/PREFIX/$(shell echo "${PREFIX}" | sed 's/\//\\\//g')/g" < pademelon.desktop > ${DESTDIR}${PREFIX}/share/xsessions/pademelon.desktop
	sed "s/PREFIX/$(shell echo "${PREFIX}" | sed 's/\//\\\//g')/g" < pademelon-setup.desktop > ${DESTDIR}${PREFIX}/share/xsessions/pademelon-setup.desktop

install-applications:
	install -Dm644 applications/* -t ${DESTDIR}${PREFIX}/share/pademelon/applications

install-all: install install-applications

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/pademelon-daemon ${DESTDIR}${PREFIX}/bin/pademelon-settings ${DESTDIR}${PREFIX}/bin/pademelon-tools
	rm -f ${DESTDIR}${PREFIX}/share/applications/pademelon-settings.desktop
	rm -f ${DESTDIR}${PREFIX}/share/applications/pademelon-wallpaper.desktop

uninstall-applications:
	rm -rf ${DESTDIR}${PREFIX}/share/pademelon/applications

uninstall-all: uninstall uninstall-applications

.PHONY: all clean install install-daemons install-all uninstall uninstall-daemons uninstall-all
