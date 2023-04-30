#!/usr/bin/env python3
#
# Copyright 2021-2022 NXP
# SPDX-License-Identifier: BSD-3-Clause
#
# TEST CODE of NXP USBSIO Library - Library-level tests
#

import unittest
import functools
import logging
import sys
import os

from test import *


def opensio(func):
    """Decorator to automatically open the SIO library"""

    @functools.wraps(func)
    def wrapper(self, *args, **kwargs):
        ok = self.sio.IsOpen()
        if not ok:
            ports = self.sio.GetNumPorts(vidpids=VIDPIDS)
            self.assertTrue(ports > 0)
            if ports > 0:
                ok = self.sio.Open(0)
                self.assertTrue(ok)
        if ok:
            return func(self, *args, **kwargs)
        else:
            raise Exception(
                "The 'opensio' decorator has failed to load or open the library"
            )

    return wrapper


class TestLib(TestBase):
    def test_Constructor(self):
        self.assertTrue(self.sio)
        self.assertTrue(self.sio._dll)
        self.assertTrue(self.sio._dllpath)
        self.assertFalse(self.sio._h)
        self.tearDown()

        with self.assertRaises(LIBUSBSIO_Exception):
            sio = LIBUSBSIO("bad_path")
            del sio

        with LIBUSBSIO(autoload=False) as sio:
            with self.assertRaises(Exception):
                sio.GetNumPorts(vidpids=VIDPIDS)
            with self.assertRaises(Exception):
                sio.Open()
            with self.assertRaises(Exception):
                sio.GetVersion()

            self.assertFalse(sio._dll)
            self.assertTrue(
                isinstance(sio.LoadDLL("bad_path", raiseOnError=False), Exception)
            )
            self.assertEqual(sio.LoadDLL(raiseOnError=False), None)
            self.assertTrue(sio._dll)

    def test_Singleton(self):
        self.tearDown()

        sio = usbsio()
        self.assertTrue(sio)
        self.assertTrue(sio._dll)
        self.assertTrue(sio._dllpath)
        self.assertFalse(sio._h)
        sio2 = usbsio()
        self.assertEqual(sio, sio2)

    def test_GetNumPorts(self):
        v = self.sio.GetNumPorts(vidpids=[LIBUSBSIO.VIDPID_LPCLINK2])
        self.logger.info("GetNumPorts(LPCLINK2) returns %s" % v)
        v = self.sio.GetNumPorts(vidpids=[LIBUSBSIO.VIDPID_MCULINK])
        self.logger.info("GetNumPorts(MCULINK) returns %s" % v)
        v = self.sio.GetNumPorts()
        self.logger.info("GetNumPorts() returns %s" % v)
        self.assertGreaterEqual(v, 1)

    def test_GetDeviceInfo(self):
        v = self.sio.GetNumPorts()
        self.logger.info("GetNumPorts() returns %s" % v)
        self.assertGreaterEqual(v, 1)
        for d in range(0, v):
            info = self.sio.GetDeviceInfo(d)
            self.logger.info("GetDeviceInfo(%d) returns %s" % (d, type(info)))
            self.assertIsNotNone(info)
            self.logger.info("  manufacturer_string=%s" % info.manufacturer_string)
            self.logger.info("  product_string=%s" % info.product_string)
            self.logger.info("  serial_number=%s" % info.serial_number)
            self.logger.info("  path=%s" % info.path)
            self.assertGreater(
                len(info.serial_number), 0, "GetDeviceInfo path not empty"
            )
            self.assertGreater(len(info.path), 0, "GetDeviceInfo path not empty")

    @opensio
    def test_OpenClose(self):
        self.assertTrue(self.sio._h)
        self.assertEqual(self.sio.Close(), 0)
        self.assertFalse(self.sio._h)

    @opensio
    def test_GetVersion(self):
        v = self.sio.GetVersion()
        self.logger.info("GetVersion returns %s" % v)
        self.assertTrue(v.find("NXP LIBUSBSIO v2") == 0)

    @opensio
    def test_Error(self):
        v = self.sio.Error()
        self.logger.info("Error returns %s" % v)
        self.assertTrue(isinstance(v, str))
        self.assertEqual(v, "No errors are recorded.")

    @opensio
    def test_GetNumI2CPorts(self):
        n = self.sio.GetNumI2CPorts()
        self.logger.info("GetNumI2CPorts returns %d" % n)
        self.assertTrue(n > 0)

    @opensio
    def test_GetNumSPIPorts(self):
        n = self.sio.GetNumSPIPorts()
        self.logger.info("GetNumSPIPorts returns %d" % n)
        self.assertTrue(n > 0)

    @opensio
    def test_GetNumGPIOPorts(self):
        n = self.sio.GetNumGPIOPorts()
        self.logger.info("GetNumGPIOPorts returns %d" % n)
        self.assertTrue(n > 0)

    @opensio
    def test_GetMaxDataSize(self):
        n = self.sio.GetMaxDataSize()
        self.logger.info("GetMaxDataSize returns %d" % n)
        self.assertTrue(n >= 1024)

    @opensio
    def test_GetLastError(self):
        n = self.sio.GetLastError()
        self.logger.info("GetLastError returns %d" % n)
        self.assertEqual(n, 0)
