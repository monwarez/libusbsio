#!/usr/bin/env python3
#
# Copyright 2021 NXP
# SPDX-License-Identifier: BSD-3-Clause
#
# TEST CODE of NXP USBSIO Library - main test entry point
#

import unittest
import argparse
import pytest
import functools
import logging
import sys
import os

# add path to local libusbsio package so it can be tested without installing it
addpath = os.path.join(os.path.dirname(__file__), '..')
sys.path.append(addpath)

from libusbsio import *

# enable basic console logging
LOGLEVEL = logging.INFO

# device dependent settings
(T_LPCLINK2, T_MCULINK_PRO, T_MCULINK_55S36) = (1, 2, 3)
#TARGET = T_MCULINK_PRO
TARGET = T_MCULINK_55S36
#TARGET = T_LPCLINK2

# LPCLINK2
if TARGET == T_LPCLINK2:
    VIDPIDS = [ LIBUSBSIO.VIDPID_LPCLINK2 ]
elif TARGET >= T_MCULINK_PRO:
    VIDPIDS = [ LIBUSBSIO.VIDPID_MCULINK ]
else:
    raise "You must set testing TARGET!"

# enable to force logging to console
#logging.basicConfig()

# change to enable tests which are normally skipped
RUN_SLOW_TESTS = 0
RUN_KNOWN_ISSUES = 0

def known_issue(func):
    '''Decorator to mark known-issues failing tests'''
    @functools.wraps(func)
    def wrapper(self, *args, **kwargs):
        if RUN_KNOWN_ISSUES:
            self.logger.warning("This test is marked as @known_issue")
            return func(self, *args, **kwargs)
        else:
            self.skipTest("Test marked as @known_issue")
    return wrapper

class TestBase(unittest.TestCase):
    def setUp(self):
        self.logger = logging.getLogger('test')
        self.logger.setLevel(LOGLEVEL)
        self.sio:LIBUSBSIO = LIBUSBSIO(loglevel=LOGLEVEL)
        self.spi:LIBUSBSIO.SPI = None
        self.i2c:LIBUSBSIO.I2C = None

    def tearDown(self):
        if(self.spi):
            del self.spi
            self.spi = None
        if(self.i2c):
            del self.i2c
            self.i2c = None
        if(self.sio):
            del self.sio
            self.sio = None

if __name__ == '__main__':
    unittest.main()
