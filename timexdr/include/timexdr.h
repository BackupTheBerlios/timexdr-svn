/* 
 * Timex Data Recorder userspace control utility
 *
 * Copyright (C) 2005-2006 Jan Merka <merka@highsphere.net>
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

#ifndef TDR_TIMEXDR_H
#define TDR_TIMEXDR_H 1

//#define TDR_DEBUG 1

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
#define TIMEXDR_PRODUCT_ID1    0xc700
#define TIMEXDR_PRODUCT_ID2    0xc701
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

//#define TIME_CHECKSUM             0xb9     /* for time synchronization */

#define TDR_ADDRESS(b0, b1, b2)     ((b0) | ((b1) << 8) | ((b2) << 16)) 

#define TDR_YR(yr)       (2000 + (yr))
#define TDR_MD(md)       (1 + (md))


/* Multi-device session ID (both HRM and GPS data */
#define SESSION_MASK                0xf0
#define MULTI_DEVICE_SESSION        0xff 
#define HRM_SESSION                 0x00
#define GPS_SESSION                 0x20   /* ID varies 0x20-0x2f */

#define HRM_FILE_EXT                "hrm"
#define GPS_FILE_EXT                "gps"

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
#define ALT_UNIT                    1      /* foot */
#define HEADING_UNIT                2      /* degrees */
#define LL_UNIT                     M_PI/pow(2,23)     /* radian */
#define LL_UNIT_DEG                 180/pow(2,23)      /* degrees */

/* Altitude offset: 0 value represents 2000 feet below sea level */
#define ALT_OFFSET                  2000   /* feet */

/* Odometer quirks */
#define ODO_MAX                     4.096  /* miles */

/* Unit conversion */
#define MILES_TO_KM(mi)             (1.609344 * (mi)) 
#define FT_TO_M(ft)                 (0.3048 * (ft))

/* Output report array: Byte 0 is the Report Number, byte 1 specifies the 
 * configuration, byte 2 encodes command type and the number n of bytes to send 
 * counting this byte as the first, byte n+2 keeps the checksum, and the
 * rest is insignificant. 
 */
#define TIMEXDR_CTRL_SIZE           0xa
char ctrl_cmd[TIMEXDR_CTRL_SIZE] = {0x01, 0x0, 0x0, 0x0, 0x0, 
				    0x0, 0x0, 0x0, 0x0, 0x0};

/* Vendor command definitions */
#define EEPROM_USAGE               0
#define EEPROM_CLEAR               1
#define DATA_UPLOAD                2
#define UPLOAD_CANCEL              3
#define SYNC_TIME                  4
#define UPLOAD_DONE                5
#define SW_TIMEOUT                 6
#define FW_VERSION                 7
#define EEPROM_TEST                8
#define ROM_TEST                   9
#define RAM_TEST                  10
#define READ_BOND_OPTION          11
#define MODIFY_BOND_OPTION        12
#define RESTORE_BOND_OPTION       13
#define EEPROM_CAPACITY           14
#define RAM_ROM_DEBUG             15

/* Vendor configuration commands */
/* Configuration byte structure
 * -----------------------------------------------------------------
 * | bit 7 | bit 6 | bit 5 | bit 4 | bit 3 | bit 2 | bit 1 | bit 0 |
 * -----------------------------------------------------------------
 * bit 5 - bit 0  =  data size expected from device (?)
 *         bit 6  =  command direction
 *                   0 - for USB alone (USB Microcontroller)
 *                   1 - for data recorder (Main Microcontroller)
 *         bit 7  =  reply type
 *                   0 - ACK command only
 *                   1 - ACK command with data
 */
#define CMD_EEPROM_USAGE               0x40  |  0xc2
#define CMD_EEPROM_CLEAR               0x40
#define CMD_DATA_UPLOAD                         0xc2
#define CMD_UPLOAD_CANCEL              0x40
#define CMD_SYNC_TIME                  0x40
#define CMD_UPLOAD_DONE                0x40
#define CMD_SW_TIMEOUT                 0x40
#define CMD_FW_VERSION_USB_MICRO                       0x83                
#define CMD_FW_VERSION_MAIN_MICRO               0xc3                
#define CMD_EEPROM_TEST                0x40
#define CMD_ROM_TEST_USB_MICRO                                0x00
#define CMD_ROM_TEST_MAIN_MICRO        0x40
#define CMD_RAM_TEST_USB_MICRO                                0x00
#define CMD_RAM_TEST_MAIN_MICRO        0x40
#define CMD_READ_BOND_OPTION           0x40  |         0x81
#define CMD_MODIFY_BOND_OPTION         0x40
#define CMD_RESTORE_BOND_OPTION        0x40
#define CMD_EEPROM_CAPACITY                     0xc3
#define CMD_RAM_ROM_DEBUG              0x40  |  0xc1

/* Microcontroller targets */
#define DEFAULT_MICRO   0
#define USB_MICRO       1
#define MAIN_MICRO      2

/* Return codes for control commands */
#define CTRL_COMMAND_SUCCESS    0xfe
#define CTRL_COMMAND_FAILURE    0x1

/* Encode command type and number of bytes */
#define ENC_CMD_TYPE(ct, nb)           (((ct) << 4) + (nb)) 

/* Decode firmware version */
#define DEC_FW_VER(b3, b4, b5)                      \
  (((b5) >> 4 ) * 100000 + ((b5) & 0x0F) * 10000 +  \
   ((b4) >> 4)  * 1000   + ((b4) & 0x0F) * 100   +  \
   ((b3) >> 4)  * 10     + ((b3) & 0x0F))

/* Decode the last used EEPROM address */
#define DEC_EEPROM_USAGE(b2, b3, b4) ((((b2) & 0xf) << 16) | ((b4) << 8) | (b3)) 

/* Data types */
struct tdr_session; 

struct tdr_header {
  char dev;                                          /* Device identifier */
  unsigned int year, month, day, hour, min, sec;     /* Time stamp */
};

struct tdr_session {
  time_t start;
  struct tdr_session *next, *prev;
  struct tdr_header header, footer;
  unsigned char *data;
  unsigned long int nbytes;
};

struct tdr_info {
  char vendor[TIMEXDR_STRLEN];
  char product[TIMEXDR_STRLEN];
  long int eeprom_size;
  long int fw_main;
  long int fw_usb;
};

/* Global settings */
int dist_units = 1;                 /* Distance units: 0 - miles, 1 - km */
time_t initial_time = 0;            /* Download only sessions newer than 
				       init_time */
int write_session_to_file = 0;
FILE *sfp;                          /* Session file pointer (stdout) */

int verbosity = 0;                  /* Verbosity level */

struct tdr_info tdr_info = {vendor:"", product:"", 
			    eeprom_size:0, fw_main:0, fw_usb:0};

#endif /* TDR_TIMEXDR_H */
