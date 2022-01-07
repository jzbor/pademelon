CC 			= gcc
EXTRAFLAGS 	= -g -Wshadow -Wformat=2 -Wconversion -Wextra
CPPFLAGS	=
CFLAGS 		= -std=c11 -pedantic -Wall -Werror -Wno-error=unused-function -Wno-error=unused-label \
		 	  -Wno-error=unused-value -Wno-error=unused-variable -D_XOPEN_SOURCE=700
LDFLAGS		=
PREFIX		= /usr/local

# dependency lists
DEPENDENCIES	= inih

# x11 support
DEPENDENCIES	+= x11
CFLAGS		+= -DX11

# xrandr support
DEPENDENCIES	+= xrandr
CFLAGS		+= -DXRANDR

# imlib2 support
DEPENDENCIES	+= imlib2
CFLAGS		+= -DIMLIB2


CFLAGS		+= `pkg-config --cflags $(DEPENDENCIES)`
LIBS		+= `pkg-config --libs $(DEPENDENCIES)`
