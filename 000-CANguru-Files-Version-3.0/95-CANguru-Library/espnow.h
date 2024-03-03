
/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <CANguru-Buch@web.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gustav Wostrack
 * ----------------------------------------------------------------------------
 */

#ifndef ESP_NOW_H
#define ESP_NOW_H

#include <Arduino.h>
#include <OWN_LED.h>

const uint8_t WIFI_CHANNEL = 0;
const uint8_t macLen = 6;
uint8_t DEVTYPE;
/* Im using the MAC addresses of my specific ESP32 modules, you will need to
   modify the code for your own:
   The SLAVE Node needs to be initialized as a WIFI_AP
   you will need to get its MAC via WiFi.softAPmacAddress()
*/

enum statustype
{
  even = 0,
  odd
};
statustype currStatus;

const uint8_t masterCustomMac[] = {0x30, 0xAE, 0xA4, 0x89, 0x92, 0x71};

esp_now_peer_info_t master;
const esp_now_peer_info_t *masterNode = &master;
uint8_t opFrame[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
bool got1CANmsg = false;
byte cnt = 0;
String ssid0 = "CNgrSLV";
uint8_t hasharr[] = {0x00, 0x00};

Ticker tckr1;
const float tckr1Time = 0.025;

uint16_t secs;

//*********************************************************************************************************
// das Aufblitzen der LED auf dem ESP32-Modul wird mit diesem Timer
// nach Anstoß durch die CANguru-Bridge (BlinkAlive) umgesetzt
void timer1s()
{
  if (secs > 0)
  {
    // jede 0.025te Sekunde Licht an oder aus
    if (secs % 2 == 0)
    {
      // bei gerader Zahl aus
      LED_off();
#ifdef ESP32_STEPPER
      if (!bBlinkAlive)
        secs = 10;
#endif
    }
    else
    {
      // bei gerader Zahl an
      LED_on();
    }
    secs--;
  }
  else
  {
    LED_off();
  }
}

// meldet den Timer an
void stillAliveBlinkSetup(uint8_t LED)
{
  tckr1.attach(tckr1Time, timer1s); // each sec
  // initialize LED digital pin as an output.
  LED_begin(LED);
  secs = 0;
}

/*
Kollisionsfreiheit zum CS1 Protokoll:
Im CAN Protokoll der CS1 wird der Wert 6 für den "com-Bereich der ID",
dies sind die Bits 7..9, d.h. Highest Bit im Lowest-Byte (0b0xxxxxxx)
und die beiden Bits darüber (0bxxxxxx11), nicht benutzt. Diese
Bitkombination wird daher zur Unterscheidung fest im Hash verwendet.
Kollisionsauflösung:
Der Hash dient dazu, die CAN Meldungen mit hoher Wahrscheinlichkeit kollisionsfrei zu gestalten.
Dieser 16 Bit Wert wird gebildet aus der UID Hash.
Berechnung: 16 Bit High UID XOR 16 Bit Low der UID. Danach
werden die Bits entsprechend zur CS1 Unterscheidung gesetzt.
*/
// generiert den Hash
void generateHash(uint8_t offset)
{
  uint32_t uid = UID_BASE + offset;
  uid_device[0] = (uint8_t)(uid >> 24);
  uid_device[1] = (uint8_t)(uid >> 16);
  uid_device[2] = (uint8_t)(uid >> 8);
  uid_device[3] = (uint8_t)uid;

  uint16_t highbyte = uid >> 16;
  uint16_t lowbyte = uid;
  uint16_t hash = highbyte ^ lowbyte;
  bitWrite(hash, 7, 0);
  bitWrite(hash, 8, 1);
  bitWrite(hash, 9, 1);
  hasharr[0] = hash >> 8;
  hasharr[1] = hash;
}

// startet WLAN im AP-Mode, damit meldet sich der Decoder beim Master
void startAPMode()
{
  log_i("");
  log_i("WIFI Connect AP Mode");
  log_i("--------------------");
  WiFi.persistent(false); // Turn off persistent to fix flash crashing issue.
  WiFi.mode(WIFI_OFF);    // https://github.com/esp8266/Arduino/issues/3100
  WiFi.mode(WIFI_AP);

  // Connect to Wi-Fi
  String ssid1 = WiFi.softAPmacAddress();
  ssid0 = ssid0 + ssid1;
  char ssid[30];
  ssid0.toCharArray(ssid, 30);
  if (!WiFi.softAP(ssid)) // Name des Access Points
    log_i("WIFI %s failed", ssid);
  else
    log_i("WIFI %s OK", ssid);
}

// Fehlermeldungen, die hoffentlich nicht gebraucht werden
void printESPNowError(esp_err_t Result)
{
  if (Result == ESP_ERR_ESPNOW_NOT_INIT)
  {
    // How did we get so far!!
    log_e("ESPNOW not Init.");
  }
  else if (Result == ESP_ERR_ESPNOW_ARG)
  {
    log_e("Invalid Argument");
  }
  else if (Result == ESP_ERR_ESPNOW_INTERNAL)
  {
    log_e("Internal Error");
  }
  else if (Result == ESP_ERR_ESPNOW_NO_MEM)
  {
    log_e("ESP_ERR_ESPNOW_NO_MEM");
  }
  else if (Result == ESP_ERR_ESPNOW_NOT_FOUND)
  {
    log_e("Peer not found.");
  }
  else if (Result == ESP_ERR_ESPNOW_IF)
  {
    log_e("Interface Error.");
  }
  else
  {
    log_e("\r\nNot sure what happened\t%d", Result);
  }
}

#include "OnDataRecv.h"

// Daten werden über ESPNow versendet
void sendTheData()
{
  esp_now_send(master.peer_addr, opFrame, CAN_FRAME_SIZE);
}

// Auswertung nach dem Sendevorgang
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  if (status == ESP_NOW_SEND_SUCCESS)
    log_i("OK");
  else
    log_e("Fail");
}

