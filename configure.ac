AC_INIT(runzip, 1.2, vlm@lionet.info)

AC_CONFIG_SRCDIR([src/runzip.c])
AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE(foreign)
AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_HEADER([config.h])
AC_CONFIG_SUBDIRS([libzip-0.7.1])

AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_LIBTOOL

AC_CHECK_HEADERS(iconv.h)
AC_CHECK_LIB(iconv, iconv_open)

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
