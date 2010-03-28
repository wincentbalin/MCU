/**
    mcu.hpp

    Main header file.

    Part of Magnetic stripe Card Utility.

    Copyright (c) 2010 Wincent Balin
*/

#ifndef MCU_HPP
#define MCU_HPP

#include <string>

using namespace std;

/**
    Definition of the magnetic bitstring parser.
*/
class magnetic_bitstring_parser
{
public:
    virtual ~magnetic_bitstring_parser(void) {  }
    void parse(string& bitstring, string& result);
    void set_name(char* parser_name) { name = parser_name; }
    string get_name(void) { return name; }
    void set_char_length(unsigned int length) { char_length = length; }
protected:
    string name;
    unsigned int char_length; // in bits
};

/**
    Definition of IATA parser.
*/
class iata_parser : public magnetic_bitstring_parser
{
public:
    iata_parser(void) { set_name("IATA"); set_char_length(7); }
    virtual ~iata_parser(void) {  }
    void parse(string& bitstring, string& result);
};

/**
    Definition of ABA parser.
*/
class aba_parser : public magnetic_bitstring_parser
{
public:
    aba_parser(void) { set_name("ABA"); set_char_length(5); }
    virtual ~aba_parser(void) {  }
    void parse(string& bitstring, string& result);
};


#endif /* MCU_HPP */

