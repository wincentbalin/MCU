#
# Makefile for the Magnetic stripe Card Utility project
#

CC=gcc

RTAUDIO_VERSION=4.0.7
RTAUDIO_SRC=rtaudio-$(RTAUDIO_VERSION)
INCLUDES=-I"." -I$(RTAUDIO_SRC) -I$(RTAUDIO_SRC)/include
CFLAGS=$(INCLUDES) -O2 -c
OBJS=mcu.o RtAudio.o

ifeq ($(findstring CYGWIN,$(shell uname)), CYGWIN)
CFLAGS+=-mno-cygwin
LDFLAGS+=-mno-cygwin
endif

ifdef COMSPEC
CFLAGS+=-D__WINDOWS_DS__
LIBS=-lole32 -lwinmm -lWsock32 -ldsound -lstdc++ -lm
else
CFLAGS+=-D__LINUX_ALSA__
LIBS=-lasound -lstdc++ -lm
endif

all: mcu

Debug: all

Release: all

mcu: $(OBJS)
	$(CC) -o mcu $(OBJS) $(LIBS)

mcu.o:	mcu.cpp mcu.hpp
	$(CC) $(CFLAGS) mcu.cpp

RtAudio.o:
	$(CC) $(CFLAGS) $(RTAUDIO_SRC)/$*.cpp

clean:
	rm -f mcu mcu.exe *~ *.o

