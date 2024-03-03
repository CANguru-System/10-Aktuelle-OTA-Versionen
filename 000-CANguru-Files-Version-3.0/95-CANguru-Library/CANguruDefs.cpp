
#include <Arduino.h>
#include "EEPROM.h"
#include "Preferences.h"
#include "CANguruDefs.h"

// Funktion stellt sicher, dass keine unerlaubten 8-Bit-Werte geladen werden können
uint8_t readValfromEEPROM(uint16_t adr, uint8_t val, uint8_t min, uint8_t max)
{
  uint8_t v = EEPROM.readByte(adr);
  if ((v >= min) && (v <= max))
    return v;
  else
  {
    EEPROM.write(adr, val);
    EEPROM.commit();
    return val;
  }
}

// Funktion stellt sicher, dass keine unerlaubten 8-Bit-Werte geladen werden können
uint8_t readValfromPreferences(Preferences& preferences, const char* key, uint8_t val, uint8_t min, uint8_t max)
{
  uint8_t v = preferences.getUChar(key, val);
  if ((v >= min) && (v <= max))
    return v;
  else
  {
    preferences.putUChar(key, val);
    return val;
  }
}

// Funktion stellt sicher, dass keine unerlaubten 16-Bit-Werte geladen werden können
uint16_t readValfromEEPROM16(uint16_t adr, uint16_t val, uint16_t min, uint16_t max)
{
  uint16_t v = EEPROM.readUShort(adr);
  if ((v >= min) && (v <= max))
    return v;
  else
  {
    EEPROM.writeUShort(adr, val);
    EEPROM.commit();
    return val;
  }
}

// Funktion stellt sicher, dass keine unerlaubten 16-Bit-Werte geladen werden können
uint16_t readValfromPreferences16(Preferences& preferences, const char* key, uint16_t val, uint16_t min, uint16_t max)
{
  uint16_t v = preferences.getUShort(key, val);
  if ((v >= min) && (v <= max))
    return v;
  else
  {
    preferences.putUShort(key, val);
    return val;
  }
}

// Mit testMinMax wird festgestellt, ob ein Wert innerhalb der
// Grenzen von min und max liegt
bool testMinMax(uint16_t oldval, uint16_t val, uint16_t min, uint16_t max)
{
  return (oldval != val) && (val >= min) && (val <= max);
}

char highbyte2char(int num){
  num /= 10;
  return char ('0' + num);
}

char lowbyte2char(int num){
  num = num - num / 10 * 10;
  return char ('0' + num);
}

uint8_t oneChar(uint16_t val, uint8_t pos) {
	char buffer [5];
	itoa (val, buffer, 10);
	return buffer[4 - pos];
}

uint8_t dev_type = DEVTYPE_BASE;

uint8_t hex2dec(uint8_t h){
  return h / 16 * 10 + h % 16;
}

