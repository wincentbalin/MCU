# Magnetic stripe Card Utility

MCU (short for *Magnetic stripe Card Utility*) is a software for reading
and decoding signals from an ultra-simple magnetic stripe card reader
described in the **2600** magazine Volume 22, issue #1, in the article called
*"Magnetic Stripe Reading"* on the page 28. This article is also republished
at http://sephail.net/articles/magstripe/ .

Technically, the application binds the [RtAudio library](https://github.com/thestk/rtaudio),
needed for platform-independent audio I/O, with the routines from `dab.c`/`dmsb.c`
files from the aforementioned article. The software is written in C++,
as it is needed by RtAudio.

## Compiling

Currently the program is trimmed to compile on Win* platform. Linux users
may either work with the original `dab.c`/`dmsb.c` sources or make MCU compile
under Linux (or any other UNIX) too.

### Prerequisites

0. MinGW compiler (for example: https://nuwen.net/mingw.html)

### Steps

1. Get RtAudio-4.1.0 library from https://github.com/thestk/rtaudio/releases/tag/4.1.0
   and unpack it into the directory with MCU sources

2. Type

```bash
make
```

And voilà, you have the executable. Run it with

```bash
./mcu -h
```

to get acquainted with the available options.


## TODO

* Integrate libsndfile for decoding files recorded previously
* Implement other bitstream decoders
