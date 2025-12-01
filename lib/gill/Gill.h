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
  DecodedDataResult  decodeSerialData(char *data);
  DecodedDataResult  decodeSerialData1(char *data);
  void              setWindSpeedUnit(WindSpeedUnit speedUnit);
  WindSpeedUnit     getWindSpeedUnit();
  DecodedDataResult  getSerialDataResult();
  float             getSpeed();
  int               getDirection();

 private:
  WindSpeedUnit    _windSpeedUnit;
  float            _speed;
  int              _direction;
  DecodedDataResult _serialDataResult;
};

#endif
