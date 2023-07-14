
#include <Arduino.h>
#include "CARguruDefs.h"
#include "OTA_include.h"
#include "WebServer.h"
#include <WiFi.h>
#include "esp_timer.h"
#include "ticker.h"
#include "EEPROM.h"


const uint8_t LED_BUILTIN_OWN = LED_BUILTIN;

// EEPROM-Adressen
const uint8_t eepromAdr = 0x00;
const uint8_t adrMax = 0xFF;
const uint8_t eepromChr = 0xFF;
const uint16_t EEPROM_SIZE = eepromChr;

Ticker tckr;
const float tckrTime = 0.01;

enum blinkStatus
{
  blinkFast = 0, // wartet auf Password
  blinkSlow,     // wartet auf WiFi
  blinkNo        // mit WiFi verbunden
};
blinkStatus blink;

const uint8_t veryBright = 0xFF;
const uint8_t bright = veryBright / 2;
const uint8_t smallBright = bright / 2;
const uint8_t dark = 0x00;

uint8_t setup_todo;

String liesEingabe()
{
  boolean newData = false;
  char receivedChars[numChars]; // das Array für die empfangenen Daten
  static byte ndx = 0;
  char endMarker = '\r';
  char rc;

  while (newData == false)
  {
    while (Serial.available() > 0)
    {
      rc = Serial.read();

      if (rc != endMarker)
      {
        receivedChars[ndx] = rc;
        Serial.print(rc);
        ndx++;
        if (ndx >= numChars)
        {
          ndx = numChars - 1;
        }
      }
      else
      {
        receivedChars[ndx] = '\0'; // Beendet den String
        Serial.println();
        ndx = 0;
        newData = true;
      }
    }
  }
  return receivedChars;
}

String netzwerkScan()
{
  // Zunächst Station Mode und Trennung von einem AccessPoint, falls dort eine Verbindung bestand
  //  WiFi.mode(WIFI_STA);
  //  WiFi.disconnect();
  delay(100);

  Serial.println("Scan-Vorgang gestartet");

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("Scan-Vorgang beendet");
  if (n == 0)
  {
    Serial.println("Keine Netzwerke gefunden!");
  }
  else
  {
    Serial.print(n);
    Serial.println(" Netzwerke gefunden");
    for (int i = 0; i < n; ++i)
    {
      // Drucke SSID and RSSI für jedes gefundene Netzwerk
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
      delay(10);
    }
  }
  uint8_t number;
  do
  {
    Serial.println("Bitte Netzwerk auswaehlen: ");
    String no = liesEingabe();
    number = uint8_t(no[0]) - uint8_t('0');
  } while ((number > n) || (number == 0));

  return WiFi.SSID(number - 1);
}

// der Timer steuert das Scannen der Slaves, das Blinken der LED
// sowie das Absenden des PING
void timer1s()
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

void eraseEEPROM()
{
  for (uint8_t adr = 0; adr < adrMax; adr++)
  {
    EEPROM.write(eepromAdr + adr, eepromChr);
    EEPROM.commit();
  }
  // setup in der Anwndung erzwingen
  setup_todo = setup_NOT_done;
  Serial.println("EEPROM erfolgreich geloescht!");
}

void setup()
{
  Serial.begin(115200);
  delay(500);
  while (!Serial)
  {
  }
  Serial.setDebugOutput(false);
  // Initialize all board peripherals, call this first
  // Brightness is 0-255. We set it to 1/3 brightness here
  Serial.println("\r\n\rS t a r t   OTA");
  Serial.println("Variante ESP32");

  Serial.println("\r\nZeigt die eigene IP-Adresse");
  Serial.println("und bereitet OTA vor\r\n");

  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("EEPROM-Fehler");
  }
  tckr.attach(tckrTime, timer1s); // each sec
  blink = blinkFast;
  setup_todo = EEPROM.readByte(adr_setup_done);
  String answer;
  do
  {
    Serial.println("EEPROM loeschen (J/N)?: ");
    answer = liesEingabe();
    answer.toUpperCase();
  } while ((answer != "J") && (answer != "N"));
  if (answer == "J")
    eraseEEPROM();
  else
  {
    answer = "";
    do
    {
      Serial.println("Neue Netzwerkdaten (J/N)?: ");
      answer = liesEingabe();
      answer.toUpperCase();
    } while ((answer != "J") && (answer != "N"));
  }
  String ssid = "";
  String password = "";
  if (answer == "J")
  {
    // alles fürs erste Mal
    //
    ssid = netzwerkScan();
    EEPROM.writeString(adr_ssid, ssid);
    EEPROM.commit();
    Serial.println();
    // liest das password ein
    Serial.print("Bitte das Passwort eingeben (Falls Sie sich dabei vertippen, muessen Sie den Prozess neu starten!!): ");
    password = liesEingabe();
    EEPROM.writeString(adr_password, password);
    EEPROM.commit();
    Serial.println();
  }
  else
  {
    blink = blinkSlow;
    ssid = EEPROM.readString(adr_ssid);
    password = EEPROM.readString(adr_password);
  }
  char ssidCh[ssid.length() + 1];
  ssid.toCharArray(ssidCh, ssid.length() + 1);
  char passwordCh[password.length() + 1];
  password.toCharArray(passwordCh, password.length() + 1);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Verbinde mit dem Netzwerk -");
  Serial.print(ssidCh);
  Serial.println("-");
  Serial.print("Mit dem Passwort -");
  Serial.print(passwordCh);
  Serial.println("-");
  WiFi.begin(ssidCh, passwordCh);
  uint8_t trials = 0;
  blink = blinkSlow;
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    delay(2000);
    Serial.print(".");
    trials++;
    if (trials > 5)
    {
      // zuviele Versuche für diese Runde
      setup_todo = setup_NOT_done;
      EEPROM.commit();
      ESP.restart();
    }
  }
  // WLAN hat funktioniert
  blink = blinkNo;
  // setup_done setzen
  EEPROM.writeByte(adr_setup_done, setup_todo);
  EEPROM.commit();
  // Print local IP address and start web server
  Serial.println();
  Serial.print("Eigene IP-Adresse: ");
  IPAddress IP = WiFi.localIP();
  Serial.println(IP);
  for (uint8_t ip = 0; ip < 4; ip++)
  {
    EEPROM.writeByte(adr_IP0 + ip, IP[ip]);
    EEPROM.commit();
  }

  tckr.detach();

  Serial.println();
  Serial.println("Bitte dieses Programm schließen!");
}

void loop()
{
}