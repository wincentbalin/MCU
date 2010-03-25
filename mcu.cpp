/**
    mcu.cpp

    Main source file.

    Part of Magnetic stripe Card Utility.

    Copyright (c) 2010 Wincent Balin

    Based heavily upon dab.c and dmsb.c by Joseph Battaglia.

    As the mentioned above inspirations the program is licensed
    under the MIT License. See LICENSE file for further information.
*/

#include "mcu.hpp"
#include "RtAudio.h"
#include <iostream>
#include <cstdlib>
#include <getopt.h>


// Version of the program
#define VERSION 0.1

// Initial silence threshold
#define SILENCE_THRES 5000

// Percent of highest value to set silence_thres to
#define AUTO_THRES 30

// Frequency threshold (in percent)
#define FREQ_THRES 60


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
    // Configuration variables
    int auto_thres = AUTO_THRES;
    bool max_level = false;
    bool verbose = true;
    int silence_thres = SILENCE_THRES;
    int device_number = 0;
    bool list_devices = false;

    // Getopt variables
    int ch, option_index;
    static struct option long_options[] =
    {
        {"auto-thres",   0, 0, 'a'},
        {"device",       1, 0, 'd'},
        {"list-devices", 0, 0, 'l'},
        {"help",         0, 0, 'h'},
        {"max-level",    0, 0, 'm'},
        {"silent",       0, 0, 's'},
        {"threshold",    1, 0, 't'},
        {"version",      0, 0, 'v'},
        { 0,             0, 0,  0 }
    };

    // Process command line arguments
    while(true)
    {
        ch = getopt_long(argc, argv, "a:d:lhmst:v", long_options, &option_index);

        if(ch == -1)
            break;

        switch(ch)
        {
            // Auto threshold
            case 'a':
                auto_thres = atoi(optarg);
                break;

            // Device (number)
            case 'd':
                device_number = atoi(optarg);
                break;

            // List devices
            case 'l':
                list_devices = true;
                break;

            // Help
            case 'h':
                print_help();
                exit(EXIT_SUCCESS);
                break;

            // Maximal level
            case 'm':
                max_level = true;
                break;

            // Silent
            case 's':
                verbose = false;
                break;

            // Threshold
            case 't':
                auto_thres = 0;
                silence_thres = atoi(optarg);
                break;

            // Version
            case 'v':
                print_version();
                exit(EXIT_SUCCESS);
                break;

            // Unknown options
            default:
                print_help();
                exit(EXIT_FAILURE);
                break;
        }
    }

    // Print version
    if(verbose)
    {
        print_version();
        cerr << endl;
    }




    return EXIT_SUCCESS;
}
