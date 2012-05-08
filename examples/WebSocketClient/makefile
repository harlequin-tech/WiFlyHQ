#_______________________________________________________________________________
#
#                         edam's arduino makefile
#_______________________________________________________________________________
#                                                                    version 0.2
#
# Copyright (c) 2011 Tim Marston <tim@ed.am>.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
#_______________________________________________________________________________
#
#
# This is a general purpose makefile for use with Arduino hardware and
# software.  It works with the arduino-1.0 release and requires that software
# to be downloaded separately (see http://arduino.cc/).  To download the latest
# version of this makefile, visit the following website, where you can also
# find more information and documentation on it's use.  The following text can
# only really be considered a reference to it's use.
#
#   http://ed.am/dev/make/arduino-mk
#
# This makefile can be used as a drop-in replacement for the Arduino IDE's
# build system.  To use it, save arduino.mk somewhere (I keep mine at
# ~/src/arduino.mk) and create a symlink to it in your project directory named
# "Makefile".  For example:
#
#   $ ln -s ~/src/arduino.mk Makefile
#
# You also need to set up a couple of environment variables. ARDUINODIR should
# be set to the path where you unpacked the arduino software from arduino.cc
# (it defaults to ~/opt/arduino if unset).  You might be best to set this in
# your ~/.profile by adding something like this:
#
#   export ARDUINODIR=~/somewhere/arduino-1.0
ARDUINODIR=/Applications/Arduino.app/Contents/Resources/Java
SKETCHES=$(HOME)/Documents/Arduino
BOARD=uno
BAUD=115200
#SERIALDEV=/dev/tty.usbserial-A400h9rM
#SERIALDEV=/dev/tty.usbserial
#SERIALDEV=/dev/tty.usbmodem621
#
# You will also need to set BOARD to the type of arduino you're using.  This
# can be done when running make (or you could set a default in ~/.profile and
# override it as necessary).  For example:
#
#   $ export BOARD=uno
#   $ make
#
# You may also need to set SERIALDEV if it is not detected correctly.
#
# The presence of a .ino (or .pde) file causes the arduino.mk to automatically
# determine values for SOURCES, TARGET and LIBRARIES.  Any .c, .cc and .cpp
# files in the project directory (or any "util" or "utility" subdirectories)
# are automatically included in the build and are scanned for Arduino libraries
# that have been #included. Note, there can only be one .ino (or .pde) file.
#
# Alternatively, if you want to manually specify build variables, create a
# Makefile that defines SOURCES and LIBRARIES and then includes arduino.mk.
# (There is no need to define TARGET).  Here is an example Makefile:
#
#   SOURCES := main.cc other.cc
#   LIBRARIES := EEPROM
#   include ~/src/arduino.mk
#
# Here is a complete list of configuration parameters:
#
# ARDUINODIR   The path where you have installed/unpacked the arduino software
#              (from http://arduino.cc/)
#
# ARDUINOCONST The arduino software version, as an integer, used to define the
#              ARDUINO version constant. This defaults to 100 if undefined.
#
# AVRDUDECONF  The avrdude.conf to use. If undefined, this defaults to a guess
#              based on where the avrdude in use is. If empty, no avrdude.conf
#              is passed to avrdude (to the system default is used).
#
# AVRTOOLSPATH A space-separated list of directories to search in order when
#              lookin for the avr build tools. This defaults to the system PATH
#              followed by subdirectories in ARDUINODIR if undefined.
#
# BOARD        Specify a target board type.  Run `make boards` to see available
#              board types.
#
# LIBRARIES    A list of arduino libraries to build and include.  This is set
#              automatically if a .ino (or .pde) is found.
#
# SERIALDEV    The unix device name of the serial device that is the arduino.
#              If unspecified, an attempt is made to determine the name of a
#              connected arduino's serial device.
#
# SOURCES      A list of all source files of whatever language.  The language
#              type is determined by the file extension.  This is set
#              automatically if a .ino (or .pde) is found.
#
# TARGET       The name of the target file.  This is set automatically if a
#              .ino (or .pde) is found, but it is not necessary to set it
#              otherwise.
#
# This makefile also defines the following goals for use on the command line
# when you run make:
#
# all          This is the default if no goal is specified.  It builds the
#              target and uploads it.
#
# target       Builds the target.
#
# upload       Uploads the last built target to an attached arduino.
#
# clean        Deletes files created during the build.
#
# boards       Display a list of available board names, so that you can set the
#              BOARD environment variable appropriately.
#
# monitor      Start `screen` on the serial device.  This is meant to be an
#              equivalent to the arduino serial monitor.
#
# <file>       Builds the specified file, either an object file or the target,
#              from those that that would be built for the project.
#_______________________________________________________________________________
#

