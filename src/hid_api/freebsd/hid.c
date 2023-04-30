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

#include "hidapi_mock.h"

int HID_API_EXPORT hid_write_timeout(hid_device *dev, const unsigned char *data, size_t length, int milliseconds)
{
    unsigned char* padded = NULL;
    int bytes_written = 0;

    if (length <= 0)
        return 0;

#if 0
    if (length < (size_t)dev->output_report_length)
    {
        padded = calloc(1, dev->output_report_length);

        if(padded != NULL)
        {
            /* use buffer of report_length size padded with zeroes instead of original data */
            memcpy(padded, data, length);
            length = (size_t)dev->output_report_length;
            data = padded;
        }
    }
#endif

    /*
     * Note:
     * 1. Blocking Write for USB is not real blocking. There is a build-in timeout in Linux, which
     *    is defined by USB_CTRL_SET_TIMEOUT in linux/include/linux/usb.h
     * 2. Do not use poll()/ppoll() for timeout control. POLLOUT wouldn't be triggered by HIDRAW.
     */
    bytes_written = hid_write(dev, data, length);

    if(padded)
        free(padded);

    return bytes_written;
}

int HID_API_EXPORT hid_get_report_lengths(hid_device* device, unsigned short* output_report_length, unsigned short* input_report_length)
{
    if (output_report_length)
        *output_report_length =  64;//device->output_report_length;
    if (input_report_length)
        *input_report_length =  64;//device->input_report_length;
    return 0;
} 

int HID_API_EXPORT hid_get_usage(hid_device* device, unsigned short* usage_page, unsigned short* usage)
{
    if (usage_page)
        *usage_page = 0; // TODO: this seems commons for hidapi on Linux without hidraw
    if (usage)
        *usage = 0; // TODO: device->usage;  (does a single usage value even make sense?)
    return 0;
}
