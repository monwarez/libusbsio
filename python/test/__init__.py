#!/usr/bin/env python3
#
# Copyright 2021 NXP
# SPDX-License-Identifier: BSD-3-Clause
#
# TEST CODE of NXP USBSIO Library
#

# import the TestBase class and all global constants
from .test_base import *

# import only global decorators
from .test_00_lib import opensio
from .test_10_spi import use_spi
from .test_20_i2c import use_i2c
