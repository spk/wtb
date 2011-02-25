# wtb - print id or title of currently open bug
# most taken from dwm Makefile
SRC = wtb.c
OBJ = ${SRC:.c=.o}
EXEC = ${SRC:.c=}

include config.mk

all: options ${EXEC}

options:
	@echo ${EXEC} build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<


${EXEC}: ${OBJ}
	@echo CC -o $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f ${EXEC} ${OBJ} ${EXEC}-${VERSION}.tar.gz

dist: clean
	@echo creating dist tarball
	@mkdir -p ${EXEC}-${VERSION}
	@cp -R LICENSE Makefile README config.mk \
		${EXEC}.1 ${SRC} ${EXEC}-${VERSION}
	@tar -cf ${EXEC}-${VERSION}.tar ${EXEC}-${VERSION}
	@gzip ${EXEC}-${VERSION}.tar
	@rm -rf ${EXEC}-${VERSION}

manpage:
	@echo generate manual page
	@a2x -f manpage -D . ${EXEC}.1.txt

install: all
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f ${EXEC} ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/${EXEC}
	@echo installing manual page to ${DESTDIR}${MANPREFIX}/man1
	@mkdir -p ${DESTDIR}${MANPREFIX}/man1
	@cp -f ${EXEC}.1 ${DESTDIR}${MANPREFIX}/man1/
	@chmod 644 ${DESTDIR}${MANPREFIX}/man1/${EXEC}.1

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/${EXEC}
	@echo removing manual page from ${DESTDIR}${MANPREFIX}/man1
	@rm -f ${DESTDIR}${MANPREFIX}/man1/${EXEC}.1

.PHONY: all options clean dist install uninstall
