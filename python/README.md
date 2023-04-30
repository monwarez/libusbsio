# Python wrapper for NXP libusbsio binary library

The [NXP libusbsio](https://www.nxp.com/design/software/development-software/library-for-windows-macos-and-ubuntu-linux:LIBUSBSIO) 
is a binary library for Win/Linux/MacOS systems used to exercise SPI, I2C bus and GPIO pins over USBSIO interface of NXP
[LPCLink2](https://www.nxp.com/design/microcontrollers-developer-resources/lpc-microcontroller-utilities/lpc-link2:OM13054)
and [MCUlink Pro](https://www.nxp.com/design/microcontrollers-developer-resources/mcu-link-pro-debug-probe:MCU-LINK-PRO)
devices.

This Python component provides a wrapper object which encapsulates the binary library
and exposes its API to Python applications.

Author: michal.hanak@nxp.com (https://www.nxp.com)

## Dependencies
There are no dependencies to any external modules needed to use the LIBUSBSIO
module.

The binary libraries use the [HID_API](https://github.com/signal11/hidapi) library code and partially also its 
new [libusb/hidapi](https://github.com/libusb/hidapi) version to access 
the USB HID interface on all supported OS platforms.

## Installation
### pypi.org
Use pip to download and install the package
```
python -m pip install libusbsio
```
### Local
Use the following `pip` command to install the libusbsio module from
the local NXP LIBUSBSIO installation package available at www.nxp.com:
```
python -m pip install dist/libusbsio-2.1.11-py3-none-any.whl
```

## Running example code
Running the example code is easy. You do not even need to install the package,
the example code will locate the module in the local directory (in ../libusbsio).

Go to `examples` directory and see the demo scripts there.
Examine the script and the way how it creates the `LIBUSBSIO` object.

Without any constructor parameters, the USBSIO library is automatically
located. There are also options to load the library from a given path.
```
from libusbsio import *
sio = LIBUSBSIO()
```

Use the `loglevel` parameter to specify logging verbosity:
```
import logging
from libusbsio import *

logging.basicConfig()
sio = LIBUSBSIO(loglevel=logging.INFO)
```

## Running test code
The test code is located in the `test` directory and it is ready to be used with the
`unittest` or `pytest`. *Note that most of the tests assume that the target MCU application 
runs the `siotest1` application test code which answers on SPI and I2C buses using a simple
command/response protocol.* The source code of the `siotest` application for 
different target boards is available as an optional part of the NXP MCUXpresso SDK package.

Run one of the following commands in the base 'python' directory. Note that some long
duration tests and known-issues tests are skipped by default. 
See more in `test/test_base.py` main test file.

```
python -m unittest
.............s.......s...s...sssss.........s.
----------------------------------------------------------------------
Ran 45 tests in 11.034s

OK (skipped=9)
```

or:

```
pyttest

===================================== test session starts ======================================
platform win32 -- Python 3.8.7, pytest-6.2.3, py-1.10.0, pluggy-0.13.1
rootdir: d:\gitwork\libusbsio\python
collected 45 items

test\test_00_lib.py ..........                                                            [ 22%]
test\test_10_spi.py ...s....                                                              [ 40%]
test\test_20_i2c.py ...s...s...s...ssss                                                   [ 82%]
test\test_30_gpio.py ......s.                                                             [100%]

================================ 36 passed, 9 skipped in 11.38s ================================
```

## History
### v2.1.11 - February 2022
- Source code of the library made available under BSD-3-Clause license.
- macOS now use the libusb/hidapi version to use a new format of HID device path.
### v2.1.10 - February 2022
- Add GetDeviceInfo method to retrieve hid_api low-level information of the SIO port
### v2.1.8 - November 2021
- Add binaries for linux_armv7l and linux_aarch64
### v2.1.5 - August 2021
- Add binaries and support for arm64 macOS
### v2.1.4 - July 2021
- Extend by low-level HID_API access, fix read buffer length when reading data.
- Refactor "PIDVID" variables, arguments and tuples to a correctly ordered "VIDPID"
- Fix example code.
### v2.1.0 - April 2021
- The initial Python libusbsio library wrapper release supporting USBSIO library v2.1

____
Copyright NXP 2021-2022
