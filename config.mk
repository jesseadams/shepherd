# Application Version
VERSION = 0.1b

# Customize below to fit your system

# Paths
PREFIX = ${HOME}/.local
MANPREFIX = ${PREFIX}/share/man

# Includes and Libraries
INCS = -I. -I/usr/include -I/usr/local/include
LIBS = -L/usr/lib -L/usr/local/lib -lc

# Compiler Flags
CFLAGS = -std=c99 -pedantic -Wall -Os ${INCS} ${CPPFLAGS}
LDFLAGS = -s ${LIBS}

# Compiler and Linker
CC = cc
