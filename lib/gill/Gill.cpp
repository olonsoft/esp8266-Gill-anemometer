#include <Gill.h>
#include <helper_general.h>

// Define protocol constants locally
#define GILL_STX '\x02'
#define GILL_ETX '\x03'
#define MAX_BUFFER_LEN 60 // Safe size for Gill protocol

Gill::Gill() {
  _speed = 0.0;
  _direction = 0;
  _serialDataResult = DecodedDataResult::NoControlChars;
  _targetWindSpeedUnit = WindSpeedUnit::MetresPerSecond; // Default
}

DecodedDataResult Gill::decodeSerialData(const char *data) {
  // example data: Q,325,009.80,N,00,18

  // 1. Locate Start and End
  const char *stxPtr = strchr(data, GILL_STX);
  const char *etxPtr = strchr(data, GILL_ETX);

  if (stxPtr == nullptr || etxPtr == nullptr) {
    return DecodedDataResult::NoControlChars;
  }

  // 2. Validate Length
  // Distance between STX and ETX (excluding STX)
  int payloadLen = etxPtr - stxPtr - 1;

  if (payloadLen <= 0 || payloadLen >= MAX_BUFFER_LEN) {
    return DecodedDataResult::LongMessageError;
  }

  // 3. Create a working buffer (Stack allocated, safe)
  char workingBuffer[MAX_BUFFER_LEN];
  // Copy payload (everything between STX and ETX)
  memcpy(workingBuffer, stxPtr + 1, payloadLen);
  workingBuffer[payloadLen] = '\0'; // Null terminate

  // 4. Verify Checksum
  // Logic: XOR sum of bytes between STX and ETX
  char calcChecksum = 0;
  for (int i = 0; i < payloadLen; i++) {
    calcChecksum ^= workingBuffer[i];
  }

  // The checksum provided is usually the 2 chars AFTER ETX
  char givenChecksum = (char)strtol(etxPtr + 1, nullptr, 16);

  TLOGDEBUGF_P(PSTR("Msg: %s | Calc: %X | Given: %X\n"), workingBuffer, calcChecksum, givenChecksum);

  if (calcChecksum != givenChecksum) {
    return DecodedDataResult::CheckSumError;
  }

  // 5. Parse Tokens
  // Expected Format: Q,DDD,SSS.SS,U,AA
  // We use strtok_r (reentrant version) for safety, though strtok is okay here too.

  char* nextToken;
  char* token = strtok_r(workingBuffer, ",", &nextToken);

  int tokenIndex = 0;
  char rawUnit = 'M';
  int deviceStatus = 0;

  while (token != nullptr) {
    switch (tokenIndex) {
      case 0: // Node ID (e.g., Q)
        break;
      case 1: // Direction (e.g., 325)
        _direction = atoi(token);
        break;
      case 2: // Speed (e.g., 009.80)
        _speed = atof(token);
        break;
      case 3: // Unit (e.g., N)
        rawUnit = token[0];
        break;
      case 4: // Status (e.g., 00 or V)
        if (strcmp("V", token) == 0) deviceStatus = 11; // Void
        else if (strcmp("A", token) == 0) deviceStatus = 10; // Alignment
        else deviceStatus = (int)strtol(token, nullptr, 16);
        break;
    }
    token = strtok_r(nullptr, ",", &nextToken);
    tokenIndex++;
  }

  if (deviceStatus != 0) {
    return DecodedDataResult::DeviceError;
  }

  // 6. Convert Units
  WindSpeedUnit receivedUnit = parseUnitChar(rawUnit);

  // Use the member object _wfGill, not the global
  _speed = _wfGill.convertUnit(_speed, receivedUnit, _targetWindSpeedUnit);

  TLOGDEBUGF_P(PSTR("Res: Dir %d, Spd %.2f\n"), _direction, _speed);

  return DecodedDataResult::Ok;
}

WindSpeedUnit Gill::parseUnitChar(char c) {
  switch (c) {
    case 'M': return WindSpeedUnit::MetresPerSecond;
    case 'N': return WindSpeedUnit::Knots;
    case 'P': return WindSpeedUnit::MilesPerHour;
    case 'K': return WindSpeedUnit::KmPerHour;
    case 'F': return WindSpeedUnit::FeetPerMinute;
    default:  return WindSpeedUnit::MetresPerSecond;
  }
}

void Gill::setWindSpeedUnit(WindSpeedUnit speedUnit) {
  _targetWindSpeedUnit = speedUnit;
}

WindSpeedUnit Gill::getWindSpeedUnit() { return _targetWindSpeedUnit; }

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