// der Master (CANguru-Bridge) wird registriert
void addMaster()
{
  if (esp_now_init() == ESP_OK)
  {
    log_i("ESPNow Init Success!");
  }
  else
  {
    log_e("ESPNow Init Failed....");
  }
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);
  memcpy(&master.peer_addr, &masterCustomMac, macLen);
  master.channel = WIFI_CHANNEL; // pick a channel
  master.encrypt = 0;            // no encryption
  master.ifidx = (wifi_interface_t)ESP_IF_WIFI_AP;
  // Add the remote master node to this slave node
  if (esp_now_add_peer(masterNode) == ESP_OK)
  {
    log_i("Master Added As Peer!");
  }
}

/*
Response
Bestimmt, ob CAN Meldung eine Anforderung oder Antwort oder einer
vorhergehende Anforderung ist. Grundsätzlich wird eine Anforderung
ohne ein gesetztes Response Bit angestoßen. Sobald ein Kommando
ausgeführt wurde, wird es mit gesetztem Response Bit, sowie dem
ursprünglichen Meldungsinhalt oder den angefragten Werten, bestätigt.
Jeder Teilnehmer am Bus, welche die Meldung ausgeführt hat, bestätigt ein
Kommando.
*/
void sendCanFrame()
{
  // opFrame[Framelng] ist die Anzahl der gesetzten Datenbytes
  // alle Bytes jenseits davon werden auf 0x00 gesetzt
  for (uint8_t i = CAN_FRAME_SIZE - 1; i < 8 - opFrame[Framelng]; i--)
    opFrame[i] = 0x00;
  // durch Increment wird das Response-Bit gesetzt
  opFrame[CANcmd]++;
  // opFrame[hash0] und opFrame[hash1] enthalten den Hash, der zu Beginn des Programmes
  // einmal errechnet wird
  opFrame[hash0] = hasharr[0];
  opFrame[hash1] = hasharr[1];
  sendTheData();
}

/*
CAN Grundformat
Das CAN Protokoll schreibt vor, dass Meldungen mit einer 29 Bit Meldungskennung,
4 Bit Meldungslänge sowie bis zu 8 Datenbyte bestehen.
Die Meldungskennung wird aufgeteilt in die Unterbereiche Priorit�t (Prio),
Kommando (Command), Response und Hash.
Die Kommunikation basiert auf folgendem Datenformat:

Meldungskennung
Prio	2+2 Bit Message Prio			28 .. 25
Command	8 Bit	Kommando Kennzeichnung	24 .. 17
Resp.	1 Bit	CMD / Resp.				16
Hash	16 Bit	Kollisionsaufl�sung		15 .. 0
DLC
DLC		4 Bit	Anz. Datenbytes
Byte 0	D-Byte 0	8 Bit Daten
Byte 1	D-Byte 1	8 Bit Daten
Byte 2	D-Byte 2	8 Bit Daten
Byte 3	D-Byte 3	8 Bit Daten
Byte 4	D-Byte 4	8 Bit Daten
Byte 5	D-Byte 5	8 Bit Daten
Byte 6	D-Byte 6	8 Bit Daten
Byte 7	D-Byte 7	8 Bit Daten
*/

// sendPING ist die Antwort der Decoder auf eine PING-Anfrage
void sendPING()
{
  statusPING = false;
  opFrame[CANcmd] = PING;
  opFrame[Framelng] = data3;
  for (uint8_t i = 0; i < uid_num; i++)
  {
    opFrame[i + 5] = uid_device[i];
  }
  opFrame[data4] = VERS_HIGH;
  opFrame[data5] = VERS_LOW;
  opFrame[data6] = DEVTYPE >> 8;
  opFrame[data7] = DEVTYPE;
  sendCanFrame();
  #ifdef TIME2POLL
  if (time2Poll == 0)
    time2Poll = millis();
  #endif
}

void sendDecoderIsAlive()
{
  bDecoderIsAlive = false;
  sendCanFrame();
}

// sendIP ist die Antwort der Decoder auf eine Abfrage der IP-Adresse
void sendIP()
{
  SEND_IP_Request = false;
  opFrame[CANcmd] = SEND_IP;
  opFrame[Framelng] = data3;
  // IP-Adresse eintragen
  for (uint8_t ip = 0; ip < 4; ip++)
    opFrame[5 + ip] = IP[ip];
  sendCanFrame();
}

#endif