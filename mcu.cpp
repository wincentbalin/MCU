/**
    mcu.cpp

    Main source file.

    Part of Magnetic stripe Card Utility.

    Copyright (c) 2010-2011 Wincent Balin

    Based heavily upon dab.c and dmsb.c by Joseph Battaglia.

    Uses RtAudio by Gary P. Scavone for audio input. Input routines
    are based on RtAudio examples.

    As both of the mentioned above inspirations the program is licensed
    under the MIT License. See LICENSE file for further information.
*/

#include "mcu.hpp"

#include <map>
#include <algorithm>

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


void
MagneticBitstringParser::parse(std::string& bitstring, std::string& result)
{
    // Clear contents of the string
    result.clear();

    // initial condition is LRC of the start sentinel
    int lrc[char_length];

    // Initialize LRC
    for(size_t i = 0; i < char_length; i++)
    {
        lrc[i] = start_sentinel[i] - '0';
    }

    // Find start of encoded string
    size_t start_decode = bitstring.find(start_sentinel);

    // If no start sentinel found, cancel processing
    if(start_decode == std::string::npos)
    {
        return;
    }

    // Move start pointer to the next character past the start sentinel
    start_decode += char_length;

    // Set starting point for searching the end sentinel
    size_t end_decode = start_decode;

    // Find end of encoded string; ensure it's correct position
    do
    {
        end_decode = bitstring.find(end_sentinel, end_decode + 1);
    }
    while(end_decode != std::string::npos &&
          (end_decode - start_decode) % char_length != 0);

    // If no end sentinel found, cancel processing
    if(end_decode == std::string::npos)
    {
        return;
    }

    // Enter start sentinel
    result.push_back(decode_char(start_sentinel));

    // Decoded character for character
    for(size_t i = start_decode; i < end_decode + char_length; i += char_length)
    {
        // Extract bits
        std::string char_bits;
        copy(bitstring.begin() + i,
             bitstring.begin() + i + char_length,
             std::back_inserter(char_bits));

        if(! check_parity(char_bits))
        {
            // Parity mismatch
            std::cerr << "Character parity mismatch!" << std::endl;
            return;
        }

        // Decode bits
        result.push_back(decode_char(char_bits));

        // Update LRC
        for(size_t i = 0; i < parity_bit; i++)
        {
            lrc[i] ^= char_bits[i] == '1' ? 1 : 0;
        }
    }

    // Check for correct LRC
    std::string lrc_bits;
    for(size_t i = 0; i < char_length; i++)
    {
        lrc_bits.push_back('0' + lrc[i]);
    }

    if(! check_parity(lrc_bits))
    {
        // Parity mismatch
        std::cerr << "Information parity mismatch!" << std::endl;
        return;
    }
}

unsigned char
MagneticBitstringParser::decode_char(std::string& bits)
{
    unsigned char c = 48; // = '0'

    for(size_t i = 0, value = 1;
        i < parity_bit;
        i++, value *= 2)
    {
        c += bits[i] == '1' ? value : 0;
    }

    return c;
}

bool
MagneticBitstringParser::check_parity(std::string& bits)
{
    unsigned int parity = 0;

    for(size_t i = 0; i < parity_bit; i++)
    {
        if(bits[i] == '1')
        {
            parity++;
        }
    }

    if('0' + parity % 2 == (unsigned int) bits[parity_bit])
    {
        // Parity mismatch
        return false;
    }

    return true;
}


MCU::MCU(int argc, char** argv) :
        buffer_index(0), silence_thres(SILENCE_THRES),
        auto_thres(AUTO_THRES), max_level(false), verbose(true),
        list_input_devices(false), device_number(0)
{
    // Parse command line arguments
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

}

