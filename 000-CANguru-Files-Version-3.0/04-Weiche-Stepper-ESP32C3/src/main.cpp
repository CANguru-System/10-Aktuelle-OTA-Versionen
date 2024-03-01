
/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <CANguru-Buch@web.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gustav Wostrack
 * ----------------------------------------------------------------------------
 */

#include <WiFi.h>
#include <WiFiUdp.h>
#include "Preferences.h"
#include "Stepper.h"
#include <esp_now.h>
#include <esp_wifi.h>
#include <Ticker.h>
#include <ArduinoOTA.h>
#include <OTA_include.h>
#include "CANguruDefs.h"
#include "OWN_LED.h"

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

Preferences preferences;

// config-Daten
// Parameter-Kanäle
enum Kanals
{
  Kanal00,
  Kanal01,
  Kanal02,
  endofKanals
};

Kanals CONFIGURATION_Status_Index = Kanal00;

uint8_t uid_device[uid_num];

// Zeigen an, ob eine entsprechende Anforderung eingegangen ist
bool CONFIG_Status_Request = false;
bool SYS_CMD_Request = false;
bool SEND_IP_Request = false;
bool bBlinkAlive;

// Timer
boolean statusPING;
boolean initialData2send;
boolean bDecoderIsAlive;

#define VERS_HIGH 0x00 // Versionsnummer vor dem Punkt
#define VERS_LOW 0x01  // Versionsnummer nach dem Punkt

/*
Variablen der steppers & Magnetartikel
*/
Stepper stepper;

uint8_t decoderadr;
uint8_t stepperDelay;
uint16_t stepsToEnd;
position rightORleft;

const uint16_t stepsToEnd_min = 0;
const uint16_t stepsToEnd_std = 1000;
const uint16_t stepsToEnd_max = 1200;

IPAddress IP;

// Forward declaration
void switchAcc();
void acc_report();
void sendConfig();
void generateHash(uint8_t offset);

#include "espnow.h"

void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout detector
  Serial.begin(bdrMonitor);
  delay(500);
  log_i("\r\n\r\nCANguru - Stepper - Weiche");
  log_i("\n on %s", ARDUINO_BOARD);
  log_i("CPU Frequency = %d Mhz", F_CPU / 1000000);
  //  log_e("ERROR!");
  //  log_d("VERBOSE");
  //  log_w("WARNING");
  //  log_i("INFO");
  // der Decoder strahlt mit seiner Kennung
  // damit kennt die CANguru-Bridge (der Master) seine Decoder findet
  DEVTYPE = DEVTYPE_STEPPER;
  //  LED_begin(GPIO_NUM_8);
  // der Decoder strahlt mit seiner Kennung
  // damit kennt die CANguru-Bridge (der Master) seine Decoder findet
  startAPMode();
  // der Master (CANguru-Bridge) wird registriert
  addMaster();
  // WLAN -Verbindungen können wieder ausgeschaltet werden
  WiFi.disconnect();

  // die preferences-Library wird gestartet

  if (preferences.begin("STEPPER", false))
  {
    log_i("Preferences erfolgreich gestartet");
  }

  uint8_t setup_todo = preferences.getUChar("setup_done", 0xFF);
  if (setup_todo != setup_done)
  {
    // alles fürs erste Mal
    //
    // wurde das Setup bereits einmal durchgeführt?
    // dann wird dieser Anteil übersprungen
    // 47, weil das preferences (hoffentlich) nie ursprünglich diesen Inhalt hatte

    // setzt die Boardnum anfangs auf 1
    decoderadr = 1;
    preferences.putUChar("decoderadr", decoderadr);
    // Anfangswerte
    // Gesamtumdrehungen des Steppers
    stepsToEnd = 0x00;
    preferences.putUShort("stepsToEnd", stepsToEnd);
    //
    // Verzögerung
    stepperDelay = stdstepperdelay;
    preferences.putUChar("SrvDel", stepperDelay);
    //
    // Status der Magnetartikel zu Beginn auf rechts setzen
    rightORleft = right;
    preferences.putUChar("acc_state", rightORleft);
    //
    // ota auf "FALSE" setzen
    preferences.putUChar("ota", startWithoutOTA);
    //
    // setup_done auf "TRUE" setzen
    preferences.putUChar("setup_done", setup_done);
    //
  }
  else
  {
    uint8_t ota = preferences.getUChar("ota", false);
    if (ota == startWithoutOTA)
    {
      // nach dem ersten Mal Einlesen der gespeicherten Werte
      // Adresse
      decoderadr = readValfromPreferences(preferences, "decoderadr", minadr, minadr, maxadr);
      // Verzögerung
      stepperDelay = readValfromPreferences(preferences, "SrvDel", stdstepperdelay, minstepperdelay, maxstepperdelay);
      // Gesamtumdrehung
      stepsToEnd = readValfromPreferences16(preferences, "stepsToEnd", stepsToEnd_std, stepsToEnd_min, stepsToEnd_max);
      // Status der Magnetartikel versenden an die steppers
      rightORleft = (position)readValfromPreferences(preferences, "acc_state", right, right, left);
    }
    else
    {
      // ota auf "FALSE" setzen
      preferences.putUChar("ota", startWithoutOTA);
      //
      Connect2WiFiandOTA(preferences);
    }
  }
  // ab hier werden die Anweisungen bei jedem Start durchlaufen
  // IP-Adresse
  char ip[4]; // prepare a buffer for the data
  preferences.getBytes("IP0", ip, 4);
  for (uint8_t i = 0; i < 4; i++)
  {
    IP[i] = ip[i];
  }

  // Flags
  got1CANmsg = false;
  SYS_CMD_Request = false;
  SEND_IP_Request = false;
  statusPING = false;
  initialData2send = false;
  bBlinkAlive = true;
  bDecoderIsAlive = true;
  // Variablen werden gemäß der eingelsenen Werte gesetzt
  // evtl. werden auch die steppers verändert
  // steppers mit den PINs verbinden, initialisieren & Artikel setzen wie gespeichert
  stepper.Set_to_address(decoderadr);
  stepper.SetDelay(stepperDelay);
  stepper.Set_stepsToSwitch(stepsToEnd);
  stepper.SetPosCurr(rightORleft);
  stepper.Attach();
  stepper.SetPosition();
  // Vorbereiten der Blink-LED
  stillAliveBlinkSetup(GPIO_NUM_8);
}

