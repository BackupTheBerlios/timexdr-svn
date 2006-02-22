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

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "common.h"
#include "timexdr.h"


//static const char *version = "version 1.0, April 14, 2005";
static const char *version = "version " VERSION;

int verbose = 1;

/* Odometer quirks: 
 *   dist_offset - to eliminate non-zero session offset
 *   dist_prev   - remember the previous value in order to control
 *                 rollovers at ODO_MAX (4.096 miles) and to disallow
 *                 any decrease in distance
 *   dist_base   - increment this on each rollover by ODO_MAX
 */
static double dist_offset = -1, dist_prev = -1, dist_base = 0;

static char *progname;

static void print_session(const struct tdr_session *session);

/*
 * Print version information and exit
 */
static void timexdr_version(void) {
  fprintf(stdout, PACKAGE " " VERSION "\n");
  exit(EXIT_SUCCESS);
}

/*
 * Prints the program usage.
 *
 *      program: program filename
 */
static void timexdr_usage(const char *program) {
  
  fprintf(stderr,
	  "Timex Data Recorder control program "
          "version " VERSION "\n\n"
	  "Usage: %s [COMMAND] [OPTION]...\n"
	  "\nCommands:\n"
	  "  -a, --all-sessions\tPrint all sessions.\n"
	  "  -c, --clear-eeprom\tClear the EEPROM memory (delete all stored sessions).\n"
	  "  -d NUM, --days=NUM\tPrint sessions recorded within the last NUM days.\n"
	  "\t\t\tIf NUM is omitted or zero, today's sessions are printed.\n"
	  "  -e, --eeprom-dump\tDump the content of EEPROM (for debugging).\n"
	  "  -f, --file\t\tCreate file(s) YYYYMMDD_HHMMSS-HHMMSS.{gps,hrm} for the\n"
	  "\t\t\tsession data in the working directory.\n" 
	  "  -h, --help\t\tDisplay this usage information.\n"
	  "  -m, --miles\t\tShow distance and speed in miles and mph, respectively.\n"
	  "\t\t\tThe default units are kilometers and kph.\n"
	  "  -t, --time-sync\tSynchronize device's clock with system local time.\n"
	  "  -V, --version\t\tPrint version information and exit.\n", 
	  program);

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
	  ((dev->descriptor.idProduct == TIMEXDR_PRODUCT_ID1) ||
	   (dev->descriptor.idProduct == TIMEXDR_PRODUCT_ID2))) {
	*tdr = *dev;
	count++;
      }
    }
  }
  return count;
}

/* Maximum length of the vendor/product strings */
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

/* This started causing problems (fails every time) some time between the 
 * release of timexdr 1.0 and February 2006. Probably due to changes in 
 * kernel usb drivers. On the other hand, it doesn't appear to be necessary
 * anymore
    if ( usb_set_configuration(udev, TIMEXDR_CONFIG) < 0 ) {
      fatal("Couldn't set configuration");
      }
 */

/* Non-portable Linux-only code */
#ifdef LIBUSB_HAS_GET_DRIVER_NP
    if ( usb_get_driver_np(udev, TIMEXDR_INTERFACE, dname, DNAMELEN) == 0 ) {
      if ( usb_detach_kernel_driver_np(udev, TIMEXDR_INTERFACE) < 0 ) {
	fprintf(stderr, "Couldn't detach kernel driver %s (%m).\n", dname);
	exit(EXIT_FAILURE);
      } 
    } /* Don't stop here if usb_get_driver_np fails because that only 
	 means there is no kernel driver to be detached */
#endif

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
  int ret;

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
  printf("timex_int_read: %d bytes transferred\n\t", ret);
  {
    int i;

    for (i=0; (ret<10) && (i<ret); i++) {
      printf("%02x ", buf[i] & 0xff);
    }
    printf("\n");
  }
#endif

  return ret;
}

/*
 * Prepare the control command (output report) to be sent to the device
 * Inputs: cmdtype - specify command type as defined in the header file
 *         micro - optional, specify which microcontroller is the recipient
 */

