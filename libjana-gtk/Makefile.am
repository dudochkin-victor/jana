SUBDIRS = . data doc

localedir = $(datadir)/locale
AM_CPPFLAGS = -DLOCALEDIR=\"$(localedir)\" -DPKGDATADIR=\"$(pkgdatadir)\" $(GOBJECT_CFLAGS) $(GTK_CFLAGS) $(GTHREAD_CFLAGS) $(GLADE_CFLAGS) -Wall
AM_LDFLAGS = $(GOBJECT_LIBS) $(GTK_LIBS) $(GTHREAD_LIBS) $(GLADE_LIBS)

source_h = jana-gtk.h \
	jana-gtk-event-store.h \
	jana-gtk-cell-renderer-event.h \
	jana-gtk-tree-layout.h \
	jana-gtk-event-list.h \
	jana-gtk-date-time.h \
	jana-gtk-month-view.h \
	jana-gtk-day-view.h \
	jana-gtk-year-view.h \
	jana-gtk-recurrence.h \
	jana-gtk-clock.h \
	jana-gtk-world-map.h \
	jana-gtk-world-map-marker.h \
	jana-gtk-world-map-marker-pixbuf.h \
	jana-gtk-note-store.h \
	jana-gtk-cell-renderer-note.h \
	jana-gtk-utils.h

source_c = jana-gtk-event-store.c \
	jana-gtk-cell-renderer-event.c \
	jana-gtk-tree-layout.c \
	jana-gtk-event-list.c \
	jana-gtk-date-time.c \
	jana-gtk-month-view.c \
	jana-gtk-day-view.c \
	jana-gtk-year-view.c \
	jana-gtk-recurrence.c \
	jana-gtk-clock.c \
	jana-gtk-world-map.c \
	jana-gtk-world-map-marker.c \
	jana-gtk-world-map-marker-pixbuf.c \
	jana-gtk-note-store.c \
	jana-gtk-cell-renderer-note.c \
	jana-gtk-utils.c

lib_LTLIBRARIES = libjana-gtk.la
libjana_gtk_la_LIBADD = $(top_srcdir)/libjana/libjana.la
libjana_gtk_la_SOURCES = $(source_c) $(source_h)

library_includedir=$(includedir)/jana/libjana-gtk
library_include_HEADERS = $(source_h)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libjana-gtk.pc

