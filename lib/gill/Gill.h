#ifndef gill_h
#define gill_h

#include <WindFunc.h>  // needed for units conversion

enum class DecodedDataResult { // received serial data status
  Ok,                         // is Valid. WindSpeed and WindDirection were extracted.
  NoControlChars,             // does not contain control characters
  CheckSumError,              // checksum error
  LongMessageError,           // is longer than desired
  DeviceError                 // contain Device Error flag
};

class Gill {
 public:
  Gill();
  DecodedDataResult decodeSerialData(const char *data);
  void setWindSpeedUnit(WindSpeedUnit speedUnit);
  WindSpeedUnit getWindSpeedUnit();
  DecodedDataResult getSerialDataResult();
  float getSpeed();
  int getDirection();

 private:
  WindSpeedUnit _targetWindSpeedUnit;
  float _speed;
  int _direction;
  DecodedDataResult _serialDataResult;
  WindFunc _wfGill; // Member variable instead of global
  WindSpeedUnit parseUnitChar(char c);
};

#endif