static void prepare_cmd(char cmdtype, char micro) {
  int n = 1;   // number of bytes to send
  int cs= 0;   // checksum

  /* Report number */
  ctrl_cmd[0] = 0x01;
  /* Most of the commands send only 1 byte so this is a good default */
  ctrl_cmd[2] = ENC_CMD_TYPE(cmdtype, n); 

  switch (cmdtype) {
  case EEPROM_USAGE:
    ctrl_cmd[1] = CMD_EEPROM_USAGE; 
    break;
  case EEPROM_CLEAR:
  case UPLOAD_CANCEL:
  case UPLOAD_DONE:
  case SW_TIMEOUT:
    /* Command 0x40 */
    ctrl_cmd[1] = CMD_EEPROM_CLEAR;
    break;
  case DATA_UPLOAD:
    ctrl_cmd[1] = CMD_DATA_UPLOAD;
    break;
  case FW_VERSION:
    /* Default microcontroller is MAIN_MICRO */
    ctrl_cmd[1] = (micro == USB_MICRO) ? 
      CMD_FW_VERSION_USB_MICRO : CMD_FW_VERSION_MAIN_MICRO;
    break;
  case EEPROM_CAPACITY:
    ctrl_cmd[1] = CMD_EEPROM_CAPACITY;
    break;
  default:
    fatal("prepare_cmd: Unknown command type");
    break;
  }

  /* Calculate the checksum */
  cs =+ (int) ctrl_cmd[2];
  ctrl_cmd[n+2] = ( ~cs + 1 ) % 256;

  /* Fill up the rest of ctrl_cmd with zeros */
  /* Not needed for functionality but convenient for debugging */
  { 
    int i;
    for (i=n+3; i<TIMEXDR_CTRL_SIZE; i++) {
      ctrl_cmd[i] = 0x0; 
    }
  }
}

/*
 * Sends a control message of cmdtype to the device's microcontroller micro.
 */
static int timex_ctrl2(usb_dev_handle *dev, char cmdtype, char micro,  
		      char *buf, int bufsize) {
  int ret;

  /* Prepare the control message (output report) */
  prepare_cmd(cmdtype, micro);

  /* Send the report */
  ret = usb_control_msg(dev, 
			USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
			USB_REQ_SET_CONFIGURATION, 
			0x201, 
			0, 
			ctrl_cmd, 
			TIMEXDR_CTRL_SIZE, 
			TIMEXDR_CTRL_TIMEOUT);
#ifdef TDR_DEBUG
  printf("---\ntimex_ctrl: %d bytes transferred (%s)\n\t", ret,  
	 (ret < 0) ? usb_strerror() : NULL);
  if (ret > 0) {
    {
      int i;

      for (i=0; i<ret; i++) {
	printf("%02x ", ctrl_cmd[i] & 0xff);
      }
      printf("\n");
    }
  }
#endif

  return (ret < 0) ? ret : 
    timex_int_read(dev, buf, bufsize, TIMEXDR_CTRL_TIMEOUT);
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
	 (ret < 0) ? usb_strerror() : NULL);
  if (ret > 0) {
    {
      int i;

      for (i=0; i<ret; i++) {
	printf("%02x ", ctrldata[i] & 0xff);
      }
      printf("\n");
    }
  }
#endif

  return (ret < 0) ? ret : 
    timex_int_read(dev, buf, bufsize, TIMEXDR_CTRL_TIMEOUT);
}

/*
 * Get the firmware version of the device's main microcontroller 
 */
static struct tdr_fw_ver get_fw_version(usb_dev_handle *dev) {
  char buf[RESPONSE_BUFSIZE];
  struct tdr_fw_ver fw;

  if (!( timex_ctrl2(dev, FW_VERSION, MAIN_MICRO, buf, RESPONSE_BUFSIZE) == 7)) {
    fatal("get_fw_version: Incorrect response size");
  }
  fw.main = DEC_FW_VER(buf[3], buf[4], buf[5]);

  if (!( timex_ctrl2(dev, FW_VERSION, USB_MICRO, buf, RESPONSE_BUFSIZE) == 7)) {
    fatal("get_fw_version: Incorrect response size");
  }
  fw.usb = DEC_FW_VER(buf[3], buf[4], buf[5]);
  /*  (buf[5] >> 4) * 100000 + (buf[5] & 0x0F) * 10000 +
    (buf[4] >> 4) * 1000 + (buf[4] & 0x0F) * 100 +
    (buf[3] >> 4) * 10 + (buf[3] & 0x0F);
  */
  return fw;
}

/*
 * Synchronize device's clock with PC (use localtime)
 */
