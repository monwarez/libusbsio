#!/usr/bin/env python3
#
# Copyright 2021 NXP
# SPDX-License-Identifier: BSD-3-Clause
#
# TEST CODE of NXP USBSIO Library - GPIO tests
#

import unittest
import functools
import logging
import sys
import os

from test import *

# global GPIO test parameters
if TARGET == T_LPCLINK2:
    GPIO_PORTS = [ (0,15) ]   # tuples of port.pin values which are available as GPIO ports
    GPIO_INOUT = []
elif TARGET == T_MCULINK_PRO:
    GPIO_PORTS = [ (0,0) ]    # MCU Link Pro (standalone) does not support GPIO interface
    GPIO_INOUT = []
elif TARGET == T_MCULINK_55S36:
    GPIO_PORTS = [ (1,1), (1,7), (1,9), (1,20), (1,21), (1,31) ]  # (0,20) is the SSEL
    GPIO_INOUT = [ [ (1,1), (1,9) ], [ (1,7), (1,31) ], [ (1,20), (1,21) ] ]  # pairs of pins for in-out testing (interconnected externally on J133)


GPIO_REPEATS = 10         # number of test iterations for each port.pin
GPIO_DO_INOUT_TEST = 0    # enable GPIO hardware in-out test (needs GPIO_INOUT wiring)


def need_gpio_wiring(func):
    '''Decorator to mark tests which need GPIO_INOUT physical wiring'''
    @functools.wraps(func)
    def wrapper(self, *args, **kwargs):
        if GPIO_DO_INOUT_TEST and len(GPIO_INOUT):
            return func(self, *args, **kwargs)
        else:
            self.skipTest("Test skipped as it is marked @need_gpio_wiring")
    return wrapper


