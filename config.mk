VERSION = 1.1.0

CC 			= gcc
EXTRAFLAGS 	= -g -Wshadow -Wformat=2 -Wconversion -Wextra
CPPFLAGS	=
CFLAGS 		= -std=c11 -pedantic -Wall -Wshadow -Wconversion -D_XOPEN_SOURCE=700
LDFLAGS		=

# additional cflags
ifeq ($(DEBUG),1)
	CFLAGS += -g -DDEBUG
else
	CFLAGS += -O3
endif


# paths
PREFIX		= /usr/local
MANPREFIX 	= ${PREFIX}/share/man
DOCPREFIX 	= ${PREFIX}/share/doc

# dependency lists
DEPENDENCIES	= inih

# configure lib support (uncomment to disable)
X11_SUPPORT			= true 		# requires xrandr
IMLIB2_SUPPORT		= true
CANBERRA_SUPPORT	= true
LIBNOTIFY_SUPPORT 	= true

# x11 support
ifdef X11_SUPPORT
DEPENDENCIES	+= x11 xrandr xi
CFLAGS		+= -DX11
endif

# xrandr support
ifdef XRANDR_SUPPORT
endif

# imlib2 support
ifdef IMLIB2_SUPPORT
DEPENDENCIES	+= imlib2
CFLAGS		+= -DIMLIB2
endif

# libcanberra support
ifdef CANBERRA_SUPPORT
DEPENDENCIES	+= libcanberra
CFLAGS		+= -DCANBERRA
endif

# libcanberra support
ifdef LIBNOTIFY_SUPPORT
DEPENDENCIES	+= libnotify
CFLAGS		+= -DLIBNOTIFY
endif

CFLAGS		+= `pkg-config --cflags $(DEPENDENCIES)`
LIBS		+= `pkg-config --libs $(DEPENDENCIES)`
