
/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <CANguru-Buch@web.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gustav Wostrack
 * ----------------------------------------------------------------------------
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include "CANguruDefs.h"
#include "EEPROM.h"
#include "esp32-hal-ledc.h"
#include "esp_system.h"
#include <esp_now.h>
#include <esp_wifi.h>
#include <Ticker.h>
#include <ArduinoOTA.h>
#include <OTA_include.h>

const uint8_t maxCntChannels = 16;

// EEPROM-Adressen
#define setup_done 0x47
// EEPROM-Belegung
// EEPROM-Speicherplätze der Local-IDs
const uint16_t adr_index = lastAdr0 + 0x01;
const uint16_t lastAdr = adr_index + maxCntChannels;
#define EEPROM_SIZE lastAdr

// config-Daten
// Parameter-Kanäle
enum Kanals
{
  Kanal00,
  Kanal01,
  Kanal02,
  Kanal03,
  Kanal04,
  Kanal05,
  Kanal06,
  Kanal07,
  Kanal08,
  Kanal09,
  Kanal10,
  Kanal11,
  Kanal12,
  Kanal13,
  Kanal14,
  Kanal15,
  Kanal16,
  endofKanals
};

Kanals CONFIGURATION_Status_Index = Kanal00;

const uint8_t decoderadr = 1;
uint8_t uid_device[uid_num];

// Zeigen an, ob eine entsprechende Anforderung eingegangen ist
bool CONFIG_Status_Request = false;
bool SYS_CMD_Request = false;
bool START_OTA_Request = false;
bool SEND_IP_Request = false;

// Timer
boolean statusPING;

#define VERS_HIGH 0x00 // Versionsnummer vor dem Punkt
#define VERS_LOW 0x02  // Versionsnummer nach dem Punkt

/*
Variablen der Signals & Magnetartikel
*/
const uint8_t zPin = GPIO_NUM_4;
const uint8_t s3Pin = GPIO_NUM_16;
const uint8_t s2Pin = GPIO_NUM_17;
const uint8_t s1Pin = GPIO_NUM_5;
const uint8_t s0Pin = GPIO_NUM_18;
const uint8_t enablePin = GPIO_NUM_19;
uint8_t pins[] = {enablePin, s0Pin, s1Pin, s2Pin, s3Pin};

const uint8_t isFree = 0;
const uint8_t isOccupied = 1;
uint8_t channels[maxCntChannels];

uint8_t msecs[maxCntChannels];
uint8_t inputValue[maxCntChannels];

uint8_t channel_index[maxCntChannels] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};

// Protokollkonstante
#define PROT MM_ACC

IPAddress IP;
// Forward declaration
void sendCanFrame();
void process_sensor_event(uint8_t channel);

unsigned long time2Poll;

#include "espnow.h"

Ticker tckr0; // each msec
const float tckr0Time = 0.0075 / maxCntChannels;

// Routine meldet an die CANguru-Bridge, wenn eine Statusänderung
// eines Blockabschnittes bemerkt wurde
void process_sensor_event(uint8_t channel)
{
  memset(opFrame, 0, sizeof(opFrame));
  opFrame[CANcmd] = S88_EVENT;
  // Adresse (maximal 255), d.h. 10 Module

  //  opFrame[data3] = channel + (decoderadr - 1) * maxCntChannels + 1;
  opFrame[data3] = channel_index[channel];
  // Zustand
  // alt und neu
  if (channels[channel] == isOccupied)
  {
    opFrame[data4] = isFree;
    opFrame[data5] = isOccupied;
    log_i("occupied: %d", opFrame[data3]);
  }
  else
  {
    opFrame[data4] = isOccupied;
    opFrame[data5] = isFree;
  }
  opFrame[Framelng] = 8;
  sendCanFrame();
}

// Auswertung der Melderinformationen
int readPin(uint8_t inputPin)
{
  for (uint8_t bits = 0; bits < 4; bits++)
    digitalWrite(pins[bits + 1], bitRead(inputPin, bits));
  delayMicroseconds(1);
  return digitalRead(zPin);
}

// Timer steuert die Abfrage der Melder
void timer1ms()
{
  static uint8_t currChannel = 0;
  if (readPin(currChannel))
  {
    inputValue[currChannel]++;
  }
  msecs[currChannel]++;
  if (msecs[currChannel] > 50)
  {
    if ((inputValue[currChannel] < 45) && (inputValue[currChannel] > 0))
    {
      if (channels[currChannel] != isOccupied)
      {
        // speichern
        channels[currChannel] = isOccupied;
        process_sensor_event(currChannel);
      }
    }
    else
    {
      if (channels[currChannel] != isFree)
      {
        // speichern
        channels[currChannel] = isFree;
        process_sensor_event(currChannel);
      }
    }
    msecs[currChannel] = 0;
    inputValue[currChannel] = 0;
  }
  currChannel++;
  if (currChannel >= maxCntChannels)
  {
    currChannel = 0;
  }
}

