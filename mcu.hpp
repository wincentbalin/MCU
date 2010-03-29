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

// For assertions
#include <cassert>
#include <cstring>

using namespace std;


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

// We use signed 16 bit value as a sample
typedef int16_t sample_t;


/**
    Definition of the magnetic bitstring parser.
*/
class magnetic_bitstring_parser
{
public:
    virtual ~magnetic_bitstring_parser(void) {  }
    virtual void parse(string& bitstring, string& result);
    void set_name(char* parser_name) { name = parser_name; }
    string get_name(void) { return name; }
    void set_char_length(unsigned int length) { char_length = length; parity_bit = length - 1; }
    void set_start_sentinel(char* sentinel) { assert(strlen(sentinel) == char_length); start_sentinel = sentinel; }
    void set_end_sentinel(char* sentinel) { assert(strlen(sentinel) == char_length); end_sentinel = sentinel; }
    unsigned char decode_char(string& bits);
    bool check_parity(string& bits);
protected:
    string name; // of the encoding
    unsigned int char_length; // in bits
    unsigned int parity_bit;
    string start_sentinel;
    string end_sentinel;
};

/**
    Definition of IATA parser.
*/
class iata_parser : public magnetic_bitstring_parser
{
public:
    iata_parser(void)
    {
        set_name("IATA");
        set_char_length(7);
        set_start_sentinel("1010001");
        set_end_sentinel("1111100");
    }
    virtual ~iata_parser(void) {  }
};

/**
    Definition of ABA parser.
*/
class aba_parser : public magnetic_bitstring_parser
{
public:
    aba_parser(void)
    {
        set_name("ABA");
        set_char_length(5);
        set_start_sentinel("11010");
        set_end_sentinel("11111");
    }
    virtual ~aba_parser(void) {  }
};

/**
    RtAudio input function.
*/
int input(void* out_buffer, void* in_buffer, unsigned int n_buffer_frames,
          double stream_time, RtAudioStreamStatus status, void* data);


/**
    Definition of the MCU.
*/
class mcu
{
public:
    mcu(int argc, char** argv);
    void run(RtAudioCallback input_function, vector<sample_t>* b);
private:
    // Methods
    void print_version(void);
    void print_help(void);
    void list_devices(vector<RtAudio::DeviceInfo>& dev, vector<int>& index);
    void print_devices(vector<RtAudio::DeviceInfo>& dev);
    unsigned int greatest_sample_rate(int device_index);
    void print_max_level(unsigned int sample_rate);
    void silence_pause(void);
    void get_dsp(unsigned int sample_rate);
    void decode_aiken_biphase(vector<sample_t>& input);
    sample_t evaluate_max(void);
    void cleanup(void);

    // Properties
    RtAudio adc;    // Sound input
    vector<RtAudio::DeviceInfo> devices;    // List of devices
    vector<int> device_indexes; // List of original device indexes
    vector<sample_t>* buffer;
    unsigned int buffer_index;  // Current buffer index  = 0
    // Start and end index of sample
    unsigned int sample_start;
    unsigned int sample_end;
    string bitstring;   // String of bits
    sample_t silence_thres; // Silence threshold     = SILENCE_THRES

    // Configuration properties
    int auto_thres; //  = AUTO_THRES
    bool max_level; //  = false
    bool verbose;   //  = true
    bool list_input_devices;    //  = false
    int device_number;  //  = 0
};


#endif /* MCU_HPP */

