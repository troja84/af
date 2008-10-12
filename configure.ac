AC_PREREQ(2.52)
AC_INIT(animation-framework, 0.0.1)
AC_CONFIG_SRCDIR(af/af-timeline.c)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

AM_CONFIG_HEADER(config.h)

AM_MAINTAINER_MODE

AM_PROG_LIBTOOL

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AM_PATH_GLIB_2_0

# libtool versioning
#
# See http://sources.redhat.com/autobook/autobook/autobook_91.html#SEC91 for details
#
LT_CURRENT=0
LT_REVISION=0
LT_AGE=0
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)

GTK_REQUIRED=2.14.0

dnl =====================================================
dnl required packages detection
dnl =====================================================
PKG_CHECK_MODULES(AF, [
		  gtk+-2.0    >= $GTK_REQUIRED
		  ])

AC_SUBST(AF_LIBS)
AC_SUBST(AF_CFLAGS)

AC_OUTPUT([
Makefile
af/Makefile
data/Makefile
data/animation-framework-1.0.pc
tests/Makefile
])
