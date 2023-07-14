#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include "EEPROM.h"
#include <OTA_include.h>
#include "ticker.h"

#include <ElegantOTA.h>

Ticker tckrOTA;
const float tckrOTATime = 0.01;

enum blinkStatus
{
  blinkFast = 0, // wartet auf Password
  blinkSlow,     // wartet auf WiFi
  blinkNo        // mit WiFi verbunden
};
blinkStatus blink;

// der Timer steuert das Scannen der Slaves, das Blinken der LED
// sowie das Absenden des PING
void timerOTA()
{
  static uint8_t secs = 0;
  static uint8_t slices = 0;
  slices++;
  switch (blink)
  {
  case blinkFast:
    if (slices >= 10)
    {
      slices = 0;
      secs++;
    }
    break;
  case blinkSlow:
    if (slices >= 40)
    {
      slices = 0;
      secs++;
    }
    break;
  case blinkNo:
      secs = 2;
    break;
  }
  if (secs % 2 == 0)
    // turn the LED on by making the voltage HIGH
    digitalWrite(BUILTIN_LED, HIGH);
  else
    // turn the LED off by making the voltage LOW
    digitalWrite(BUILTIN_LED, LOW);
}

// Create AsyncWebServer object on port 80
WebServer server(80);

void Connect2WiFiandOTA()
{
  tckrOTA.attach(tckrOTATime, timerOTA); // each sec
  blink = blinkFast;

  String ssid = EEPROM.readString(adr_ssid);
  String password = EEPROM.readString(adr_password);
  char ssidCh[ssid.length() + 1];
  ssid.toCharArray(ssidCh, ssid.length() + 1);
  char passwordCh[password.length() + 1];
  password.toCharArray(passwordCh, password.length() + 1);
  WiFi.begin(ssidCh, passwordCh);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    log_i("Connecting to WiFi..");
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", []()
            { server.send(200, "text/plain", "\r\n\rBitte geben Sie ein:\r\n\r'IP-adresse des Decoders'/update"); });

  // Start ElegantOTA
  ElegantOTA.begin(&server);
  // Start server
  server.begin();
  while (true)
  {
   server.handleClient();
  } 
}
