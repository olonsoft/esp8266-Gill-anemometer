#include <Gill.h>
#include <arduino.h>
#include <helper_general.h>

#define STX '\x02'
#define ETX '\x03'
#define MAX_TOKENS 5

WindFunc wfGill;

SerialDataResult_t Gill::decodeSerialData(char *data) {
  char msg[25];
  // start of text
  if (*data != STX) return SerialDataResult_t::srNoControlChars;
  ++data;

  // actual message
  char cs = 0;  // checksum
  int i = 0;
  while (*data && ETX != *data) {
    cs ^= *data;
    msg[i++] = *data++;

    if (i > 18) {  // 18
      return SerialDataResult_t::srLongMessageError;
    }
  }
  msg[i - 1] = '\0';

  // end of text
  if (*data != ETX) return SerialDataResult_t::srNoControlChars;
  ++data;

  // checksum
  char scs = (char)strtol(data, nullptr, 16);
  TLOGDEBUGF_P(PSTR("msg: %s cs: %X crc: %X\n"), msg, cs, scs);
  if (scs != cs)
    return SerialDataResult_t::srCheckSumError;
  else {
    const char *delim = ",";
    int token_count = 0;
    char deviceID = '\0';
    char unit = '\0';
    int error = 0;

    char *tofree, *tok, *end;
    tofree = end = strdup(msg);
    while ((tok = strsep(&end, delim)) != nullptr && token_count < MAX_TOKENS) {
      switch (token_count) {
        case 0:
          deviceID = tok[0];
          break;
        case 1:
          _direction = atoi(tok);
          break;
        case 2:
          _speed = atof(tok);
          break;
        case 3:
          unit = tok[0];
          break;
        case 4:
          if (strcmp("V", tok) == 0) {  // NMEA data Void
            error = 11;
          } else {
            error = (int)strtol(tok, nullptr, 16);
          }
        default:
          break;
      }
      token_count++;

      TLOGDEBUGF_P(PSTR("%s\n"), tok);
    }
    //DEBUG_PRINTF("\n");
    free(tofree);

    TLOGDEBUGF_P(PSTR("Tokens: %c %d %.2f %c %d\n"), deviceID, _direction,
                    _speed, unit, error);

    if (error > 0) {
      return SerialDataResult_t::srDeviceError;
    }
    windSpeedUnit_t receivedUnit;
    switch (unit) {
      case 'M':
        receivedUnit = windSpeedUnit_t::wsMetersPerSecond;
        break;
      case 'N':
        receivedUnit = windSpeedUnit_t::wsKnots;
        break;
      case 'P':
        receivedUnit = windSpeedUnit_t::wsMilesPerHour;
        break;
      case 'K':
        receivedUnit = windSpeedUnit_t::wsKmPerHour;
        break;
      case 'F':
        receivedUnit = windSpeedUnit_t::wsFeetPerMinute;
        break;
      default:
        receivedUnit = windSpeedUnit_t::wsMetersPerSecond;
        break;
    }
    _speed = wfGill.convertUnit(_speed, receivedUnit, _windSpeedUnit);
    TLOGDEBUGF_P(PSTR("Converted: %.2f\n"), _speed);
  }

  return SerialDataResult_t::srOK;
}

void Gill::setSpeedUnit(windSpeedUnit_t speedUnit) {
  _windSpeedUnit = speedUnit;
}

windSpeedUnit_t Gill::getSpeedUnit() { return _windSpeedUnit; }

SerialDataResult_t Gill::getSerialDataResult() { return _serialDataResult; }

float Gill::getSpeed() { return _speed; }

int Gill::getDirection() { return _direction; }

/*
uint16_t calcCRC(char *str)
{
  uint16_t crc = 0;                          // starting value as you like, must
be the same before each calculation for (uint16_t i = 0; i < strlen(str); i++)
// for each character in the string
  {
    crc ^= str[i]; // update the crc value
  }
  DEBUG_PRINT_F(PSTR("CRC: %X\n"), crc);
  return crc;
}
*/