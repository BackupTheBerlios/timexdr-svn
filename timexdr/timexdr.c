/* 
 * Timex Data Recorder userspace control utility
 *
 * Copyright (C) 2005 Jan Merka <merka@highsphere.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *      
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *      
 */             

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <getopt.h>

#include <usb.h>
#include "timexdr.h"


static const char *version = "version 0.1, Mar 28, 2005";

int verbose = 0;

static char *progname;
static char *empty=" ";

static void print_session(const struct tdr_session *session);

/*
 * Prints the program usage.
 *
 *      program: program filename
 */
static void timexdr_usage(const char *program, const char *ver) {
  
  fprintf(stderr,
	  "Timex Data Recorder control program %s\n\n"
	  "Usage: %s [COMMAND] [OPTION]...\n"
	  "\nCommands:\n"
	  "\t-a, --all-sessions\tPrint all sessions.\n"
	  "\t-e, --eeprom-dump\tDump the content of EEPROM (for debugging).\n"
	  "\t-h, --help\t\tDisplay this usage information.\n",
	  ver, program);

  exit(EXIT_FAILURE);
}


/*
 * Prints an error message and exits the program.
 */
static void fatal (const char *message) {
  fprintf(stderr, "%s: %s (%m).\n", progname, message);
  exit(EXIT_FAILURE);
}
 
/* 
 * Finds the Timex Data Recorder device.
 */
static int find_timexdr(struct usb_device *tdr) {
  struct usb_bus *bus;
  struct usb_device *dev;
  int count = 0;
  
  for (bus = usb_busses; bus; bus = bus->next) {
    for (dev = bus->devices; dev; dev = dev->next) {
      if ((dev->descriptor.idVendor == TIMEXDR_VENDOR_ID) &&
	  (dev->descriptor.idProduct == TIMEXDR_PRODUCT_ID)) {
	*tdr = *dev;
	count++;
      }
    }
  }
  return count;
}

#define DNAMELEN                50 

/*
 * Opens and initializes the device.
 */ 
static usb_dev_handle *timexdr_open(void) {
  struct usb_dev_handle *udev;
  struct usb_device *dev;
  char vend_name[TIMEXDR_STRLEN], prod_name[TIMEXDR_STRLEN];
  char dname[DNAMELEN];
  int count, ret;

  usb_init();

  usb_find_busses();
  usb_find_devices();
  
  dev = malloc(sizeof(*dev));
 
  if ( (count = find_timexdr(dev)) == 0) {
    fatal("Device not found. Check the connection");
  }

#ifdef TDR_DEBUG
  printf("Found %d device(s).\n", count);
#endif

  if ((udev = usb_open(dev))) {

    ret = usb_get_string_simple(udev, TIMEXDR_STR_VENDOR, vend_name, 
				sizeof(vend_name));
    ret = usb_get_string_simple(udev, TIMEXDR_STR_PRODUCT, prod_name, 
				sizeof(prod_name));

#ifdef TDR_DEBUG
    printf("Vendor:  [0x%04x] %s\nProduct: [0x%04x] %s\n", 
	   dev->descriptor.idVendor, vend_name, 
	   dev->descriptor.idProduct, prod_name);
#endif

    if ( usb_set_configuration(udev, TIMEXDR_CONFIG) < 0 ) {
      fatal("Couldn't set configuration");
    }

    if ( usb_get_driver_np(udev, TIMEXDR_INTERFACE, dname, DNAMELEN) == 0 ) {
      if ( usb_detach_kernel_driver_np(udev, TIMEXDR_INTERFACE) < 0 ) {
	fprintf(stderr, "Couldn't detach kernel driver %s (%m).\n", dname);
	exit(EXIT_FAILURE);
      } 
    }

    if ( usb_claim_interface(udev, TIMEXDR_INTERFACE) < 0 ) {
      fatal("Couldn't claim interface");
    }

  } else {
    fatal("Couldn't open the device. Check your access rights");
  }

  return udev;
}

/* 
 * Releases the interface and closes the device.
 */
static void timexdr_close(usb_dev_handle *dev) {
  int ret;

  if ( (ret = usb_release_interface(dev, 0)) < 0 ) {
    fatal("Couldn't release device");
  }
#ifdef TDR_DEBUG
  printf("release interface: %d\n", ret);
#endif
  usb_close(dev);
}

/*
 * Interrupt read from the device
 */
