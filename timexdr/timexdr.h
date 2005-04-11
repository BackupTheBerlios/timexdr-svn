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

#ifndef _TDR_H_
#define _TDR_H_

//#define TDR_DEBUG  

/*-------------------------------------------------------------------------*/

/* CONTROL REQUEST SUPPORT */

/*
 * USB directions
 *
 * This bit flag is used in endpoint descriptors' bEndpointAddress field.
 * It's also one of three fields in control requests bRequestType.
 */         
#define USB_DIR_OUT                     0               /* to device */
#define USB_DIR_IN                      0x80            /* to host */

/*
 * USB types, the second of three bRequestType fields
 */
#define USB_TYPE_MASK                   (0x03 << 5)
#define USB_TYPE_STANDARD               (0x00 << 5)
#define USB_TYPE_CLASS                  (0x01 << 5)
#define USB_TYPE_VENDOR                 (0x02 << 5)
#define USB_TYPE_RESERVED               (0x03 << 5)

/*
 * USB recipients, the third of three bRequestType fields
 */
#define USB_RECIP_MASK                  0x1f
#define USB_RECIP_DEVICE                0x00
#define USB_RECIP_INTERFACE             0x01
#define USB_RECIP_ENDPOINT              0x02
#define USB_RECIP_OTHER                 0x03

/*
 * Standard requests, for the bRequest field of a SETUP packet.
 *
 * These are qualified by the bRequestType field, so that for example
 * TYPE_CLASS or TYPE_VENDOR specific feature flags could be retrieved
 * by a GET_STATUS request.
 */
#define USB_REQ_GET_STATUS              0x00
#define USB_REQ_CLEAR_FEATURE           0x01
#define USB_REQ_SET_FEATURE             0x03
#define USB_REQ_SET_ADDRESS             0x05
#define USB_REQ_GET_DESCRIPTOR          0x06
#define USB_REQ_SET_DESCRIPTOR          0x07
#define USB_REQ_GET_CONFIGURATION       0x08
#define USB_REQ_SET_CONFIGURATION       0x09
#define USB_REQ_GET_INTERFACE           0x0A
#define USB_REQ_SET_INTERFACE           0x0B
#define USB_REQ_SYNCH_FRAME             0x0C

/*-------------------------------------------------------------------------*/

#define TIMEXDR_VENDOR_ID      0x0cc2
#define TIMEXDR_PRODUCT_ID     0xc700
#define TIMEXDR_USAGE          0xffa00000

#define TIMEXDR_EP             0x81

#define MAXBUFSIZE             4096        /* in bytes */
#define RESPONSE_BUFSIZE          7        /* in bytes */
#define EEPROM_PAGESIZE         256        /* in bytes */
#define DATA_PAGESIZE  (EEPROM_PAGESIZE-1) /* in bytes */

#define TIMEXDR_STRLEN         256
#define TIMEXDR_STR_VENDOR       1
#define TIMEXDR_STR_PRODUCT      2

#define TIMEXDR_CONFIG           1
#define TIMEXDR_INTERFACE        0

#define TIMEXDR_FIRSTSESSION     0x180     /* Position of the first session */
#define TIMEXDR_ATABLESIZE         384     /* Bytes in the access table  */
#define TDR_ASIZE                    3     /* Address size in bytes */

#define TIME_STEP_HRM                2     /* in seconds */
#define TIME_STEP_GPS                3.57  /* in seconds */

#define TIME_CHECKSUM             0xb9     /* for time synchronization */

#define TDR_ADDRESS(b0, b1, b2) \
  ((b0) | ((b1) << 8) | ((b2) << 16)) 

#define TDR_YR(yr)       (2000 + (yr))
#define TDR_MD(md)       (1 + (md))


/* Multi-device session ID (both HRM and GPS data */
#define SESSION_MASK                0xf0
#define MULTI_DEVICE_SESSION        0xff 
#define HRM_SESSION                 0x00
#define GPS_SESSION                 0x20   /* ID varies 0x20-0x2f */

#define TIMEXDR_CTRL_TIMEOUT         600   /* in miliseconds */


/* Packet error codes */
#define MISSING_PACKET              0x00
#define CORRUPTED_PACKET            0x04

