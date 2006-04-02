#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(timexdr, 1.20, merka@highsphere.net)
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
	#OSLIBS="-Wl,-framework -Wl,IOKit -Wl,-framework -Wl,CoreFoundation"
	OSLIBS=""

        # It may be called "cc", but it's really a GCC derivative
        # with a problematic special precompiler and precompiled
        # headers; turn off the special precompiler, as some
        # apparently-legal code won't compile with its precompiled
        # headers.			
	CFLAGS="-no-cpp-precomp $CFLAGS"
	LIBS="$LIBS -framework IOKit -framework CoreFoundation"
	AC_MSG_RESULT(Aplle GCC - added -no-cpp-precomp)
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

# Declare --enable-* args and collect ac_help strings
AC_ARG_ENABLE(static, AC_HELP_STRING([--enable-static], 
	      [build a statically linked executable (default is no)]),,
	      [enable_static=no])

if test "x$enable_static" = "xyes"; then
	CFLAGS="$CFLAGS -static"
fi

# Enable plotting scripts by default
AC_ARG_ENABLE(plot-scripts, AC_HELP_STRING([--enable-plot-scripts],
	      [enable plotting scripts (default is yes)]),,
	      [enable_plot_scripts=yes])
if test "x$enable_plot_scripts" == "xyes"; then
	SCRIPTS_DIR="$datadir/$PACKAGE_NAME/scripts"
fi
AC_SUBST(SCRIPTS_DIR)

# Checks for programs.
AC_PROG_CC

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h strings.h errno.h usb.h math.h])

# Checks for libraries.
AC_CHECK_LIB([usb], [usb_init],,
	     AC_MSG_ERROR(*** libusb required. Linking -lusb failed.))
AC_CHECK_LIB([m], [fmod],,
	     AC_MSG_ERROR(*** libm with function fmod() required. Linking -lm failed.))
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
		 scripts/Makefile
		 scripts/plot_timex_data
		 src/Makefile])
AC_OUTPUT
