## Makefile.am -- Process this file with automake to produce Makefile.in

INCLUDES	= -I$(top_builddir) -I$(top_builddir)/include

bin_PROGRAMS	= timexdr
timexdr_SOURCES = timexdr.c

install-exec-hook:
	chown root $(bindir)/timexdr
	chmod u+s $(bindir)/timexdr
