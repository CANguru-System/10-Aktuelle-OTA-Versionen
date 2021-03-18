
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

#include "display2use.h"
#ifdef LCD28
#include <MOD-LCD.h>
#endif
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

//CAN_FRAME_SIZE
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
const unsigned int localPortDelta = 2;                                // local port to listen on
const unsigned int localPortoutSYS = 15730;                           // local port to send on
const unsigned int localPortinSYS = 15731;                            // local port to listen on
const unsigned int localPortoutGW = localPortoutSYS + localPortDelta; // local port to send on
const unsigned int localPortinGW = localPortinSYS + localPortDelta;   // local port to listen on

//create UDP instance
// EthernetUDP instances to let us send and receive packets over UDP
WiFiUDP UdpOUTSYS;
WiFiUDP UdpOUTGW;

WiFiUDP UdpINSYS;
WiFiUDP UdpINGW;

WiFiClient TCPclient;

IPAddress ipGateway;

// Set web server port number to 80
const unsigned int httpPort = 80;
AsyncWebServer AsyncWebSrvr(httpPort);

WiFiServer TCPINSYS(localPortinSYS);

//events
//*********************************************************************************************************
// behandelt die diversen Stadien des ETHERNET-Aufbaus
void WiFiEvent(WiFiEvent_t event)
{
#ifdef OLED
  Adafruit_SSD1306 *displ = getDisplay();
#endif
  switch (event)
  {
  case SYSTEM_EVENT_ETH_START:
#ifdef OLED
    displ->println(F("ETH Started"));
#endif
#ifdef LCD28
    displayLCD("ETHERNET Started");
#endif
    break;
  case SYSTEM_EVENT_ETH_CONNECTED:
#ifdef OLED
    displ->println(F("ETH Connected"));
#endif
#ifdef LCD28
    displayLCD("ETHERNET Connected");
#endif
    //set eth hostname here
    ETH.setHostname("CANguru-Bridge");
    break;
  case SYSTEM_EVENT_ETH_GOT_IP:
#ifdef OLED
    displ->setTextSize(2); // Draw 2X-scale text
    displ->println(ETH.localIP());
    displ->display();
    displ->setTextSize(1); // Draw 1X-scale text
    displ->println(F(""));
    displ->display();
    displ->setTextSize(2); // Draw 2X-scale text
    displ->println(F("Connect!"));
    displ->display();
    displ->setTextSize(1); // Draw 1X-scale text
#endif
#ifdef LCD28
    displayIP(ETH.localIP());
    displayLCD("");
    displayLCD("Connect!");
#endif
    Serial.println(F("Connect!")); // %%
    setEthStatus(true);
    break;
  case SYSTEM_EVENT_ETH_DISCONNECTED:
#ifdef OLED
    displ->println(F("ETH Disconnected"));
#endif
#ifdef LCD28
    displayLCD("ETHERNET Disconnected");
#endif
    setEthStatus(false);
    break;
  case SYSTEM_EVENT_ETH_STOP:
#ifdef OLED
    displ->println(F("ETH Stopped"));
#endif
#ifdef LCD28
    displayLCD("ETHERNET Stopped");
#endif
    setEthStatus(false);
    break;
  default:
    break;
  }
#ifdef OLED
  displ->display();
#endif
}

// der Timer steuert das Scannen der Slaves, das Blinken der LED
// sowie das Absenden des PING
void timer1s()
{
  static uint8_t time2SendPing = 1;
  secs++;
  if (get_time4Scanning() == true)
  {
    if (secs > (maxDevices / 2))
    {
      set_time4Scanning(false);
      produceFrame(M_CAN_PING);
      proc2CAN(M_PATTERN, toClnt);
      sendOutTCP(M_PATTERN);
    }
  }
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
      sendOutTCP(M_PATTERN);
    }
  }
}

// Start des Timers
void stillAliveBlinkSetup()
{
  tckr.attach(tckrTime, timer1s); // each sec
  lastStatus = undef;
}

// hiermit wird die Aufforderung zum Blinken an die Decoder verschickt
// sowie einmalig die Aufforderung zur Sendung der Initialdaten an die Decoder
// Initialdaten sind bsplw. die anfängliche Stellung der Weichen, Signale oder
// Gleisbesetztmelder; anschließend gibt es eine Wartezeit, die für die einzelnen
// Decodertypen unterschiedlich lang ist
void stillAliveBlinking()
{
  static uint8_t slv = 0;
  uint8_t M_BLINK[] = {0x00, BlinkAlive, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  uint8_t M_IDATA[] = {0x00, sendInitialData, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  if (get_waiting4Handshake() == false)
  {
    if (currStatus != lastStatus)
    {
      lastStatus = currStatus;
      M_BLINK[0x05] = currStatus;
      sendTheData(slv, M_BLINK, CAN_FRAME_SIZE);
      if (get_initialData2send(slv))
      {
        switch (get_decoder_type(slv))
        {
        case DEVTYPE_SERVO:
        case DEVTYPE_SIGNAL:
        case DEVTYPE_LEDSIGNAL:
          sendTheData(slv, M_IDATA, CAN_FRAME_SIZE);
          delay(4 * wait_time_medium);
          break;
        case DEVTYPE_RM:
          sendTheData(slv, M_IDATA, CAN_FRAME_SIZE);
          delay(24 * wait_time_medium);
          break;
        case DEVTYPE_LIGHT:
        case DEVTYPE_CANFUSE:
        case DEVTYPE_GATE:
          break;
        }
        reset_initialData2send(slv);
      }
      slv++;
      if (slv >= get_slaveCnt())
        slv = 0;
    }
  }
}

/*************************** End of file ****************************/
#endif
