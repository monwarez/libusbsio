/*******************************************************
 HIDAPI - Multi-Platform library for
 communication with HID devices.

 Alan Ott
 Signal 11 Software

 8/22/2009
 Linux Version - 6/2/2009

 Copyright 2009, All Rights Reserved.

 At the discretion of the user of this library,
 this software may be licensed under the terms of the
 GNU General Public License v3, a BSD-Style license, or the
 original HIDAPI license as outlined in the LICENSE.txt,
 LICENSE-gpl3.txt, LICENSE-bsd.txt, and LICENSE-orig.txt
 files located at the root of the source distribution.
 These files may also be found in the public source
 code repository located at:
        http://github.com/signal11/hidapi .
********************************************************/

/*
 * Copyright 2014, 2021 NXP
 * Modified for use in NXP LIBUSBSIO Library
 */

#pragma once
/* C */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <errno.h>

/* Unix */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <poll.h>

#include <libudev.h>

#include <hidapi.h>
#include <hidapi_libusb.h>

int HID_API_EXPORT hid_write_timeout(hid_device *dev, const unsigned char *data, size_t length, int milliseconds);

int HID_API_EXPORT hid_get_report_lengths(hid_device* device, unsigned short* output_report_length, unsigned short* input_report_length);

int HID_API_EXPORT hid_get_usage(hid_device* device, unsigned short* usage_page, unsigned short* usage);
