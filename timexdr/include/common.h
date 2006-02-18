#ifndef TDR_COMMON_H
#define TDR_COMMON_H 1

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>

#if STDC_HEADERS
#  include <stdlib.h>
#  include <string.h>
#elif HAVE_STRINGS_H
#  include <strings.h>
#endif /*STDC_HEADERS*/

#if HAVE_ERRNO_H
#  include <errno.h>
#endif /*HAVE_ERRNO_H*/
#ifndef errno
/* Some systems #define this! */
extern int errno;
#endif

#include <getopt.h>
#include <time.h>

#include <usb.h>

#endif /* !TDR_COMMON_H */