static int timex_int_read(usb_dev_handle *dev, char *buf, int size, 
			  int timeout) {
  int ret,i;

  errno = 0;
  ret = usb_interrupt_read(dev, TIMEXDR_EP, buf, size, timeout);
  if (ret < 0) {
    /* The return code is often negative when reading the main data from the
     * device. It seems to be related to the length of the timeout. 
     * Try setting it larger.
     */
    //printf("Interrupt read (%d: %m) (%s).\n", errno, usb_strerror());
    fatal("Interrupt read timeout");
  } 

#ifdef TDR_DEBUG
  for (i=0; (ret<10) && (i<ret); i++) {
    printf("%02x ", buf[i] & 0xff);
  }
  printf("\n");
  printf("interrupt read: %d bytes transferred\n", ret);
#endif

  return ret;
}

/*
 * Sends a control message to the device.
 */
static int timex_ctrl(usb_dev_handle *dev, char *ctrldata,  
		      char *buf, int bufsize) {
  int ret;

  ret = usb_control_msg(dev, 
			USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
			USB_REQ_SET_CONFIGURATION, 
			0x201, 
			0, 
			ctrldata, 
			TIMEXDR_CTRL_SIZE, 
			TIMEXDR_CTRL_TIMEOUT);
#ifdef TDR_DEBUG
  printf("---\ntimex_ctrl: %d bytes transferred (%s)\n\t", ret,  
	 (ret < 0) ? usb_strerror() : empty);
#endif

  return (ret < 0) ? ret : 
    timex_int_read(dev, buf, bufsize, TIMEXDR_CTRL_TIMEOUT);
}

/*
 * Calculates number of pages of size "psize" for given number of bytes.
 */
static unsigned long int num_of_pages(const unsigned long int bytes, 
				      const int psize) {
  return bytes / psize
    + ( (bytes % psize) == 0 ? 0 : 1);
}

/* Find how many bytes are used in the TDR's EEPROM for data. Note that
 * the address of last byte as returned by the control command is NOT the
 * total number of bytes used because there is an extra byte 0x02 for 
 * each 255 bytes of data.
 */
static long int eeprom_usage(usb_dev_handle *dev) {
  unsigned char buf[RESPONSE_BUFSIZE];
  unsigned long int bytes;
  int ret;

  ret = timex_ctrl(dev, vendor_ctrl_last_address, buf, RESPONSE_BUFSIZE);
  if (!(ret == 7)) {
    printf("get_mem_usage: response size = %d\n", ret);
    fatal("get_mem_usage: Incorrect response size");
  }

  bytes = TDR_ADDRESS(buf[3], buf[4], buf[5]);

  return bytes + num_of_pages(bytes, DATA_PAGESIZE);
}

/*
 * Dumps the EEPROM to stdout.
 */
static void print_eeprom(unsigned char *buf, unsigned long int pages) {
  unsigned long int i;

  for (i=0; i < (pages*EEPROM_PAGESIZE); i++) {
    if ((i>0) && ((i % EEPROM_PAGESIZE) == 0)) printf("\n");
    if ((i % 16) == 0) printf("\n%08x:\t", i);
    printf("%02x ", buf[i] & 0xff);	
  }
  printf("\n");
}

/*
 * Removes transfer control/status bytes from the received EEPROM data.
 */
static void squeeze_data(unsigned char *databuf, 
			 unsigned long int bytes) {
  unsigned char *newbuf, *pnew, *pold;
  unsigned long int newbytes, bleft = bytes, i, pages;
  
  pages = num_of_pages(bytes, EEPROM_PAGESIZE);
  newbytes = bytes - pages;

  if (!(newbuf = malloc(newbytes))) {
    fatal("Couldn't allocate memory");
  }

  pnew = newbuf;
  pold = databuf + 1;

  for (i=0; i<pages; i++) {
    memcpy(pnew, pold, (bleft > EEPROM_PAGESIZE) ? DATA_PAGESIZE : bleft - 1);
    pnew = pnew + DATA_PAGESIZE;
    pold = pold + EEPROM_PAGESIZE;
    bleft = bleft - EEPROM_PAGESIZE;
  }

  databuf = (unsigned char *)realloc((void *)databuf, newbytes);
  memcpy(databuf, newbuf, newbytes);

  free(newbuf);

  bytes = newbytes;
}

/*
 * Splits the EEPROM data into sessions.
 */
