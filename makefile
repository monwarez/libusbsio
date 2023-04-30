# 
#  Copyright 2014, 2021-2022 NXP
# 
#  SPDX-License-Identifier: BSD-3-Clause
# 
#  NXP USBSIO Library Makefile
# 

CC = gcc
CXX = g++
AR = ar -rcs
UNAME := $(shell uname)
UNAME_M := $(shell uname -m)

VPATH := src
SRCS := lpcusbsio.c hid.c
CFLAGS += -Iinclude -Isrc/hid_api/hidapi -fPIC -Wall -c
dir_guard = @mkdir -p $(@D)


#
# Linux
#
ifeq ($(UNAME), Linux)
# do not forget to
#  sudo apt-get install libudev-dev
#  sudo apt-get install libusb-1.0-0-dev
#  sudo chmod 666 /dev/hidraw*  or add udev rule (use lsusb to find out VID/PID)
#     KERNEL=="hidraw*", ATTRS{idVendor}=="1fc9", ATTRS{idProduct}=="XXXX", MODE="0666"
BINDIR = linux_$(UNAME_M)
VPATH += src/hid_api/linux
LDFLAGS +=  -shared
LIBS += `pkg-config libusb-1.0 libudev --libs`
LIBNAME_A = libusbsio.a
LIBNAME_SO = libusbsio.so
endif

#
# macOS
#
ifeq ($(UNAME), Darwin)
BINDIR = osx_$(UNAME_M)
VPATH += src/hid_api/mac
LDFLAGS += -dynamiclib
LIBS += -framework IOKit -framework CoreFoundation -framework AppKit
LIBNAME_A = libusbsio.a
LIBNAME_SO = libusbsio.dylib
endif

OBJS = $(SRCS:.c=.o)

#
# Debug build settings
#
DBGDIR = bin_debug/$(BINDIR)
DBGOBJ = obj/debug
DBGLIB_A = $(DBGDIR)/$(LIBNAME_A)
DBGLIB_SO = $(DBGDIR)/$(LIBNAME_SO)
DBGOBJS = $(addprefix obj/debug/, $(OBJS))
DBGCFLAGS = $(CFLAGS) -g -O0 -DDEBUG -D_DEBUG

#
# Release build settings
#
RELDIR = bin/$(BINDIR)
RELOBJ = obj/release
RELLIB_A = $(RELDIR)/$(LIBNAME_A)
RELLIB_SO = $(RELDIR)/$(LIBNAME_SO)
RELOBJS = $(addprefix $(RELOBJ)/, $(OBJS))
RELCFLAGS = $(CFLAGS) -O3 -DNDEBUG

#
# Rules
#
all: release
both: release debug

#
# Debug rules
#
debug: $(DBGLIB_A) $(DBGLIB_SO)

$(DBGLIB_A): $(DBGOBJS)
	$(dir_guard)
	$(AR) $@ $^

$(DBGLIB_SO): $(DBGOBJS)
	$(dir_guard)
	$(CC) -o $@ $(LDFLAGS) $^ $(LIBS)

$(DBGOBJ)/%.o: %.c
	$(dir_guard)
	$(CC) -o $@ $(DBGCFLAGS) $<

#
# Release rules
#
release: $(RELLIB_A) $(RELLIB_SO)

$(RELLIB_A): $(RELOBJS)
	$(dir_guard)
	$(AR) $@ $^

$(RELLIB_SO): $(RELOBJS)
	$(dir_guard)
	$(CC) -o $@ $(LDFLAGS) $^ $(LIBS)

$(RELOBJ)/%.o: %.c
	$(dir_guard)
	$(CC) -o $@ $(RELCFLAGS) $<

clean:
	rm -f $(RELOBJS) $(RELLIB_A) $(RELLIB_SO)
	rm -f $(DBGOBJS) $(DBGLIB_A) $(DBGLIB_SO)

.PHONY: clean
.PHONY: all
