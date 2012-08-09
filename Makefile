include config.mk

SRC = daemon.c
OBJ = ${SRC:.c=.o}
DST = ${SRC:.c=}

all: options shepherd install

options:
	@echo "Shepherd Build Options"
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

shepherd:
	$(CC) $(CFLAGS) $(SRC) -o $(DST)

install:
	@echo "$(DST) build COMPLETED"
