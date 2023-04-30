#!/usr/bin/env python3
#
# Copyright 2021 NXP
# SPDX-License-Identifier: BSD-3-Clause
#
# NXP USBSIO Library - SPI example code
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

# SPI SSEL port.pin
spi_ssel = (0, 15)  # port 0.15 used by LPCLink2. The MCULink Pro ignores it

############################################################################
############################################################################
# the main code

# calling GetNumPorts is mandatory as it also scans for all connected USBSIO devices
numports = sio.GetNumPorts()
print("SIO ports = %d" % numports)

if numports > 0 and sio.Open(0):
    print("LIB version = '%s'" % sio.GetVersion())
    print("SPI ports = %d" % sio.GetNumSPIPorts())
    print("Max data size = %d" % sio.GetMaxDataSize())

    if(sio.GetNumSPIPorts() > 0):
        spi = sio.SPI_Open(1000000, portNum=0, dataSize=8, preDelay=100)
        if spi:
            data, ret = spi.Transfer(spi_ssel[0], spi_ssel[1], b"Hello World")

            if ret > 0:
                print("Sent %d bytes, received %s" % (ret, data))
            else:
                print("Error %d sending to SPI" % ret)

            spi.Close()
        else:
            print("Could not test SPI")

    sio.Close()

else:
    print("No USBSIO device found")
