/**
    mcu.cpp

    Main source file.

    Part of Magnetic stripe Card Utility.

    Copyright (c) 2010 Wincent Balin

    Based heavily upon dab.c and dmsb.c by Joseph Battaglia.

    Uses RtAudio by Gary P. Scavone for audio input. Input routines
    are based on RtAudio examples.

    As both of the mentioned above inspirations the program is licensed
    under the MIT License. See LICENSE file for further information.
*/

#include "mcu.hpp"
#include "RtAudio.h"

#include <iostream>
#include <map>

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

// List of devices
vector<RtAudio::DeviceInfo> devices;

// List of original device indexes
vector<int> device_indexes;

// Input data buffer
struct input_data
{
    int16_t* buffer;
    uint32_t buffer_bytes;
    uint32_t total_frames;
    uint32_t frame_counter;
    uint32_t channels;
};



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

void
list_devices(vector<RtAudio::DeviceInfo>& dev, vector<int>& index)
{
    // Audio interface
    RtAudio audio;

    // Get devices
    for(unsigned int i = 0; i < audio.getDeviceCount(); i++)
    {
        RtAudio::DeviceInfo info = audio.getDeviceInfo(i);

        // If device unprobed, go to the next one
        if(!info.probed)
            continue;

        // If no input channels, skip this device
        if(info.inputChannels < 1)
            continue;

        // If no natively supported formats, skip this device
        if(info.nativeFormats == 0)
            continue;

        // We need S16 format. If unavailable, skip this device
        if(!(info.nativeFormats & RTAUDIO_SINT16))
            continue;

        // If no sample rates supported, skip this device
        if(info.sampleRates.size() < 1)
            continue;

        // Add new audio input device
        dev.push_back(info);
        index.push_back(i);
    }
}

void
print_devices(vector<RtAudio::DeviceInfo>& dev)
{
    // API map
    map<int, string> api_map;

    // Initialize API map
    api_map[RtAudio::MACOSX_CORE] = "OS-X Core Audio";
    api_map[RtAudio::WINDOWS_ASIO] = "Windows ASIO";
    api_map[RtAudio::WINDOWS_DS] = "Windows Direct Sound";
    api_map[RtAudio::UNIX_JACK] = "Jack Client";
    api_map[RtAudio::LINUX_ALSA] = "Linux ALSA";
    api_map[RtAudio::LINUX_OSS] = "Linux OSS";
    api_map[RtAudio::RTAUDIO_DUMMY] = "RtAudio Dummy";

    // Audio interface
    RtAudio audio;

    // Print current API
    cerr << "Current API: " << api_map[audio.getCurrentApi()] << endl;

    // Print every device
    for(unsigned int i = 0; i < dev.size(); i++)
    {
        RtAudio::DeviceInfo info = dev[i];

        // Print number of the device
        cerr.width(3);
        cerr << i << " "
             << info.name
             << (info.isDefaultInput ? " (Default input device)" : "") << endl;
    }
}

int
input(void* out_buffer, void* in_buffer, unsigned int n_buffer_frames,
           double stream_time, RtAudioStreamStatus status, void *data)
{
    (void) out_buffer;
    (void) stream_time;
    (void) status;

    struct input_data* in_data = (struct input_data*) data;

    // Copy data to the allocated buffer
    unsigned int frames = n_buffer_frames;

    if(in_data->frame_counter + n_buffer_frames > in_data->total_frames)
    {
        frames = in_data->total_frames - in_data->frame_counter;
        in_data->buffer_bytes = frames * in_data->channels * sizeof(int16_t);
    }

    uint32_t offset = in_data->frame_counter * in_data->channels;
    memcpy(in_data->buffer + offset, in_buffer, in_data->buffer_bytes);
    in_data->frame_counter += frames;

    // If buffer overflown, return with unnormal value
    if(in_data->frame_counter >= in_data->total_frames)
        return 2;

    return 0;
}


int
main(int argc, char** argv)
{
    // Sound input
    RtAudio adc;

    // Configuration variables
    int auto_thres = AUTO_THRES;
    bool max_level = false;
    bool verbose = true;
    int silence_thres = SILENCE_THRES;
    bool list_input_devices = false;
    int device_number = 0;

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
                list_input_devices = true;
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

    // Make RtAudio part verbose too
    if(verbose)
        adc.showWarnings(true);

    // If no sound devices found, exit
    if(adc.getDeviceCount() < 1)
    {
        cerr << "No audio devices found!" << endl;
        exit(EXIT_FAILURE);
    }

    // Get list of device
    list_devices(devices, device_indexes);

    // If requested, print list of devices and exit
    if(list_input_devices)
    {
        print_devices(devices);
        exit(EXIT_SUCCESS);
    }

    // Specify parameters of the audio stream
    unsigned int buffer_frames = 512;
    unsigned int fs = 192000;
    RtAudio::StreamParameters input_params;
    input_params.deviceId = device_indexes[device_number];
    input_params.nChannels = 1;
    input_params.firstChannel = 0;

    // Define data buffer
    struct input_data data;
    data.buffer = 0;

    // Open audio stream
    try
    {
        adc.openStream(NULL, &input_params, RTAUDIO_SINT16, fs,
                       &buffer_frames, &input, (void *) &data);
    }
    catch(RtError& e)
    {
        cerr << endl << e.getMessage() << endl;
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}