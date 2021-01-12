
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
#include "PWM.h"

// EEPROM-Adressen
#define setup_done 0x47
// EEPROM-Belegung
// EEPROM-Speicherplätze der Local-IDs
const uint8_t adr_decoderadr = lastAdr0 + 0x01;
const uint16_t adr_SrvDelLED = lastAdr0 + 0x02;
const uint16_t adr_SrvDelForm = lastAdr0 + 0x03;
const uint16_t adr_StartAngle = lastAdr0 + 0x04;
const uint16_t adr_StopAngle = lastAdr0 + 0x05;
const uint16_t adr_EndAngle = lastAdr0 + 0x06;
const uint16_t acc_status = lastAdr0 + 0x07;
// ab dieser Adresse werden zunächst Stellungen der Formsignale und
// dahinter die der LED-Signale gespeichert
const uint16_t lastAdr = acc_status + num_FormSignals + num_LEDSignals + 1;
#define EEPROM_SIZE lastAdr

const uint16_t acc_statusForm = acc_status;
const uint16_t acc_statusLED = acc_statusForm + num_FormSignals;

#define CAN_FRAME_SIZE 13 /* maximum datagram size */

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
  endofKanals
};

Kanals CONFIGURATION_Status_Index = Kanal00;

uint8_t decoderadr;
uint8_t uid_device[uid_num];

// Zeigen an, ob eine entsprechende Anforderung eingegangen ist
bool CONFIG_Status_Request = false;
bool SYS_CMD_Request = false;
bool RESET_MEM_Request = false;
bool START_OTA_Request = false;
bool SEND_IP_Request = false;

// Timer
boolean statusPING;
boolean initialData2send;

#define VERS_HIGH 0x00 // Versionsnummer vor dem Punkt
#define VERS_LOW 0x01  // Versionsnummer nach dem Punkt

/*
Variablen der Signals & Magnetartikel
*/

// 4 Form-Signale
FormSignalClass FormSignal00;
FormSignalClass FormSignal01;
FormSignalClass FormSignal02;
FormSignalClass FormSignal03;
FormSignalClass FormSignal04;
FormSignalClass FormSignal05;
FormSignalClass FormSignal06;
FormSignalClass FormSignal07;
FormSignalClass FormSignal08;
FormSignalClass FormSignal09;
FormSignalClass FormSignal10;
FormSignalClass FormSignal11;
FormSignalClass FormSignal12;
FormSignalClass FormSignal13;
FormSignalClass FormSignal14;
FormSignalClass FormSignal15;
FormSignalClass FormSignals[num_FormSignals] = {FormSignal00, FormSignal01, FormSignal02, FormSignal03,
                                                FormSignal04, FormSignal05, FormSignal06, FormSignal07,
                                                FormSignal08, FormSignal09, FormSignal10, FormSignal11,
                                                FormSignal12, FormSignal13, FormSignal14, FormSignal15};

// 4 LED-Signale
LEDSignalClass LEDSignal00;
LEDSignalClass LEDSignal01;
LEDSignalClass LEDSignal02;
LEDSignalClass LEDSignal03;
LEDSignalClass LEDSignal04;
LEDSignalClass LEDSignal05;
LEDSignalClass LEDSignal06;
LEDSignalClass LEDSignal07;
LEDSignalClass LEDSignals[num_LEDSignals] = {LEDSignal00, LEDSignal01, LEDSignal02, LEDSignal03,
                                             LEDSignal04, LEDSignal05, LEDSignal06, LEDSignal07};

uint8_t LEDsignalDelay;
uint8_t FormsignalDelay;
uint8_t StartAngle;
uint8_t StopAngle;
uint8_t EndAngle;

// Protokollkonstante
#define PROT MM_ACC

IPAddress IP;

// Forward declaration
void switchLED(uint16_t acc_num);
void switchForm(uint16_t acc_num);
void LED_report(uint8_t num);
void Form_report(uint8_t num);
void calc_to_address();
void sendConfig();
void generateHash(uint8_t offset);

#include "espnow.h"