void
MCU::run(RtAudioCallback input_function, std::vector<sample_t>* b)
{
    // Save reference to the buffer
    buffer = b;

    // Print version
    if(verbose)
    {
        print_version();
        std::cerr << std::endl;
    }

    // Make RtAudio part verbose too
    if(verbose)
        adc.showWarnings(true);

    // If no sound devices found, exit
    if(adc.getDeviceCount() < 1)
    {
        std::cerr << "No audio devices found!" << std::endl;
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
    size_t buffer_frames = 512;
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
                       sample_rate, &buffer_frames, input_function, NULL);
        adc.startStream();
    }
    catch(RtAudioError& e)
    {
        std::cerr << std::endl << e.getMessage() << std::endl;
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
        std::cerr << "Error: Invalid silence threshold!" << std::endl;
        cleanup();
        exit(EXIT_FAILURE);
    }

    // Wait for a sample
    if(verbose)
    {
        std::cerr << "Waiting for sample..." << std::endl;
    }

    silence_pause();


    // Get samples
    get_dsp(sample_rate);

    // Extract samples
    size_t samples = sample_end - sample_start;
    std::vector<sample_t> sample_buffer(samples);
    copy(buffer->begin() + sample_start,
         buffer->begin() + sample_end,
         std::back_inserter(sample_buffer));

    // Automatically set threshold if requested
    if(auto_thres > 0)
    {
        silence_thres = auto_thres * evaluate_max() / 100;
    }

    // Print silence threshold
    if(verbose)
    {
        std::cerr << "Silence threshold: " << silence_thres
                  << " (" << auto_thres << "% of max)" << std::endl;
    }

    // Decode result
    decode_aiken_biphase(sample_buffer);

    // Print bit string if needed
    if(verbose)
    {
        std::cout << std::endl << "Bit string: " << bitstring << std::endl << std::endl;
    }

    // Create reversed bit string
    std::string reversed_bitstring = bitstring;
    std::reverse(reversed_bitstring.begin(), reversed_bitstring.end());

    // Instantiate parsers
    IATAParser iata_parser;
    ABAParser aba_parser;
    std::string decoded_string;

    // Try decoding using all available parsers
    std::cout << std::endl;

    std::cout << "Decoding bitstring using " << iata_parser.get_name()
              << " code:" << std::endl;
    iata_parser.parse(bitstring, decoded_string);
    std::cout << decoded_string << std::endl << std::endl;

    std::cout << "Decoding bitstring using " << aba_parser.get_name()
              << " code:" << std::endl;
    aba_parser.parse(bitstring, decoded_string);
    std::cout << decoded_string << std::endl << std::endl;

    std::cout << "Decoding reversed bitstring using " << iata_parser.get_name()
              << " code:" << std::endl;
    iata_parser.parse(reversed_bitstring, decoded_string);
    std::cout << decoded_string << std::endl << std::endl;

    std::cout << "Decoding reversed bitstring using " << aba_parser.get_name()
              << " code:" << std::endl;
    aba_parser.parse(reversed_bitstring, decoded_string);
    std::cout << decoded_string << std::endl << std::endl;

    // Stop and close audio stream
    cleanup();

}

void
MCU::print_version(void)
{
    std::cerr << "mcu - Magnetic stripe Card Utility" << std::endl
              << "Version " << VERSION << std::endl
              << "Copyright (c) 2010-2011 Wincent Balin" << std::endl;
}

void
MCU::print_help(void)
{
    print_version();

    std::cerr << "Usage: mcu [OPTIONS]" << std::endl
              << std::endl
              << "  -a,  --auto-thres   Set auto-thres percentage" << std::endl
              << "                      (default: " << AUTO_THRES << ")" << std::endl
              << "  -d,  --device       Device (number) to read audio data from" << std::endl
              << "                      (default: 0)" << std::endl
              << "  -l,  --list-devices List compatible devices (enumerated)" << std::endl
              << "  -h,  --help         Print help information" << std::endl
              << "  -m,  --max-level    Shows the maximum level" << std::endl
              << "                      (use to determine threshold)" << std::endl
              << "  -s,  --silent       No verbose messages" << std::endl
              << "  -t,  --threshold    Set silence threshold" << std::endl
              << "                      (default: automatic detect)" << std::endl
              << "  -v,  --version      Print version information" << std::endl
              << std::endl;
}

