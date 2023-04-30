#!/usr/bin/env python3
#
# Copyright 2021 NXP
# SPDX-License-Identifier: BSD-3-Clause
#
# NXP USBSIO Library - HID devices enumeration example
#

import os
import sys
import logging
import logging.config

# add path to local libusbsio package so it can be tested without installing it
addpath = os.path.join(os.path.dirname(__file__), '..')
sys.path.append(addpath)

from libusbsio import *

# enable basic console logging
logging.basicConfig()

sio = LIBUSBSIO(loglevel=logging.DEBUG)

for i in sio.HIDAPI_Enumerate((0x1FC9, 0), True):
    print("HID DEVICE interface %d" % i.interface_number)
    print("    path: %s" % i.path)
    print("    serial_number: %s" % i.serial_number)
    print("    manufacturer_string: %s" % i.manufacturer_string)
    print("    product_string: %s" % i.product_string)
    print("    vendor_id: 0x%04x" % i.vendor_id)
    print("    product_id: 0x%04x" % i.product_id)
    print("    release_number: %d" % i.release_number)
    if i.ex.is_valid:
        print("    Extended information:")
        print("      output_report_length: %d" % i.ex.output_report_length)
        print("      input_report_length: %d" % i.ex.input_report_length)
        print("      usage_page: 0x%04x" % i.ex.usage_page)
        print("      usage: %d" % i.ex.usage)

    # example of device usage detection
    if i.ex.is_valid and i.ex.usage_page == 0xffea:
        dev = sio.HIDAPI_DeviceOpen(i.path)
        # result = dev.Write(b"hello", timeout_ms=1000)
        # (data, result) = dev.Read(16, timeout_ms=1000)
        dev.Close()

sio.Close()
