#ifndef wind_functions_h
#define wind_functions_h

#include <Arduino.h>

#define DEG2RAD 0.017453292f // M_PI / 180.0f; // convert degrees to radian

typedef enum
{
  wsMetersPerSecond,
  wsKnots,
  wsMilesPerHour,
  wsKmPerHour,
  wsFeetPerMinute
} windSpeedUnit_t;

class WindFunc
{
public:
  float convertUnit(float speed, windSpeedUnit_t fromUnit, windSpeedUnit_t toUnit);
  void addWindSpeed(float speed);
  void addWindDirection(uint16_t windDirection);
  bool getWindSpeed(float &windSpeedCurrent, float &windSpeedAverage, float &windSpeedMin, float &windSpeedMax);
  bool getWindDirection(uint16_t &windDirectionCurrent, uint16_t &windDirectionAverage);
  void resetWindSpeed();
  void resetWindDirection();
  int meterPerSecToBeaufort(float m_per_sec);
  String getBearingStr(int degrees);
private:
  uint16_t _windSpeedSamples = 0;
  float _windSpeedCurrent; 
  float _windSpeedMin; 
  float _windSpeedMax; 
  float _windSpeedSum;

  uint16_t _windDirectionCurrent;
  uint16_t _windDirectionSamples = 0;
  float _windDirectionCosTotal;
  float _windDirectionSinTotal;
};

#endif //weather_functions_h