# default arduino software directory, check software exists
ifndef ARDUINODIR
ARDUINODIR := $(wildcard ~/opt/arduino)
endif
ifeq "$(wildcard $(ARDUINODIR)/hardware/arduino/boards.txt)" ""
$(error ARDUINODIR is not set correctly; arduino software not found)
endif

# default arduino version
ARDUINOCONST ?= 100

# default path for avr tools
ifndef AVRTOOLSPATH
AVRTOOLSPATH := $(subst :, , $(PATH))
AVRTOOLSPATH += $(ARDUINODIR)/hardware/tools
AVRTOOLSPATH += $(ARDUINODIR)/hardware/tools/avr/bin
endif

# auto mode?
INOFILE := $(wildcard *.ino *.pde)
ifdef INOFILE
ifneq "$(words $(INOFILE))" "1"
$(error There is more than one .pde or .ino file in this directory!)
endif

# automatically determine sources and targeet
TARGET := $(basename $(INOFILE))
SOURCES := $(INOFILE) \
	$(wildcard *.c *.cc *.cpp) \
	$(wildcard $(addprefix util/, *.c *.cc *.cpp)) \
	$(wildcard $(addprefix utility/, *.c *.cc *.cpp))

# automatically determine included libraries
ARDUINOLIBSAVAIL := $(notdir $(wildcard $(ARDUINODIR)/libraries/*))
LIBRARIES := $(filter $(ARDUINOLIBSAVAIL), \
	$(shell sed -ne "s/^ *\# *include *[<\"]\(.*\)\.h[>\"]/\1/p" $(SOURCES)))

LOCAL_LIBS := $(filter $(notdir $(wildcard $(SKETCHES)/libraries/*)), \
	$(shell sed -ne "s/^ *\# *include *[<\"]\(.*\)\.h[>\"]/\1/p" $(SOURCES)))

endif

# no serial device? make a poor attempt to detect an arduino
SERIALDEVGUESS := 0
ifeq "$(SERIALDEV)" ""
SERIALDEV := $(firstword $(wildcard \
	/dev/ttyACM? /dev/ttyUSB? /dev/tty.usbmodem*))
SERIALDEVGUESS := 1
endif

# software
findsoftware = $(firstword $(wildcard $(addsuffix /$(1), $(AVRTOOLSPATH))))
CC := $(call findsoftware,avr-gcc)
CXX := $(call findsoftware,avr-g++)
LD := $(call findsoftware,avr-ld)
AR := $(call findsoftware,avr-ar)
OBJCOPY := $(call findsoftware,avr-objcopy)
AVRDUDE := $(call findsoftware,avrdude)

# files
TARGET := $(if $(TARGET),$(TARGET),a.out)
OBJECTS := $(addsuffix .o, $(basename $(SOURCES)))
ARDUINOSRCDIR := $(ARDUINODIR)/hardware/arduino/cores/arduino
ARDUINOLIBSDIR := $(ARDUINODIR)/libraries
ARDUINOLIB := _arduino.a
ARDUINOLIBTMP := $(ARDUINOLIB).tmp
ARDUINOLIBOBJS := $(patsubst %, $(ARDUINOLIBTMP)/%.o, $(basename $(notdir \
	$(wildcard $(addprefix $(ARDUINOSRCDIR)/, *.c *.cpp)))))
ARDUINOLIBOBJS += $(foreach lib, $(LIBRARIES), \
	$(patsubst %, $(ARDUINOLIBTMP)/%.o, $(basename $(notdir \
	$(wildcard $(addprefix $(ARDUINOLIBSDIR)/$(lib)/, *.c *.cpp)) \
	$(wildcard $(addprefix $(ARDUINOLIBSDIR)/$(lib)/utility/, *.c *.cpp)) ))))
ARDUINOLIBOBJS += $(foreach lib, $(LOCAL_LIBS), \
	$(patsubst %, $(ARDUINOLIBTMP)/%.o, $(basename $(notdir \
	$(wildcard $(addprefix $(SKETCHES)/libraries/$(lib)/, *.c *.cpp)) ))))
ifeq "$(AVRDUDECONF)" ""
ifeq "$(AVRDUDE)" "$(ARDUINODIR)/hardware/tools/avr/bin/avrdude"
AVRDUDECONF := $(ARDUINODIR)/hardware/tools/avr/etc/avrdude.conf
else
AVRDUDECONF := $(wildcard $(AVRDUDE).conf)
endif
endif

# no board?
ifndef BOARD
ifneq "$(MAKECMDGOALS)" "boards"
ifneq "$(MAKECMDGOALS)" "clean"
$(error BOARD is unset.  Type 'make boards' to see possible values)
endif
endif
endif

# obtain board parameters from the arduino boards.txt file
BOARDS_FILE := $(ARDUINODIR)/hardware/arduino/boards.txt
BOARD_BUILD_MCU := \
	$(shell sed -ne "s/$(BOARD).build.mcu=\(.*\)/\1/p" $(BOARDS_FILE))
BOARD_BUILD_FCPU := \
	$(shell sed -ne "s/$(BOARD).build.f_cpu=\(.*\)/\1/p" $(BOARDS_FILE))
BOARD_BUILD_VARIANT := \
	$(shell sed -ne "s/$(BOARD).build.variant=\(.*\)/\1/p" $(BOARDS_FILE))
BOARD_UPLOAD_SPEED := \
	$(shell sed -ne "s/$(BOARD).upload.speed=\(.*\)/\1/p" $(BOARDS_FILE))
BOARD_UPLOAD_PROTOCOL := \
	$(shell sed -ne "s/$(BOARD).upload.protocol=\(.*\)/\1/p" $(BOARDS_FILE))

# invalid board?
ifeq "$(BOARD_BUILD_MCU)" ""
ifneq "$(MAKECMDGOALS)" "boards"
ifneq "$(MAKECMDGOALS)" "clean"
$(error BOARD is invalid.  Type 'make boards' to see possible values)
endif
endif
endif

# flags
CPPFLAGS := -Os -Wall -fno-exceptions -ffunction-sections -fdata-sections
CPPFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CPPFLAGS += -mmcu=$(BOARD_BUILD_MCU)
CPPFLAGS += -DF_CPU=$(BOARD_BUILD_FCPU) -DARDUINO=$(ARDUINOCONST)
CPPFLAGS += -I. -Iutil -Iutility -I$(ARDUINOSRCDIR)
CPPFLAGS += -I$(ARDUINODIR)/hardware/arduino/variants/$(BOARD_BUILD_VARIANT)/
CPPFLAGS += $(addprefix -I$(ARDUINODIR)/libraries/, $(LIBRARIES))
CPPFLAGS += $(patsubst %, -I$(ARDUINODIR)/libraries/%/utility, $(LIBRARIES))
CPPFLAGS += $(addprefix -I$(SKETCHES)/libraries/, $(LOCAL_LIBS))
AVRDUDEFLAGS := $(addprefix -C , $(AVRDUDECONF)) -DV
AVRDUDEFLAGS += -p $(BOARD_BUILD_MCU) -P $(SERIALDEV)
AVRDUDEFLAGS += -c $(BOARD_UPLOAD_PROTOCOL) -b $(BOARD_UPLOAD_SPEED)
LINKFLAGS := -Os -Wl,--gc-sections -mmcu=$(BOARD_BUILD_MCU)

# figure out which arg to use with stty
STTYFARG := $(shell stty --help > /dev/null 2>&1 && echo -F || echo -f)

.SECONDARY:

# default rule
.DEFAULT_GOAL := all

#_______________________________________________________________________________
#                                                                          RULES

.PHONY:	all target upload clean boards monitor

all: target upload

target: $(TARGET).hex

upload:
	@echo "\nUploading to board..."
	@test -n "$(SERIALDEV)" || { \
		echo "error: SERIALDEV could not be determined automatically." >&2; \
		exit 1; }
	@test 0 -eq $(SERIALDEVGUESS) || { \
		echo "*GUESSING* at serial device:" $(SERIALDEV); \
		echo; }
	stty $(STTYFARG) $(SERIALDEV) hupcl
	$(AVRDUDE) $(AVRDUDEFLAGS) -U flash:w:$(TARGET).hex:i

clean:
	rm -f $(OBJECTS)
	rm -f $(TARGET).elf $(TARGET).hex $(ARDUINOLIB) *~
	rm -rf $(ARDUINOLIBTMP)

boards:
	@echo Available values for BOARD:
	@sed -ne '/^#/d;s/^\(.*\).name=\(.*\)/\1            \2/;T' \
		-e 's/\(.\{12\}\) *\(.*\)/\1 \2/;p' $(BOARDS_FILE)

monitor:
	@test -n "$(SERIALDEV)" || { \
		echo "error: SERIALDEV could not be determined automatically." >&2; \
		exit 1; }
	@test -n `which screen` || { \
		echo "error: can't find GNU screen, you might need to install it." >&2 \
		ecit 1; }
	@test 0 -eq $(SERIALDEVGUESS) || { \
		echo "*GUESSING* at serial device:" $(SERIALDEV); \
		echo; }
	#screen $(SERIALDEV) $(BAUD)
	minicom -D $(SERIALDEV) -b $(BAUD) -o

# building the target

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

.INTERMEDIATE: $(TARGET).elf

$(TARGET).elf: $(ARDUINOLIB) $(OBJECTS)
	$(CC) $(LINKFLAGS) $(OBJECTS) $(ARDUINOLIB) -o $@

%.o: %.ino
	$(COMPILE.cpp) -o $@ -x c++ -include $(ARDUINOSRCDIR)/Arduino.h $<

%.o: %.pde
	$(COMPILE.cpp) -o $@ -x c++ -include $(ARDUINOSRCDIR)/Arduino.h $<

# building the arduino library

$(ARDUINOLIB): $(ARDUINOLIBOBJS)
	$(AR) rcs $@ $?
	rm -rf $(ARDUINOLIBTMP)

.INTERMEDIATE: $(ARDUINOLIBOBJS)

$(ARDUINOLIBTMP)/%.o: $(SKETCHES)/libraries/*/%.c
	@test -d $(ARDUINOLIBTMP) || mkdir $(ARDUINOLIBTMP)
	$(COMPILE.c) -o $@ $<

$(ARDUINOLIBTMP)/%.o: $(SKETCHES)/libraries/*/%.cpp
	@test -d $(ARDUINOLIBTMP) || mkdir $(ARDUINOLIBTMP)
	$(COMPILE.cpp) -o $@ $<

$(ARDUINOLIBTMP)/%.o: $(ARDUINOSRCDIR)/%.c
	@test -d $(ARDUINOLIBTMP) || mkdir $(ARDUINOLIBTMP)
	$(COMPILE.c) -o $@ $<

$(ARDUINOLIBTMP)/%.o: $(ARDUINOSRCDIR)/%.cpp
	@echo hello
	echo $(ARDUINOLIBOBJS)
	echo $(LOCAL_LIBS)
	@test -d $(ARDUINOLIBTMP) || mkdir $(ARDUINOLIBTMP)
	$(COMPILE.cpp) -o $@ $<

$(ARDUINOLIBTMP)/%.o: $(ARDUINODIR)/libraries/*/%.c
	@test -d $(ARDUINOLIBTMP) || mkdir $(ARDUINOLIBTMP)
	$(COMPILE.c) -o $@ $<

$(ARDUINOLIBTMP)/%.o: $(ARDUINODIR)/libraries/*/%.cpp
	@test -d $(ARDUINOLIBTMP) || mkdir $(ARDUINOLIBTMP)
	$(COMPILE.cpp) -o $@ $<

$(ARDUINOLIBTMP)/%.o: $(ARDUINODIR)/libraries/*/utility/%.c
	@test -d $(ARDUINOLIBTMP) || mkdir $(ARDUINOLIBTMP)
	$(COMPILE.c) -o $@ $<

$(ARDUINOLIBTMP)/%.o: $(ARDUINODIR)/libraries/*/utility/%.cpp
	@test -d $(ARDUINOLIBTMP) || mkdir $(ARDUINOLIBTMP)
	$(COMPILE.cpp) -o $@ $<
