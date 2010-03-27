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
#include <vector>

#include <cstdlib>
#include <getopt.h>


// Platform-dependent sleep routines; taken from RtAudio example
#if defined( __WINDOWS_ASIO__ ) || defined( __WINDOWS_DS__ )
  #include <windows.h>
  #define SLEEP( milliseconds ) Sleep( (DWORD) milliseconds )
#else // Unix variants
  #include <unistd.h>
  #define SLEEP( milliseconds ) usleep( (unsigned long) (milliseconds * 1000.0) )
#endif


// Version of the program
#define VERSION 0.1

// Initial silence threshold
#define SILENCE_THRES 5000

// Percent of highest value to set silence_thres to
#define AUTO_THRES 30

// Frequency threshold (in percent)
#define FREQ_THRES 60

// Seconds before termination of print_max_level()
#define MAX_TERM 60

// Silence interval after sample (in milliseconds)
#define END_LENGTH 200


using namespace std;


// We use signed 16 bit value as a sample
typedef int16_t sample_t;

// Sound input
RtAudio adc;

// List of devices
vector<RtAudio::DeviceInfo> devices;

// List of original device indexes
vector<int> device_indexes;

// Input data buffer
vector<sample_t> buffer;

// Current buffer index
unsigned int buffer_index = 0;

// Start and end index of sample
unsigned int sample_start;
unsigned int sample_end;

// String of bits
string bitstring;

// Silence threshold
sample_t silence_thres = SILENCE_THRES;


void
iata_parser::parse(string& bitstring, string& result)
{
}

void
aba_parser::parse(string& bitstring, string& result)
{
}



