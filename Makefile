include config.mk

SRC = daemon.c lib/iniparser/libiniparser.a
OBJ = ${SRC:.c=.o}
DST = ${SRC:.c=}

all: options sentinal install

options:
	@echo "Sentinal Build Options"
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

sentinal:
	libtool --mode=link --tag=CC $(CC) $(CFLAGS) $(SRC) -o $(DST) lib/rabbitmq/librabbitmq/librabbitmq.la lib/rabbitmq/examples/libutils.la

install:
	@echo "$(DST) build COMPLETED"
