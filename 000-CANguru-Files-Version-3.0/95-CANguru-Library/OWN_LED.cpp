
#include <Arduino.h>

uint8_t LED_BUILTIN_OWN;

void LED_begin(uint8_t LED_)
{
#ifdef ESP32_DECODER
  LED_BUILTIN_OWN = LED_;
  pinMode(LED_BUILTIN_OWN, OUTPUT);
  // turn the LED off by making the voltage LOW
  digitalWrite(LED_BUILTIN_OWN, LOW);
#endif
}

void LED_on()
{
#ifdef ESP32_DECODER
  digitalWrite(LED_BUILTIN_OWN, HIGH);
#endif
}

void LED_off()
{
#ifdef ESP32_DECODER
  digitalWrite(LED_BUILTIN_OWN, LOW);
#endif
}
