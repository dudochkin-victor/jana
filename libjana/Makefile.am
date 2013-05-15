SUBDIRS = . doc

localedir = $(datadir)/locale
AM_CPPFLAGS = -DLOCALEDIR=\"$(localedir)\" -DPKGDATADIR=\"$(pkgdatadir)\" $(GOBJECT_CFLAGS) -Wall
AM_LDFLAGS = $(GOBJECT_LIBS) -lm

source_c = jana-component.c \
	jana-event.c \
	jana-note.c \
	jana-store.c \
	jana-store-view.c \
	jana-task.c \
	jana-time.c \
	jana-utils.c

source_h = jana.h \
	jana-component.h \
	jana-event.h \
	jana-note.h \
	jana-store.h \
	jana-store-view.h \
	jana-task.h \
	jana-time.h \
	jana-utils.h

lib_LTLIBRARIES = libjana.la
libjana_la_SOURCES = $(source_c) $(source_h)

library_includedir=$(includedir)/jana/libjana
library_include_HEADERS = $(source_h)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libjana.pc

