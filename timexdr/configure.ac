#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(timexdr, 1.11, merka@highsphere.net)
AC_CONFIG_AUX_DIR(config)
#AC_CONFIG_SRCDIR([src/timexdr.c])
AC_CONFIG_SRCDIR([include/config.h.in])

AH_TOP(
#undef LINUX_API
#undef DARWIN_API
#undef BSD_API
)
AC_CONFIG_HEADERS([include/config.h])

AC_CANONICAL_HOST()

AM_INIT_AUTOMAKE

LINUX_API=0
DARWIN_API=0
BSD_API=0

AC_MSG_CHECKING(for what USB OS support)
case $host in
  *-linux*) 
  	AC_DEFINE(LINUX_API, 1)
	AC_DEFINE(BSD_API, 0)
	AC_DEFINE(DARWIN_API, 0)
	LINUX_API=1
	os_support=linux
	AC_MSG_RESULT(Linux)
	OSLIBS=""
	;;
  *-freebsd*|*-openbsd*|*-netbsd*)
	AC_DEFINE(BSD_API, 1)
	AC_DEFINE(LINUX_API, 0)
	AC_DEFINE(DARWIN_API, 0)
	BSD_API=1
	os_support=bsd
	AC_MSG_RESULT(FreeBSD, OpenBSD and/or NetBSD)
	OSLIBS=""
	;;
  *-darwin*)
	AC_DEFINE(DARWIN_API, 1)
	AC_DEFINE(LINUX_API, 0)
	AC_DEFINE(BSD_API, 0)
	DARWIN_API=1
	os_support=darwin
	AC_MSG_RESULT(Darwin and/or MacOS 10)
	OSLIBS="-Wl,-framework -Wl,IOKit -Wl,-framework -Wl,CoreFoundation -Wl,-prebind -no-undefined"
	;;
  *)
	AC_MSG_RESULT(unknown operating system)
	AC_MSG_ERROR(timexdr does not support compiling for $host)
	;;
esac

AC_SUBST(DARWIN_API)
AC_SUBST(LINUX_API)
AC_SUBST(BSD_API)

AM_CONDITIONAL(LINUX_API, test "$os_support" = "linux")
AM_CONDITIONAL(BSD_API, test "$os_support" = "bsd")
AM_CONDITIONAL(DARWIN_API, test "$os_support" = "darwin")

AC_SUBST(OSLIBS)

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
		 include/Makefile
		 src/Makefile])
AC_OUTPUT
