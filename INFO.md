# LIBUSBSIO Library

## Mission
The aim of this project is to deliver a dynamic library which enables USB-HID communication between PC Host 
and a target MCU over SPI, I2C or GPIO using a USB-connected bridge device. Two types of bridge devices are currently 
supported: LPCLink2 and MCULink. The library is supported on Windows/Linux/MacOS platforms.

The project uses the [HID_API](https://github.com/signal11/hidapi) open source library code and partially also its 
new [libusb/hidapi](https://github.com/libusb/hidapi) version to access 
the USB HID interface on all supported OS platforms.

## Links
- [Home page](https://www.nxp.com/design/software/development-software/library-for-windows-macos-and-ubuntu-linux:LIBUSBSIO)
- [MCUlink Pro](https://www.nxp.com/design/microcontrollers-developer-resources/mcu-link-pro-debug-probe:MCU-LINK-PRO)
- [LPCLink2](https://www.nxp.com/design/microcontrollers-developer-resources/lpc-microcontroller-utilities/lpc-link2:OM13054)

## Build
The library binaries are available in the repository. Default output path is bin/[platform]:
- `linux_x86_64`  ... ubuntu 20.04 x86_64 64bit version
- `linux_i686`    ... ubuntu 16.04 386 32bit version
- `linux_armv7l`  ... debian arm 32bit version, tested with imx6ulevk
- `linux_aarch64` ... debian arm 64bit version, tested with imx8mqevk
- `osx_x86_64`    ... macOS 11.3 64bit version
- `osx_arm64`     ... macOS ARM 64bit version (M1)
- `Win32`         ... Windows 32bit version (VS2015, build tools v14.0)
- `x64`           ... Windows 64bit version (VS2015, build tools v14.0)

The `bin_debug` folder contains the Debug build outputs.

### Windows
- Use Microsoft Visual Studio 2012 or later
- Open `vsproj/libusbsio.sln` or import the `vcxproj` project
- Build configurations exist for both Win32 and x64
- Select static library bulild (ReleaseS, DebugS targets)
  or DLL library build (Release, Debug)

### macOS
- The `makefile` is available
- Use `make` or `make debug` to build both static `.a` and dynamic `.dylib` libraries 

### Linux
- The `makefile` is available
- Use `make` or `make debug` to build both static `.a` and dynamic `.so` libraries 

----------------------------------
Copyright 2014, 2021-2022 NXP
