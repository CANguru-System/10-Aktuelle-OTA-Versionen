#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <OTA_include.h>
#include "ticker.h"
#include "OWN_LED.h"
#include "Preferences.h"

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
    LED_on();
  else
    // turn the LED off by making the voltage LOW
    LED_off();
}

// Create AsyncWebServer object on port 80
WebServer server(80);

unsigned long ota_progress_millis = 0;

void onOTAStart() {
  // Log when OTA has started
  Serial.println("OTA update started!");
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\r\n", current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Serial.println("OTA update finished successfully!");
  } else {
    Serial.println("There was an error during OTA update!");
  }
  // <Add your own code here>
}

void Connect2WiFiandOTA(Preferences preferences)
{
  tckrOTA.attach(tckrOTATime, timerOTA); // each sec
  blink = blinkFast;

  String ssid;
  ssid = preferences.getString("ssid", "No SSID");
  String password;
  password = preferences.getString("password", "No password");
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
            { server.send(200, "text/plain", "\r\n\rCANguru update function\r\n"); });

  // Start ElegantOTA
  ElegantOTA.begin(&server);
  // ElegantOTA callbacks
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);
    // Start server
  server.begin();
  while (true)
  {
    server.handleClient();
    ElegantOTA.loop();
  }
}
