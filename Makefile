#
# Makefile for the Magnetic stripe Card Utility project
#

CC=gcc -mno-cygwin
RTAUDIO_VERSION=4.0.7
RTAUDIO_SRC=rtaudio-$(RTAUDIO_VERSION)
INCLUDES=-I"." -I$(RTAUDIO_SRC) -I$(RTAUDIO_SRC)/include
CFLAGS=-D__WINDOWS_DS__ $(INCLUDES) -O2 -c
LIBS=-lole32 -lwinmm -lWsock32 -ldsound -lstdc++ -lm
OBJS=mcu.o RtAudio.o

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