// Zu Beginn des Programmablaufs werden die aktuellen Statusmeldungen an WDP geschickt
void sendTheInitialData()
{
  acc_report();
}

// receiveKanalData dient der Parameterübertragung zwischen Decoder und CANguru-Server
// es erhält die evtuelle auf dem Server geänderten Werte zurück
void receiveKanalData()
{
  SYS_CMD_Request = false;
  uint16_t oldval;
  switch (opFrame[data5])
  {
  // Kanalnummer #1 - Decoderadresse
  case 1:
  {
    oldval = decoderadr;
    decoderadr = (opFrame[data6] << 8) + opFrame[data7];
    if (testMinMax(oldval, decoderadr, minadr, maxadr) && preferences.getUChar("receiveTheData", true))
    {
      // speichert die neue Adresse
      preferences.putUChar("decoderadr", decoderadr);
      stepper.Set_to_address(decoderadr);
      //
    }
    else
    {
      decoderadr = oldval;
    }
  }
  break;
  // Kanalnummer #2 - stepperverzögerung
  case 2:
  {
    oldval = stepperDelay;
    stepperDelay = (opFrame[data6] << 8) + opFrame[data7];
    if (testMinMax(oldval, stepperDelay, minstepperdelay, maxstepperdelay) && preferences.getUChar("receiveTheData", true))
    {
      preferences.putUChar("SrvDel", stepperDelay);
      stepper.SetDelay(stepperDelay);
      //
    }
    else
    {
      stepperDelay = oldval;
    }
  }
  break;
  }
  //
  opFrame[data6] = 0x01;
  opFrame[Framelng] = 0x07;
  sendCanFrame();
}

// Routine meldet an die CANguru-Bridge, wenn eine Statusänderung
// einer Weiche/Signal eingetreten ist
void acc_report()
{
  opFrame[CANcmd] = SWITCH_ACC;
  opFrame[Framelng] = 0x05;
  // Weichenadresse
  opFrame[data0] = 0x00;
  opFrame[data1] = 0x00;
  opFrame[data2] = (uint8_t)(stepper.Get_to_address() >> 8);
  opFrame[data3] = (uint8_t)stepper.Get_to_address();
  // Meldung der Lage
  opFrame[data4] = stepper.GetPosCurr();
  sendCanFrame();
  delay(wait_time); // Delay added just so we can have time to open up
}

// Diese Routine leitet den Positionswechsel einer Weiche/Signal ein.
void switchAcc()
{
  position set_pos = stepper.GetPosDest();
  switch (set_pos)
  {
  case left:
  {
    stepper.GoLeft();
  }
  break;
  case right:
  {
    stepper.GoRight();
  }
  break;
  }
  stepper.SetPosCurr(set_pos);
  preferences.putUChar("acc_state", (uint8_t)set_pos);
  //
  acc_report();
}

