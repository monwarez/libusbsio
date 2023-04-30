#!/usr/bin/env python3
#
# Copyright 2021-2022 NXP
# SPDX-License-Identifier: BSD-3-Clause
#
# NXP USBSIO Library - I2C example code
#

import os
import sys
import logging
import logging.config

# add path to local libusbsio package so it can be tested without installing it
addpath = os.path.join(os.path.dirname(__file__), "..")
sys.path.append(addpath)

from libusbsio import *

# enable basic console logging
logging.basicConfig()

# example on how to load DLL from a specific directory
#sio = LIBUSBSIO(r"..\..\bin_debug\x64", loglevel=logging.DEBUG)

# load DLL from default directory
sio = LIBUSBSIO()

############################################################################
############################################################################
# the main code

# calling GetNumPorts is mandatory as it also scans for all connected USBSIO devices
numports = sio.GetNumPorts()
print("SIO ports = %d" % numports)

if not numports:
    print("No USBSIO device found")

for i in range(0, numports):
    info = sio.GetDeviceInfo(i)

    if sio.Open(i):
        print(f"Instance {i}:")
        if info:
            print(f"    HID product: {info.manufacturer_string} {info.product_string}")
            print(f"    HID serial: {info.serial_number}")
            print(f"    HID path: {info.path}")
        print("    LIB version = '%s'" % sio.GetVersion())
        print("    I2C ports = %d" % sio.GetNumI2CPorts())
        print("    SPI ports = %d" % sio.GetNumSPIPorts())
        print("    Max data size = %d" % sio.GetMaxDataSize())
        sio.Close()
        # re-enumerate after closing port
        sio.GetNumPorts()

