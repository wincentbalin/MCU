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
    Definition of the parser interface.
*/
class parser
{
public:
    virtual ~parser(void) {  }
    void parse(string& bitstring, string& result);
    void set_name(char* parser_name) { name = parser_name; }
    virtual string get_name(void) { return name; }
protected:
    string name;
};

/**
    Definition of IATA parser.
*/
class iata_parser : public parser
{
public:
    iata_parser(void) { set_name("IATA"); }
    virtual ~iata_parser(void) {  }
    void parse(string& bitstring, string& result);
};

/**
    Definition of ABA parser.
*/
class aba_parser : public parser
{
public:
    aba_parser(void) { set_name("ABA"); }
    virtual ~aba_parser(void) {  }
    void parse(string& bitstring, string& result);
};


#endif /* MCU_HPP */