static struct tdr_session *split_data(unsigned char *databuf) {
  struct tdr_session *first, *prev, *next;
  unsigned long int pstart=TIMEXDR_FIRSTSESSION, pend;
  int i;

  first = NULL;
  prev = NULL;

  /* ***FIXME*** There can be possibly 128 sessions but we don't know the
   * maximum EEPROM address (or size of the EEPROM memory). Until we figure 
   * that out, allow only up to 127 sessions.
   */

  for (i=0; 
       (i<(TIMEXDR_ATABLESIZE/TDR_ASIZE - 1)) && 
	 ((pend = TDR_ADDRESS(databuf[TDR_ASIZE*(i+1)], 
			      databuf[TDR_ASIZE*(i+1) + 1], 
			      databuf[TDR_ASIZE*(i+1) + 2])) > 0); 
       i++) {

    next = malloc(sizeof(*next));
    if (!next) {
      fatal("Couldn't allocate memory");
    }

    if (prev) {
      prev->next = next;
    }
    next->prev = prev;
    next->next = NULL;
    next->header.dev = databuf[pstart];
    next->header.year = (unsigned int) TDR_YR(databuf[pstart+6]);
    next->header.month = (unsigned int) TDR_MD(databuf[pstart+5]);
    next->header.day = (unsigned int) TDR_MD(databuf[pstart+4]);
    next->header.hour = (unsigned int) databuf[pstart+3];
    next->header.min = (unsigned int) databuf[pstart+2];
    next->header.sec = (unsigned int) databuf[pstart+1];
    
    next->footer.dev = databuf[pend-7];
    next->footer.year = (unsigned int) TDR_YR(databuf[pend-1]);
    next->footer.month = (unsigned int) TDR_MD(databuf[pend-2]);
    next->footer.day = (unsigned int) TDR_MD(databuf[pend-3]);
    next->footer.hour = (unsigned int) databuf[pend-4];
    next->footer.min = (unsigned int) databuf[pend-5];
    next->footer.sec = (unsigned int) databuf[pend-6];

    next->nbytes = pend - pstart - 14;

    next->data = (unsigned char *)malloc(next->nbytes);
    memcpy(next->data, databuf+pstart+7, next->nbytes); 
    
    pstart = pend;

    if (i == 0) {
      first = next;
    }
    prev = next;
  }

  return (i == 0) ? NULL : first;
}

#define TIME_STR_LENGTH                    9      /* in bytes */
static char time_str[TIME_STR_LENGTH];

/*
 * Converts seconds into a time string 
 */
static void time2str(char *str_time, const unsigned long int seconds) {
  unsigned long int hr, min, sec; 

  hr  = seconds / 3600;
  min = (seconds - hr*3600) / 60;
  sec = seconds % 60;
  sprintf(str_time, "%02u:%02u:%02u", hr, min, sec);

}


/* 
 * Prints session header
 */
static void session_header(char *sname, const struct tdr_header *hdr, 
			   const struct tdr_header *ftr) {
  
  printf("%s: %04u-%02u-%02u %02u:%02u:%02u - %04u-%02u-%02u %02u:%02u:%02u\n",
	 sname, 
	 hdr->year, hdr->month, hdr->day, hdr->hour, hdr->min, hdr->sec,
	 ftr->year, ftr->month, ftr->day, ftr->hour, ftr->min, ftr->sec);

}

/*
 * Prints information about a packet error
 */
static void packet_error(double time, unsigned char token) {
  
  time2str(time_str, time + 0.5);

  switch (token) {
  case MISSING_PACKET:
    printf("%s\tMissing packet.\n", time_str);
    break;
  case CORRUPTED_PACKET:
    printf("%s\tCorrupted packet.\n", time_str);
    break;
  default:
    fatal("Unknown packet error");
    break;
  }

}

/*
 * Prints HRM session data to stdout.
 */
static void hr_session(const struct tdr_session *ses) {
  unsigned long int i;

  session_header("HRM session", &(ses->header), &(ses->footer));
  printf("  Split        bpm\n");

  for (i=0; i < ses->nbytes; i++) {
    switch (ses->data[i]) {
    case MISSING_PACKET:
    case CORRUPTED_PACKET:
      packet_error(i * TIME_STEP_HRM, ses->data[i]);
      break;
    default:
      time2str(time_str, i * TIME_STEP_HRM);
      printf("%s\t%3u\n", time_str, ses->data[i]);
      break;
    }
  }

}

