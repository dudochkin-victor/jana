SUBDIRS = . doc

localedir = $(datadir)/locale
AM_CPPFLAGS = -DLOCALEDIR=\"$(localedir)\" -DPKGDATADIR=\"$(pkgdatadir)\" $(GOBJECT_CFLAGS) $(ECAL_CFLAGS) $(EDATASERVERUI_CFLAGS) $(GCONF_CFLAGS) -Wall
AM_LDFLAGS = $(GOBJECT_LIBS) $(ECAL_LIBS) $(GCONF_LIBS) $(EDATASERVERUI_LIBS)

source_h = jana-ecal.h \
	jana-ecal-component.h \
	jana-ecal-event.h \
	jana-ecal-note.h \
	jana-ecal-store.h \
	jana-ecal-store-view.h \
	jana-ecal-time.h \
	jana-ecal-utils.h \
	jana-ecal-task.h

source_c = jana-ecal-component.c \
	jana-ecal-event.c \
	jana-ecal-note.c \
	jana-ecal-store.c \
	jana-ecal-store-view.c \
	jana-ecal-time.c \
	jana-ecal-utils.c \
	jana-ecal-task.c

lib_LTLIBRARIES = libjana-ecal.la
libjana_ecal_la_LIBADD = $(top_srcdir)/libjana/libjana.la
libjana_ecal_la_SOURCES = $(source_c) $(source_h)

library_includedir=$(includedir)/jana/libjana-ecal
library_include_HEADERS = $(source_h)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libjana-ecal.pc

