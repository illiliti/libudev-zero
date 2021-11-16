# SPDX-License-Identifier: ISC
.POSIX:

PREFIX = /usr/local
LIBDIR = ${PREFIX}/lib
INCLUDEDIR = ${PREFIX}/include
PKGCONFIGDIR = ${LIBDIR}/pkgconfig
XCFLAGS = ${CPPFLAGS} ${CFLAGS} -std=c99 -fPIC -pthread -D_XOPEN_SOURCE=700 \
		  -Wall -Wextra -Wpedantic -Wmissing-prototypes -Wstrict-prototypes \
		  -Wno-unused-parameter
XLDFLAGS = ${LDFLAGS} -shared -Wl,-soname,libudev.so.1
XARFLAGS = -rc

OBJ = \
	  udev.o \
	  udev_list.o \
	  udev_device.o \
	  udev_monitor.o \
	  udev_enumerate.o

all: libudev.so.1 libudev.a

.c.o:
	${CC} ${XCFLAGS} -c -o $@ $<

libudev.a: ${OBJ}
	${AR} ${XARFLAGS} $@ ${OBJ}

libudev.so.1: ${OBJ}
	${CC} ${XCFLAGS} -o $@ ${OBJ} ${XLDFLAGS}

libudev.pc: libudev.pc.in
	libdir="${LIBDIR}"; \
	if [ "$${libdir#${PREFIX}}" != "$$libdir" ]; then \
		libdir="\$${exec_prefix}$${libdir#${PREFIX}}"; \
	fi; \
	includedir="${INCLUDEDIR}"; \
	if [ "$${includedir#${PREFIX}}" != "$$includedir" ]; then \
		includedir="\$${prefix}$${includedir#${PREFIX}}"; \
	fi; \
	sed -e 's|@prefix@|${PREFIX}|g' \
		-e 's|@exec_prefix@|${PREFIX}|g' \
		-e "s|@libdir@|$$libdir|g" \
		-e "s|@includedir@|$$includedir|g" \
		-e 's|@VERSION@|243|g' \
		libudev.pc.in > libudev.pc

install: libudev.so.1 libudev.a libudev.pc
	mkdir -p         ${DESTDIR}${INCLUDEDIR} ${DESTDIR}${LIBDIR} ${DESTDIR}${PKGCONFIGDIR}
	cp -f udev.h  	 ${DESTDIR}${INCLUDEDIR}/libudev.h
	cp -f libudev.a  ${DESTDIR}${LIBDIR}/libudev.a
	cp -f libudev.so.1 ${DESTDIR}${LIBDIR}/libudev.so.1
	ln -fs libudev.so.1 ${DESTDIR}${LIBDIR}/libudev.so
	cp -f libudev.pc ${DESTDIR}${PKGCONFIGDIR}/libudev.pc

uninstall:
	rm -f ${DESTDIR}${LIBDIR}/libudev.a \
          ${DESTDIR}${LIBDIR}/libudev.so \
          ${DESTDIR}${LIBDIR}/libudev.so.1 \
          ${DESTDIR}${PKGCONFIGDIR}/libudev.pc \
          ${DESTDIR}${INCLUDEDIR}/libudev.h

clean:
	rm -f libudev.so.1 libudev.a libudev.pc ${OBJ}

.PHONY: all clean install uninstall
