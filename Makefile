.POSIX:

PREFIX = /usr/local
LIBDIR = ${PREFIX}/lib
INCLUDEDIR = ${PREFIX}/include
XCFLAGS = ${CFLAGS} -std=c99 -fPIC -D_POSIX_VERSION=200809L -D_XOPEN_SOURCE=700 \
		  -Wall -Wextra -Wpedantic -Wmissing-prototypes -Wstrict-prototypes -Wno-unused-parameter
XLDFLAGS = ${LDFLAGS} -shared -Wl,-soname,libudev.so.1
XARFLAGS = -rc

OBJ = \
	  udev.o \
	  udev_hwdb.o \
	  udev_list.o \
	  udev_device.o \
	  udev_monitor.o \
	  udev_enumerate.o

all: libudev.so libudev.a libudev.pc

.c.o:
	${CC} ${XCFLAGS} -c -o $@ $<

libudev.a: ${OBJ}
	${AR} ${XARFLAGS} $@ ${OBJ}

libudev.so: ${OBJ}
	${CC} ${XCFLAGS} -o $@ ${OBJ} ${XLDFLAGS}

libudev.pc: libudev.pc.in
	sed -e 's|@libdir@|${LIBDIR}|g' \
		-e 's|@includedir@|${INCLUDEDIR}|g' \
		-e 's|@VERSION@|243|g' \
		libudev.pc.in > libudev.pc

install: libudev.so libudev.a libudev.pc
	mkdir -p         ${DESTDIR}${INCLUDEDIR} ${DESTDIR}${LIBDIR}/pkgconfig
	cp -f udev.h  	 ${DESTDIR}${INCLUDEDIR}/libudev.h
	chmod 0644       ${DESTDIR}${INCLUDEDIR}/libudev.h
	cp -f libudev.a  ${DESTDIR}${LIBDIR}/libudev.a
	chmod 0644       ${DESTDIR}${LIBDIR}/libudev.a
	cp -f libudev.so ${DESTDIR}${LIBDIR}/libudev.so
	chmod 0755       ${DESTDIR}${LIBDIR}/libudev.so
	ln -s libudev.so ${DESTDIR}${LIBDIR}/libudev.so.1
	cp -f libudev.pc ${DESTDIR}${LIBDIR}/pkgconfig/
	chmod 0644       ${DESTDIR}${LIBDIR}/pkgconfig/libudev.pc

uninstall:
	rm -f ${DESTDIR}${LIBDIR}/libudev.a \
          ${DESTDIR}${LIBDIR}/libudev.so \
          ${DESTDIR}${LIBDIR}/libudev.so.1 \
          ${DESTDIR}${LIBDIR}/pkgconfig/libudev.pc \
          ${DESTDIR}${INCLUDEDIR}/libudev.h

clean:
	rm -f libudev.so libudev.a libudev.pc ${OBJ}

.PHONY: all clean install uninstall