static int timex_sync_time(usb_dev_handle *dev, char *buf, int bufsize) {
  struct tm *sys_time;
  time_t tp;

  tp = time(NULL);
  sys_time = localtime(&tp);

  vendor_ctrl_set_time[3] = (char) sys_time->tm_sec;
  vendor_ctrl_set_time[4] = (char) sys_time->tm_min;
  vendor_ctrl_set_time[5] = (char) sys_time->tm_hour;
  vendor_ctrl_set_time[6] = (char) (sys_time->tm_mday - 1);
  vendor_ctrl_set_time[7] = (char) sys_time->tm_mon;
  vendor_ctrl_set_time[8] = (char) (sys_time->tm_year - 100);
  vendor_ctrl_set_time[9] = TIME_CHECKSUM - (
    vendor_ctrl_set_time[3] + vendor_ctrl_set_time[4] +
    vendor_ctrl_set_time[5] + vendor_ctrl_set_time[6] +
    vendor_ctrl_set_time[7] + vendor_ctrl_set_time[8] );
  //printf("checksum: 0x%x\n", vendor_ctrl_set_time[9]);
  
  return timex_ctrl(dev, vendor_ctrl_set_time, buf, bufsize);
}

/*
 * Set the initial time for session download. The parameter is number
 * of days before now. The initial time is (now - days) adjusted
 * down to the midnight.
 */
static void set_initial_time(time_t days) {
  struct tm btime;

  initial_time = time(NULL) - days*86400;
  localtime_r(&initial_time, &btime);
  btime.tm_sec = 0;
  btime.tm_min = 0;
  btime.tm_hour = 0;
  initial_time = mktime(&btime);
}

/*
 * Return 1 if the session time stamp is newer than initial_time
 */
static int newer_session(const struct tdr_header *ses_time) {
  struct tm btime;
  time_t sys_time;

  if (initial_time == 0) {
    return 1;
  }
  sys_time = time(NULL);
  localtime_r(&sys_time, &btime); 
  btime.tm_sec = ses_time->sec;
  btime.tm_min = ses_time->min;
  btime.tm_hour = ses_time->hour;
  btime.tm_mday = ses_time->day;
  btime.tm_mon = ses_time->month - 1;
  btime.tm_year = ses_time->year - 1900;
  
  return mktime(&btime) >= initial_time;
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
 * total number of bytes used because there is an extra byte 0x02 at the 
 * beginning of each 255 bytes of data.
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
    if ((i % 16) == 0) printf("\n%08lx:\t", i);
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
  sprintf(str_time, "%02lu:%02lu:%02lu", hr, min, sec);

}

/* 
 * Prints session header
 */
static void session_header(char *sname, const struct tdr_header *hdr, 
			   const struct tdr_header *ftr) {
  
  if (fprintf(sfp, "%s: %04u-%02u-%02u %02u:%02u:%02u - "
	      "%04u-%02u-%02u %02u:%02u:%02u\n",
	      sname, 
	      hdr->year, hdr->month, hdr->day, hdr->hour, hdr->min, hdr->sec,
	      ftr->year, ftr->month, ftr->day, ftr->hour, ftr->min, ftr->sec)
      < 0) {
    fatal("Error writing to a file");
  }
}

/*
 * Prints information about a packet error
 */
static void packet_error(double split_time, unsigned char token) {
  
  time2str(time_str, split_time + 0.5);

  switch (token) {
  case MISSING_PACKET:
    if (fprintf(sfp, "%s\tMissing packet.\n", time_str) < 0) {
      fatal("Error writing to a file");
    }
    break;
  case CORRUPTED_PACKET:
    if (fprintf(sfp, "%s\tCorrupted packet.\n", time_str) < 0) {
      fatal("Error writing to a file");
    }
    break;
  default:
    fatal("Unknown packet error");
    break;
  }

}

/*
 * Open output file for a session
 */
static void open_session_file(char *sname, 
			      const struct tdr_header *hdr, 
			      const struct tdr_header *ftr) {
  char s[TIMEXDR_STRLEN];

  if (write_session_to_file) {
    sprintf(s, "%04u%02u%02u_%02u%02u%02u-%02u%02u%02u.%s",   
	    hdr->year, hdr->month, hdr->day, hdr->hour, hdr->min, hdr->sec,
	    ftr->hour, ftr->min, ftr->sec, sname);
  
#ifdef TDR_DEBUG
    printf("file name: %s\tsession: %s\n", s,sname);
#endif

    if ((sfp = fopen(s, "w")) == NULL) {
      fprintf(stderr, "%s: Can't open session file %s (%m).\n", progname, s);
      exit(EXIT_FAILURE);
    } 
  } else {
    sfp = stdout;
  }
}

