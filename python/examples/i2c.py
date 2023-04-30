#!/usr/bin/env python3
#
# Copyright 2021 NXP
# SPDX-License-Identifier: BSD-3-Clause
#
# NXP USBSIO Library - I2C example code
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

# example on how to load DLL from a specific directory
#sio = LIBUSBSIO(r"..\..\bin_debug\x64", loglevel=logging.DEBUG)

# load DLL from default directory
sio = LIBUSBSIO(loglevel=logging.DEBUG)

# I2C settings
I2C_CLOCK = 100000
I2C_ADDR = 20

############################################################################
############################################################################
# the main code

# calling GetNumPorts is mandatory as it also scans for all connected USBSIO devices
numports = sio.GetNumPorts()
print("SIO ports = %d" % numports)

if numports > 0 and sio.Open(0):
    print("LIB version = '%s'" % sio.GetVersion())
    print("I2C ports = %d" % sio.GetNumI2CPorts())
    print("Max data size = %d" % sio.GetMaxDataSize())

    if(sio.GetNumSPIPorts() > 0):
        i2c = sio.I2C_Open(I2C_CLOCK, portNum=0)
        if i2c:
            data, ret = i2c.FastXfer(I2C_ADDR, b"Hello World", rxSize=10, ignoreNAK=True)

            if ret > 0:
                print("Read %d bytes, received %s" % (ret, data))
            else:
                print("Error %d reading %d bytes from I2C" % (ret, 10))

            i2c.Close()
        else:
            print("Could not test I2C")

    sio.Close()

else:
    print("No USBSIO device found")
