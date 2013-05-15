
SUBDIRS=data

localedir = $(datadir)/locale
AM_CPPFLAGS = -DLOCALEDIR=\"$(localedir)\" -DPKGDATADIR=\"$(pkgdatadir)\" $(ECAL_CFLAGS) $(GTK_CFLAGS) $(GTHREAD_CFLAGS) -Wall -DHANDLE_LIBICAL_MEMORY
AM_LDFLAGS = $(ECAL_LIBS) $(GTK_LIBS) $(GTHREAD_LIBS)


bin_PROGRAMS = jana-gtk-example-1 jana-gtk-example-2 jana-gtk-example-3 \
	jana-gtk-example-4 jana-gtk-example-5 jana-gtk-example-6 \
	jana-gtk-example-7 jana-gtk-example-8 jana-ecal-example-1

jana_gtk_example_1_SOURCES = jana-gtk-example-1.c
jana_gtk_example_1_LDADD = ../libjana/libjana.la \
			   ../libjana-ecal/libjana-ecal.la \
			   ../libjana-gtk/libjana-gtk.la

jana_gtk_example_2_SOURCES = jana-gtk-example-2.c
jana_gtk_example_2_LDADD = ../libjana/libjana.la \
			   ../libjana-ecal/libjana-ecal.la \
			   ../libjana-gtk/libjana-gtk.la

jana_gtk_example_3_SOURCES = jana-gtk-example-3.c
jana_gtk_example_3_LDADD = ../libjana/libjana.la \
			   ../libjana-ecal/libjana-ecal.la \
			   ../libjana-gtk/libjana-gtk.la

jana_gtk_example_4_SOURCES = jana-gtk-example-4.c
jana_gtk_example_4_LDADD = ../libjana/libjana.la \
			   ../libjana-ecal/libjana-ecal.la \
			   ../libjana-gtk/libjana-gtk.la

jana_gtk_example_5_SOURCES = jana-gtk-example-5.c
jana_gtk_example_5_LDADD = ../libjana/libjana.la \
			   ../libjana-ecal/libjana-ecal.la \
			   ../libjana-gtk/libjana-gtk.la

jana_gtk_example_6_SOURCES = jana-gtk-example-6.c
jana_gtk_example_6_LDADD = ../libjana/libjana.la \
			   ../libjana-ecal/libjana-ecal.la \
			   ../libjana-gtk/libjana-gtk.la

jana_gtk_example_7_SOURCES = jana-gtk-example-7.c
jana_gtk_example_7_LDADD = ../libjana/libjana.la \
			   ../libjana-ecal/libjana-ecal.la \
			   ../libjana-gtk/libjana-gtk.la

jana_gtk_example_8_SOURCES = jana-gtk-example-8.c
jana_gtk_example_8_LDADD = ../libjana/libjana.la \
			   ../libjana-ecal/libjana-ecal.la \
			   ../libjana-gtk/libjana-gtk.la

jana_ecal_example_1_SOURCES = jana-ecal-example-1.c
jana_ecal_example_1_LDADD = ../libjana/libjana.la \
			    ../libjana-ecal/libjana-ecal.la