void setup()
{
  Serial.begin(bdrMonitor);
  delay(500);
  log_i("\r\n\r\nC A N g u r u - B e s e t z t m e l d e r");
  log_i("\n on %s", ARDUINO_BOARD);
  log_i("CPU Frequency = %d Mhz", F_CPU / 1000000);
//  log_e("ERROR!");
//  log_d("VERBOSE");
//  log_w("WARNING");
//  log_i("INFO");
  // der Decoder strahlt mit seiner Kennung
  // damit kennt die CANguru-Bridge (der Master) seine Decoder findet
  DEVTYPE = DEVTYPE_RM;
  startAPMode();
  // der Master wird registriert
  addMaster();
  WiFi.disconnect();
  if (!EEPROM.begin(EEPROM_SIZE))
  {
    log_e("Failed to initialise EEPROM");
  }
  uint8_t setup_todo = EEPROM.read(adr_setup_done);
  if (setup_todo != setup_done)
  {
    // alles fürs erste Mal
    //
    // wurde das Setup bereits einmal durchgeführt?
    // dann wird dieser Anteil übersprungen
    // 47, weil das EEPROM (hoffentlich) nie ursprünglich diesen Inhalt hatte

    for (uint8_t ch = 0; ch < maxCntChannels; ch++)
    {
      EEPROM.write(adr_index + ch, channel_index[ch]);
      EEPROM.commit();
    }

    // ota auf "FALSE" setzen
    EEPROM.write(adr_ota, startWithoutOTA);
    EEPROM.commit();
    // setup_done auf "TRUE" setzen
    EEPROM.write(adr_setup_done, setup_done);
    EEPROM.commit();
  }
  else
  {
    uint8_t ota = EEPROM.readByte(adr_ota);
    if (ota == startWithoutOTA)
    {
      // nach dem ersten Mal Einlesen der gespeicherten Werte
      for (uint8_t ch = 0; ch < maxCntChannels; ch++)
      {
        channel_index[ch] = readValfromEEPROM(adr_index + ch, minadr, minadr, maxadr);
      }
    }
    else
    {
      // ota auf "FALSE" setzen
      EEPROM.write(adr_ota, startWithoutOTA);
      EEPROM.commit();
      Connect2WiFiandOTA();
    }
  }
  // ab hier werden die Anweisungen bei jedem Start durchlaufen
  // IP-Adresse

  for (uint8_t ip = 0; ip < 4; ip++)
  {
    IP[ip] = EEPROM.read(adr_IP0 + ip);
  }
  // Flags
  got1CANmsg = false;
  SYS_CMD_Request = false;
  START_OTA_Request = false;
  SEND_IP_Request = false;
  statusPING = false;
  time2Poll = 0;
  memset(msecs, 0, sizeof(msecs));
  memset(inputValue, 0, sizeof(inputValue));
  memset(channels, isFree, sizeof(channels));
  // initialize digital pins as an input / output.
  pinMode(zPin, INPUT);
  for (uint8_t p = 0; p < sizeof(pins); p++)
    pinMode(pins[p], OUTPUT);
  digitalWrite(enablePin, LOW);
  // Vorbereiten der Blink-LED
  stillAliveBlinkSetup();
}

// receiveKanalData dient der Parameterübertragung zwischen Decoder und CANguru-Server
// es erhält die evtuelle auf dem Server geänderten Werte zurück
void receiveKanalData()
{
  SYS_CMD_Request = false;

  uint8_t oldval = channel_index[opFrame[data5] - 1];
  channel_index[opFrame[data5] - 1] = (opFrame[data6] << 8) + opFrame[data7];
  if (testMinMax(oldval, channel_index[opFrame[data5] - 1], minadr, maxadr))
  {
    // speichert die neue Adresse
    EEPROM.write(adr_index + opFrame[data5] - 1, channel_index[opFrame[data5] - 1]);
    EEPROM.commit();
  }
  else
  {
    channel_index[opFrame[data5] - 1] = oldval;
  }
  // antworten
  opFrame[data6] = 0x01;
  opFrame[Framelng] = 0x07;
  sendCanFrame();
}

