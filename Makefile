# .POSIX ??

PREFIX = /usr/local
XCFLAGS = ${CFLAGS} -pedantic -fPIC -fvisibility=hidden \
		  -D_POSIX_VERSION=200809L -D_XOPEN_SOURCE=700 -std=c99 \
		  -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes \
		  -Wno-return-type -Wno-unused-parameter
XLDFLAGS = ${LDFLAGS} -shared -Wl,-soname,libudev.so.1
XARFLAGS = -rc

OBJ = \
	udev.o \
	udev_list.o \
	udev_util.o \
	udev_device.o \
	udev_monitor.o \
	udev_enumerate.o

all: libudev.so libudev.a

.c.o:
	${CC} ${XCFLAGS} -c -o $@ $<

libudev.a: ${OBJ}
	${AR} ${XARFLAGS} $@ ${OBJ}

libudev.so: ${OBJ}
	${CC} ${XCFLAGS} -o $@ ${OBJ} ${XLDFLAGS}

install: libudev.so libudev.a
	mkdir -p         ${DESTDIR}${INCLUDEDIR} ${DESTDIR}${LIBDIR}
	cp -f udev.h  	 ${DESTDIR}${INCLUDEDIR}/libudev.h
	chmod 0644       ${DESTDIR}${INCLUDEDIR}/libudev.h
	cp -f libudev.a  ${DESTDIR}${LIBDIR}/libudev.a
	chmod 0644       ${DESTDIR}${LIBDIR}/libudev.a
	cp -f libudev.so ${DESTDIR}${LIBDIR}/libudev.so
	chmod 0755       ${DESTDIR}${LIBDIR}/libudev.so
	ln -s libudev.so ${DESTDIR}${LIBDIR}/libudev.so.1

uninstall:
	rm -f ${DESTDIR}${LIBDIR}/libudev.a \
          ${DESTDIR}${LIBDIR}/libudev.so \
          ${DESTDIR}${LIBDIR}/libudev.so.1 \
          ${DESTDIR}${INCLUDEDIR}/libudev.h

clean:
	rm -f libudev.so libudev.a ${OBJ}

.PHONY: all clean install uninstall