// auf Anforderung des CANguru-Servers sendet der Decoder
// mit dieser Prozedur sendConfig seine Parameterwerte
void sendConfig()
{
  const uint8_t Kanalwidth = 8;
  const uint8_t numberofKanals = endofKanals - 1;
  const uint8_t NumLinesKanal00 = 4 * Kanalwidth;
  uint8_t arrKanal00[NumLinesKanal00] = {
      /*1*/ Kanal00, numberofKanals, (uint8_t)0, (uint8_t)0, (uint8_t)0, (uint8_t)0, (uint8_t)0, decoderadr,
      /*2.1*/ (uint8_t)highbyte2char(hex2dec(uid_device[0])), (uint8_t)lowbyte2char(hex2dec(uid_device[0])),
      /*2.2*/ (uint8_t)highbyte2char(hex2dec(uid_device[1])), (uint8_t)lowbyte2char(hex2dec(uid_device[1])),
      /*2.3*/ (uint8_t)highbyte2char(hex2dec(uid_device[2])), (uint8_t)lowbyte2char(hex2dec(uid_device[2])),
      /*2.4*/ (uint8_t)highbyte2char(hex2dec(uid_device[3])), (uint8_t)lowbyte2char(hex2dec(uid_device[3])),
      /*3*/ 'C', 'A', 'N', 'g', 'u', 'r', 'u', ' ',
      /*4*/ 'S', 't', 'e', 'p', 'p', 'e', 'r', (uint8_t)0};
  const uint8_t NumLinesKanal01 = 4 * Kanalwidth;
  uint8_t arrKanal01[NumLinesKanal01] = {
      // #2 - WORD immer Big Endian, wie Uhrzeit
      /*1*/ Kanal01, 2, 0, minadr, 0, maxadr, 0, decoderadr,
      /*2*/ 'M', 'o', 'd', 'u', 'l', 'a', 'd', 'r',
      /*3*/ 'e', 's', 's', 'e', 0, '1', 0, (maxadr / 100) + '0',
      /*4*/ (maxadr - (uint8_t)(maxadr / 100) * 100) / 10 + '0', (maxadr - (uint8_t)(maxadr / 10) * 10) + '0', 0, 'A', 'd', 'r', 0, 0};
  const uint8_t NumLinesKanal02 = 5 * Kanalwidth;
  uint8_t arrKanal02[NumLinesKanal02] = {
      /*1*/ Kanal02, 2, 0, minstepperdelay, 0, maxstepperdelay, 0, stepperDelay,
      /*2*/ 'S', 'e', 'r', 'v', 'o', 'v', 'e', 'r',
      /*3*/ 'z', '\xF6', 'g', 'e', 'r', 'u', 'n', 'g',
      /*4*/ 0, minstepperdelay + '0', 0, (maxstepperdelay / 100) + '0', (maxstepperdelay - (uint8_t)(maxstepperdelay / 100) * 100) / 10 + '0', (maxstepperdelay - (uint8_t)(maxstepperdelay / 10) * 10) + '0', 0, 'x',
      /*5*/ 0, 0, 0, 0, 0, 0, 0, 0};
  uint8_t NumKanalLines[numberofKanals + 1] = {
      NumLinesKanal00, NumLinesKanal01, NumLinesKanal02};

  preferences.putUChar("receiveTheData", true);
  uint8_t paket = 0;
  uint8_t outzeichen = 0;
  CONFIG_Status_Request = false;
  for (uint8_t inzeichen = 0; inzeichen < NumKanalLines[CONFIGURATION_Status_Index]; inzeichen++)
  {
    opFrame[CANcmd] = CONFIG_Status + 1;
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
    case endofKanals:
    {
      // der Vollständigkeit geschuldet
    }
    break;
    }
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
  // die steppers werden permant abgefragt, ob es ein Delta zwischen
  // tatsächlicher und gewünschter stepperstellung gibt
  if (stepper.Get_set_stepsToSwitch())
  {
    stepsToEnd = stepper.Get_stepsToSwitch();
    preferences.putUShort("stepsToEnd", stepsToEnd);
    //
    stepper.Reset_stepsToSwitch();
  }
  stepper.Update();
  bBlinkAlive = stepper.is_no_Correction();
  if (got1CANmsg)
  {
    got1CANmsg = false;
    // auf PING Antworten
    if (statusPING)
    {
      sendPING();
    }
    if (bDecoderIsAlive)
    {
      sendDecoderIsAlive();
    }
    // Parameterwerte vom CANguru-Server erhalten
    if (SYS_CMD_Request)
    {
      receiveKanalData();
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
