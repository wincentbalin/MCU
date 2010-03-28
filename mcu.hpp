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

// For assertions
#include <cassert>
#include <cstring>

using namespace std;

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
    bool check_char_parity(string& bits);
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


#endif /* MCU_HPP */

