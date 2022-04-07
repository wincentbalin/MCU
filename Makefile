#
# Makefile for the Magnetic stripe Card Utility project
#

CC=gcc

RTAUDIO_VERSION=4.1.0
RTAUDIO_SRC=rtaudio-$(RTAUDIO_VERSION)
INCLUDES=-I"." -I$(RTAUDIO_SRC) -I$(RTAUDIO_SRC)/include
CFLAGS=$(INCLUDES) -O2 -c
LDFLAGS=-s
OBJS=mcu.o RtAudio.o

ifdef OS
CFLAGS+=-D__WINDOWS_DS__
LIBS=-lole32 -lwinmm -lWsock32 -ldsound -lstdc++ -lm
RM=del
else
CFLAGS+=-D__LINUX_ALSA__
LIBS=-lasound -lstdc++ -lm
RM=rm -f
endif

all: mcu

mcu: $(OBJS)
	$(CC) -o mcu $(LDFLAGS) $(OBJS) $(LIBS)

mcu.o:	mcu.cpp mcu.hpp
	$(CC) $(CFLAGS) mcu.cpp

RtAudio.o:
	$(CC) $(CFLAGS) $(RTAUDIO_SRC)/$*.cpp

clean:
	$(RM) mcu mcu.exe *~ *.o

