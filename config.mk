VERSION = 0.1

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

# flags
CFLAGS = -g -std=c99 -pedantic -Wall -O0 -DVERSION=\"${VERSION}\"
LDFLAGS = -g -lX11

# compiler and linker
CC = cc
