CC 			= gcc
EXTRAFLAGS 	= -g -Wshadow -Wformat=2 -Wconversion -Wextra
CPPFLAGS	=
CFLAGS 		= -std=c11 -pedantic -Wall -Werror -Wno-error=unused-function -Wno-error=unused-label \
		 	  -Wno-error=unused-value -Wno-error=unused-variable -D_XOPEN_SOURCE=700
LDFLAGS		=
PREFIX		= /usr/local

X11INC 		= -I/usr/X11R6/include
X11LIB 		= -L/usr/X11R6/lib -lX11 -lXrandr
INIHLIB		= -linih
IMLIB2LIB	= -lImlib2