/*
 * Close the output session file
 */
static void close_session_file(void) {
  if (write_session_to_file) {
    fclose(sfp);
  } 
}

/*
 * Prints HRM session data to stdout.
 */
static void hr_session(const struct tdr_session *ses) {
  unsigned long int i;

  open_session_file(HRM_FILE_EXT, &(ses->header), &(ses->footer));

  session_header("HRM session", &(ses->header), &(ses->footer));
  if (fprintf(sfp, "  Time        HR[bpm]\n") < 0) {
    fatal("Error writing to a file");
  }

  for (i=0; i < ses->nbytes; i++) {
    switch (ses->data[i]) {
    case MISSING_PACKET:
    case CORRUPTED_PACKET:
      packet_error(i * TIME_STEP_HRM, ses->data[i]);
      break;
    default:
      time2str(time_str, i * TIME_STEP_HRM);
      if (fprintf(sfp, "%s\t%3u\n", time_str, ses->data[i]) < 0) {
	fatal("Error writing to a file");
      }
      break;
    }
  }

  close_session_file();
}

/*
 * Convert distance units
 */
static double unit_conv(const double dist) {
  return (dist_units == 0) ? dist : MILES_TO_KM(dist); 
}

/*
 * Adjust the distance for odometer quirks
 */
static void dist_corrections(double *dist){
  
  /* First add any rollovers */
  *dist += dist_base;

  if (dist_offset < 0) {
    dist_offset = *dist;
  }

  /* Remove session offset */
  *dist -= dist_offset;

  /* Check for a rollover: We assume that the distance can fall back
   * by more than 0.5 * ODO_MAX only when a rollover occured.
   */
  if ((dist_prev - *dist) > (ODO_MAX * 0.5)) {
    *dist += ODO_MAX;
    dist_base += ODO_MAX;
  }
  
  /* Don't allow a decrease in distance. Such decrease can happen when
   * the GPS suddenly stops because it normally anticipates where its 
   * location will be at the time of packet transmission. After a sudden
   * stop, it may need to correct, i.e. decrease, the distance. */
  if (*dist < dist_prev) {
    *dist = dist_prev;
  } else {
    dist_prev = *dist;
  }
}

/*
 * Decodes and prints GPS packet type 1 (Status, Speed and Distance)
 */
static void gps_packet_1(double *split_time, unsigned char status_byte, 
			 unsigned char hsb, unsigned char lsb_speed, 
			 unsigned char lsb_odo) {
  
  unsigned char status, acq, battery;
  double speed, dist; 

  if (dist_offset < 0) {
    switch (dist_units) {
    case 0:          /* Imperial units: miles */
      if (fprintf(sfp, "  Time\t\tStatus\tACQ\tBAT\tV [mph]\tD [miles]\n")
	  < 0) {
	fatal("Error writing to a file");
      }
      break;
    case 1:          /* SI units: kilometers */
    default:
      if (fprintf(sfp, "  Time\t\tStatus\tACQ\tBAT\tV [kph]\t   D [km]\n")
	  < 0) {
	fatal("Error writing to a file");
      }
      break;
    }
  }

  status = ( status_byte & 0xf0 ) >> 4;
  acq = ( status_byte & 0x0c) >> 2;
  battery = ( status_byte & 0x03 );
  speed = (double)((long int) lsb_speed + 
		   ( ((long int) (hsb & 0xf0)) << 4 )) * SPEED_UNIT;
  dist = (double)((long int) lsb_odo +
		  ( ((long int) (hsb & 0x0f)) << 8 )) * DIST_UNIT;
  dist_corrections(&dist);

  time2str(time_str, *split_time + 0.5);
  if (fprintf(sfp, "%s\t0x%x\t0x%x\t0x%x\t%5.1f\t%9.3f\n", 
	      time_str, status, acq, battery, 
	      unit_conv(speed), unit_conv(dist)) < 0) {
    fatal("Error writing to a file");
  }

  *split_time += TIME_STEP_GPS;
}

/*
 * Decodes and prints GPS packet type 4 (Time and Date)
 */
