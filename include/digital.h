#ifndef Z80PAL_DIGITAL_H
#define Z80PAL_DIGITAL_H

#include <avr/io.h>

enum PinDirection { Input = 0, Output = 1 };
enum PinState { Low = 0, High = 1 };

class IOPin {
protected:
  volatile uint8_t* _port;
  volatile uint8_t* _pins;
  volatile uint8_t* _ddr;
  uint8_t _pin;
public:
  constexpr IOPin(volatile uint8_t* portRegister,
                  volatile uint8_t* pinsRegister,
                  volatile uint8_t* ddrRegister,
                  uint8_t pin) :
    _port(portRegister),
    _pins(pinsRegister),
    _ddr(ddrRegister),
    _pin(pin) {};

  inline void setOutput() { (*_ddr) |= _BV(_pin); }
  inline void setInput()  { (*_ddr) &= ~_BV(_pin); }
  inline void setHigh()   { (*_port) |= _BV(_pin); }
  inline void setLow()    { (*_port) &= ~_BV(_pin); }
  inline void toggle()    { (*_port) ^= _BV(_pin); }
  inline bool isHigh()    { return bit_is_set(_pins, _pin) > 0; }
  inline bool isLow()     { return bit_is_clear(_pins, _pin); }
};

class IOPort {
protected:
  volatile uint8_t* _port;
  volatile uint8_t* _pins;
  volatile uint8_t* _ddr;
public:
  constexpr IOPort(volatile uint8_t* portRegister,
                   volatile uint8_t* pinsRegister,
                   volatile uint8_t* ddrRegister) :
    _port(portRegister),
    _pins(pinsRegister),
    _ddr(ddrRegister) {}

  inline uint8_t readByte() { return *_pins; }
  inline uint8_t writeByte(uint8_t byt) { *_port = byt; }

  inline void setPortDirection(uint8_t dir) { (*_ddr) = dir; }

  IOPin operator[](int pin) { return IOPin(_port, _pins, _ddr, pin); };
};

#endif
