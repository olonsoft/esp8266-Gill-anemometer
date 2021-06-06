#include <WindFunc.h>
#include <arduino.h>
#include <helper_general.h>

float WindFunc::convertUnit(float speed, WindSpeedUnit fromUnit,
                            WindSpeedUnit toUnit) {
  float tmpKmPerHour = 0.0f;
  float result = speed;
  if (fromUnit != toUnit) {
    switch (fromUnit) {
      case WindSpeedUnit::MetersPerSecond:
        tmpKmPerHour = 3.6f * speed;
        break;
      case WindSpeedUnit::Knots:
        tmpKmPerHour = 1.852f * speed;
        break;
      case WindSpeedUnit::MilesPerHour:
        tmpKmPerHour = 1.6093f * speed;
        break;
      case WindSpeedUnit::KmPerHour:
        tmpKmPerHour = speed;
        break;
      case WindSpeedUnit::FeetPerMinute:
        tmpKmPerHour = 0.0183f * speed;
        break;
      default:
        break;
    }
    switch (toUnit) {
      case WindSpeedUnit::MetersPerSecond:
        result = tmpKmPerHour * 0.2777f;
        break;
      case WindSpeedUnit::Knots:
        result = tmpKmPerHour * 0.5399f;
        break;
      case WindSpeedUnit::MilesPerHour:
        result = tmpKmPerHour * 0.6213f;
        break;
      case WindSpeedUnit::KmPerHour:
        result = tmpKmPerHour;
        break;
      case WindSpeedUnit::FeetPerMinute:
        result = tmpKmPerHour * 54.6806f;
        break;
      default:
        break;
    }
  }
  return result;
}

void WindFunc::addWindSpeed(float speed) {
  _windSpeedSamples++;
  _windSpeedCurrent = speed;
  if (_windSpeedSamples == 1) {
    _windSpeedMin = _windSpeedMax = _windSpeedSum = speed;
  } else {
    if (speed < _windSpeedMin) _windSpeedMin = speed;
    if (speed > _windSpeedMax) _windSpeedMax = speed;
    _windSpeedSum += speed;
  }
}

void WindFunc::addWindDirection(uint16_t direction) {
  if (direction >= 0 && direction < 360) {
    _windDirectionSamples++;
    _windDirectionCurrent   = direction;
    _windDirectionCosTotal += cos(direction * DEG2RAD);
    _windDirectionSinTotal += sin(direction * DEG2RAD);
  }
}

bool WindFunc::getWindSpeed(float &windSpeedCurrent, float &windSpeedAverage,
                            float &windSpeedMin, float &windSpeedMax) {
  if (_windSpeedSamples > 0) {
    windSpeedAverage  = _windSpeedSum / _windSpeedSamples;
    windSpeedMin      = _windSpeedMin;
    windSpeedMax      = _windSpeedMax;
    windSpeedCurrent  = _windSpeedCurrent;
    return true;
  } else {
    windSpeedCurrent = windSpeedAverage = windSpeedMin = windSpeedMax = 0;
    return false;
  }
}

bool WindFunc::getWindDirection(uint16_t &windDirectionCurrent,
                                uint16_t &windDirectionAverage) {
  if (_windDirectionSamples > 0) {
    // ! (int) or (uint16_t) ?
    windDirectionCurrent = _windDirectionCurrent;
    windDirectionAverage =
        (int)((atan2(_windDirectionSinTotal, _windDirectionCosTotal) /
               DEG2RAD) +
              360) %
        360;
    TLOGDEBUGF_P(PSTR("[DEBUG] SinTotal %.3f CosTotal: %.3f Average Wind "
                      "Direction: %d.\n"),
                 _windDirectionSinTotal, _windDirectionCosTotal,
                 windDirectionAverage);
    return true;
  } else
    return false;
}

void WindFunc::resetWindSpeed() {
  _windSpeedSamples = 0;
  _windSpeedSum     = 0;
  _windSpeedCurrent = 0.0f;
}

void WindFunc::resetWindDirection() {
  _windDirectionSamples   = 0;
  _windDirectionCurrent   = 0;
  _windDirectionCosTotal  = 0.0f;
  _windDirectionSinTotal  = 0.0f;
}

int WindFunc::meterPerSecToBeaufort(float m_per_sec) {
  if (m_per_sec < 0.3) {
    return 0;
  }
  if (m_per_sec < 1.6) {
    return 1;
  }
  if (m_per_sec < 3.4) {
    return 2;
  }
  if (m_per_sec < 5.5) {
    return 3;
  }
  if (m_per_sec < 8.0) {
    return 4;
  }
  if (m_per_sec < 10.8) {
    return 5;
  }
  if (m_per_sec < 13.9) {
    return 6;
  }
  if (m_per_sec < 17.2) {
    return 7;
  }
  if (m_per_sec < 20.8) {
    return 8;
  }
  if (m_per_sec < 24.5) {
    return 9;
  }
  if (m_per_sec < 28.5) {
    return 10;
  }
  if (m_per_sec < 32.6) {
    return 11;
  }
  return 12;
}

String WindFunc::getBearingStr(int degrees) {
  const int nr_directions = 16;
  float stepsize = (360.0 / nr_directions);

  if (degrees < 0) {
    degrees += 360;
  }  // Allow for bearing -360 .. 359
  int bearing_idx =
      int((degrees + (stepsize / 2.0)) / stepsize) % nr_directions;

  String dirStr = F("-");
  switch (bearing_idx) {
    case 0:
      dirStr = F("N");
    case 1:
      dirStr = F("NNE");
    case 2:
      dirStr = F("NE");
    case 3:
      dirStr = F("ENE");
    case 4:
      dirStr = F("E");
    case 5:
      dirStr = F("ESE");
    case 6:
      dirStr = F("SE");
    case 7:
      dirStr = F("SSE");
    case 8:
      dirStr = F("S");
    case 9:
      dirStr = F("SSW");
    case 10:
      dirStr = F("SW");
    case 11:
      dirStr = F("WSW");
    case 12:
      dirStr = F("W");
    case 13:
      dirStr = F("WNW");
    case 14:
      dirStr = F("NW");
    case 15:
      dirStr = F("NNW");
    default:
      dirStr = F("-");
  }
  return dirStr;
}