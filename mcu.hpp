/**
    mcu.hpp

    Main header file.

    Part of Magnetic stripe Card Utility.

    Copyright (c) 2010 Wincent Balin
*/

#ifndef MCU_HPP
#define MCU_HPP

#include <iostream>
#include <string>
#include <vector>

#include "RtAudio.h"

#include <inttypes.h>

// For assertions
#include <cassert>
#include <cstring>


// Version of the program
#define VERSION 1.1

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

// We use signed 16 bit value as a sample
typedef int16_t sample_t;


/**
    Definition of the magnetic bitstring parser.
*/
class MagneticBitstringParser
{
public:
    virtual ~MagneticBitstringParser(void) {  }
    virtual void parse(std::string& bitstring, std::string& result);
    void set_name(const char* parser_name) { name = parser_name; }
    std::string get_name(void) { return name; }
    void set_char_length(unsigned int length) { char_length = length; parity_bit = length - 1; }
    void set_start_sentinel(const char* sentinel) { assert(strlen(sentinel) == char_length); start_sentinel = sentinel; }
    void set_end_sentinel(const char* sentinel) { assert(strlen(sentinel) == char_length); end_sentinel = sentinel; }
    unsigned char decode_char(std::string& bits);
    bool check_parity(std::string& bits);
protected:
    std::string name; // of the encoding
    unsigned int char_length; // in bits
    unsigned int parity_bit;
    std::string start_sentinel;
    std::string end_sentinel;
};

/**
    Definition of IATA parser.
*/
class IATAParser : public MagneticBitstringParser
{
public:
    IATAParser(void)
    {
        set_name("IATA");
        set_char_length(7);
        set_start_sentinel("1010001");
        set_end_sentinel("1111100");
    }
    virtual ~IATAParser(void) {  }
};

/**
    Definition of ABA parser.
*/
class ABAParser : public MagneticBitstringParser
{
public:
    ABAParser(void)
    {
        set_name("ABA");
        set_char_length(5);
        set_start_sentinel("11010");
        set_end_sentinel("11111");
    }
    virtual ~ABAParser(void) {  }
};

/**
    RtAudio input function.
*/
int input(void* out_buffer, void* in_buffer, unsigned int n_buffer_frames,
          double stream_time, RtAudioStreamStatus status, void* data);

/**
    Definition of the MCU.
*/
class MCU
{
public:
    MCU(int argc, char** argv);
    void run(RtAudioCallback input_function, std::vector<sample_t>* b);
private:
    // Methods
    void print_version(void);
    void print_help(void);
    void list_devices(std::vector<RtAudio::DeviceInfo>& dev, std::vector<int>& index);
    void print_devices(std::vector<RtAudio::DeviceInfo>& dev);
    unsigned int greatest_sample_rate(int device_index);
    void print_max_level(unsigned int sample_rate);
    void silence_pause(void);
    void get_dsp(unsigned int sample_rate);
    void decode_aiken_biphase(std::vector<sample_t>& input);
    sample_t evaluate_max(void);
    void cleanup(void);

    // Properties
    RtAudio adc;    // Sound input
    std::vector<RtAudio::DeviceInfo> devices;    // List of devices
    std::vector<int> device_indexes; // List of original device indexes
    std::vector<sample_t>* buffer;
    unsigned int buffer_index;  // Current buffer index  = 0
    // Start and end index of sample
    unsigned int sample_start;
    unsigned int sample_end;
    std::string bitstring;   // String of bits
    sample_t silence_thres; // Silence threshold     = SILENCE_THRES

    // Configuration properties
    int auto_thres; //  = AUTO_THRES
    bool max_level; //  = false
    bool verbose;   //  = true
    bool list_input_devices;    //  = false
    int device_number;  //  = 0
};


#endif /* MCU_HPP */

