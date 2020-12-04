
/*

// alternative functions to parse serial data
SerialDataResult parseWindData( const char* data, float& speed, uint16_t& dir)
{
  char msg[25];
  // start of text
  if ( *data != STX )
    return srNoControlChars;
  ++data;

  // actual message
  char cs = 0; // checksum
  int i = 0;
  while ( *data && ETX != *data )
  {
    cs ^= *data;
    msg[i++] = *data++;
    
    if (i > 18) {
       return srLongMessageError;
    }    
  }
  msg[i-1] = '\0';

  // end of text
  if ( *data != ETX )
    return srNoControlChars;
  ++data;

  
  // sent checksum
  char scs = (char)strtol( data, 0, 16 );
  
  if (scs != cs) 
    return srCheckSumError;
  else 
  {
      const char* delim = ",";
      const char* tokens[ MAX_TOKENS ];
      int token_count = 0;
      for ( char* cursor = strtok( msg, delim ); cursor; cursor = strtok( 0, delim ) )
      {
        tokens[ token_count ] = cursor;
        ++token_count;
      }
    
      if (tokens[1] && tokens[2] && tokens[4]) {
        dir = atoi( tokens[ 1 ] );   // assumes the token is valid
        speed = atof( tokens[ 2 ] ); // assumes the token is valid
        if (strcmp("00", tokens[ 4 ]) != 0) {
          printf("Device error.\n");
          return srDeviceError;
        }
      }
      
  }  
  DEBUG_PRINT_F(PSTR("[PARSE] dir: %d, speed: %3.2f\n"), dir, speed);
  return srOK;    
  // test this code:
    char *data1 = "Q,320,020.00,N,00,1E",
        addr[16],
        unit[16],
        devicestatus[16];

    int winddir;
    float windspeed;

    if (sscanf (data1, "\x02 %15[^,],%d,%f, %15[^,], %15[^,], \x03",
                addr, &winddir, &windspeed, unit, devicestatus) != 5) {
        fputs ("error parsing data.\n", stderr);
        return srDeviceError;
    }

    Serial.printf ("addr         : %s\n"
            "windir       : %d\n"
            "windspeed    : %.2f\n"
            "unit         : %s\n"
            "devicestatus : %s\n", addr, winddir, windspeed, unit, devicestatus);
}




SerialDataResult parseWindData(char *str, float& wSpeed, uint16_t& wDir) {
    // data format is like : "Q,327,012.44,N,00,18"
    DEBUG_PRINT_F(PSTR("\n[PARSE] Serial input: |%s|\n"), str);
    
    char *pos1 = strchr(str, STX);
    char *pos2 = strchr(str, ETX);
    int p1,p2;
    if (pos1 && pos2) {
        p1 = pos1-str+1;
        p2 = pos2-str+1;        
    } else {
        DEBUG_PRINT_F(PSTR("[PARSE] No wind data found error.\n"));
        return srNoControlChars;
    }

    char *checksumStr;
    checksumStr = strrchr(str, ETX);
    int checksum = 0;
    if (checksumStr) {
        checksum = strtol(checksumStr+1, NULL, 16);    
    }
    
    char cleanData[25];    
    if (p2-p1-1 < 24) {
        strncpy(cleanData, str+p1, p2-p1-1);
        cleanData[p2-p1-1] = '\0';
    } else {
        DEBUG_PRINT_F(PSTR("[PARSE] Long input data error.\n"));
        return srLongMessageError;
    }
     
    uint16_t crc = calcCRC(cleanData);
    DEBUG_PRINT_F(PSTR("[PARSE] Clean data: |%s|\n"), cleanData);
    if (crc != checksum) {
        DEBUG_PRINT_F(PSTR("[PARSE] Checksum Error.\n"));
        return srCheckSumError;
    } else {
        DEBUG_PRINT_F(PSTR("[PARSE] Checksum ok.\n"));
    }
    char *addr = strtok(cleanData, ",");    // Q
    char *dir = strtok(NULL, ",");
    wDir = 0;
    if (dir) wDir = atoi(dir);             // 327
    char *speed = strtok(NULL, ",");
    wSpeed = 0;
    if (speed) wSpeed = atof(speed);       // 12.44
    char *unit = strtok(NULL, ",");        // N
    char* deviceStatus = strtok(NULL,","); // 00

    DEBUG_PRINT_F(PSTR("[PARSE] addr: %s dir: %d, speed: %3.2f unit: %s status: %s checksum %X\n"), addr, wDir, wSpeed, unit, deviceStatus, checksum);
    // device status must be 00 if it is working normally
    if (strcmp("00", deviceStatus) != 0) {
        DEBUG_PRINT_F(PSTR("[DEVICE] Status error: %s\n"), deviceStatus);
        return srDeviceError;
    }        
    return srOK;
}
*/

