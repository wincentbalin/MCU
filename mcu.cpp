/**
    mcu.cpp

    Main source file.

    Part of Magnetic stripe Card Utility.

    Copyright (c) 2010 Wincent Balin

    Based heavily upon dab.c and dmsb.c by Joseph Battaglia.
*/

#include "mcu.hpp"
#include "RtAudio.h"
#include <iostream>
#include <cstdlib>
#include <getopt.h>


#define VERSION 0.1
#define AUTO_THRES 5000


using namespace std;


void
print_version(void)
{
    cerr << "mcu - Magnetic stripe Card Utility" << endl
         << "Version " << VERSION << endl
         << "Copyright (c) 2010 Wincent Balin" << endl
         << endl;
}

void
print_help(void)
{
    print_version();

    cerr << "Usage: mcu [OPTIONS]" << endl
         << endl
         << "  -a,  --auto-thres   Set auto-thres percentage" << endl
         << "                      (default: " << AUTO_THRES << ")" << endl
         << "  -d,  --device       Device (number) to read audio data from" << endl
         << "                      (default: 0)" << endl
         << "  -l,  --list-devices List compatible devices (enumerated)" << endl
         << "  -h,  --help         Print help information" << endl
         << "  -m,  --max-level    Shows the maximum level" << endl
         << "                      (use to determine threshold)" << endl
         << "  -s,  --silent       No verbose messages" << endl
         << "  -t,  --threshold    Set silence threshold" << endl
         << "                      (default: automatic detect)" << endl
         << "  -v,  --version      Print version information" << endl
         << endl;
}

int main(int argc, char** argv)
{
    print_help();

    return EXIT_SUCCESS;
}