static void gps_packet_4(double *split_time, unsigned char mo_yr, 
			 unsigned char mi_da, unsigned char da_hr, 
			 unsigned char bsec) {
  int year, month, day, hour, min;
  float sec;

  /* Note: Year in GPS time packets is 2001 based in contrary to the
   * 2000 year base in headers/footers of sessions. The time is GMT.
   */

  year  = (int)(mo_yr & 0x0f) + 2001;
  month = (int)(mo_yr & 0xf0) >> 4;
  day   = ((int)(mi_da & 0x03) << 3) + ((int)(da_hr & 0xe0) >> 5);
  hour  = (int)(da_hr & 0x1f);
  min   = (int)(mi_da & 0xfc) >> 2;
  sec   = (float)((int)(bsec & 0xfc) >> 2) + (float)((int)(bsec & 0x03))*0.25;
  
  time2str(time_str, *split_time + 0.5);
  if (fprintf(sfp, "%s\t%i-%02i-%02i %2i:%02i:%05.2f GMT\n", time_str, 
	      year, month, day, hour, min, sec) < 0) {
    fatal("Error writing to a file");
  }

  *split_time += TIME_STEP_GPS;
}

/*
 * Prints GPS session data to stdout. 
 */
static void gps_session(const struct tdr_session *ses) { 
  unsigned long int i, psize, bytes = ses->nbytes;
  double split_time = 0.0;
  
  if ( ses->nbytes < GPS_PACKET_MIN_LENGTH ) {
    fprintf(stderr, "Skipping GPS session: Packet too short.");
    return;
  }

  dist_offset = -1;
  dist_prev = -1;
  dist_base = 0;

  open_session_file(GPS_FILE_EXT, &(ses->header), &(ses->footer));

  session_header("GPS session", &(ses->header), &(ses->footer));
 
  for (i=0; (i < bytes) && 
	 (psize = (ses->data[i] & PACKET_LENGTH_MASK)) <= (bytes - i); 
       i += psize) {
    
    switch (ses->data[i]) {
    case PACKET_TYPE_ERROR:
      packet_error(split_time, ses->data[i+1]);
      split_time += TIME_STEP_GPS;
      break;
    case PACKET_TYPE_1:
      gps_packet_1(&split_time, ses->data[i+1], ses->data[i+2], 
		   ses->data[i+3], ses->data[i+4]);
      break;
    case PACKET_TYPE_4:
      gps_packet_4(&split_time, ses->data[i+1], ses->data[i+2], 
		   ses->data[i+3], ses->data[i+4]);
      break;  
    case PACKET_TYPE_2:
    case PACKET_TYPE_3:
    case PACKET_TYPE_5:
    case PACKET_TYPE_15:
    default:
      fatal("This GPS packet handling is not implemented yet");
      break;
    }
  }

  close_session_file();
  
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
    switch (session->data[i] & SESSION_MASK) {

    case HRM_SESSION:
      hrm_ses->data[hrm_ses->nbytes++] = session->data[i+1];
      i += 2;
      break;

    case GPS_SESSION:
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

  for (ses = session; ses;  ses = ses->next) {
    
    if (newer_session(&ses->header)) {
      switch (ses->header.dev & SESSION_MASK) {
      case HRM_SESSION:
	hr_session(ses);
	break;
      case GPS_SESSION:
	gps_session(ses);
	break;
      case MULTI_DEVICE_SESSION & SESSION_MASK:
	multi_session(ses);
	break;
      default:
	errno = 0;
	fatal("Unknown session");
	break;
      }
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
  int i, clear_eeprom=0;
  unsigned long int bytes, bufsize, timeout;
  unsigned char buf[RESPONSE_BUFSIZE], *databuf;
  char c, choice='h';
  struct tdr_session  *session;
  static struct option long_options[] = {
    {"all-sessions", 0, NULL, 'a'},
    {"clear-eeprom", 0, NULL, 'c'},
    {"days", 2, NULL, 'd'},             /* Takes an optional argument */
    {"eeprom-dump", 0, NULL, 'e'},
    {"file", 0, NULL, 'f'},
    {"help",  0, NULL, 'h'},
    {"miles", 0, NULL, 'm'},
    {"time-sync", 0, NULL, 't'},
    {"version", 0, NULL, 'V'},
    {NULL, 0, NULL, 0}
  };
  
  progname = argv[0];
  //  sfp = stdout;

  while (1) {
    c = getopt_long(argc, argv, "acd::efhmtV",
		    long_options, NULL);

    if (c == -1) {
      break;
    }
    
    switch (c) {
    case 'a':
    case 'e':
      choice = c;
      break;

    case 'c':
      clear_eeprom = 1;
      if (choice == 'h') choice = '\0';
      break;
    
    case 'd':
      if (optarg) {
	set_initial_time(atol(optarg));
      } else {
	set_initial_time(0);
      }
      choice = c;
      break;

    case 'f':
      write_session_to_file = 1;
      if (choice == 'f') choice = '\0';
      break;

    case 'm':
      dist_units = 0;
      break;

    case 't':
      dev = timexdr_open();
      if (timex_sync_time(dev, buf, RESPONSE_BUFSIZE) < 0) {
	fatal("Time synchronization failed");
      }
      timexdr_close(dev);
      if (choice == 'h') choice = '\0';
      break;

    case 'V':
      timexdr_version();
      break;
   
    case 'h':
    default:
      timexdr_usage(argv[0]);
      break;
    }
  }

  switch (choice) {

  case 'a':
  case 'd':
  case 'e':
    dev = timexdr_open();

    firmware = get_fw_version(dev);
#ifdef TDR_DEBUG
    printf("Firmware version:\tMain:\t%u\n\t\t\tUSB:\t%u\n", 
	   firmware.main, firmware.usb);
#endif

    i = timex_ctrl2(dev, EEPROM_CAPACITY, DEFAULT_MICRO, buf, RESPONSE_BUFSIZE);
#ifdef TDR_DEBUG
    printf("Memory capacity:\t%u bytes\n", 
	   (long int) (buf[3] + (buf[4] << 8) + (buf[5] << 16)) + 1);
#endif
    i = timex_ctrl2(dev, EEPROM_USAGE, DEFAULT_MICRO, buf, RESPONSE_BUFSIZE);
#ifdef TDR_DEBUG
    printf("Memory used:    \t%u bytes\n", 
	   (long int) (buf[3] + (buf[4] << 8) + (buf[5] << 16)) + 1);
#endif

    i = timex_ctrl(dev, vendor_ctrl_2, buf, RESPONSE_BUFSIZE);
    bytes = eeprom_usage(dev);

#ifdef TDR_DEBUG
    printf("bytes: %u 0x%x\tpages: %u\n", bytes, bytes, 
	   num_of_pages(bytes, EEPROM_PAGESIZE));
#endif

    timexdr_close(dev);
    return 0;

    i = timex_ctrl(dev, vendor_ctrl_4, buf, RESPONSE_BUFSIZE);
    i = timex_ctrl(dev, vendor_ctrl_read_memory, buf, RESPONSE_BUFSIZE);

    if ((bytes == (TIMEXDR_ATABLESIZE + 
		   num_of_pages(bytes, EEPROM_PAGESIZE))) && 
	timex_ctrl(dev, vendor_ctrl_no_session, buf, RESPONSE_BUFSIZE)) {
      fprintf(stderr, "EEPROM empty.\n");
      exit(EXIT_FAILURE);
    }

    bufsize = EEPROM_PAGESIZE * num_of_pages(bytes, EEPROM_PAGESIZE); 
    databuf = (char *)calloc(bufsize, sizeof(char));
    if (databuf == 0) fatal("databuf not initialized");
   
    /* The timeout may be unnecessary long but that is better than too short.
     * This way we make sure that all data get tranferred.
     */
    timeout = (bytes / 2048 + 1) * 20 * TIMEXDR_CTRL_TIMEOUT;

#ifdef TDR_DEBUG
    printf("timeout: %d ms\n", timeout);
#endif

    i = timex_int_read(dev, databuf, bufsize, timeout);
    i = timex_ctrl(dev, vendor_ctrl_after_memread, buf, RESPONSE_BUFSIZE);
    
    switch (choice) {
    case 'e': 
      print_eeprom(databuf, num_of_pages(bytes, EEPROM_PAGESIZE));
      break;
    case 'a':
    case 'd':
      squeeze_data(databuf, bytes);
      session = split_data(databuf);
      print_session(session);
      break;
    default:
      break;
    }
    
    timexdr_close(dev);
    break;

  case 'h':
    timexdr_usage(argv[0]);
    break;

  default:
    break;
  }

  /* This should be the last action of the program in case we want to 
   * download the data before clearing the EEPROM
   */
  if (clear_eeprom) {
    dev = timexdr_open();
    i = timex_ctrl(dev, vendor_ctrl_clear_eeprom, buf, RESPONSE_BUFSIZE);
    timexdr_close(dev);
  }

  return 0;
}

