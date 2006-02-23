## Makefile.am -- Process this file with automake to produce Makefile.in

INCLUDES	= -I$(top_builddir) -I$(top_builddir)/include

#noinst_HEADERS	= timexdr.h common.h

if DARWIN_API
LDADDS = -Wl,-framework -Wl,IOKit -Wl,-framework -Wl,CoreFoundation -Wl,-prebind -no-undefined
endif

bin_PROGRAMS	= timexdr
timexdr_SOURCES = timexdr.c

#EXTRA_DIST = 
