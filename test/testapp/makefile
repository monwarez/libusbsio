# 
#  Copyright 2014, 2021-2022 NXP
# 
#  SPDX-License-Identifier: BSD-3-Clause
# 
#  NXP USBSIO Library: test application Makefile
# 

CC=gcc
CXX=g++
UNAME := $(shell uname)
UNAME_M := $(shell uname -m)
CFLAGS := -c -I../../include -Wall -Wno-unused-result
SRCS = testapp.c test_gpio.c test_i2c.c test_spi.c

dir_guard=@mkdir -p $(@D)

#
# Linux
#
ifeq ($(UNAME), Linux)
BINDIR = linux_$(UNAME_M)
LIBS = `pkg-config libusb-1.0 libudev --libs`
CFLAGS += `pkg-config libusb-1.0 --cflags`
LDFLAGS += -pthread
LIBNAME_A = libusbsio.a
LIBNAME_SO = libusbsio.so
endif

#
# macOS
#
ifeq ($(UNAME), Darwin)
BINDIR = osx_$(UNAME_M)
LIBS = -framework IOKit -framework CoreFoundation -framework AppKit
LDFLAGS += -pthread
LIBNAME_A = libusbsio.a
LIBNAME_SO = libusbsio.dylib
endif

OBJS = $(SRCS:.c=.o)

#
# Debug build settings
#
DBGDIR = ../../bin_debug/$(BINDIR)
DBGOBJ = obj/debug
DBGAPP = $(DBGDIR)/testapp
DBGLIB_A = $(DBGDIR)/$(LIBNAME_A)
DBGLIB_SO = $(DBGDIR)/$(LIBNAME_SO)
DBGOBJS = $(addprefix obj/debug/, $(OBJS))
DBGCFLAGS = $(CFLAGS) -g -O0 -DDEBUG -D_DEBUG

#
# Release build settings
#
RELDIR = ../../bin/$(BINDIR)
RELOBJ = obj/release
RELAPP = $(RELDIR)/testapp
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
debug: $(DBGAPP)

$(DBGAPP): $(DBGOBJS) $(DBGLIB_A)
	$(dir_guard)
	$(CC) -o $@ $(LDFLAGS) $^ $(DBGLIB_A) $(LIBS)

$(DBGOBJ)/%.o: %.c
	$(dir_guard)
	$(CC) -o $@ $(DBGCFLAGS) $<

#
# Release rules
#
release: $(RELAPP)

$(RELAPP): $(RELOBJS) $(RELLIB_A)
	$(dir_guard)
	$(CC) -o $@ $(LDFLAGS) $^ $(RELLIB_A) $(LIBS)

$(RELOBJ)/%.o: %.c
	$(dir_guard)
	$(CC) -o $@ $(RELCFLAGS) $<

clean:
	rm -f $(RELOBJS) $(RELAPP)
	rm -f $(DBGOBJS) $(DBGAPP)





.PHONY: clean
.PHONY: all