void setup()
{
  Serial.begin(bdrMonitor);
  Serial.println("\r\n\r\nF o r m - / L E D - S i g n a l");
  // der Decoder strahlt mit seiner Kennung
  // damit kennt die CANguru-Bridge (der Master) seine Decoder findet
  startAPMode();
  // der Master (CANguru-Bridge) wird registriert
  addMaster();
  // WLAN -Verbindungen können wieder ausgeschaltet werden
  WiFi.disconnect();
  // die EEPROM-Library wird gestartet
  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("Failed to initialise EEPROM");
  }
  uint8_t setup_todo;
  uint8_t reset_todo = EEPROM.read(adr_reset);
  if (reset_todo == startWithRESET)
    setup_todo = 0xFF;
  else
    setup_todo = EEPROM.read(adr_setup_done);
  if (setup_todo != setup_done)
  {
    // wurde das Setup bereits einmal durchgeführt?
    // dann wird dieser Anteil übersprungen
    // 47, weil das EEPROM (hoffentlich) nie ursprünglich diesen Inhalt hatte

    // setzt die Boardnum anfangs auf 1
    decoderadr = 1;
    EEPROM.write(adr_decoderadr, decoderadr);
    EEPROM.commit();
    // Verzögerung LED-Umschaltung
    EEPROM.write(adr_SrvDelLED, stdLEDsignaldelay);
    EEPROM.commit();
    // Verzögerung Form-Signal-Umschaltung
    EEPROM.write(adr_SrvDelForm, stdFormsignaldelay);
    EEPROM.commit();
    // Startwinkel Formsignal
    EEPROM.write(adr_StartAngle, stdStartAngle);
    EEPROM.commit();
    // Startwinkel Formsignal
    EEPROM.write(adr_StopAngle, stdStopAngle);
    EEPROM.commit();
    // Überschwingwinkel Formsignal
    EEPROM.write(adr_EndAngle, stdendAngle);
    EEPROM.commit();
    // Status der Formsignale zu Beginn auf rechts setzen
    for (uint8_t form = 0; form < num_FormSignals; form++)
    {
      EEPROM.write(acc_statusForm + form, red);
      EEPROM.commit();
    }
    // Status der LEDignale zu Beginn auf rechts setzen
    for (uint8_t signal = 0; signal < num_LEDSignals; signal++)
    {
      EEPROM.write(acc_statusLED + signal, red);
      EEPROM.commit();
    }
    // ota auf "FALSE" setzen
    EEPROM.write(adr_reset, startWithoutRESET);
    EEPROM.commit();
    // ota auf "FALSE" setzen
    EEPROM.write(adr_ota, startWithoutOTA);
    EEPROM.commit();
    // setup_done auf "TRUE" setzen
    EEPROM.write(adr_setup_done, setup_done);
    EEPROM.commit();
  }
  else
  {
    // Basisadresse des Decoders
    // darüber sind die Ampaln / Andreaskreuze erreichbar
    uint8_t ota = EEPROM.readByte(adr_ota);
    if (ota == startWithoutOTA)
    {
      // nach dem ersten Mal Einlesen der gespeicherten Werte
      // Adresse
      decoderadr = readValfromEEPROM(adr_decoderadr, minadr, minadr, maxadr);
    }
    else
    {
      // ota auf "FALSE" setzen
      EEPROM.write(adr_ota, startWithoutOTA);
      EEPROM.commit();
      Connect2WiFiandOTA();
    }
  }
  // IP-Adresse
  for (uint8_t ip = 0; ip < 4; ip++)
  {
    IP[ip] = EEPROM.read(adr_IP0 + ip);
  }
  // ab hier werden die Anweisungen bei jedem Start durchlaufen
  // Flags
  got1CANmsg = false;
  SYS_CMD_Request = false;
  statusPING = false;
  RESET_MEM_Request = false;
  START_OTA_Request = false;
  SEND_IP_Request = false;
  /////////////
  initPWM_Form();
  FormsignalDelay = readValfromEEPROM(adr_SrvDelForm, stdFormsignaldelay, minFormsignaldelay, maxFormsignaldelay);
  StartAngle = readValfromEEPROM(adr_StartAngle, stdStartAngle, stdStartAngle, stdStopAngle);
  StopAngle = readValfromEEPROM(adr_StopAngle, stdStopAngle, stdStartAngle, stdStopAngle);
  EndAngle = readValfromEEPROM(adr_EndAngle, stdendAngle, minendAngle, maxendAngle);
  for (uint8_t form = 0; form < num_FormSignals; form++)
  {
    // Status der Magnetartikel versenden an die Servos
    FormSignals[form].Attach(form);
    FormSignals[form].SetStartAngle(StartAngle);
    FormSignals[form].SetStopAngle(StopAngle);
    FormSignals[form].SetEndAngle(EndAngle);
    colorLED status = (colorLED)EEPROM.read(acc_statusForm + form);
    FormSignals[form].SetLightDest(status);
    // Signals mit den PINs verbinden, initialisieren & Artikel setzen wie gespeichert
    FormSignals[form].SetDelay(FormsignalDelay);
    FormSignals[form].SetcolorLED();
  }
  /////////////
  initPWM_LED();
  LEDsignalDelay = readValfromEEPROM(adr_SrvDelLED, stdLEDsignaldelay, minLEDsignaldelay, maxLEDsignaldelay);
  for (uint8_t signal = 0; signal < num_LEDSignals; signal++)
  {
    // Status der Magnetartikel versenden an die Servos
    LEDSignals[signal].Attach(signal);
    // Status der Magnetartikel einlesen in lokale arrays
    LEDSignals[signal].SetLightDest((colorLED)EEPROM.read(acc_statusLED + signal));
    // Signals mit den PINs verbinden, initialisieren & Artikel setzen wie gespeichert
    LEDSignals[signal].SetDelay(LEDsignalDelay);
    LEDSignals[signal].SetcolorLED();
  }
  // berechnet die _to_address aus der Adresse und der Protokollkonstante
  calc_to_address();
  // Vorbereiten der Blink-LED
  stillAliveBlinkSetup();
}

