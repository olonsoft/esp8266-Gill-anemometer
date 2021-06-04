#ifndef gill_h
#define gill_h

#include <WindFunc.h>  // needed for units conversion

enum class SerialDataResult_t
{                     // received serial data status
  srOK,               // is OK. WindSpeed and WindDirection were extracted.
  srNoControlChars,   // do not contain control characters
  srCheckSumError,    // checksum error
  srLongMessageError, // is longer than desired
  srDeviceError       // contain Device Error flag
};

class Gill
{
public:
  SerialDataResult_t decodeSerialData(char *data);
  void setSpeedUnit(windSpeedUnit_t speedUnit);
  windSpeedUnit_t getSpeedUnit();
  SerialDataResult_t getSerialDataResult();
  float getSpeed();
  int getDirection();

private:
  windSpeedUnit_t _windSpeedUnit;
  float _speed;
  int _direction;
  SerialDataResult_t _serialDataResult;
};

#endif
