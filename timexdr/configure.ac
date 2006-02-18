#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(timexdr, 1.0, merka@highsphere.net)
AC_CONFIG_AUX_DIR(config)
#AC_CONFIG_SRCDIR([src/timexdr.c])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB(usb,usb_find_busses)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h strings.h errno.h usb.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_CHECK_FUNCS([localtime_r])

AC_CONFIG_FILES([Makefile
		 doc/Makefile
		 src/Makefile])
AC_OUTPUT