// Zu Beginn des Programmablaufs werden die aktuellen Statusmeldungen an WDP geschickt
void sendTheInitialData()
{
  initialData2send = false;
  for (uint8_t signal = 0; signal < num_LEDSignals; signal++)
  {
    // Status der LED-Signale melden
    LED_report(signal);
  }
  for (uint8_t form = 0; form < num_FormSignals; form++)
  {
    // Status der Form-Signale melden
    Form_report(form);
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
  // to Server
  for (uint8_t i = CAN_FRAME_SIZE - 1; i < 8 - opFrame[Framelng]; i--)
    opFrame[i] = 0x00;
  opFrame[CANcmd]++;
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

// Die Adressen der einzelnen Servos werden berechnet,
// dabei wird die Decoderadresse einbezogen, so dass
// das erste Servo die Adresse
// Protokollkonstante (0x3000) + (Decoderadresse-1) * Anzahl Servos pro Decoder
// erhält. Das zweite Servo hat dann die Adresse Servo1 plus 1; die
// folgende analog
// das Kürzel to an verschiedenen Stellen steht für den englischen Begriff
// für Weiche, nämlich turnout
void calc_to_address()
{
  uint16_t baseAddress = (decoderadr - 1) * num_LEDSignals;
  // berechnet die _to_address aus der Adresse und der Protokollkonstante
  // Formsignale
  for (uint8_t form = 0; form < num_FormSignals; form++)
  {
    uint16_t to_address = PROT + baseAddress + form;
    // _to_addresss einlesen in lokales array
    FormSignals[form].Set_to_address(to_address);
    Form_report(form);
  }
  baseAddress += num_FormSignals;
  // LED-Signale
  for (uint8_t signal = 0; signal < num_LEDSignals; signal++)
  {
    uint16_t to_address = PROT + baseAddress + signal;
    // _to_addresss einlesen in lokales array
    LEDSignals[signal].Set_to_address(to_address);
    LED_report(signal);
  }
}

// receiveKanalData dient der Parameterübertragung zwischen Decoder und CANguru-Server
// es erhält die evtuelle auf dem Server geänderten Werte zurück
void receiveKanalData()
{
  SYS_CMD_Request = false;
  // Wenn Reset im CANguru-Server gedrückt wurde, dürfen nicht
  // die alten Werte, die noch angezeigt werden, wieder gespeichert werden
  if (EEPROM.read(adr_reset) == startWithRESET)
  {
    if (opFrame[data5] == (endofKanals - 1))
    {
      EEPROM.write(adr_reset, startWithoutRESET);
      EEPROM.commit();
    }
    return;
  }
  uint8_t oldval;
  switch (opFrame[data5])
  {
  // Kanalnummer #1 - Decoderadresse
  case Kanal01:
  {
    oldval = decoderadr;
    decoderadr = (opFrame[data6] << 8) + opFrame[data7];
    if (testMinMax(oldval, decoderadr, minadr, maxadr))
    {
      // speichert die neue Adresse
      EEPROM.write(adr_decoderadr, decoderadr);
      EEPROM.commit();
      // neue Adressen
      calc_to_address();
    }
    else
    {
      decoderadr = oldval;
    }
  }
  break;
  // Kanalnummer #2 - Signalverzögerung LED-Umschaltung
  case Kanal02:
  {
    oldval = LEDsignalDelay;
    LEDsignalDelay = (opFrame[data6] << 8) + opFrame[data7];
    if (testMinMax(oldval, LEDsignalDelay, minLEDsignaldelay, maxLEDsignaldelay))
    {
      EEPROM.write(adr_SrvDelLED, LEDsignalDelay);
      EEPROM.commit();
      for (int signal = 0; signal < num_LEDSignals; signal++)
      {
        // neue Verzögerung
        LEDSignals[signal].SetDelay(LEDsignalDelay);
      }
    }
    else
    {
      LEDsignalDelay = oldval;
    }
  }
  break;
  // Kanalnummer #3 - Signalverzögerung Form-Signal-Umschaltung
  case Kanal03:
  {
    oldval = FormsignalDelay;
    FormsignalDelay = (opFrame[data6] << 8) + opFrame[data7];
    if (testMinMax(oldval, FormsignalDelay, minFormsignaldelay, maxFormsignaldelay))
    {
      EEPROM.write(adr_SrvDelForm, FormsignalDelay);
      EEPROM.commit();
      for (int form = 0; form < num_FormSignals; form++)
      {
        // neue Verzögerung
        FormSignals[form].SetDelay(FormsignalDelay);
      }
    }
    else
    {
      FormsignalDelay = oldval;
    }
  }
  break;
  // Kanalnummer #4 - Startwinkel Form-Signal
  case Kanal04:
  {
    oldval = StartAngle;
    StartAngle = (opFrame[data6] << 8) + opFrame[data7];
    if (testMinMax(oldval, StartAngle, stdStartAngle, stdStopAngle))
    {
      EEPROM.write(adr_StartAngle, StartAngle);
      EEPROM.commit();
      for (int form = 0; form < num_FormSignals; form++)
      {
        // neue Verzögerung
        FormSignals[form].SetStartAngle(StartAngle);
      }
    }
    else
    {
      StartAngle = oldval;
    }
  }
  break;
  // Kanalnummer #5 - Stoppwinkel Form-Signal
  case Kanal05:
  {
    oldval = StopAngle;
    StopAngle = (opFrame[data6] << 8) + opFrame[data7];
    if (testMinMax(oldval, StopAngle, stdStartAngle, stdStopAngle))
    {
      EEPROM.write(adr_StopAngle, StopAngle);
      EEPROM.commit();
      for (int form = 0; form < num_FormSignals; form++)
      {
        // neue Verzögerung
        FormSignals[form].SetStopAngle(StopAngle);
      }
    }
    else
    {
      StopAngle = oldval;
    }
  }
  break;
  // Kanalnummer #6 - Überschwingwinkel Form-Signal
  case Kanal06:
  {
    oldval = EndAngle;
    EndAngle = (opFrame[data6] << 8) + opFrame[data7];
    if (testMinMax(oldval, EndAngle, minendAngle, maxendAngle))
    {
      EEPROM.write(adr_EndAngle, EndAngle);
      EEPROM.commit();
      for (int form = 0; form < num_FormSignals; form++)
      {
        // neuer Überschwingwinkel
        FormSignals[form].SetEndAngle(EndAngle);
      }
    }
    else
    {
      EndAngle = oldval;
    }
  }
  break;
  }
  //
  opFrame[11] = 0x01;
  opFrame[4] = 0x07;
  sendCanFrame();
}

// sendPING ist die Antwort der Decoder auf eine PING-Anfrage
void sendPING()
{
  statusPING = false;
  opFrame[1] = PING;
  opFrame[4] = 0x08;
  for (uint8_t i = 0; i < uid_num; i++)
  {
    opFrame[i + 5] = uid_device[i];
  }
  opFrame[9] = VERS_HIGH;
  opFrame[10] = VERS_LOW;
  opFrame[11] = DEVTYPE_LEDSIGNAL >> 8;
  opFrame[12] = DEVTYPE_LEDSIGNAL;
  sendCanFrame();
}

// sendIP ist die Antwort der Decoder auf eine Abfrage der IP-Adresse
void sendIP()
{
  SEND_IP_Request = false;
  opFrame[1] = SEND_IP;
  opFrame[4] = 0x08;
  // IP-Adresse eintragen
  for (uint8_t ip = 0; ip < 4; ip++)
    opFrame[5 + ip] = IP[ip];
  sendCanFrame();
}

// Routine meldet an die CANguru-Bridge, wenn eine Statusänderung
// einer Ampel eingetreten ist
void Form_report(uint8_t num)
{
  opFrame[CANcmd] = SWITCH_ACC;
  opFrame[Framelng] = 0x05;
  // Weichenadresse
  opFrame[data0] = 0x00;
  opFrame[data1] = 0x00;
  opFrame[data2] = (uint8_t)(FormSignals[num].Get_to_address() >> 8);
  opFrame[data3] = (uint8_t)FormSignals[num].Get_to_address();
  // Meldung der Lage
  opFrame[data4] = FormSignals[num].GetLightCurr();
  sendCanFrame();
  delay(wait_time); // Delay added just so we can have time to open up
}

// Routine meldet an die CANguru-Bridge, wenn eine Statusänderung
// einer Ampel eingetreten ist
void LED_report(uint8_t num)
{
  opFrame[CANcmd] = SWITCH_ACC;
  opFrame[Framelng] = 0x05;
  // Weichenadresse
  opFrame[data0] = 0x00;
  opFrame[data1] = 0x00;
  opFrame[data2] = (uint8_t)(LEDSignals[num].Get_to_address() >> 8);
  opFrame[data3] = (uint8_t)LEDSignals[num].Get_to_address();
  // Meldung der Lage
  opFrame[data4] = LEDSignals[num].GetLightCurr();
  sendCanFrame();
  delay(wait_time); // Delay added just so we can have time to open up
}

void switchForm(uint16_t acc_num)
{
  colorLED set_light = FormSignals[acc_num].GetLightDest();
  FormSignals[acc_num].SetcolorLED();
  Form_report(acc_num);
  EEPROM.write(acc_statusForm + acc_num, (uint8_t)set_light);
  EEPROM.commit();
}

// Diese Routine leitet den Positionswechsel einer Weiche/Signal ein.
void switchLED(uint16_t acc_num)
{
  colorLED set_light = LEDSignals[acc_num].GetLightDest();
  LEDSignals[acc_num].SetLightCurr(set_light);
  LEDSignals[acc_num].SetcolorLED();
  LED_report(acc_num);
  EEPROM.write(acc_statusLED + acc_num, (uint8_t)set_light);
  EEPROM.commit();
}

// auf Anforderung des CANguru-Servers sendet der Decoder
// mit dieser Prozedur sendConfig seine Parameterwerte
void sendConfig()
{
  const uint8_t Kanalwidth = 8;
  const uint8_t numberofKanals = endofKanals - 1;

  const uint8_t NumLinesKanal00 = 5 * Kanalwidth;
  uint8_t arrKanal00[NumLinesKanal00] = {
      /*1*/ Kanal00, numberofKanals, (uint8_t)0, (uint8_t)0, (uint8_t)0, (uint8_t)0, (uint8_t)0, decoderadr,
      /*2.1*/ (uint8_t)highbyte2char(hex2dec(uid_device[0])), (uint8_t)lowbyte2char(hex2dec(uid_device[0])),
      /*2.2*/ (uint8_t)highbyte2char(hex2dec(uid_device[1])), (uint8_t)lowbyte2char(hex2dec(uid_device[1])),
      /*2.3*/ (uint8_t)highbyte2char(hex2dec(uid_device[2])), (uint8_t)lowbyte2char(hex2dec(uid_device[2])),
      /*2.4*/ (uint8_t)highbyte2char(hex2dec(uid_device[3])), (uint8_t)lowbyte2char(hex2dec(uid_device[3])),
      /*3*/ 'C', 'A', 'N', 'g', 'u', 'r', 'u', ' ',
      /*4*/ 'M', 'a', 'x', 'i', '-', 'S', 'i', 'g',
      /*5*/ 'n', 'a', 'l', 0, 0, 0, 0, 0};
  const uint8_t NumLinesKanal01 = 4 * Kanalwidth;
  uint8_t arrKanal01[NumLinesKanal01] = {
      // #2 - WORD immer Big Endian, wie Uhrzeit
      /*1*/ Kanal01, 2, 0, 1, 0, maxadr, 0, decoderadr,
      /*2*/ 'M', 'o', 'd', 'u', 'l', 'a', 'd', 'r',
      /*3*/ 'e', 's', 's', 'e', 0, '1', 0, (maxadr / 100) + '0',
      /*4*/ (maxadr - (uint8_t)(maxadr / 100) * 100) / 10 + '0', (maxadr - (uint8_t)(maxadr / 10) * 10) + '0', 0, 'A', 'd', 'r', 0, 0};
  const uint8_t NumLinesKanal02 = 4 * Kanalwidth;
  uint8_t arrKanal02[NumLinesKanal02] = {
      /*1*/ Kanal02, 2, 0, minLEDsignaldelay, 0, maxLEDsignaldelay, 0, LEDsignalDelay,
      /*2*/ 'D', 'e', 'l', 'a', 'y', ' ', 'L', 'e',
      /*3*/ 'd', 0, minLEDsignaldelay + '0', 0, (maxLEDsignaldelay / 100) + '0', (maxLEDsignaldelay - (uint8_t)(maxLEDsignaldelay / 100) * 100) / 10 + '0', (maxLEDsignaldelay - (uint8_t)(maxLEDsignaldelay / 10) * 10) + '0', 0,
      /*4*/ 'm', 's', 0, 0, 0, 0, 0, 0};
  const uint8_t NumLinesKanal03 = 4 * Kanalwidth;
  uint8_t arrKanal03[NumLinesKanal03] = {
      /*1*/ Kanal03, 2, 0, minFormsignaldelay, 0, maxFormsignaldelay, 0, FormsignalDelay,
      /*2*/ 'D', 'e', 'l', 'a', 'y', ' ', 'F', 'o',
      /*3*/ 'r', 'm', 0, minFormsignaldelay + '0', 0, (maxFormsignaldelay / 100) + '0', (maxFormsignaldelay - (uint8_t)(maxFormsignaldelay / 100) * 100) / 10 + '0', (maxFormsignaldelay - (uint8_t)(maxFormsignaldelay / 10) * 10) + '0',
      /*4*/ 0, 'm', 's', 0, 0, 0, 0, 0};
  const uint8_t NumLinesKanal04 = 4 * Kanalwidth;
  uint8_t arrKanal04[NumLinesKanal04] = {
      /*1*/ Kanal04, 2, 0, stdStartAngle, 0, stdStopAngle, 0, StartAngle,
      /*2*/ 'S', 't', 'a', 'r', 't', 'w', 'i', 'n',
      /*3*/ 'k', 'e', 'l', 0, stdStartAngle + '0', 0, (stdStopAngle / 100) + '0', (stdStopAngle - (uint8_t)(stdStopAngle / 100) * 100) / 10 + '0',
      /*4*/ (stdStopAngle - (uint8_t)(stdStopAngle / 10) * 10) + '0', 0, 'g', 'r', 'a', 'd', 0, 0};
  const uint8_t NumLinesKanal05 = 4 * Kanalwidth;
  uint8_t arrKanal05[NumLinesKanal05] = {
      /*1*/ Kanal05, 2, 0, stdStartAngle, 0, stdStopAngle, 0, StopAngle,
      /*2*/ 'S', 't', 'o', 'p', 'p', 'w', 'i', 'n',
      /*3*/ 'k', 'e', 'l', 0, stdStartAngle + '0', 0, (stdStopAngle / 100) + '0', (stdStopAngle - (uint8_t)(stdStopAngle / 100) * 100) / 10 + '0',
      /*4*/ (stdStopAngle - (uint8_t)(stdStopAngle / 10) * 10) + '0', 0, 'g', 'r', 'a', 'd', 0, 0};
  const uint8_t NumLinesKanal06 = 5 * Kanalwidth;
  uint8_t arrKanal06[NumLinesKanal06] = {
      /*1*/ Kanal06, 2, 0, minendAngle, 0, maxendAngle, 0, readValfromEEPROM(adr_EndAngle, stdendAngle, minendAngle, maxendAngle),
      /*2*/ 0xDC, 'b', 'e', 'r', 's', 'c', 'h', 'w',
      /*3*/ 'i', 'n', 'g', 'w', 'i', 'n', 'k', 'e',
      /*4*/ 'l', 0, minendAngle + '0', 0, maxendAngle / 10 + '0', (maxendAngle - (uint8_t)(maxendAngle / 10) * 10) + '0', 0, 'g',
      /*5*/ 'r', 'a', 'd', 0, 0, 0, 0, 0};
  uint8_t NumKanalLines[numberofKanals + 1] = {
      NumLinesKanal00, NumLinesKanal01, NumLinesKanal02, NumLinesKanal03, NumLinesKanal04, NumLinesKanal05, NumLinesKanal06};

  uint8_t paket = 0;
  uint8_t outzeichen = 0;
  CONFIG_Status_Request = false;
  for (uint8_t inzeichen = 0; inzeichen < NumKanalLines[CONFIGURATION_Status_Index]; inzeichen++)
  {
    opFrame[1] = CONFIG_Status + 1;
    switch (CONFIGURATION_Status_Index)
    {
    case Kanal00:
    {
      opFrame[outzeichen + 5] = arrKanal00[inzeichen];
    }
    break;
    case Kanal01:
    {
      opFrame[outzeichen + 5] = arrKanal01[inzeichen];
    }
    break;
    case Kanal02:
    {
      opFrame[outzeichen + 5] = arrKanal02[inzeichen];
    }
    break;
    case Kanal03:
    {
      opFrame[outzeichen + 5] = arrKanal03[inzeichen];
    }
    break;
    case Kanal04:
    {
      opFrame[outzeichen + 5] = arrKanal04[inzeichen];
    }
    break;
    case Kanal05:
    {
      opFrame[outzeichen + 5] = arrKanal05[inzeichen];
    }
    break;
    case Kanal06:
    {
      opFrame[outzeichen + 5] = arrKanal06[inzeichen];
    }
    break;
    case endofKanals:
    {
      // der Vollständigkeit geschuldet
    }
    break;
    }
    outzeichen++;
    if (outzeichen == 8)
    {
      opFrame[4] = 8;
      outzeichen = 0;
      paket++;
      opFrame[2] = 0x00;
      opFrame[3] = paket;
      sendTheData();
      delay(wait_time_small);
    }
  }
  //
  memset(opFrame, 0, sizeof(opFrame));
  opFrame[1] = CONFIG_Status + 1;
  opFrame[2] = hasharr[0];
  opFrame[3] = hasharr[1];
  opFrame[4] = 0x06;
  for (uint8_t i = 0; i < 4; i++)
  {
    opFrame[i + 5] = uid_device[i];
  }
  opFrame[9] = CONFIGURATION_Status_Index;
  opFrame[10] = paket;
  sendTheData();
  delay(wait_time_small);
}

// In dieser Schleife verbringt der Decoder die meiste Zeit
void loop()
{
  static uint8_t cross = 0;
  static uint8_t signal = 0;
  // die Formsignale werden permant abgefragt, ob es ein Delta zwischen
  // tatsächlicher und gewünschter Stellung gibt und ggf. korrigiert
  FormSignals[cross].Update();
  cross++;
  if (cross >= num_FormSignals)
    cross = 0;
  // die Ampeln werden permant abgefragt, ob es ein Delta zwischen
  // tatsächlichem und gewünschtem Signallicht gibt und ggf. korrigiert
  LEDSignals[signal].Update();
  signal++;
  if (signal >= num_LEDSignals)
    signal = 0;
  // die boolsche Variable got1CANmsg zeigt an, ob vom Master
  // eine Nachricht gekommen ist; der Zustand der Flags
  // entscheidet dann, welche Routine anschließend aufgerufen wird
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
    // Modul wird neu gestartet und setzt alle Werte auf Anfang
    if (RESET_MEM_Request)
    {
      //
      // adr_reset auf "FALSE" setzen
      EEPROM.write(adr_reset, startWithRESET);
      EEPROM.commit();
      ESP.restart();
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
    // beim ersten Durchlauf werden Initialdaten an die
    // CANguru-Bridge gesendet
    if (initialData2send)
    {
      sendTheInitialData();
    }
  }
}