class TestGPIO(TestBase):
    @opensio
    def test_GPIO_ReadPort(self):
        for p in GPIO_PORTS:
            port = p[0]
            pin = p[1]
            pinmask = 1 << p[1]
            ret = self.sio.GPIO_SetPortInDir(port, pinmask)
            self.assertTrue(ret > 0, "GPIO port%d set pin%d as input, ret=%d" % (port, pin, ret))
            val0, ret = self.sio.GPIO_ReadPort(port)
            self.logger.info("GPIO port%d read ret=%d, read=0x%08x" % (port, ret, val0))
            self.assertTrue(ret > 0, "GPIO port%d read should return nonzero, ret=%d" % (port, ret))
            val1, ret = self.sio.GPIO_ReadPort(port)
            self.logger.info("GPIO port%d read ret=%d, read=0x%08x" % (port, ret, val1))
            self.assertTrue(ret > 0, "GPIO port%d read should return nonzero, ret=%d" % (port, ret))
            self.assertBitsEqual(val0, val1, pinmask, "GPIO port%d read should return same values 0x%08x and 0x%08x at bit 0x%08x" % (port, val0, val1, pinmask))

    def assertBitsEqual(self, val1, val2, bitmask, msg):
        '''Two values must match in selected bits'''
        self.assertEqual((val1 ^ val2) & bitmask, 0, msg)

    @opensio
    def test_GPIO_WritePort_ReadPort(self, useAutomaticReadBack=False, useSetClearPort=False, useSetClearPin=False, useTogglePin=False, useGetPin=False):
        '''Testing port by repeatedly writing 1 and 0 and reading it back

        Args:
        - `useAutomaticReadBack`: use WritePort's automatic readback value as the ReadPort value, otherwise, use ReadPort or GetPin
        - `useGetPin`: use GetPin to read port value instead of ReadPort
        - `useSetClearPort`: Use SetPort/ClearPort instead of WritePort
        - `useSetClearPin`: Use SetPin/ClearPin instead of WritePort
        - `useTogglePin`: Use only with `useSetClearPin` to set it opposite and then toggle
        '''
        for p in GPIO_PORTS:
            port = p[0]
            pin = p[1]
            pinmask = 1 << p[1]
            self.logger.info("GPIO will test port=%d and pin=%d (pinmask=0x%08x)" % (port, pin, pinmask))

            # hard-reconfigure to GPIO mode
            #mode = 0x100
            #self.logger.info("Reconfiguring pins port=%d and pin=%d to mode 0x%x" % (port, pin, mode))
            #self.sio.GPIO_ConfigIOPin(port, pin, mode)

            ret = self.sio.GPIO_SetPortOutDir(port, pinmask)
            self.assertTrue(ret > 0, "GPIO port%d set pin%d as output, ret=%d" % (port, pin, ret))
            results = []

            for i in range(GPIO_REPEATS):
                # toggle value in each even/odd round
                wr = 0 if (i % 2) == 0 else 0xffffffff

                # different methods how to write value to GPIO pin
                if(useSetClearPort):
                    # use SetPort/ClearPort
                    if(wr):
                        ret = self.sio.GPIO_SetPort(port, pinmask)
                        self.logger.info("GPIO port%d SetPort ret=%d, pinmask=0x%08x" % (port, ret, pinmask))
                    else:
                        ret = self.sio.GPIO_ClearPort(port, pinmask)
                        self.logger.info("GPIO port%d ClearPort ret=%d, pinmask=0x%08x" % (port, ret, pinmask))

                elif(useSetClearPin):
                    # use SetPin/ClearPin, optionally with extra toggling
                    set = wr != 0
                    if(useTogglePin):
                        set = not set

                    if(set):
                        ret = self.sio.GPIO_SetPin(port, pin)
                        self.logger.info("GPIO port%d SetPin ret=%d, pin=%d" % (port, ret, pin))
                    else:
                        ret = self.sio.GPIO_ClearPin(port, pin)
                        self.logger.info("GPIO port%d ClearPin ret=%d, pin=%d" % (port, ret, pin))
                    self.assertTrue(ret > 0, "GPIO Set/ClearPort should return nonzero")

                    # extra toggle
                    if(useTogglePin):
                        ret = self.sio.GPIO_TogglePin(port, pin)
                        self.logger.info("GPIO port%d TogglePin ret=%d, pin=%d" % (port, ret, pin))

                else:
                    # use normal WritePort
                    val, ret = self.sio.GPIO_WritePort(port, wr)
                    self.logger.info("GPIO port%d WritePort ret=%d, wrote=0x%08x" % (port, ret, wr))
                    self.assertTrue(ret > 0, "GPIO WritePort should return nonzero")

                if(not useAutomaticReadBack):
                    # use extra port reading to find out a new port value
                    if(useGetPin):
                        # use GetPin
                        val = self.sio.GPIO_GetPin(port, pin)
                        self.logger.info("GPIO port%d GetPin pin=%d, value=%d" % (port, pin, val))
                        val = val << pin    # we want to work with value as a pinmask
                    else:
                        # use ReadPort
                        val, ret = self.sio.GPIO_ReadPort(port)
                        self.logger.info("GPIO port%d ReadPort ret=%d, value=0x%08x" % (port, ret, val))
                        self.assertTrue(ret > 0, "GPIO Set/Clear port should return nonzero")
                else:
                    self.logger.info("GPIO port%d WritePort readback value was 0x%08x" % (port, val))

                self.assertBitsEqual(wr, val, pinmask, "GPIO port written and read values should match 0x%08x and 0x%08x at bit 0x%08x" % (wr, val, pinmask))


    @opensio
    def test_GPIO_WritePort_ReadBack(self):
        self.test_GPIO_WritePort_ReadPort(useAutomaticReadBack=True)

    @opensio
    def test_GPIO_SetClearPort_ReadPort(self):
        self.test_GPIO_WritePort_ReadPort(useSetClearPort=True)

    @opensio
    def test_GPIO_SetClearPin_ReadPort(self):
        self.test_GPIO_WritePort_ReadPort(useSetClearPin=True)

    @opensio
    def test_GPIO_SetClearTogglePin_ReadPort(self):
        self.test_GPIO_WritePort_ReadPort(useSetClearPin=True, useTogglePin=True)

    @opensio
    def test_GPIO_SetClearPin_GetPin(self):
        self.test_GPIO_WritePort_ReadPort(useSetClearPin=True, useGetPin=True)

    @opensio
    def test_GPIO_SetPortDir_GetPortDir(self):
        for p in GPIO_PORTS:
            port = p[0]
            pin = p[1]
            pinmask = 1 << p[1]

            ret = self.sio.GPIO_SetPortInDir(port, pinmask)
            self.assertTrue(ret > 0, "GPIO SetPortInDir should return nonzero")
            dir, ret = self.sio.GPIO_GetPortDir(port)
            self.assertTrue(ret > 0, "GPIO GetPortDir should return nonzero")
            self.assertBitsEqual(dir, 0, pinmask, "GPIO port direction should be 0")

            ret = self.sio.GPIO_SetPortOutDir(port, pinmask)
            self.assertTrue(ret > 0, "GPIO SetPortOutDir should return nonzero")
            dir, ret = self.sio.GPIO_GetPortDir(port)
            self.assertTrue(ret > 0, "GPIO GetPortDir should return nonzero")
            self.assertBitsEqual(dir, 0xffffffff, pinmask, "GPIO port direction should be 1")

    @need_gpio_wiring
    @opensio
    def test_GPIO_InOut_Wired(self):
        for pair in GPIO_INOUT:
            for test in range(0,2):
                if(test):
                    inp = pair[0]
                    out = pair[1]
                else:
                    inp = pair[1]
                    out = pair[0]

                self.logger.info("GPIO test between output %d.%d connected to input %d.%d" % (out[0], out[1], inp[0], inp[1]))
                ret = self.sio.GPIO_SetPortOutDir(out[0], 1<<out[1])
                self.assertTrue(ret > 0, "GPIO SetPortOutDir should return nonzero")
                ret = self.sio.GPIO_SetPortInDir(inp[0], 1<<inp[1])
                self.assertTrue(ret > 0, "GPIO SetPortInDir should return nonzero")

                for n in range(0, GPIO_REPEATS):
                    bitval = n % 2
                    old, ret = self.sio.GPIO_WritePort(out[0], bitval << out[1])
                    self.assertTrue(ret > 0, "GPIO WritePort should return nonzero")
                    self.logger.debug("GPIO wrote port %d value 0x%x (pin%d=%d)" % (out[0], bitval << out[1], out[1], bitval))
                    val = self.sio.GPIO_GetPin(inp[0], inp[1])
                    self.logger.debug("GPIO read port.pin %d.%d, value=%d" % (inp[0], inp[1], val))
                    self.assertTrue(val == bitval, "GPIO input pin should get output value of connected pin")

                    val, ret = self.sio.GPIO_ReadPort(out[0])
                    self.assertTrue(ret > 0, "GPIO ReadPort should return nonzero")
                    self.logger.debug("GPIO read port %d, value=0x%d (pin%d=%d)" % (inp[0], val, inp[1], (val >> inp[1]) & 1))
                    self.assertTrue(((val >> inp[1]) & 1) == bitval, "GPIO input pin should get output value of connected pin")