// auf Anforderung des CANguru-Servers sendet der Decoder
// mit dieser Prozedur sendConfig seine Parameterwerte
void sendConfig()
{
  const uint8_t Kanalwidth = 8;
  const uint8_t numberofKanals = endofKanals - 1;
  uint8_t NumKanalLines;
  // Kanal 0
  const uint8_t NumKanalLines00 = 5 * Kanalwidth;
  uint8_t arrKanal00[NumKanalLines00] = {0};
  // Kanal 01 .. 16
  const uint8_t NumKanalLinesCH = 3 * Kanalwidth;
  uint8_t arrKanalCH[NumKanalLinesCH] = {0};
  if (CONFIGURATION_Status_Index == Kanal00)
  {
    NumKanalLines = NumKanalLines00;
    uint arrKanal0[NumKanalLines] = {
        /*1*/ Kanal00, numberofKanals, (uint8_t)0, (uint8_t)0, (uint8_t)0, (uint8_t)0, (uint8_t)0, decoderadr,
        /*2.1*/ (uint8_t)highbyte2char(hex2dec(uid_device[0])), (uint8_t)lowbyte2char(hex2dec(uid_device[0])),
        /*2.2*/ (uint8_t)highbyte2char(hex2dec(uid_device[1])), (uint8_t)lowbyte2char(hex2dec(uid_device[1])),
        /*2.3*/ (uint8_t)highbyte2char(hex2dec(uid_device[2])), (uint8_t)lowbyte2char(hex2dec(uid_device[2])),
        /*2.4*/ (uint8_t)highbyte2char(hex2dec(uid_device[3])), (uint8_t)lowbyte2char(hex2dec(uid_device[3])),
        /*3*/ 'C', 'A', 'N', 'g', 'u', 'r', 'u', ' ',
        /*4*/ 'B', 'e', 's', 'e', 't', 'z', 't', '-',
        /*5*/ 'M', 'e', 'l', 'd', 'e', 'r', 0, 0};
        for (uint8_t c = 0; c < NumKanalLines; c++)
          arrKanal00[c] = arrKanal0[c];
  }
  else
  {
    NumKanalLines = NumKanalLinesCH;
    char c0 = '0' + (CONFIGURATION_Status_Index % 10);
    char c1;
    if (CONFIGURATION_Status_Index >= 10)
      c1 = '1';
    else
      c1 = '0';
    uint8_t arrKanal1[NumKanalLines] = {
        // #2 - WORD immer Big Endian, wie Uhrzeit
        /*1*/ (uint8_t)(CONFIGURATION_Status_Index), 2, 0, minadr, 0, maxadr, 0, channel_index[CONFIGURATION_Status_Index - 1],
        /*2*/ 'K', 'a', 'n', 'a', 'l', ':', ' ', c1,
        /*3*/ c0, 0, (maxadr / 100) + '0', (maxadr - (uint8_t)(maxadr / 100) * 100) / 10 + '0', (maxadr - (uint8_t)(maxadr / 10) * 10) + '0', 0, 0, 0};
        for (uint8_t c = 0; c < NumKanalLines; c++)
          arrKanalCH[c] = arrKanal1[c];
  }

  uint8_t paket = 0;
  uint8_t outzeichen = 0;
  CONFIG_Status_Request = false;
  for (uint8_t inzeichen = 0; inzeichen < NumKanalLines; inzeichen++)
  {
    opFrame[CANcmd] = CONFIG_Status + 1;
    if (CONFIGURATION_Status_Index == Kanal00)
      opFrame[outzeichen + 5] = arrKanal00[inzeichen];
    else
      opFrame[outzeichen + 5] = arrKanalCH[inzeichen];
    outzeichen++;
    if (outzeichen == 8)
    {
      opFrame[Framelng] = 8;
      outzeichen = 0;
      paket++;
      opFrame[hash0] = 0x00;
      opFrame[hash1] = paket;
      sendTheData();
      delay(wait_time_small);
    }
  }
  //
  memset(opFrame, 0, sizeof(opFrame));
  opFrame[CANcmd] = CONFIG_Status + 1;
  opFrame[hash0] = hasharr[0];
  opFrame[hash1] = hasharr[1];
  opFrame[Framelng] = 0x06;
  for (uint8_t i = 0; i < 4; i++)
  {
    opFrame[i + 5] = uid_device[i];
  }
  opFrame[data4] = CONFIGURATION_Status_Index;
  opFrame[data5] = paket;
  sendTheData();
  delay(wait_time_small);
}

// In dieser Schleife verbringt der Decoder die meiste Zeit
void loop()
{
  // die boolsche Variable got1CANmsg zeigt an, ob vom Master
  // eine Nachricht gekommen ist; der Zustand der Flags
  // entscheidet dann, welche Routine anschließend aufgerufen wird
  if (time2Poll > 0)
  {
    // die Melder werden periodisch nach Statusänderungen abgefragt
    if (millis() > (time2Poll + 500))
    {
      tckr0.attach(tckr0Time, timer1ms); // each msec
      time2Poll = 0;
    }
  }
  if (got1CANmsg)
  {
    got1CANmsg = false;
    // auf PING Antworten
    if (statusPING)
    {
      sendPING();
    }
    // Parameterwerte vom CANguru-Server erhalten
    if (SYS_CMD_Request)
    {
      receiveKanalData();
    }
    // Modul wird neu gestartet und wartet anschließend auf neue Software (OTA)
    if (START_OTA_Request)
    {
      //
      EEPROM.write(adr_ota, startWithOTA);
      EEPROM.commit();
      ESP.restart();
    }
    // die eigene IP-Adresse wird über die Bridge an den Server zurück geliefert
    if (SEND_IP_Request)
    {
      sendIP();
    }
    // Parameterwerte an den CANguru-Server liefern
    if (CONFIG_Status_Request)
    {
      sendConfig();
    }
  }
}
