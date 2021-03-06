#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT([vde3], [0.1], [BUG-REPORT-ADDRESS])
AC_CONFIG_SRCDIR([src/context.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# IMPORTANT: read before touching
# http://www.gnu.org/software/libtool/manual/libtool.html#Updating-version-info
LIBVDE_VERSION=0:0:0
AC_SUBST([LIBVDE_VERSION])

# foreign doesn't require GNU standard files to be present
# http://www.gnu.org/software/automake/manual/automake.html#Strictness
AM_INIT_AUTOMAKE([dist-bzip2 subdir-objects foreign])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL

# Checks for mandatory dependencies
VDE_CHECK_LIB_HEADER([event], [event_init], [event.h], ,
                     AC_MSG_ERROR([Could not find libevent]))

PKG_CHECK_MODULES(GLIB, glib-2.0, ,
                  AC_MSG_ERROR([Could not find glib 2.0]))

PKG_CHECK_MODULES(JSONC, json, ,
                  AC_MSG_ERROR([Could not find json-c]))

# check for python and python-simplejson used by gen_checker.py
AC_PATH_PROG(PYTHON, [python], AC_MSG_ERROR([Could not find python]))
AC_SUBST([PYTHON])

AC_MSG_CHECKING([for python-simplejson module])

$PYTHON -c 'import simplejson' >/dev/null 2>&1
AS_IF([test "x$?" == "x0"], AC_MSG_RESULT([found]),
                            AC_MSG_RESULT([not found])
                            AC_MSG_ERROR([Could not find python-simplejson]))

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h stdint.h stdlib.h string.h sys/socket.h \
                  sys/time.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_CHECK_FUNCS([memchr mkdir rmdir socket strdup strerror strndup])

VDE_CFLAGS="-Wall"
# consider also these warnings
# VDE_CFLAGS="$VDE_CFLAGS -Winline -Wformat=2 -Wswitch-default -Wswitch-enum \
# -Wunused-parameter -Wfloat-equal -Wundef -Wshadow -Wunsafe-loop-optimizations \
# -Wtype-limits -Wcast-qual -Wcast-align -Wconversion -Wno-sign-conversion \
# -Wempty-body -Wsign-compare -Wlogical-op -Waggregate-return \
# -Wstrict-prototypes -Wold-style-declaration -Wold-style-definition \
# -Wmissing-parameter-type -Wmissing-prototypes -Wmissing-declarations \
# -Wmissing-noreturn -Wmissing-format-attribute -Wpacked -Wunreachable-code \
# -Wnested-externs -Wdisabled-optimization -Wunused -Wunused-parameter"

AC_ARG_ENABLE(debug,
  AS_HELP_STRING([--enable-debug],
                 [compile with debug checks and symbols (yes)]),
  [enable_debug=$enableval],
  [enable_debug=yes])
if test x$enable_debug = xyes; then
  VDE_CPPFLAGS="$VDE_CPPFLAGS -DVDE3_DEBUG"
  VDE_CFLAGS="$VDE_CFLAGS -ggdb"
  CFLAGS="" # reset CFLAGS otherwise defaults to -g -O2
else
  VDE_CFLAGS="$VDE_CFLAGS -O2"
fi

# optional check for check
PKG_CHECK_MODULES([CHECK], [check >= 0.9.4], [have_check=yes], [have_check=no])
AM_CONDITIONAL(CHECK, [test x$have_check = xyes])

# optional check for valgrind
AC_CHECK_HEADERS([valgrind/valgrind.h], have_valgrind_h=yes,
                 have_valgrind_h=no)
AC_PATH_PROG(VALGRIND, [valgrind], [no])
AC_SUBST([VALGRIND])
AM_CONDITIONAL(VALGRIND, [test x$VALGRIND != xno])

# optional check for doxygen
AC_PATH_PROG(DOXYGEN, [doxygen], [no])
AC_SUBST([DOXYGEN])
AM_CONDITIONAL(DOXYGEN, [test x$DOXYGEN != xno])

AC_SUBST([VDE_CPPFLAGS])
AC_SUBST([VDE_CFLAGS])

AC_CONFIG_FILES([Makefile doc/Doxyfile])
AC_OUTPUT