/* GPS packets and profiles */

#define PACKET_TYPE_ERROR           0x12   /* Missing/corrupted packet */
#define PACKET_TYPE_1               0x15   /* status, speed, distance */
#define PACKET_TYPE_2               0x25   /* altitude and heading */
#define PACKET_TYPE_3               0x37   /* position */
#define PACKET_TYPE_4               0x45   /* time and date */
#define PACKET_TYPE_5               0x59   /* status, speed, distance, */ 
					   /* altitude and heading */
#define PACKET_TYPE_15              0xf0   /* everything */
#define PACKET_TYPE_15_LENGTH         17   /* in bytes */

#define PACKET_LENGTH_MASK          0x0f
#define GPS_PACKET_MIN_LENGTH          2   /* in bytes */

/* SNSR Status */
#define SNSR_GPS_OK                 0x00
#define SNSR_GPS_SYSFAILURE         0x09
#define SNSR_ROMRAM_FAILURE         0x0a
#define SNSR_OSCIL_DRIFT_FAILURE    0x0b
#define SNSR_GPS_NO_CARRIER         0x01   /* Ready to power down */
#define SNSR_GPS_NO_MOTION          0x02   /* Ready to power down */   
#define SNSR_LOW_BATTERY            0x03   /* Ready to power down */

/* Data units */
#define SPEED_UNIT                  0.1    /* mph */
#define DIST_UNIT                   0.001  /* mile */

/* Odometer quirks */
#define ODO_MAX                     4.096  /* miles */

/* Unit conversion */
#define MILES_TO_KM(mi)             (1.609344 * (mi)) 

/* Global settings */
int dist_units = 1;                 /* Distance units: 0 - miles, 1 - km */

/* Vendor control commands */
#define TIMEXDR_CTRL_SIZE      0xa

char vendor_ctrl_1[] = {0x01, 0xc3, 0x71, 0x8f, 0x11, 0x11, 
			0x00, 0x00, 0x30, 0x1c};
char vendor_ctrl_2[] = {0x01, 0xc1, 0xf5, 0x20, 0xf0, 0xb1, 
			0x01, 0x49, 0x55, 0x1f};
char vendor_ctrl_last_address[] = {0x01, 0xc2, 0x01, 0xff, 0x11, 0x11, 
				   0x00, 0x00, 0x30, 0x1c};
char vendor_ctrl_4[] = {0x01, 0xc3, 0xe1, 0x1f, 0x11, 0x11, 
			0x00, 0x00, 0x30, 0x1c};
/* The 8th and 10th bytes of the vendor_ctrl_read_memory control command 
 * vary but it does not seem to matter what values are actually used.
 */
char vendor_ctrl_read_memory[] = { 0x1, 0xc2, 0x21, 0xdf, 0x0, 
				   0x7c, 0x39, 0, 0x9, 0};
/* I am not sure what this does but at least it causes the LED show a
 * re-assuring green light :) Without this command, LED blinks red after
 * unplugging the device from the USB port.
 */
char vendor_ctrl_after_memread[] = {0x1, 0x40, 0x51, 0xaf, 0, 1, 0, 0, 0, 0};

/* Set the device clock to the time specified in bytes 4-9, the purpose
 * of the 10th byte is unknown.
 */
char vendor_ctrl_set_time[] = { 0x1, 0x40, 0x47, 0, 0, 0, 0, 0, 0, 0 };

/* Clear EEPROM */
char vendor_ctrl_clear_eeprom[] = { 0x1, 0x40, 0x11, 0xef, 0x11, 0x11, 
				    0x0, 0x0, 0x30, 0x1c};

/* Control command send when no session are stored in EEPROM */
char vendor_ctrl_no_session[] = { 0x1, 0x40, 0x31, 0xcf, 0x0,
				  0x7c, 0x39, 0, 0x9, 0};

/* Data types */
struct tdr_session; 

struct tdr_header {
  char dev;                                          /* Device identifier */
  unsigned int year, month, day, hour, min, sec;     /* Time stamp */
};

struct tdr_session {
  struct tdr_session *next, *prev;
  struct tdr_header header, footer;
  unsigned char *data;
  unsigned long int nbytes;
};

#endif /* _TDR_H_ */
