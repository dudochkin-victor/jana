AC_PREREQ(2.53)
AC_INIT(jana, 0.0, http://www.openedhand.com/)
AM_INIT_AUTOMAKE()
AC_CONFIG_SRCDIR(libjana/jana-component.c)
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_LIBTOOL
IT_PROG_INTLTOOL([0.35.0])

dnl i18n support
GETTEXT_PACKAGE=Jana
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext package])
AC_SUBST(GETTEXT_PACKAGE)
AM_GLIB_GNU_GETTEXT

AC_CHECK_LIB(m, pow)

PKG_CHECK_MODULES(GOBJECT, gobject-2.0 >= 2.12.0)
PKG_CHECK_MODULES(GTHREAD, gthread-2.0)
PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.6.0)
PKG_CHECK_MODULES(ECAL, libecal-1.2 >= 1.4.0)
PKG_CHECK_MODULES(EDATASERVERUI, libedataserverui-3.0)

PKG_CHECK_MODULES(GCONF, gconf-2.0)

old_cflags=$CFLAGS
CFLAGS=$ECAL_CFLAGS
AC_CHECK_TYPE(ECalComponentId, [], [], [#include <libecal/e-cal.h>])

if test $ac_cv_type_ECalComponentId = yes; then
        AC_DEFINE(HAVE_ECALCOMPONENTID, 1, [Defined if ECalComponentId exists])
fi
CFLAGS=$old_cflags

old_libs=$LIBS
LIBS=$ECAL_LIBS
AC_CHECK_FUNC(e_cal_new_system_memos, AC_DEFINE(HAVE_ECAL_NEW_SYSTEM_MEMOS, 1, [Defined if e_cal_new_system_memos exists]))
LIBS=$old_libs

AC_ARG_ENABLE([tests],
	AC_HELP_STRING([--enable-tests],
		[Enable building of test applications]),
		[build_tests=tests])

AC_ARG_ENABLE([examples],
	AC_HELP_STRING([--enable-examples],
		[Enable building of example applications]),
		[build_examples=examples])

AC_ARG_ENABLE([glade],
	AC_HELP_STRING([--enable-glade],
		[Enable glade-3 development support]),
		[enable_glade=yes])

if test "$enable_glade" = yes; then
	PKG_PROG_PKG_CONFIG()
	AC_DEFINE(HAVE_GLADE, 1, [Defined when building with glade-3 support])
	PKG_CHECK_MODULES(GLADE, gladeui-1.0 >= 3.4.5)
	catalogdir=`${PKG_CONFIG} --variable=catalogdir gladeui-1.0`
fi

AM_CONDITIONAL(WITH_GLADE, test "$enable_glade" = yes)

AC_SUBST([build_tests])
AC_SUBST([build_examples])
AC_SUBST([catalogdir])

GTK_DOC_CHECK([1.4])

AC_OUTPUT([
Makefile
libjana/Makefile
libjana/libjana.pc
libjana/doc/Makefile
libjana/doc/reference/Makefile
libjana/doc/reference/version.xml
libjana-ecal/Makefile
libjana-ecal/libjana-ecal.pc
libjana-ecal/doc/Makefile
libjana-ecal/doc/reference/Makefile
libjana-ecal/doc/reference/version.xml
libjana-gtk/Makefile
libjana-gtk/libjana-gtk.pc
libjana-gtk/data/Makefile
libjana-gtk/doc/Makefile
libjana-gtk/doc/reference/Makefile
libjana-gtk/doc/reference/version.xml
tests/Makefile
examples/Makefile
examples/data/Makefile
po/Makefile.in
])
