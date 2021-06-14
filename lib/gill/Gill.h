#ifndef gill_h
#define gill_h

#include <WindFunc.h>  // needed for units conversion

enum class SerialDataResult { // received serial data status
  Ok,                         // is Valid. WindSpeed and WindDirection were extracted.
  NoControlChars,             // does not contain control characters
  CheckSumError,              // checksum error
  LongMessageError,           // is longer than desired
  DeviceError                 // contain Device Error flag
};

class Gill {
 public:
  SerialDataResult  decodeSerialData(char *data);
  void              setWindSpeedUnit(WindSpeedUnit speedUnit);
  WindSpeedUnit     getWindSpeedUnit();
  SerialDataResult  getSerialDataResult();
  float             getSpeed();
  int               getDirection();

 private:
  WindSpeedUnit    _windSpeedUnit;
  float            _speed;
  int              _direction;
  SerialDataResult _serialDataResult;
};

#endif