void
print_version(void)
{
    cerr << "mcu - Magnetic stripe Card Utility" << endl
         << "Version " << VERSION << endl
         << "Copyright (c) 2010 Wincent Balin" << endl;
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
    // Get devices
    for(unsigned int i = 0; i < adc.getDeviceCount(); i++)
    {
        RtAudio::DeviceInfo info = adc.getDeviceInfo(i);

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

    // Print current API
    cerr << "Current API: " << api_map[adc.getCurrentApi()] << endl;

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

unsigned int
greatest_sample_rate(int device_index)
{
    unsigned int max_rate = 0;

    RtAudio::DeviceInfo info = adc.getDeviceInfo(device_index);

    for(unsigned int i = 0; i < info.sampleRates.size(); i++)
    {
        unsigned int rate = info.sampleRates[i];

        if(rate > max_rate)
        {
            max_rate = rate;
        }
    }

    return max_rate;
}

int
input(void* out_buffer, void* in_buffer, unsigned int n_buffer_frames,
           double stream_time, RtAudioStreamStatus status, void* data)
{
    (void) out_buffer;
    (void) stream_time;
    (void) data;

    // Check for audio input overflow
    if(status == RTAUDIO_INPUT_OVERFLOW)
    {
        cerr << "Audio input overflow!"<< endl;
        return 2;
    }

    // Copy audio input data to buffer
    sample_t* src = (sample_t*) in_buffer;
    for(unsigned int i = 0, ; i < n_buffer_frames; i++, src++)
    {
        buffer.push_back(*src);
    }

    return 0;
}

void
print_max_level(unsigned int sample_rate)
{
    cout << "Terminating after " << MAX_TERM << " seconds..." << endl;

    // Calculate maximal level
    sample_t last_level = 0;
    sample_t level;
    for(unsigned int i = 0; i < MAX_TERM * sample_rate; i++)
    {
        // Wait if needed
        if(buffer.size() <= i)
            SLEEP(100);

        level = buffer[i];

        // Make level value absolute
        if(level < 0)
        {
            level = -level;
        }

        // If current level is a (local) maximum, print it
        if(level > last_level)
        {
            cout << "Maximum level: " << level << '\r';
            last_level = level;
        }
    }

    cout << endl;
}

void
silence_pause(void)
{
    while(true)
    {
        // Wait till buffer has enough data
        while(buffer.size() <= buffer_index)
        {
            SLEEP(100);
        }

        for(; buffer_index < buffer.size(); buffer_index++)
        {
            // On first sample with absolute value
            // greater than threshold bail out
            sample_t sample = buffer[buffer_index];

            if(sample < 0)
            {
                sample = -sample;
            }

            if(sample > silence_thres)
            {
                return;
            }
        }
    }
}

void
get_dsp(unsigned int sample_rate)
{
    // Set start of the sample
    sample_start = buffer_index;
    sample_end = sample_start;

    // Silence interval (in samples) indicating end of the sample
    unsigned int silence_interval = (sample_rate * END_LENGTH) / 1000;

    // Loop until the end of the sample is found
    while(true)
    {
        // Find supposed end of sample (sample below threshold)
        for(; buffer_index < buffer.size(); buffer_index++)
        {
            sample_t sample = buffer[buffer_index];

            if(sample < 0)
            {
                sample = -sample;
            }

            if(sample < silence_thres)
            {
                sample_end = buffer_index;
                break;
            }
        }

        // Wait till buffer has enough data
        while(buffer.size() - sample_end < silence_interval)
        {
            SLEEP(100);
        }

        // Check whether the suppoed end of the sample is the real one
        unsigned int silence_counter;
        for(silence_counter = 0;
            silence_counter < silence_interval;
            silence_counter++, buffer_index++)
        {
            sample_t sample = buffer[buffer_index];

            if(sample < 0)
            {
                sample = -sample;
            }

            if(sample > silence_thres)
            {
                break;
            }
        }

        // If silence continued longer than the allowed interval, end recording
        if(silence_counter == silence_interval)
        {
            return;
        }
    }
}

void cleanup(void); // Workaround!
void
decode_aiken_biphase(vector<sample_t>& input)
{
    const unsigned int input_size = input.size();

    // Make all values absolute
    for(unsigned int i = 0; i < input_size; i++)
    {
        if(input[i] < 0)
        {
            input[i] = -input[i];
        }
    }

    // Search for peaks
    unsigned int peak_index = 0;
    unsigned int old_peak_index = 0;
    vector<unsigned int> peaks;
    for(unsigned int i = 0; i < input_size; )
    {
        // Store peak index
        old_peak_index = peak_index;

        // Search for the next peak
        for(; i < input_size && input[i] <= silence_thres; i++)
        {
        }

        peak_index = 0;

        for(; i < input_size && input[i] > silence_thres; i++)
        {
            if(input[i] > input[peak_index])
            {
                peak_index = i;
            }
        }

        unsigned int peak_index_diff = peak_index - old_peak_index;
        if(peak_index_diff > 0)
        {
            peaks.push_back(peak_index_diff);
        }
    }

    // If less than two peaks found, something went wrong
    if(peaks.size() < 2)
    {
        cerr << "No bits detected!" << endl;
        cleanup();
        exit(EXIT_FAILURE);
    }

    // Decode aiken bi-phase (decode bits based on intervals between peaks)
    sample_t zero = peaks[2];
    const unsigned int peaks_size = peaks.size();
    for(unsigned int i = 2; i < peaks_size - 1; i++)
    {
        unsigned int interval0 = (FREQ_THRES * zero) / 100;
        unsigned int interval1 = interval0 / 2;

        if(peaks[i] < ((zero / 2) + interval1) &&
           peaks[i] > ((zero / 2) - interval1))
        {
            if(peaks[i + 1] < ((zero / 2) + interval1) &&
               peaks[i + 1] > ((zero / 2) - interval1))
            {
                bitstring.push_back('1');
                zero = peaks[i] * 2;
                i++;
            }
        }
        else if(peaks[i] < (zero + interval0) &&
                peaks[i] > (zero - interval0))
        {
            bitstring.push_back('0');
            zero = peaks[i];
        }
    }
}

sample_t
evaluate_max(void)
{
    sample_t max = 0;

    for(unsigned int i = 0; i < buffer.size(); i++)
    {
        sample_t value = buffer[i];
        if(value > max)
        {
            max = value;
        }
    }

    return max;
}

void
cleanup(void)
{
    // Stop audio stream
    try
    {
        adc.stopStream();
    }
    catch(RtError& e)
    {
        cerr << endl << e.getMessage() << endl;
        exit(EXIT_FAILURE);
    }

    // Close audio stream
    if(adc.isStreamOpen())
        adc.closeStream();
}


int
main(int argc, char** argv)
{
    // Configuration variables
    int auto_thres = AUTO_THRES;
    bool max_level = false;
    bool verbose = true;
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
    unsigned int device_index = device_indexes[device_number];
    unsigned int sample_rate = 44100; // Default value if nothing else found
    sample_rate = greatest_sample_rate(device_index);
    RtAudio::StreamParameters input_params;
    input_params.deviceId = device_index;
    input_params.nChannels = 1;
    input_params.firstChannel = 0;

    // Open and start audio stream
    try
    {
        adc.openStream(NULL, &input_params, RTAUDIO_SINT16,
                       sample_rate, &buffer_frames, &input, NULL);
        adc.startStream();
    }
    catch(RtError& e)
    {
        cerr << endl << e.getMessage() << endl;
        cleanup();
    }

    // If calculating maximal level is requested, do so and exit
    if(max_level)
    {
        print_max_level(sample_rate);
        cleanup();
        exit(EXIT_SUCCESS);
    }

    // Sanity check for silence threshold
    if(silence_thres == 0)
    {
        cerr << "Error: Invalid silence threshold!" << endl;
        cleanup();
        exit(EXIT_FAILURE);
    }

    // Wait for a sample
    if(verbose)
    {
        cerr << "Waiting for sample..." << endl;
    }

    silence_pause();


    // Get samples
    get_dsp(sample_rate);

    // Decode result
    unsigned int samples = sample_end - sample_start;
    vector<sample_t> sample_buffer(samples);
    copy(buffer.begin() + sample_start,
         buffer.begin() + sample_end,
         back_inserter(sample_buffer));
    decode_aiken_biphase(sample_buffer);

    // Print bit string if needed
    if(verbose)
    {
        cout << "Bit string: " << bitstring << endl;
    }

    // Automatically set threshold if requested
    if(auto_thres > 0)
    {
        silence_thres = auto_thres * evaluate_max() / 100;
    }

    // Print silence threshold
    if(verbose)
    {
        cerr << "Silence threshold: " << silence_thres
             << " (" << auto_thres << "% of max)" << endl;
    }

    // Stop and close audio stream
    cleanup();

    return EXIT_SUCCESS;
}
