
/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <CANguru-Buch@web.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gustav Wostrack
 * ----------------------------------------------------------------------------
 */

#ifndef UTILS_H
#define UTILS_H

#include <MOD-LCD.h>
#include <espnow.h>

enum statustype
{
  even = 0,
  odd,
  undef
};

statustype lastStatus;
statustype currStatus;

bool drawCircle;

// timer
const uint8_t wait_for_ping = 12;
uint16_t secs = 0;
const uint8_t maxDevices = 20;

Ticker tckr;
#define tckrTime 1.0 / maxDevices

// CAN_FRAME_SIZE
uint8_t cntTCPFramesUsed = 0;
uint8_t arrayTCPFramesUsed[maxPackets] = {0};
uint8_t arrayTCPFrames[maxPackets][CAN_FRAME_SIZE] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

uint8_t cntURLUsed = 0;
String arrayURL[maxPackets] = {};
AsyncWebServerRequest *arrayRequest[maxPackets] = {};

// die Portadressen; 15730 und 15731 sind von Märklin festgelegt
// OUT is even
const unsigned int localPortDelta = 2;                                      // local port to listen on
const unsigned int localPortToWDP = 15730;                                  // local port to send on
const unsigned int localPortFromWDP = 15731;                                // local port to listen on
const unsigned int localPortToServer = localPortToWDP + localPortDelta;     // local port to send on
const unsigned int localPortFromServer = localPortFromWDP + localPortDelta; // local port to listen on

// create UDP instance
//  EthernetUDP instances to let us send and receive packets over UDP
WiFiUDP UDPToWDP;
WiFiUDP UDPToServer;

WiFiUDP UDPFromWDP;
WiFiUDP UDPFromServer;

WiFiClient TCPclient;

IPAddress ipGateway;

// Set web server port number to 80
const unsigned int httpPort = 80;
AsyncWebServer AsyncWebSrvr(httpPort);

WiFiServer TCPINSYS(localPortFromWDP);

// events
//*********************************************************************************************************
//  behandelt die diversen Stadien des ETHERNET-Aufbaus
//  Network event callback
void iNetEvtCB(arduino_event_id_t event, arduino_event_info_t info)
{
  switch (event)
  {
  case ARDUINO_EVENT_WIFI_SCAN_DONE:
    displayLCD("Scanning finished");
    log_i("Scanning finished");
    scanningFinished = true;
    break;
  case ARDUINO_EVENT_ETH_START: // SYSTEM_EVENT_ETH_START:
    displayLCD("ETHERNET Started");
    log_i("ETHERNET Started");
    break;
  case ARDUINO_EVENT_ETH_CONNECTED: // SYSTEM_EVENT_ETH_CONNECTED:
    displayLCD("ETHERNET Connected");
    // set eth hostname here
    log_i("ETHERNET Connected");
    ETH.setHostname("CANguru-Bridge");
    break;
  case ARDUINO_EVENT_ETH_GOT_IP: // SYSTEM_EVENT_ETH_GOT_IP:
    displayIP(ETH.localIP());
    displayLCD("");
    displayLCD("Connect!");
    log_i("Connect");
    Serial.println(F("Connect!")); // %%
    break;
  case ARDUINO_EVENT_ETH_DISCONNECTED: // SYSTEM_EVENT_ETH_DISCONNECTED:
    displayLCD("ETHERNET Disconnected");
    log_i("ETHERNET Disconnected");
    setServerStatus(false);
    break;
  case ARDUINO_EVENT_ETH_STOP: // SYSTEM_EVENT_ETH_STOP:
    displayLCD("ETHERNET Stopped");
    log_i("ETHERNET Stopped");
    setServerStatus(false);
    break;
  default:
    log_i("ETHERNET ?? %X", event);
    break;
  }
}

// der Timer steuert das Scannen der Slaves, das Blinken der LED
// sowie das Absenden des PING
void timer1s()
{
  static uint8_t time2SendPing = 1;
  secs++;
  // startet den fillTheCircle()-Prozess; Ausgabe pulsiert mit einem Kreis
  if (secs % maxDevices == 0)
    setbfillRect();
  uint8_t slaveCnt = get_slaveCnt();
  if (slaveCnt > 0)
  {
    if ((secs / (maxDevices / slaveCnt)) % 2 == 0)
      currStatus = even;
    else
      currStatus = odd;
  }
  // periodic ping
  if (get_SYSseen())
  {
    if (secs % maxDevices == 0)
    {
      time2SendPing++;
    }
    if (time2SendPing % wait_for_ping == 0)
    {
      time2SendPing = 1;
      produceFrame(M_CAN_PING);
      proc2CAN(M_PATTERN, toClnt);
      //      sendOutTCP(M_PATTERN);
    }
  }
}

// Start des Timers
void stillAliveBlinkSetup()
{
  tckr.attach(tckrTime, timer1s); // each sec
  // lastStatus kann 3 Zustände haben und entscheidet, ob die Statausanzeige blinkt (an oder aus) oder ganz aus  (undef) ist
  lastStatus = undef;
}

// hiermit wird die Aufforderung zum Blinken an die Decoder verschickt
// sowie einmalig die Aufforderung zur Sendung der Initialdaten an die Decoder
// Initialdaten sind bsplw. die anfängliche Stellung der Weichen, Signale oder
// Gleisbesetztmelder; anschließend gibt es eine Wartezeit, die für die einzelnen
// Decodertypen unterschiedlich lang ist
//
// die Variable currStatus gibt an, ob die Status-LED auf dem Decoder an oder aus ist
// und wird durch den 1 Sek-Timer umgeschaltet, verschickt wird sie dann von dieser Routine
void stillAliveBlinking()
{
  static uint8_t slv = 0;
  uint8_t M_BLINK[] = {0x00, BlinkAlive, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  uint8_t M_ERROR[] = {0x00, lostDecoder, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  if (currStatus != lastStatus)
  {
    if (getAlive(slv) == isAlive)
    {
      lastStatus = currStatus;
      M_BLINK[0x05] = currStatus;
      M_BLINK[0x06] = slv;
      sendTheData(slv, M_BLINK, CAN_FRAME_SIZE);
      setAlive(slv, isLost);
    }
    else
    {
      if (getAlive(slv) == isLost)
      {
        // FEHLER: Slave hat sich nicht gemeldet!
        setAlive(slv, ErrorMsgSent);
        M_ERROR[0x05] = slv;
        sendToServer(M_ERROR, MSGfromBridge);
      }
    }
    slv++;
    if (slv >= get_slaveCnt())
      slv = 0;
  }
}

/*************************** End of file ****************************/
#endif
