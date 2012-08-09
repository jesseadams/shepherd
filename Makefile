include config.mk

SRC = daemon.c
OBJ = ${SRC:.c=.o}
DST = ${SRC:.c=}

all: options sentinal install

options:
	@echo "Sentinal Build Options"
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

sentinal:
	$(CC) $(CFLAGS) $(SRC) -o $(DST)

install:
	@echo "$(DST) build COMPLETED"
