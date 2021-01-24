#include "FOTA_ESP.h"
#include <functional>
#include <ArduinoJson.h>
#include <helper.h>

#ifndef TERMINAL_IN_COLOR
// #define TERMINAL_IN_COLOR
#endif

#ifdef TERMINAL_IN_COLOR
#define FOTA_STR "[\e[32mFOTA\e[m] "
#else
#define FOTA_STR "[FOTA] "
#endif

#ifdef ESP8266    
  #define ESP_UPD ESPhttpUpdate
#elif defined(ESP32)    
  #define ESP_UPD httpUpdate
#endif    

char buffer[200];

/*
char *strFOTA[] PROGMEM = { 
  "Start",
  "No Response Errror",
  "Parse Error",
  "UpToDate",
  "New Firmware found",
  "New Filesystem found",
  "Updating firmware",
  "Updating filesystem",
  "Firmware uploaded",
  "Filesystem uploaded",
  "Firmware update error",
  "Filesystem update error",
  "Reset",
  "End"
};
*/

void FOTAClientClass::onMessage(TMessageFunction fn) {
    _callback = fn;
}

void FOTAClientClass::setFOTAParameters(String FOTA_url, String jsonFile, String currentFwVersion, String currentFsVersion) {
    _FOTA_url         = FOTA_url;
    _jsonFile         = jsonFile;
    _currentFwVersion = currentFwVersion;
    _currentFsVersion = currentFsVersion;
}

String FOTAClientClass::getNewFirmwareVersionNumber() {
    return _newFwVersion;
}

String FOTAClientClass::getNewFirmwareURL() {
    return _newFwURL;
}

String FOTAClientClass::getNewFileSystemVersionMumber() {
    return _newFsVersion;
}

String FOTAClientClass::getNewFileSystemURL() {
    return _newFsURL;
}

int FOTAClientClass::getErrorNumber() {
    return _errorNumber;
}

String FOTAClientClass::getErrorString() {
    return _errorString;
}

void FOTAClientClass::_doCallback(fota_t message, char * parameter) {
    if (_callback != NULL) _callback(message, parameter);
}

String FOTAClientClass::_getPayload() {
    //Serial.println(__func__);

    String payload = "";
    String f = _FOTA_url + _jsonFile;
    //Serial.println(f);
    // please follow the exact order as the next two lines. Exception occurs otherwise
    WiFiClient client;
    // the next, must be declared after WiFiClient for correct destruction order, 
    // because used by http.begin(client,...)
    HTTPClient http; 
    http.begin(client, (char *) f.c_str());    
    http.useHTTP10(true);
    http.setReuse(false);
    http.setTimeout(HTTP_TIMEOUT);
    http.setUserAgent(F(HTTP_USERAGENT));

    http.addHeader(F("X-ESP-MAC"), WiFi.macAddress());
    //http.addHeader(F("X-ESP-DEVICE"), _device);
    http.addHeader(F("X-ESP-FIRMWARE-VERSION"), _currentFwVersion);
    http.addHeader(F("X-ESP-FS-VERSION"), _currentFsVersion);
    //http.addHeader(F("X-ESP Build"), String(__UNIX_TIMESTAMP__))
    http.addHeader(F("X-ESP-CHIPID"), getChipIdHex());
    http.addHeader(F("X-ESP-free-space"), String(ESP.getFreeSketchSpace()));
    http.addHeader(F("X-ESP-sketch-size"), String(ESP.getSketchSize()));

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        payload = http.getString();
    } else {
        _errorNumber = httpCode;
        _errorString = http.errorToString(httpCode);
    }
    http.end();
    //Serial.println(payload);
    return payload;
}

char* FOTAClientClass::_str(String txt) {
    snprintf_P(buffer, sizeof(buffer),
                   PSTR("%s%s"), FOTA_STR, txt.c_str());
    return buffer; //const_cast<char *>(s.c_str());
}

bool FOTAClientClass::checkUpdates() {
    //Serial.println(__func__);
    _newFirmware   = false;
    _newFileSystem = false;

    String payload = _getPayload();
    if (payload.length() == 0) {        
        snprintf_P(buffer, sizeof(buffer), PSTR("%sNo Response Error.\n"), FOTA_STR);
        _doCallback(FOTA_NO_RESPONSE_ERROR, buffer);
        return false;
    }
    StaticJsonDocument<500> doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        snprintf_P(buffer, sizeof(buffer), PSTR("%sDeserializeJson Error.\n"), FOTA_STR);
        _doCallback(FOTA_PARSE_ERROR, buffer);
        return false;
    }
    if (doc.size() == 0) {
        snprintf_P(buffer, sizeof(buffer), PSTR("%sEmpty file.\n"), FOTA_STR);
        _doCallback(FOTA_UPTODATE, buffer);
        return false;
    }
    //JsonObject obj = doc.as<JsonObject>();
    _newFwVersion =             doc["fwVersion"].as<String>();
    _newFsVersion =             doc["fsVersion"].as<String>();
    _newFwURL     = _FOTA_url + doc["fwFile"].as<String>();
    _newFsURL     = _FOTA_url + doc["fsFile"].as<String>();;

    _newFirmware   = (_newFwVersion != _currentFwVersion);
    _newFileSystem = (_newFsVersion   != _currentFsVersion);
    if (!_newFirmware && !_newFileSystem) {   
       snprintf_P(buffer, sizeof(buffer), PSTR("%sUpToDate.\n"), FOTA_STR);      
        _doCallback(FOTA_UPTODATE, buffer);
        return false;
    }
    
    if (_newFirmware) {        
        snprintf_P(buffer, sizeof(buffer), PSTR("%sNew firmware found: %s\n"), FOTA_STR, _newFwVersion.c_str());      
        _doCallback(FOTA_NEW_FIRMWARE_FOUND, buffer);
    } 
    if (_newFileSystem) {        
        snprintf_P(buffer, sizeof(buffer), PSTR("%sNew filesystem found: %s \n"), FOTA_STR, _newFsVersion.c_str());      
        _doCallback(FOTA_NEW_FILESYSTEM_FOUND, buffer); 
    }
    return true;

}