static void gps_packet_1(double *time, unsigned char status_byte, 
			 unsigned char hsb, unsigned char lsb_speed, 
			 unsigned char lsb_odo) {
  
  unsigned char status, acq, battery;
  double speed, dist; 

  status = ( status_byte & 0xf0 ) >> 4;
  acq = ( status_byte & 0x0c) >> 2;
  battery = ( status_byte & 0x03 );
  speed = (double)((long int) lsb_speed + 
		   ( ((long int) (hsb & 0xf0)) << 4 )) * SPEED_UNIT;
  dist = (double)((long int) lsb_odo +
		  ( ((long int) (hsb & 0x0f)) << 8 )) * DIST_UNIT;

  time2str(time_str, *time + 0.5);
  printf("%s\t0x%x\t0x%x\t0x%x\t%5.1f\t%9.3f\n", 
	 time_str, status, acq, battery, speed, dist);

  *time += TIME_STEP_GPS;
}

/*
 * Prints GPS session data to stdout. 
 */
static void gps_session(const struct tdr_session *ses) { 
  unsigned long int i, psize, bytes = ses->nbytes;
  double time = 0.0;
  
  if ( ses->nbytes < GPS_PACKET_MIN_LENGTH ) {
    fprintf(stderr, "Skipping GPS session: Packet too short.");
    return;
  }

  session_header("GPS session", &(ses->header), &(ses->footer));
 
  for (i=0; (i < bytes) && 
	 (psize = (ses->data[i] & PACKET_LENGTH_MASK)) <= (bytes - i); 
       i += psize) {
    
/*     printf(" ses->data[%d] = %02x\n", i, ses->data[i]); */

    switch (ses->data[i]) {
    case PACKET_TYPE_ERROR:
      packet_error(time, ses->data[i+1]);
      time += TIME_STEP_GPS;
      break;
    case PACKET_TYPE_1:
      gps_packet_1(&time, ses->data[i+1], ses->data[i+2], 
		   ses->data[i+3], ses->data[i+4]);
      break;
    case PACKET_TYPE_4:
 /*      gps_packet_4(&time, data[i+1], data[i+2], data[i+3], data[i+4]); */
      time += TIME_STEP_GPS; /* ***REMOVE*** (should be in gps_packet_4) */
      break; 
    case PACKET_TYPE_2:
    case PACKET_TYPE_3:
    case PACKET_TYPE_5:
    case PACKET_TYPE_15:
    default:
/*       printf("This GPS packet handling not implemented yet: %02x\t%d:\%d\n",  */
/* 	     ses->data[i], i, bytes - i); */
/*       printf("\t%02x %02x %02x %02x %02x\n", ses->data[i],  */
/* 	     ses->data[i+1], ses->data[i+2],  */
/* 	     ses->data[i+3], ses->data[i+4]); */
      fatal("This GPS packet handling not implemented yet");
      break;
    }
  }
  
}

/*
 * Process and print a multi-device session. First split the multi-device
 * session into HRM and GPS sessins.
 */
static void multi_session(const struct tdr_session *session) {
  struct tdr_session *hrm_ses, *gps_ses;
  unsigned long int i=0, j, plen;

  hrm_ses = malloc(sizeof(*hrm_ses));
  gps_ses = malloc(sizeof(*gps_ses));

  hrm_ses->prev = (hrm_ses->next = NULL);
  gps_ses->prev = (gps_ses->next = NULL);

  hrm_ses->header = (gps_ses->header = session->header);
  hrm_ses->footer = (gps_ses->footer = session->footer);
  hrm_ses->header.dev = (hrm_ses->footer.dev = HRM_SESSION);
  gps_ses->header.dev = (gps_ses->footer.dev = GPS_SESSION);
  
  hrm_ses->nbytes = (gps_ses->nbytes = 0);
  
  hrm_ses->data = malloc(session->nbytes);
  gps_ses->data = malloc(session->nbytes);
  
  
  while (1) {
    switch (session->data[i]) {

    case HRM_SESSION:
      hrm_ses->data[hrm_ses->nbytes++] = session->data[i+1];
      i += 2;
      break;

      /* GPS session seems to be identified differently in multi-dev mode */
    case GPS_SESSION+1:
      if (session->data[i+1] == PACKET_TYPE_15) {
	plen = PACKET_TYPE_15_LENGTH;
      } else {
	plen = session->data[i+1] & PACKET_LENGTH_MASK;
      }
      for (j=0; j<plen; j++) {
	gps_ses->data[gps_ses->nbytes + j] = session->data[i+1+j];
      }
      gps_ses->nbytes += plen;
      i += plen + 1;
      break;

    default:
      fatal("Unknown device in multi-device session");
      break;
    }
    
    if (i >= session->nbytes ) break;
  }

  print_session(hrm_ses);
  print_session(gps_ses);

  free(hrm_ses->data);
  free(hrm_ses);
  free(gps_ses->data);
  free(gps_ses);
}