/*
bool parseWindData(char* str) {
    Serial.println();
    Serial.println(str);
    // data format is like : "Q,327,012.44,N,00,18"
    int k = strlen(str);
    int j = 0;
    // remove unwanted characters
    for (int i = 0; i < k; i++) {
        if ((str[i] > 31) && (str[i] < 127)) {
            str[j] = str[i];
            j++;
        }
    }
    str[j] = 0;
    // now the str = "Q,327,012.44,N,00,18"
    uint16_t len = strlen(str);
    Serial.print("str len: "); Serial.println(len);
    if (len < 12) 
        return false;
    // get the last 2 digits which are the checksum
    char buf[3] = { str[len-2], str[len-1], '\0' };
    int check = strtol(buf, NULL, 16);
    // delete the last 2 chars
    str[len-2] = '\0';
    uint16_t crc = calcCRC(str);

    char* addr = strtok(str, ",");          // Q
    tmpWindDir = atoi(strtok(NULL, ","));   // 327
    tmpWindSpeed = atof(strtok(NULL, ",")); // 12.44
    char* unit = strtok(NULL, ",");         // N
    deviceStatus = strtok(NULL,",");        // 00
    
    DEBUG_PRINT_F(PSTR("[PARSE] addr: %s dir: %d, speed: %f unit: %s status: %s check %X\n"), addr, tmpWindDir, tmpWindSpeed, unit, deviceStatus, check);
    // device status must be 00 if it is working normally
    if (strcmp("00", deviceStatus) == 0) {
        // check crc
        // uint16_t crc = calcCRC(buf);
        if (check != crc) {
            DEBUG_PRINT_F(PSTR("[PARSE] Checksum Error.\n"));
            return false;
        } else {
            DEBUG_PRINT_F(PSTR("[PARSE] Checksum ok.\n"));
            return true;
        }
    } else {
        DEBUG_PRINT_F(PSTR("[DEVICE] Status error: %s\n"), deviceStatus);
        return false;
    }
}
*/

/*
bool parseWindData1(char* str) {
    // data format is like : "Q,327,012.44,N,00,18"

    int k = strlen(str);
    char buf[k+1]; // +1 for the terminator
    int j = 0;
    //if (k>SERIAL_BUFFER_SIZE) k = SERIAL_BUFFER_SIZE;
    // clean buffer from unwanted characters
    for (int i = 0; i < k; i++) { // ! was <=
        if ((str[i] > 31) && (str[i] < 127)) buf[j++] = str[i];
    }
    buf[j] = '\0'; // terminate it
    DEBUG_PRINT("[PARSE] %s\n", buf);
    char buf2[strlen(buf)];
    strcpy(buf2, buf);
    
    char* addr = strtok(buf, ",");
    tmpWindDir = atoi(strtok(NULL, ","));
    tmpWindSpeed = atof(strtok(NULL, ","));
    char* unit = strtok(NULL, ",");
    deviceStatus = strtok(NULL,",");
    int check = strtol(strtok(NULL,",'"), NULL, 16);
    
    DEBUG_PRINT_F(PSTR("[PARSE] addr: %s dir: %d, speed: %f unit: %s status: %s check %X\n"), addr, tmpWindDir, tmpWindSpeed, unit, deviceStatus, check);
    
    buf2[strlen(buf2)-2] = 0;  // remove last 2 checksum chars
    DEBUG_PRINT_F(PSTR("[PARSE] %s\n"),buf2);
    if (strcmp("00", deviceStatus) == 0) {  // device status must be 00 if it is working normally
        uint16_t crc = calcCRC(buf2);
        if (check != crc) {
            DEBUG_PRINT_F(PSTR("[DEVICE] Checksum Error.\n"));
            return false;
        } else {
            DEBUG_PRINT_F(PSTR("[DEVICE] Checksum ok.\n"));
            return true;
        }
    } else {
        DEBUG_PRINT_F(PSTR("[DEVICE] Status error: %s\n"), deviceStatus);
        return false;
    }
}
*/
