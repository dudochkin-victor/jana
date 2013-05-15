

localedir = $(datadir)/locale
AM_CPPFLAGS = -DLOCALEDIR=\"$(localedir)\" -DPKGDATADIR=\"$(pkgdatadir)\" $(ECAL_CFLAGS) -Wall -DHANDLE_LIBICAL_MEMORY
AM_LDFLAGS = $(ECAL_LIBS)


bin_PROGRAMS = jana-ecal-event jana-ecal-store-view jana-ecal-time-2 jana-ecal-time

jana_ecal_event_SOURCES = test-jana-ecal-event.c
jana_ecal_event_LDADD = ../libjana/libjana.la ../libjana-ecal/libjana-ecal.la

jana_ecal_store_view_SOURCES = test-jana-ecal-store-view.c
jana_ecal_store_view_LDADD = ../libjana/libjana.la ../libjana-ecal/libjana-ecal.la

jana_ecal_time_2_SOURCES = test-jana-ecal-time-2.c
jana_ecal_time_2_LDADD = ../libjana/libjana.la ../libjana-ecal/libjana-ecal.la

jana_ecal_time_SOURCES = test-jana-ecal-time.c
jana_ecal_time_LDADD = ../libjana/libjana.la ../libjana-ecal/libjana-ecal.la