void
MCU::list_devices(std::vector<RtAudio::DeviceInfo>& dev, std::vector<int>& index)
{
    // Get devices
    for(size_t i = 0; i < adc.getDeviceCount(); i++)
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
MCU::print_devices(std::vector<RtAudio::DeviceInfo>& dev)
{
    // API map
    std::map<int, std::string> api_map;

    // Initialize API map
    api_map[RtAudio::MACOSX_CORE] = "OS-X Core Audio";
    api_map[RtAudio::WINDOWS_ASIO] = "Windows ASIO";
    api_map[RtAudio::WINDOWS_DS] = "Windows Direct Sound";
    api_map[RtAudio::UNIX_JACK] = "Jack Client";
    api_map[RtAudio::LINUX_ALSA] = "Linux ALSA";
    api_map[RtAudio::LINUX_OSS] = "Linux OSS";
    api_map[RtAudio::RTAUDIO_DUMMY] = "RtAudio Dummy";

    // Print current API
    std::cerr << "Current API: " << api_map[adc.getCurrentApi()] << std::endl;

    // Print every device
    for(size_t i = 0; i < dev.size(); i++)
    {
        RtAudio::DeviceInfo info = dev[i];

        // Print number of the device
        std::cerr.width(3);
        std::cerr << i << " "
                  << info.name
                  << (info.isDefaultInput ? " (Default input device)" : "") << std::endl;
    }
}

unsigned int
MCU::greatest_sample_rate(int device_index)
{
    unsigned int max_rate = 0;

    RtAudio::DeviceInfo info = adc.getDeviceInfo(device_index);

    for(size_t i = 0; i < info.sampleRates.size(); i++)
    {
        unsigned int rate = info.sampleRates[i];

        if(rate > max_rate)
        {
            max_rate = rate;
        }
    }

    return max_rate;
}

void
MCU::print_max_level(unsigned int sample_rate)
{
    std::cout << "Terminating after " << MAX_TERM << " seconds..." << std::endl;

    // Calculate maximal level
    sample_t last_level = 0;
    sample_t level;
    for(size_t i = 0; i < MAX_TERM * sample_rate; i++)
    {
        // Wait if needed
        if(buffer->size() <= i)
            SLEEP(100);

        level = buffer->at(i);

        // Make level value absolute
        if(level < 0)
        {
            level = -level;
        }

        // If current level is a (local) maximum, print it
        if(level > last_level)
        {
            std::cout << "Maximum level: " << level << '\r';
            last_level = level;
        }
    }

    std::cout << std::endl;
}

void
MCU::silence_pause(void)
{
    while(true)
    {
        // Wait till buffer has enough data
        while(buffer->size() <= buffer_index)
        {
            SLEEP(100);
        }

        for(; buffer_index < buffer->size(); buffer_index++)
        {
            // On first sample with absolute value
            // greater than threshold bail out
            sample_t sample = buffer->at(buffer_index);

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
MCU::get_dsp(unsigned int sample_rate)
{
    // Set start of the sample
    sample_start = buffer_index;
    sample_end = sample_start;

    // Silence interval (in samples) indicating end of the sample
    size_t silence_interval = (sample_rate * END_LENGTH) / 1000;

    // Loop until the end of the sample is found
    while(true)
    {
        // Find supposed end of sample (sample below threshold)
        for(; buffer_index < buffer->size(); buffer_index++)
        {
            sample_t sample = buffer->at(buffer_index);

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
        while(buffer->size() - sample_end < silence_interval)
        {
            SLEEP(100);
        }

        // Check whether the supposed end of the sample is the real one
        size_t silence_counter;
        for(silence_counter = 0;
            silence_counter < silence_interval;
            silence_counter++, buffer_index++)
        {
            sample_t sample = buffer->at(buffer_index);

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

void
MCU::decode_aiken_biphase(std::vector<sample_t>& input)
{
    const size_t input_size = input.size();

    // Make all values absolute
    for(size_t i = 0; i < input_size; i++)
    {
        if(input[i] < 0)
        {
            input[i] = -input[i];
        }
    }

    // Search for peaks
    size_t peak_index = 0;
    size_t old_peak_index = 0;
    std::vector<size_t> peaks;
    for(size_t i = 0; i < input_size; )
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

        size_t peak_index_diff = peak_index - old_peak_index;
        if(peak_index_diff > 0)
        {
            peaks.push_back(peak_index_diff);
        }
    }

    // If less than two peaks found, something went wrong
    if(peaks.size() < 2)
    {
        std::cerr << "No bits detected!" << std::endl;
        cleanup();
        exit(EXIT_FAILURE);
    }

    // Decode aiken bi-phase (decode bits based on intervals between peaks)
    sample_t zero = peaks[2];
    const size_t peaks_size = peaks.size();
    for(size_t i = 2; i < peaks_size - 1; i++)
    {
        size_t interval0 = (FREQ_THRES * zero) / 100;
        size_t interval1 = interval0 / 2;

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
MCU::evaluate_max(void)
{
    sample_t max = 0;

    for(size_t i = 0; i < buffer->size(); i++)
    {
        sample_t value = buffer->at(i);
        if(value > max)
        {
            max = value;
        }
    }

    return max;
}

void
MCU::cleanup(void)
{
    // Stop audio stream
    try
    {
        adc.stopStream();
    }
    catch(RtAudioError& e)
    {
        std::cerr << std::endl << e.getMessage() << std::endl;
        exit(EXIT_FAILURE);
    }

    // Close audio stream
    if(adc.isStreamOpen())
        adc.closeStream();
}


// Input data buffer
std::vector<sample_t> buf;

// RtAudio input function
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
        std::cerr << "Audio input overflow!"<< std::endl;
        return 2;
    }

    // Copy audio input data to buffer
    sample_t* src = (sample_t*) in_buffer;
    for(unsigned int i = 0; i < n_buffer_frames; i++, src++)
    {
        buf.push_back(*src);
    }

    return 0;
}

int
main(int argc, char** argv)
{
    MCU mcu(argc, argv);

    mcu.run(&input, &buf);

    return EXIT_SUCCESS;
}
