#include <Gill.h>
#include <arduino.h>
#include <helper_general.h>

// #define STX '\x02'    // start transmition
// #define ETX '\x03'    // end transmition
#define MAX_TOKENS 5

WindFunc wfGill;

DecodedDataResult Gill::decodeSerialData(char *data) {
  const char STX = '\x02';
  const char ETX = '\x03';
  // Find the positions of STX and ETX in the data
  char *stxPosition = strchr(data, STX);
  char *etxPosition = strchr(data, ETX);

  if (stxPosition == nullptr || etxPosition == nullptr)
    return DecodedDataResult::NoControlChars;

  // Calculate the length of the substring between STX and ETX
  size_t length = etxPosition - stxPosition - 1;

  // Extract the substring between STX and ETX
  char extractedData[length + 1];
  strncpy(extractedData, stxPosition + 1, length);
  extractedData[length] = '\0';  // Null-terminate the string

  // calculate the extracted data checksum
  char payload_checksum = 0;
  size_t i = 0;
  while (extractedData[i] != '\0') {
    payload_checksum ^= extractedData[i++];
  }

  // extract the given checksum from end of line
  // Get the substring after ETX
  char *numberString = etxPosition + 1;

  // Convert the substring to a hex number
  char given_checksum = (char)strtol(numberString, nullptr, 16);

  // check if checksum match
  TLOGDEBUGF_P(PSTR("msg: %s crc: %X cs: %X\n"), extractedData,
               payload_checksum, given_checksum);
  if (payload_checksum != given_checksum)
    return DecodedDataResult::CheckSumError;

  // Parse the message
  char deviceID = '\0';
  char unit = '\0';
  int error = 0;
  char *token = strtok(extractedData, ",");
  int tokenCount = 0;
  while (token != nullptr) {
    switch (tokenCount) {
      case 0:
        // Process the device ID (assuming it's a character)
        deviceID = token[0];
        break;
      case 1:
        // Process the direction (assuming it's an integer)
        _direction = atoi(token);
        break;
      case 2:
        // Process the speed (assuming it's a floating point number)
        _speed = atof(token);
        break;
      case 3:
        // Process the unit (assuming it's a character)
        unit = token[0];
        break;
      case 4:
        // Process the error (assuming it's an integer)
        if (strcmp("V", token) == 0) {
          error = 11;
        } else if (strcmp("A", token) == 0) {
          error = 10;
        } else {
          error = atoi(token);
        }
        break;
      default:
        // Handle any additional tokens if necessary
        break;
    }
    TLOGDEBUGF_P(PSTR("%s\n"), token);
    token = strtok(nullptr, ",");
    tokenCount++;
  }
  if (error > 0) {
    return DecodedDataResult::DeviceError;
  }
  WindSpeedUnit receivedUnit;

  switch (unit) {
    case 'M':
      receivedUnit = WindSpeedUnit::MetresPerSecond;
      break;
    case 'N':
      receivedUnit = WindSpeedUnit::Knots;
      break;
    case 'P':
      receivedUnit = WindSpeedUnit::MilesPerHour;
      break;
    case 'K':
      receivedUnit = WindSpeedUnit::KmPerHour;
      break;
    case 'F':
      receivedUnit = WindSpeedUnit::FeetPerMinute;
      break;
    default:
      receivedUnit = WindSpeedUnit::MetresPerSecond;
      break;
  }

  _speed = wfGill.convertUnit(_speed, receivedUnit, _windSpeedUnit);
  TLOGDEBUGF_P(PSTR("Converted: %.2f\n"), _speed);

  return DecodedDataResult::Ok;
}

DecodedDataResult Gill::decodeSerialData1(char *data) {
  const char STX = '\x02';
  const char ETX = '\x03';
  // example data: Q,325,009.80,N,00,18
  char msg[25];
  // start of text
  if (*data != STX) return DecodedDataResult::NoControlChars;
  ++data;

  // data: Q,325,009.80,N,00,18
  char cs = 0;  // calculate checksum
  int i = 0;
  while (*data && ETX != *data) {
    cs ^= *data;
    msg[i++] = *data++;

    if (i > 18) {  // 18
      return DecodedDataResult::LongMessageError;
    }
  }
  msg[i - 1] = '\0';

  // msg = Q,325,009.80,N,00,
  // data = 18

  // end of text
  if (*data != ETX) return DecodedDataResult::NoControlChars;

  ++data;

  // checksum on message
  char scs = (char)strtol(data, nullptr, 16);
  TLOGDEBUGF_P(PSTR("msg: %s cs: %X crc: %X\n"), msg, cs, scs);
  if (scs != cs)
    return DecodedDataResult::CheckSumError;
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
      return DecodedDataResult::DeviceError;
    }

    WindSpeedUnit receivedUnit;

    switch (unit) {
      case 'M':
        receivedUnit = WindSpeedUnit::MetresPerSecond;
        break;
      case 'N':
        receivedUnit = WindSpeedUnit::Knots;
        break;
      case 'P':
        receivedUnit = WindSpeedUnit::MilesPerHour;
        break;
      case 'K':
        receivedUnit = WindSpeedUnit::KmPerHour;
        break;
      case 'F':
        receivedUnit = WindSpeedUnit::FeetPerMinute;
        break;
      default:
        receivedUnit = WindSpeedUnit::MetresPerSecond;
        break;
    }

    _speed = wfGill.convertUnit(_speed, receivedUnit, _windSpeedUnit);
    TLOGDEBUGF_P(PSTR("Converted: %.2f\n"), _speed);
  }

  return DecodedDataResult::Ok;
}

void Gill::setWindSpeedUnit(WindSpeedUnit speedUnit) {
  _windSpeedUnit = speedUnit;
}

WindSpeedUnit Gill::getWindSpeedUnit() { return _windSpeedUnit; }

DecodedDataResult Gill::getSerialDataResult() { return _serialDataResult; }

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