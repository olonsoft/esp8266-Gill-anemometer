#include <WindFunc.h>
#include <Arduino.h>
#include <helper_general.h>
#include <math.h>

// Define conversions constants to avoid magic numbers
const float KNOTS_TO_KMH = 1.852f;
const float MPH_TO_KMH   = 1.60934f;
const float FPS_TO_KMH   = 1.09728f; // (0.3048 * 3600) / 1000
const float MS_TO_KMH    = 3.6f;
const float FPM_TO_KMH   = 0.018288f;

float WindFunc::convertUnit(float speed, WindSpeedUnit fromUnit,
                            WindSpeedUnit toUnit) {
  if (fromUnit == toUnit) {
    return speed;
  }

  float tmpKmPerHour = 0.0f;

  // 1. Normalize source to Km/h
  switch (fromUnit) {
    case WindSpeedUnit::MetresPerSecond:
      tmpKmPerHour = speed * MS_TO_KMH;
      break;
    case WindSpeedUnit::Knots:
      tmpKmPerHour = speed * KNOTS_TO_KMH;
      break;
    case WindSpeedUnit::MilesPerHour:
      tmpKmPerHour = speed * MPH_TO_KMH;
      break;
    case WindSpeedUnit::KmPerHour:
      tmpKmPerHour = speed;
      break;
    case WindSpeedUnit::FeetPerMinute:
      tmpKmPerHour = speed * FPM_TO_KMH;
      break;
    default:
      return speed; // Safety return
  }

  // 2. Convert Km/h to target unit
  switch (toUnit) {
    case WindSpeedUnit::MetresPerSecond:
      return tmpKmPerHour / MS_TO_KMH;
    case WindSpeedUnit::Knots:
      return tmpKmPerHour / KNOTS_TO_KMH;
    case WindSpeedUnit::MilesPerHour:
      return tmpKmPerHour / MPH_TO_KMH;
    case WindSpeedUnit::KmPerHour:
      return tmpKmPerHour;
    case WindSpeedUnit::FeetPerMinute:
      return tmpKmPerHour / FPM_TO_KMH;
    default:
      return tmpKmPerHour;
  }
}

void WindFunc::addWindSpeed(float speed) {
  _windSpeedSamples++;
  _windSpeedCurrent = speed;

  if (_windSpeedSamples == 1) {
    _windSpeedMin = speed;
    _windSpeedMax = speed;
    _windSpeedSum = speed;
  } else {
    if (speed < _windSpeedMin) _windSpeedMin = speed;
    if (speed > _windSpeedMax) _windSpeedMax = speed;
    _windSpeedSum += speed;
  }
}

void WindFunc::addWindDirection(uint16_t direction) {
  // Ensure direction is 0-359
  if (direction < 360) {
    _windDirectionSamples++;
    _windDirectionCurrent = direction;
    // Accumulate vector components
    _windDirectionCosTotal += cos(direction * DEG2RAD);
    _windDirectionSinTotal += sin(direction * DEG2RAD);
  }
}

bool WindFunc::getWindSpeed(float &windSpeedCurrent, float &windSpeedAverage,
                            float &windSpeedMin, float &windSpeedMax) {
  if (_windSpeedSamples > 0) {
    windSpeedAverage = _windSpeedSum / (float)_windSpeedSamples;
    windSpeedMin     = _windSpeedMin;
    windSpeedMax     = _windSpeedMax;
    windSpeedCurrent = _windSpeedCurrent;
    return true;
  }

  // Safe defaults if no samples
  windSpeedCurrent = windSpeedAverage = windSpeedMin = windSpeedMax = 0.0f;
  return false;
}

bool WindFunc::getWindDirection(uint16_t &windDirectionCurrent,
                                uint16_t &windDirectionAverage) {
  if (_windDirectionSamples > 0) {
    windDirectionCurrent = _windDirectionCurrent;

    // Calculate average angle from vector sums using atan2
    float avgRad = atan2(_windDirectionSinTotal, _windDirectionCosTotal);

    // Convert radians to degrees
    float avgDeg = avgRad * RAD_TO_DEG; // Standard Arduino constant

    // Normalize to 0-360
    if (avgDeg < 0) {
        avgDeg += 360.0f;
    }

    windDirectionAverage = (uint16_t)fmod(avgDeg, 360.0f);

    TLOGDEBUGF_P(PSTR("[DEBUG] SinTotal %.3f CosTotal: %.3f AvgDir: %d\n"),
                 _windDirectionSinTotal, _windDirectionCosTotal,
                 windDirectionAverage);
    return true;
  }

  return false;
}

void WindFunc::resetWindSpeed() {
  _windSpeedSamples = 0;
  _windSpeedSum     = 0.0f;
  _windSpeedCurrent = 0.0f;
  _windSpeedMin     = 0.0f;
  _windSpeedMax     = 0.0f;
}

void WindFunc::resetWindDirection() {
  _windDirectionSamples   = 0;
  _windDirectionCurrent   = 0;
  _windDirectionCosTotal  = 0.0f;
  _windDirectionSinTotal  = 0.0f;
}

int WindFunc::meterPerSecToBeaufort(float m_per_sec) {
  // Lookup table for Beaufort thresholds (lower bounds)
  // B0 < 0.3, B1 < 1.6, etc.
  const float thresholds[] = {
    0.3f, 1.6f, 3.4f, 5.5f, 8.0f, 10.8f,
    13.9f, 17.2f, 20.8f, 24.5f, 28.5f, 32.6f
  };

  // Iterate through thresholds
  for (int i = 0; i < 12; i++) {
    if (m_per_sec < thresholds[i]) {
      return i;
    }
  }

  return 12; // Hurricane force
}

String WindFunc::getBearingStr(int degrees) {
  const int nr_directions = 16;
  // Direction strings array
  const char* sectors[] = {
    "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
    "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"
  };

  // Normalize degrees to 0-360
  degrees = degrees % 360;
  if (degrees < 0) degrees += 360;

  float stepsize = 360.0f / nr_directions;

  // Calculate index with rounding (add half step)
  int bearing_idx = (int)((degrees + (stepsize / 2.0f)) / stepsize) % nr_directions;

  return String(sectors[bearing_idx]);
}