/*
 * Prints session data.
 */
static void print_session(const struct tdr_session *session) {
  struct tdr_session *ses;
  int i;

  for (ses = session; ses;  ses = ses->next) {

    switch (ses->header.dev & SESSION_MASK) {
    case HRM_SESSION:
      hr_session(ses);
      break;
    case GPS_SESSION:
      gps_session(ses);
/*       for (i=0; i<ses->nbytes; i++) { */
/* 	if ((i % 16) == 0 ) printf("\n"); */
/* 	printf(" %02x", ses->data[i]); */
/*       } */
	
      break;
    case MULTI_DEVICE_SESSION:
      multi_session(ses);
/*       printf("Multi-device (HRM+GPS) session skipped.\n"); */
      break;
    default:
      errno = 0;
      fatal("Unknown session");
      break;
    }
    
  }
}

/* -------------------------------------------------------------------------
 *   Main program.
 * -------------------------------------------------------------------------
 */
int main(int argc, char *argv[])
{
  struct usb_dev_handle *dev;
  int i,j,k;
  unsigned long int bytes, bufsize, timeout;
  unsigned char buf[RESPONSE_BUFSIZE], *databuf;
  char c, choice='h';
  struct tdr_session  *session;
  static struct option long_options[] = {
    {"all-sessions", 0, NULL, 'a'},
    {"eeprom-dump", 0, NULL, 'e'},
    {"help",  0, NULL, 'h'},
    {NULL, 0, NULL, 0}
  };
  
  progname = argv[0];
  if (argc > 1 && !strcmp(argv[1], "-v"))
    verbose = 1;

  while (1) {
    c = getopt_long(argc, argv, "aeh",
		    long_options, NULL);

    if (c == -1) {
      break;
    }
    
    switch (c) {
    case 'a':
    case 'e':
    case 'h':
      choice = c;
      break;
    default:
      timexdr_usage(argv[0], version);
      break;
    }
  }

  switch (choice) {

  case 'a':
  case 'e':
    dev = timexdr_open();
    i = timex_ctrl(dev, vendor_ctrl_1, buf, RESPONSE_BUFSIZE);
    i = timex_ctrl(dev, vendor_ctrl_1, buf, RESPONSE_BUFSIZE);
    i = timex_ctrl(dev, vendor_ctrl_2, buf, RESPONSE_BUFSIZE);
    bytes = eeprom_usage(dev);

#ifdef TDR_DEBUG
    printf("bytes: %u 0x%x\tpages: %u\n", bytes, bytes, 
	   num_of_pages(bytes, EEPROM_PAGESIZE));
#endif

    i = timex_ctrl(dev, vendor_ctrl_4, buf, RESPONSE_BUFSIZE);
    i = timex_ctrl(dev, vendor_ctrl_read_memory, buf, RESPONSE_BUFSIZE);

    bufsize = EEPROM_PAGESIZE * num_of_pages(bytes, EEPROM_PAGESIZE); 
    databuf = (char *)calloc(bufsize, sizeof(char));
    if (databuf == 0) fatal("databuf not initialized");
   
    /* The timeout may be too large but that is better than smaller
     * because this way we are sure that all data get tranferred.
     */
    timeout = (bytes / 2048 + 1) * 20 * TIMEXDR_CTRL_TIMEOUT;

#ifdef TDR_DEBUG
    printf("timeout: %d ms\n", timeout);
#endif

    i = timex_int_read(dev, databuf, bufsize, timeout);
    
    switch (choice) {
    case 'e': 
      print_eeprom(databuf, num_of_pages(bytes, EEPROM_PAGESIZE));
      break;
    case 'a':
      squeeze_data(databuf, bytes);
      session = split_data(databuf);
      print_session(session);
      break;
    default:
      break;
    }
    
    timexdr_close(dev);
    break;

  default:
    timexdr_usage(argv[0], version);
    break;
  }

  return 0;
}