bool FOTAClientClass::newFirmwareFound() {
    return _newFirmware;
}

bool FOTAClientClass::newFileSystemFound() {
    return _newFileSystem;
}

bool FOTAClientClass::updateFirmware(String FOTA_bin_url) {
    //Serial.println(__func__);
    snprintf_P(buffer, sizeof(buffer), PSTR("%sUpdating firmware to: %s\n"), 
               FOTA_STR, FOTA_bin_url.c_str());
    _doCallback(FOTA_UPDATING_FIRMWARE, buffer);
    bool result = false;
 
    ESP_UPD.rebootOnUpdate(false); 
    //Serial.println(FOTA_bin_url);    
    WiFiClient client;  
    
    t_httpUpdate_return ret = ESP_UPD.update(client, FOTA_bin_url);

    if (ret == HTTP_UPDATE_FAILED) {
        result = false;
        _errorNumber = ESP_UPD.getLastError();
        _errorString = ESP_UPD.getLastErrorString();
        snprintf_P(buffer, sizeof(buffer), PSTR("%sFirmware update error: %s.\n"), FOTA_STR, _errorString.c_str());        
        _doCallback(FOTA_FIRMWARE_UPDATE_ERROR, buffer);

    } else if (ret == HTTP_UPDATE_OK) {        
        result = true;
        snprintf_P(buffer, sizeof(buffer), PSTR("%sFirmware uploaded.\n"), FOTA_STR);        
        _doCallback(FOTA_FIRMWARE_UPLOADED, buffer);
    }
    return result;
}

bool FOTAClientClass::updateFileSystem(String FOTA_fs_url) {
    //Serial.println(__func__);
    snprintf_P(buffer, sizeof(buffer), PSTR("%sUpdating filesystem to: %s\n"), 
               FOTA_STR, FOTA_fs_url.c_str());        
    _doCallback(FOTA_UPDATING_FILESYSTEM, buffer);
    bool result = false;
    ESP_UPD.rebootOnUpdate(false); 
    //Serial.println(FOTA_fs_url);     
    ESP_FS.end(); // unmound file system 
    WiFiClient client;
#ifdef ESP32    
    t_httpUpdate_return ret = ESP_UPD.updateSpiffs(client, FOTA_fs_url);
#elif defined(ESP8266)    
    t_httpUpdate_return ret = ESP_UPD.updateFS(client, FOTA_fs_url);
#endif
    if (ret == HTTP_UPDATE_FAILED) {
        result = false;
        _errorNumber = ESP_UPD.getLastError();
        _errorString = ESP_UPD.getLastErrorString();
        snprintf_P(buffer, sizeof(buffer), PSTR("%sFilesystem update error: %s.\n"), FOTA_STR, _errorString.c_str());            
        _doCallback(FOTA_FILESYSTEM_UPDATE_ERROR, buffer);

    } else if (ret == HTTP_UPDATE_OK) {        
        result = true;
        snprintf_P(buffer, sizeof(buffer), PSTR("%sFilesystem uploaded.\n"), FOTA_STR);    
        _doCallback(FOTA_FILESYSTEM_UPLOADED, buffer);
    }
    return result;
}

void FOTAClientClass::checkAndUpdateFOTA(bool reboot) {
    //Serial.println(__func__);
    snprintf_P(buffer, sizeof(buffer), 
        PSTR("%sStart.\n\tFOTA url: %s\n\tJSON_file: %s\n\tCurrent FW version: %s\n\tCurrent FS version: %s\n"), 
        FOTA_STR, 
        _FOTA_url.c_str(), 
        _jsonFile.c_str(), 
        _currentFwVersion.c_str(), 
        _currentFsVersion.c_str());
    _doCallback(FOTA_START, buffer);
    bool error = false;
    uint8_t updates = 0;
    if (checkUpdates()) {
        if (_newFileSystem) {
            error = !updateFileSystem(_newFsURL);
            if (!error) updates++;
        }
        if (_newFirmware /* && !error */) {  // ! should I update firmware if error uploading filesystem ?
            error = !updateFirmware(_newFwURL);
            if (!error) updates++;
        }
    }
    snprintf_P(buffer, sizeof(buffer), PSTR("%sEnd.\n"), FOTA_STR);    
    _doCallback(FOTA_END, buffer);
    if (!error && (updates > 0)) {
        if (reboot) {
            snprintf_P(buffer, sizeof(buffer), PSTR("%sRebooting...\n"), FOTA_STR);    
            _doCallback(FOTA_RESET, buffer);
            ESP.restart();
            delay(1000);
        }
    }
}

FOTAClientClass FOTAClient;
