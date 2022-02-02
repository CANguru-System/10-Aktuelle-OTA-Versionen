
/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <CANguru-Buch@web.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gustav Wostrack
 * ----------------------------------------------------------------------------
 */
#include <Arduino.h>

#include <driver/can.h>
#include <driver/gpio.h>
#include <esp_system.h>
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include <ETH.h>
#include "CANguruDefs.h"
#include "display2use.h"
#include <SPI.h>
#include <Wire.h>
#include <espnow.h>
#include <telnet.h>
#include <Adafruit_GFX.h>
#include "CANguru.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
//#include <miniz.h>
#include "SPIFFS.h"
#ifdef OLED
#include <Adafruit_SSD1306.h>
#endif
#ifdef LCD28
#include "MOD-LCD.h"
#endif

//#define debug

// buffer for receiving and sending data
uint8_t M_PATTERN[] = {0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t lastmfxUID[] = {0x00, 0x00, 0x00, 0x00};

uint8_t cntLoks;
struct LokBufferType
{
  uint8_t lastmfxUID[4];
  uint8_t adr;
};
LokBufferType *LokBuffer = NULL;

const uint8_t maxPackets = 30;
#define httpBufferLength 0x03FF
uint8_t httpBuffer[httpBufferLength + 10];
File locofile;
const uint8_t shortnameLng = 7;
const uint8_t shortnameLng2transfer = shortnameLng - 2;
const uint8_t cntCS2Files = 10;
String cs2Files[cntCS2Files][2] = {
    {"ger    ", "geraet.vrs"},
    {"loks   ", "lokomotive.cs2"},
    {"mags   ", "magnetartikel.cs2"},
    {"fs     ", "fahrstrassen.cs2"},
    {"gbs    ", "gleisbild.cs2"},
    {"prefs  ", "prefs.cs2"},
    {"lokstat", "lokomotive.sr2"},
    {"magstat", "magnetartikel.sr2"},
    {"gbsstat", "gleisbild.sr2"},
    {"fsstat ", "fahrstrassen.sr2"},
};
char gbsFile[8] = {'g', 'b', 's', ' ', ' ', ' ', ' ', ' '};
const uint8_t cntCS2Sizes = 6;
// size - crc
// eins mehr für gbs-X
uint8_t cs2FileSizes[cntCS2Files + 1][cntCS2Sizes] = {
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0}};

const uint8_t indent = 10;

canguruETHClient telnetClient;

bool locofileread;
bool initialDataAlreadySent;

byte locid;
byte cvIndex;
bool bLokDiscovery;

// forward declaration
void receiveLocFile(uint8_t f, bool cmprssd);
void analyseTCP(uint8_t *buffer);
void bindANDverify(uint8_t *buffer);
char readConfig(char index);

// -------------------------------------------------------------------------------------------------

#define portWAIT ( TickType_t ) 0x05

void setup_can_driver()
{
  #ifdef debug
  Serial.println("CAN Driver installation started...");
  #endif
  can_general_config_t general_config = {
      .mode = CAN_MODE_NORMAL,
      .tx_io = GPIO_NUM_5,
      .rx_io = GPIO_NUM_35,
      .clkout_io = (gpio_num_t)CAN_IO_UNUSED,
      .bus_off_io = (gpio_num_t)CAN_IO_UNUSED,
      .tx_queue_len = 5,
      .rx_queue_len = 5,
      .alerts_enabled = CAN_ALERT_NONE,
      .clkout_divider = 0};
  can_timing_config_t timing_config = CAN_TIMING_CONFIG_250KBITS();
  can_filter_config_t filter_config = CAN_FILTER_CONFIG_ACCEPT_ALL();
  esp_err_t error;

  error = can_driver_install(&general_config, &timing_config, &filter_config);
  if (error != ESP_OK)
  {
    Serial.println("CAN Driver installation fail...");
    return;
  }
  #ifdef debug
  else
  {
    Serial.println("CAN Driver installation success...");
  }
#endif
  // start CAN driver
  error = can_start();
#ifdef LCD28
  if (error == ESP_OK)
  {
  #ifdef debug
    Serial.println("CAN Driver start success...");
    #endif
    displayLCD("CAN is running!");
  }
  else
  {
  #ifdef debug
    Serial.println("CAN Driver start FAILED...");
    #endif
    displayLCD("Starting CAN failed!");
    while (1)
      delay(10);
  }
#endif
#ifdef OLED
  if (error == ESP_OK)
  {
  #ifdef debug
    Serial.println("CAN Driver start success...");
    #endif
    displ->println(F("Starting CAN was successful!"));
    displ->display();
  }
  else
  {
  #ifdef debug
    Serial.println("CAN Driver start FAILED...");
    #endif
    displ->println(F("Starting CAN failed!"));
    displ->display();
    while (1)
      delay(10);
  }
#endif
}

// -------------------------------------------------------------------------------------------------

void printMSG(uint8_t no)
{
  uint8_t MSG[] = {0x00, no, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  sendOutGW(MSG, MSGfromBridge);
}

//
void printCANFrame(uint8_t *buffer, CMD dir)
{
  // Diese Prozedur könnte mehrere Male aufgerufen werden;
  // deshalb wird die Erhöhung begrenzt
  if (buffer[Framelng] <= 0x08)
    buffer[Framelng] += 0x0F;
  sendOutGW(buffer, dir);
  return;
}

// ein CAN-Frame wird erzeugt, der Parameter noFrame gibt an, welche Daten
// in den Frame zu kopieren sind
void produceFrame(patterns noFrame)
{
  memset(M_PATTERN, 0x0, CAN_FRAME_SIZE);
  M_PATTERN[2] = 0x03;
  M_PATTERN[3] = 0x01;
  switch (noFrame)
  {
  case M_GLEISBOX_MAGIC_START_SEQUENCE:
    M_PATTERN[1] = 0x36;
    M_PATTERN[2] = 0x03;
    M_PATTERN[3] = 0x01;
    M_PATTERN[4] = 0x05;
    M_PATTERN[9] = 0x11;
    break;
  case M_GLEISBOX_ALL_PROTO_ENABLE:
    M_PATTERN[2] = 0x03;
    M_PATTERN[3] = 0x01;
    M_PATTERN[4] = 0x06;
    M_PATTERN[9] = 0x08;
    M_PATTERN[10] = 0x07;
    break;
  case M_GO:
    M_PATTERN[4] = 0x05;
    M_PATTERN[9] = 0x01;
    break;
  case M_STOP:
    M_PATTERN[1] = 0x00;
    M_PATTERN[4] = 0x05;
    M_PATTERN[9] = 0x00;
    break;
  case M_BIND:
    M_PATTERN[1] = 0x04;
    M_PATTERN[4] = 0x06;
    M_PATTERN[9] = 0x00;
    break;
  case M_VERIFY:
    M_PATTERN[1] = 0x06;
    M_PATTERN[4] = 0x06;
    M_PATTERN[9] = 0x00;
    break;
  case M_FUNCTION:
    M_PATTERN[1] = 0x0C;
    M_PATTERN[4] = 0x06;
    M_PATTERN[7] = 0x04;
    break;
  case M_CAN_PING:
    M_PATTERN[1] = 0x30;
    M_PATTERN[2] = 0x47;
    M_PATTERN[3] = 0x11;
    break;
  case M_PING_RESPONSE:
    M_PATTERN[0] = 0x00;
    M_PATTERN[1] = 0x30;
    M_PATTERN[2] = 0x00;
    M_PATTERN[3] = 0x00;
    M_PATTERN[4] = 0x00;
    break;
  case M_CAN_PING_CS2:
    M_PATTERN[1] = 0x31;
    M_PATTERN[2] = 0x47;
    M_PATTERN[3] = 0x11;
    M_PATTERN[4] = 0x08;
    M_PATTERN[9] = 0x03;
    M_PATTERN[10] = 0x08;
    M_PATTERN[11] = 0xFF;
    M_PATTERN[12] = 0xFF;
    break;
  case M_CAN_PING_CS2_1:
    M_PATTERN[1] = 0x31;
    M_PATTERN[2] = 0x63;
    M_PATTERN[3] = 0x4A;
    M_PATTERN[4] = 0x08;
    M_PATTERN[9] = 0x04;
    M_PATTERN[10] = 0x02;
    M_PATTERN[11] = 0xFF;
    M_PATTERN[12] = 0xF0;
    break;
  case M_CAN_PING_CS2_2:
    M_PATTERN[1] = 0x31;
    M_PATTERN[2] = 0x63;
    M_PATTERN[3] = 0x4B;
    M_PATTERN[4] = 0x08;
    M_PATTERN[9] = 0x03;
    M_PATTERN[10] = 0x44;
    break;
  case M_READCONFIG:
    M_PATTERN[1] = ReadConfig;
    M_PATTERN[4] = 0x07;
    M_PATTERN[7] = 0x40;
    break;
  case M_STARTCONFIG:
    M_PATTERN[1] = MfxProc_R;
    M_PATTERN[4] = 0x02;
    M_PATTERN[5] = 0x01;
    break;
  case M_FINISHCONFIG:
    M_PATTERN[1] = MfxProc_R;
    M_PATTERN[4] = 0x01;
    M_PATTERN[5] = 0x00;
    break;
  case M_DOCOMPRESS:
    M_PATTERN[1] = DoCompress;
    M_PATTERN[4] = 0x00;
    break;
  case M_DONOTCOMPRESS:
    M_PATTERN[1] = DoNotCompress;
    M_PATTERN[4] = 0x00;
    break;
  case M_GETCONFIG1:
    M_PATTERN[1] = LoadCS2Data;
    M_PATTERN[4] = 0x08;
    M_PATTERN[5] = 0x6C;
    M_PATTERN[6] = 0x6F;
    M_PATTERN[7] = 0x6B;
    M_PATTERN[8] = 0x73;
    break;
  case M_GETCONFIG2:
    M_PATTERN[1] = LoadCS2Data_R;
    M_PATTERN[4] = 0x08;
    M_PATTERN[5] = 0x6C;
    M_PATTERN[6] = 0x6F;
    M_PATTERN[7] = 0x6B;
    M_PATTERN[8] = 0x73;
    break;
  case M_SIGNAL:
    M_PATTERN[1] = 0x50;
    M_PATTERN[4] = 0x01;
    break;
  case M_CNTLOKBUFFER:
    M_PATTERN[1] = sendCntLokBuffer;
    break;
  case M_SENDLOKBUFFER:
    M_PATTERN[1] = sendLokBuffer;
    M_PATTERN[4] = 0x01;
    break;
  }
}

#include <utils.h>

//////////////// Ausgaberoutinen

// sendet den Frame, auf den der Zeiger buffer zeigt, über ESPNow
// an alle Slaves
void send2AllClients(uint8_t *buffer)
{
  uint8_t slaveCnt = get_slaveCnt();
  for (uint8_t s = 0; s < slaveCnt; s++)
  {
    sendTheData(s, buffer, CAN_FRAME_SIZE);
  }
}

// wenn aus der UID des Frames ein Slave identifizierbar ist, wird der
// Frame nur an diesen Slave geschickt, ansonsten an alle
void send2OneClient(uint8_t *buffer)
{
  uint8_t ss = matchUID(buffer);
  if (ss == 0xFF)
    send2AllClients(buffer);
  else
    sendTheData(ss, buffer, CAN_FRAME_SIZE);
}

void writeTCP(uint8_t *tcb, uint16_t strlng)
{
  if (TCPclient)
  {
    TCPclient.write(tcb, strlng);
    delay(2);
  }
}

void sendOutGW(uint8_t *buffer, CMD cmd)
{
  //%
  buffer[0] = cmd;
  UdpOUTGW.beginPacket(ipGateway, localPortoutGW);
  UdpOUTGW.write(buffer, CAN_FRAME_SIZE);
  UdpOUTGW.endPacket();
  buffer[0] = 0x00;
}

// sendet einen CAN-Frame an den Teilnehmer und gibt ihn anschließend aus
void sendOutTCP(uint8_t *buffer)
{
  writeTCP(buffer, CAN_FRAME_SIZE);
  //  printCANFrame(buffer, toTCP);
}

void sendOutTCPfromCAN(uint8_t *buffer)
{
  writeTCP(buffer, CAN_FRAME_SIZE);
  //  printCANFrame(buffer, fromCAN2TCP);
  //  print_can_frame(3, buffer);
}

void sendOutUDP(uint8_t *buffer)
{
  UdpOUTSYS.beginPacket(ipGateway, localPortoutSYS);
  UdpOUTSYS.write(buffer, CAN_FRAME_SIZE);
  UdpOUTSYS.endPacket();
  printCANFrame(buffer, toUDP);
}

void sendOutUDPfromCAN(uint8_t *buffer)
{
  UdpOUTSYS.beginPacket(ipGateway, localPortoutSYS);
  UdpOUTSYS.write(buffer, CAN_FRAME_SIZE);
  UdpOUTSYS.endPacket();
  //  printCANFrame(buffer, fromCAN2UDP);
}

void sendOutClnt(uint8_t *buffer, CMD dir)
{
  switch (buffer[0x01])
  {
  case CONFIG_Status:
    send2OneClient(buffer);
    break;
  case SYS_CMD:
    switch (buffer[0x09])
    {
    case SYS_STAT:
    case RESET_MEM:
    case START_OTA:
      send2OneClient(buffer);
      break;
    default:
      send2AllClients(buffer);
      break;
    }
    break;
  // send to all
  default:
    send2AllClients(buffer);
    break;
  }
  printCANFrame(buffer, dir);
}

// prüft, ob es Slaves gibt und sendet den CAN-Frame buffer dann an die Slaves
void proc2Clnts(uint8_t *buffer, CMD dir)
{
  // to Client
  if (get_slaveCnt() > 0)
  {
    sendOutClnt(buffer, dir);
  }
}

// sendet den CAN-Frame buffer über den CAN-Bus an die Gleisbox
void proc2CAN(uint8_t *buffer, CMD dir)
{
  // can_message_t myMessageToSend = {1, 0x123, 8, {0x01, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x99}}; // geschraubt
  //   can_message_t myMessageToSend = {1, 0x125, 8, {0x01, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x99}};  // frei
  //   can_message_t Message2Send = {0, 0x00, 0x00, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  can_message_t Message2Send = {CAN_MSG_FLAG_EXTD | CAN_MSG_FLAG_SS, 0x0000, 0x00, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  // CAN uses (network) big endian format
  // Maerklin TCP/UDP Format: always 13 (CAN_FRAME_SIZE) bytes
  //   byte 0 - 3  CAN ID
  //   byte 4      DLC
  //   byte 5 - 12 CAN data
  //
  /*
typedef struct {
    uint32_t flags;                 //< Bit field of message flags indicates frame/transmission type (see documentation)
    uint32_t identifier;            //< 11 or 29 bit identifier
    uint8_t data_length_code;       //< Data length code
    uint8_t data[CAN_MAX_DATA_LEN]; //< Data bytes (not relevant in RTR frame)
} can_message_t;
  */
  // flags
  //  Message2Send.flags = CAN_MSG_FLAG_EXTD;
  // identfier / CAN uses (network) big endian format
  // send extended packet: id is 29 bits, packet can contain up to 8 bytes of data
#ifdef debug
  uint32_t previousMillis = millis();
#endif
  memcpy(&Message2Send.identifier, buffer, 4);
  Message2Send.identifier = ntohl(Message2Send.identifier);
  // Anzahl Datenbytes
  Message2Send.data_length_code = buffer[4];
  // Datenbytes
  if (Message2Send.data_length_code > 0)
    memcpy(&Message2Send.data, &buffer[5], Message2Send.data_length_code);
  can_transmit(&Message2Send, portWAIT);
#ifdef debug
  Serial.print("o<-- ");
  Serial.print(buffer[0], HEX);
  Serial.print(buffer[1], HEX);
  Serial.print(" ");
  Serial.print(buffer[2], HEX);
  Serial.print(buffer[3], HEX);
  Serial.print(" (" + String(buffer[4]) + ") ");
  for (size_t i = 0; i < 8; i++)
  {
    Serial.print(buffer[i + 5], HEX);
    Serial.print("-");
  }
  Serial.println("T: " + String(millis() - previousMillis));
#endif
  printCANFrame(buffer, dir);
}

//////////////// Empfangsroutinen

// Behandlung der Kommandos, die der CANguru-Server aussendet
void proc_fromGW2CANandClnt()
{
  uint8_t Lokno;
  uint8_t UDPbuffer[CAN_FRAME_SIZE]; // buffer to hold incoming packet,
  int packetSize = UdpINGW.parsePacket();
  // if there's data available, read a packet
  if (packetSize)
  {
    // read the packet into packetBufffer
    UdpINGW.read(UDPbuffer, CAN_FRAME_SIZE);
    // send received data via usb and CAN
    switch (UDPbuffer[0x1])
    {
    case SYS_CMD:
    case 0x36:
      proc2CAN(UDPbuffer, fromGW2CAN);
      proc2Clnts(UDPbuffer, fromGW2Clnt);
      break;
    case 0x02:
    case 0x04:
    case 0x06:
    case SEND_IP:
    case CONFIG_Status:
      proc2Clnts(UDPbuffer, fromGW2Clnt);
      break;
    case ReadConfig:
    case WriteConfig:
      proc2CAN(UDPbuffer, fromGW2CAN);
      break;
    case MfxProc:
      // received next locid
      locid = UDPbuffer[0x05];
      produceFrame(M_SIGNAL);
      sendOutGW(M_PATTERN, fromCAN);
      break;
    case MfxProc_R:
      // there is a new file lokomotive.cs2 to send
      produceFrame(M_SIGNAL);
      sendOutGW(M_PATTERN, fromCAN);
      receiveLocFile(0, false);
      break;
    // PING
    case PING:
      produceFrame(M_CAN_PING);
      proc2CAN(M_PATTERN, fromGW2CAN);
      break;
    case sendCntLokBuffer_R:
      cntLoks = 0;
      cntLoks = UDPbuffer[0x05];
      if (cntLoks > 0)
      {
        // erste Lok abrufen
        // evtl. alte Zuweisung löschen
        if (LokBuffer != NULL)
        {
          delete[] LokBuffer; // Free memory allocated for the buffer array.
          LokBuffer = NULL;   // Be sure the deallocated memory isn't used.
        }
        // Speicher für alle Loks reservieren
        if (LokBuffer == NULL)
          LokBuffer = new LokBufferType[cntLoks]; // Allocate cntLoks LokBufferType and save ptr in buffer
        produceFrame(M_SENDLOKBUFFER);
        M_PATTERN[5] = 0;
        sendOutGW(M_PATTERN, toGW);
      }
      break;
    case sendLokBuffer_R:
      Lokno = UDPbuffer[0x05];
      if (LokBuffer != NULL)
      {
        //        saveFrame(UDPbuffer);
        LokBuffer[Lokno].adr = UDPbuffer[0x06];
        for (uint8_t b = 0; b < 4; b++)
        {
          LokBuffer[Lokno].lastmfxUID[b] = UDPbuffer[0x07 + b];
        }
        Lokno++;
        if (cntLoks > Lokno)
        {
          // nächste Lok abrufen
          produceFrame(M_SENDLOKBUFFER);
          M_PATTERN[5] = Lokno;
          sendOutGW(M_PATTERN, toGW);
        }
      }
      break;
    case restartBridge:
      ESP.restart();
      break;
    }
  }
}

// sendet CAN-Frames vom SYS zum CAN (Gleisbox)
void proc_fromUDP2CAN()
{
  uint8_t UDPbuffer[CAN_FRAME_SIZE]; // buffer to hold incoming packet
  uint8_t M_PING_RESPONSEx[] = {0x00, 0x30, 0x00, 0x00, 0x00};
  int packetSize = UdpINSYS.parsePacket();
  // if there's data available, read a packet
  if (packetSize)
  {
    // read the packet into packetBufffer
    UdpINSYS.read(UDPbuffer, CAN_FRAME_SIZE);
    // send received data via usb and CAN
    if (UDPbuffer[0x01] == SYS_CMD && UDPbuffer[0x09] == SYS_GO)
      set_SYSseen(true);
    proc2CAN(UDPbuffer, fromUDP2CAN);
    switch (UDPbuffer[0x01])
    {
    case PING:
      produceFrame(M_CAN_PING_CS2); //% M_CAN_PING_CS2_2
      sendOutUDP(M_PATTERN);
      produceFrame(M_CAN_PING_CS2_2);
      sendOutUDP(M_PATTERN);
      set_SYSseen(true);
      break;
    case PING_R:
      if ((UDPbuffer[11] == 0xEE) && (UDPbuffer[12] == 0xEE))
      {
        proc2CAN(UDPbuffer, fromUDP2CAN);
        delay(100);
        memcpy(UDPbuffer, M_PING_RESPONSEx, 5);
        sendOutUDP(UDPbuffer);
        //        proc2CAN(UDPbuffer, fromUDP2CAN);
        delay(20);
      }
      produceFrame(M_CAN_PING_CS2_1);
      sendOutUDP(M_PATTERN);
      produceFrame(M_CAN_PING_CS2_2);
      sendOutUDP(M_PATTERN);
      break;
    case Lok_Speed:
    case Lok_Direction:
    case Lok_Function:
      // send received data via wifi to clients
      proc2Clnts(UDPbuffer, fromUDP2CAN);
      break;
    case SWITCH_ACC:
      // send received data via wifi to clients
      proc2Clnts(UDPbuffer, toClnt);
      break;
    case S88_Polling:
      UDPbuffer[0x01]++;
      UDPbuffer[0x04] = 7;
      sendOutUDPfromCAN(UDPbuffer);
      break;
    case S88_EVENT:
      UDPbuffer[0x01]++;
      UDPbuffer[0x09] = 0x00; // is free
      UDPbuffer[0x04] = 8;
      sendOutUDPfromCAN(UDPbuffer);
      break;
    }
  }
}

void proc_fromTCPServer()
{
  uint16_t TCPbufferUsed = 0;
  uint8_t *TCPbuffer = NULL; // Pointer to int, initialize to nothing.
  uint16_t availableChars = TCPclient.available();
  if (availableChars > 0)
  {
    TCPbuffer = new uint8_t[availableChars]; // Allocate size ints and save ptr in buffer
  }
  while (availableChars)
  {
    yield();
    TCPbuffer[TCPbufferUsed] = TCPclient.read();
    TCPbufferUsed++;
    availableChars--;
  }
  if ((TCPbufferUsed % CAN_FRAME_SIZE) == 0)
    for (uint16_t l = 0; l < TCPbufferUsed; l += CAN_FRAME_SIZE)
    {
      // den nächsten freien Arrayplatz suchen
      uint8_t p;
      for (p = 0; p < maxPackets; p++)
      {
        if (arrayTCPFramesUsed[p] == 0)
        {
          arrayTCPFramesUsed[p] = 1;
          cntTCPFramesUsed++;
          break;
        }
      }
      memcpy(&arrayTCPFrames[p][0], &TCPbuffer[l], CAN_FRAME_SIZE);
    }
  delete[] TCPbuffer;
}

void proc_onTCP()
{
  uint8_t TCPbuffer[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  if (cntTCPFramesUsed > 0)
  {
    // den nächsten belgten Arrayplatz suchen
    uint8_t p;
    for (p = 0; p < maxPackets; p++)
    {
      if (arrayTCPFramesUsed[p] != 0)
      {
        arrayTCPFramesUsed[p] = 0;
        cntTCPFramesUsed--;
        break;
      }
    }
    memcpy(&TCPbuffer[0], &arrayTCPFrames[p][0], CAN_FRAME_SIZE);
    analyseTCP(TCPbuffer);
  }
}

// sendet CAN-Frames vom  CAN (Gleisbox) zum SYS
void proc_fromCAN2SYSandGW()
{
#define CAN_EFF_MASK 0x1FFFFFFFU /* extended frame format (EFF) */
  can_message_t MessageReceived;
#ifdef debug
  uint32_t previousMillis = millis();
#endif
  if (can_receive(&MessageReceived, portWAIT) == ESP_OK)
  {
    // read a packet from CAN
    uint8_t UDPbuffer[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    MessageReceived.identifier &= CAN_EFF_MASK;
    MessageReceived.identifier = htonl(MessageReceived.identifier);
    memcpy(UDPbuffer, &MessageReceived.identifier, 4);
    UDPbuffer[4] = MessageReceived.data_length_code;
    memcpy(&UDPbuffer[5], MessageReceived.data, MessageReceived.data_length_code);
#ifdef debug
    Serial.print("-->o ");
    Serial.print(UDPbuffer[0], HEX);
    Serial.print(UDPbuffer[1], HEX);
    Serial.print(" ");
    Serial.print(UDPbuffer[2], HEX);
    Serial.print(UDPbuffer[3], HEX);
    Serial.print(" (" + String(UDPbuffer[4]) + ") ");
    for (size_t i = 0; i < 8; i++)
    {
      Serial.print(UDPbuffer[i + 5], HEX);
      Serial.print("-");
    }
    Serial.println("T: " + String(millis() - previousMillis));
#endif
    // now dispatch
    switch (UDPbuffer[0x01])
    {
    case PING: // PING
      sendOutGW(UDPbuffer, fromCAN);
      break;
    case PING_R: // PING
      sendOutGW(UDPbuffer, fromCAN);
      sendOutUDPfromCAN(UDPbuffer);
      sendOutTCPfromCAN(UDPbuffer);
      if (UDPbuffer[12] == DEVTYPE_GB && get_slaveCnt() == 0 && get_waiting4Handshake() == true)
      {
        printMSG(NoSlaves);
#ifdef OLED
        Adafruit_SSD1306 *displ = getDisplay();
        displ->println(F(" -- No Slaves!"));
        displ->display();
#endif
#ifdef LCD28
        displayLCD(" -- No Slaves!");
#endif
        goSYS();
        set_waiting4Handshake(false);
      }
      break;
    case LokDiscovery_R:
      // mfxdiscovery war erfolgreich
      if (UDPbuffer[4] == 0x05)
      {
        bindANDverify(UDPbuffer);
        // an gateway den anfang melden
      }
      break;
    case MFXVerify:
      bLokDiscovery = true;
      produceFrame(M_STARTCONFIG);
      // LocID
      M_PATTERN[6] = UDPbuffer[10];
      // MFX-UID
      memcpy(&M_PATTERN[7], lastmfxUID, 4);
      // to Gateway
      sendOutGW(M_PATTERN, fromCAN);
      cvIndex = readConfig(0);
      break;
    case ReadConfig_R:
      // Rückmeldungen von config
      sendOutGW(UDPbuffer, fromCAN);
      sendOutUDPfromCAN(UDPbuffer);
      sendOutTCPfromCAN(UDPbuffer);
      if ((UDPbuffer[10] == 0x03) && (bLokDiscovery == true))
      {
        if (UDPbuffer[11] == 0x00)
        {
          // das war das letzte Zeichen
          // an gateway den schluss melden
          bLokDiscovery = false;
          // verwendete locid, damit stellt der Server fest, ob
          // die erkannte Lok neu oder bereits bekannt war
          M_PATTERN[6] = locid;
          produceFrame(M_FINISHCONFIG);
          // to Gateway
          sendOutGW(M_PATTERN, fromCAN);
        }
        else
        {
          // to Gateway
          cvIndex = readConfig(cvIndex);
        }
      }
      break;
    // Magnetartikel schalten
    case SWITCH_ACC_R:
      /*
    Der Schaltbefehl vom Steuerungsprogramm wurde an die Decoder, aber auch
    direkt an die Gleisbox gesendet. Also antwortet auch die Gleisbox und bestätigt
    damit den Befehl, obwohl möglicherweise gar kein Decoder angeschlossen ist. Deshalb wird
    mit diesem Konstrukt die Antwort der Gleisbox unterdrückt.
    Die Bestätigungsmeldung kommt ausschließlich vom Decoder und wird unter espnow.cpp bearbeitet.

    Wird momentan nicht umgesetzt, da ältere WinDigi-Pet-Versionen die Rückmeldung von der Gleisbox erwarten!
    */
      sendOutGW(UDPbuffer, fromCAN);
      sendOutUDPfromCAN(UDPbuffer);
      sendOutTCPfromCAN(UDPbuffer);

      break;
    case WriteConfig_R:
      sendOutGW(UDPbuffer, fromCAN);
      sendOutUDPfromCAN(UDPbuffer);
      sendOutTCPfromCAN(UDPbuffer);
      break;
    default:
      sendOutUDPfromCAN(UDPbuffer);
      sendOutTCPfromCAN(UDPbuffer);
      break;
    }
  }
}

String findGB(uint8_t gb)
{
  String str = "";
  uint8_t GBbufferOut[] = {0x00, LoadCS2Data, 0x30, 0x00, 0x08, 0x67, 0x62, 0x73, 0x2B, 0x00, 0x00, 0x00, 0x00};
  if (gb < 10)
    GBbufferOut[0x09] = 0x30 + gb; // '0' ff
  else
  {
    GBbufferOut[0x09] = 0x31;             // '1'
    GBbufferOut[0x0A] = 0x30 + (gb - 10); // '0' ff
  }
  sendOutGW(GBbufferOut, toGW);
  char *GBbufferIn = NULL;
  uint8_t packetSize = 0;
  while (packetSize == 0)
  {
    yield();
    packetSize = UdpINGW.parsePacket();
  }
  if (packetSize)
  {
    GBbufferIn = new char[packetSize + 1]; // Allocate size ints and save ptr in buffer
    memset(&GBbufferIn, 0x00, packetSize + 1);
    // read the packet into packetBuffer
    // from gateway via udp
    UdpINGW.read(GBbufferIn, packetSize);
    uint8_t s;
    for (s = 0; s < packetSize; s++)
    {
      str += GBbufferIn[s];
    }
    delete[] GBbufferIn; // Free memory allocated for the buffer array.
    GBbufferIn = NULL;   // Be sure the deallocated memory isn't used.
  }
  return str;
}

void onRequest(AsyncWebServerRequest *request)
{
  for (uint8_t p = 0; p < maxPackets; p++)
    if (arrayURL[p] == "")
    {
      arrayURL[p] = request->url();
      arrayRequest[p] = request;
      cntURLUsed++;
      break;
    }
}

#include "icons/train.h"
#include "icons/bus.h"
#include "icons/lkw.h"

void analyseHTTP()
{
  if (cntURLUsed == 0)
    return;
  bool urlfound;
  bool ffound;
  uint8_t fNmbr = 0;
  String currentLine;
  AsyncWebServerRequest *request;
  urlfound = false;
  for (uint8_t p = 0; p < maxPackets; p++)
    if (arrayURL[p] != "")
    {
      currentLine = arrayURL[p];
      cntURLUsed--;
      arrayURL[p] = "";
      request = arrayRequest[p];
      urlfound = true;
      break;
    }
  if (urlfound == false)
    return;
  ffound = false;
  currentLine.toLowerCase();
  for (fNmbr = 0; fNmbr < cntCS2Files; fNmbr++)
  {
    if (currentLine.indexOf(cs2Files[fNmbr][1]) > 0)
    {
      ffound = true;
      break;
    }
  }
  if (ffound == false)
  {
    if (currentLine.indexOf("gleisbilder") > 0)
    {
      for (uint8_t gb = 0; gb < 19; gb++)
      {
        if (currentLine.indexOf(findGB(gb)) > 0)
        {
          gbsFile[0x03] = 0x2D;      // '-'
          gbsFile[0x04] = 0x30 + gb; // '0'
          fNmbr = cntCS2Files;
          ffound = true;
          break;
        }
      }
    }
  }
  if (ffound == false)
  {
    if (currentLine.indexOf(".png") > 0)
    {
      /*  File root = SPIFFS.open("/");
      File file = root.openNextFile();
      while (file)
      {
        String fn(file.name());
        telnetClient.printTelnet(true, "FILE: " + fn, indent);
        file = root.openNextFile();
      }*/
      AsyncWebServerResponse *response;
      if (currentLine.indexOf("bus") > 0)
        response = request->beginResponse_P(200, "image/png", bus_ico, bus_ico_len);
      else if (currentLine.indexOf("lkw") > 0)
        response = request->beginResponse_P(200, "image/png", lkw_ico, lkw_ico_len);
      else
        response = request->beginResponse_P(200, "image/png", train_ico, train_ico_len);
      request->send(response);
      return;
    }
  }
  if (ffound)
  {
    set_SYSseen(true);
    if (initialDataAlreadySent == false)
    {
      initialDataAlreadySent = true;
      uint8_t no_slv = get_slaveCnt();
      for (uint8_t slv = 0; slv < no_slv; slv++)
      {
        set_initialData2send(slv);
      }
    }
    produceFrame(M_DONOTCOMPRESS);
    sendOutGW(M_PATTERN, toGW);
    receiveLocFile(fNmbr, false);
    String fName;
    if (fNmbr == cntCS2Files)
    {
      // gbs-X
      String fn(gbsFile);
      fName = "/" + fn;
    }
    else
      // sonst
      fName = "/" + cs2Files[fNmbr][1];
    telnetClient.printTelnet(true, "Sendet per HTTP: " + request->url());
    request->send(SPIFFS, fName, "text/html");
    return;
  }
  // Handle Unknown Request
  request->send(404, "text/plain", "Not found: " + currentLine);
  telnetClient.printTelnet(true, "Not found: " + currentLine, indent);
}

//////////////// Startroutine

// Standardroutine; diverse Anfangsbedingungen werden hergestellt
void setup()
{
  Serial.begin(bdrMonitor);
  //  Serial.setDebugOutput(true);
  Serial.println("\r\n\r\nC A N g u r u - B r i d g e - " + CgVersionnmbr);
  drawCircle = false;
#ifdef OLED
  // das Display wird initalisiert
  initDisplay_OLED();
  // das Display zeigt den Beginn der ersten Routinen
  Adafruit_SSD1306 *displ = getDisplay();
  displ->display();
#endif
#ifdef LCD28
  // das Display wird initalisiert
  initDisplayLCD28();
#endif
  // noch nicht nach Slaves scannen
  set_time4Scanning(false);
  // der Timer wird initialisiert
  stillAliveBlinkSetup();
  // start the CAN bus at 250 kbps
  setup_can_driver();
  // ESPNow wird initialisiert
  espInit();
  // das gleiche mit ETHERNET
  WiFi.onEvent(WiFiEvent);
  ETH.begin();
  // Variablen werden gesetzt
  set_SYSseen(false);
  set_cntConfig();
  bLokDiscovery = false;
  locofileread = false;
  initialDataAlreadySent = false;
  locid = 1;
  // start the telnetClient
  telnetClient.telnetInit();
  // This initializes udp and transfer buffer
  if (UdpINSYS.begin(localPortinSYS) == 0)
  {
#ifdef OLED
    displ->println(F("ERROR inSYS"));
    displ->display();
#endif
#ifdef LCD28
#endif
  }
  if (UdpINGW.begin(localPortinGW) == 0)
  {
#ifdef OLED
    displ->println(F("ERROR inGW"));
    displ->display();
#endif
#ifdef LCD28
    displayLCD("ERROR inSYS");
#endif
  }
  if (UdpOUTGW.begin(localPortoutGW) == 0)
  {
#ifdef OLED
    displ->println(F("ERROR outSYS"));
    displ->display();
#endif
#ifdef LCD28
    displayLCD("ERROR inGW");
#endif
  }
  if (UdpOUTSYS.begin(localPortoutSYS) == 0)
  {
#ifdef OLED
    displ->println(F("ERROR outGW"));
    displ->display();
#endif
#ifdef LCD28
    displayLCD("ERROR outGW");
#endif
  }
  // start the TCP-server
  TCPINSYS.begin();
  //
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  if (!SPIFFS.format())
  {
    Serial.println("An Error has occurred while formatting SPIFFS");
    return;
  }
  //
  // Catch-All Handlers
  // Any request that can not find a Handler that canHandle it
  // ends in the callbacks below.
  AsyncWebSrvr.onNotFound(onRequest);
  // start the http-server
  AsyncWebSrvr.begin();
  //
  // app starts here
  //
}

// damit wird die Gleisbox zum Leben erweckt
void send_start_60113_frames()
{
  produceFrame(M_GLEISBOX_MAGIC_START_SEQUENCE);
  proc2CAN(M_PATTERN, toCAN);
  produceFrame(M_GLEISBOX_ALL_PROTO_ENABLE);
  proc2CAN(M_PATTERN, toCAN);
}

uint8_t getLocID()
{
  bool found = false;
  // noch keine Lok registriert ?
  if (cntLoks == 0)
    return locid;
  // vergleiche die erkannte mit allen bekannten Loks
  for (uint8_t lok = 0; lok < cntLoks; lok++)
  {
    for (uint8_t b = 0; b < 4; b++)
    {
      found = LokBuffer[lok].lastmfxUID[b] == lastmfxUID[b];
    }
    // wenn schon bekannt, nimmt deren Adresse
    if (found)
    {
      return LokBuffer[lok].adr;
    }
  }
  return locid;
}
// wird für die Erkennung von mfx-Loks gebraucht
void bindANDverify(uint8_t *buffer)
{
  locid = getLocID();
  // BIND
  produceFrame(M_BIND);
  M_PATTERN[10] = locid;
  // MFX-UID
  memcpy(lastmfxUID, &buffer[5], 4);
  memcpy(&M_PATTERN[5], lastmfxUID, 4);
  proc2CAN(M_PATTERN, toCAN);
  delay(10);
  // VERIFY
  produceFrame(M_VERIFY);
  M_PATTERN[10] = locid;
  // MFX-UID
  memcpy(&M_PATTERN[5], lastmfxUID, 4);
  proc2CAN(M_PATTERN, toCAN);
  delay(10);
  // FUNCTION
  produceFrame(M_FUNCTION);
  M_PATTERN[8] = locid;
  M_PATTERN[10] = 0x01;
  proc2CAN(M_PATTERN, toCAN);
  produceFrame(M_FUNCTION);
  M_PATTERN[8] = locid;
  M_PATTERN[10] = 0x00;
  delay(10);
  proc2CAN(M_PATTERN, toCAN);
}

// Auswertung der config-Datei
char readConfig(char index)
{
  produceFrame(M_READCONFIG);
  M_PATTERN[8] = locid;
  M_PATTERN[9] = index;
  M_PATTERN[9] += 0x04;
  M_PATTERN[10] = 0x03;
  M_PATTERN[11] = 0x01;
  proc2CAN(&M_PATTERN[0], toCAN);
  delay(5);
  return M_PATTERN[9];
}

// Anfrage, wieviele Daten sind zu übertragen
uint32_t getDataSize(uint8_t f)
{
  uint8_t UDPbuffer[] = {0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  // vorsichtshalber die deflated sizes auf null setzen
  memset(&cs2FileSizes[f][0], 0x00, cntCS2Sizes);
  produceFrame(M_GETCONFIG1);
  if (f == cntCS2Files)
    // gbs-X
    memcpy(&M_PATTERN[0x05], gbsFile, 8);
  else
    // sonst
    memcpy(&M_PATTERN[0x05], &cs2Files[f][0], cs2Files[f][0].length()); // 2transfer);
  // to Gateway
  sendOutGW(M_PATTERN, toGW);
  uint16_t packetSize = 0;
  //  uint8_t loctimer = secs;
  // maximal 2 Sekunden warten
  while (packetSize == 0) // && secs < loctimer + 2)
  {
    yield();
    packetSize = UdpINGW.parsePacket();
  }
  if (packetSize)
  {
    // read the packet into packetBufffer
    UdpINGW.read(UDPbuffer, CAN_FRAME_SIZE);
    if (UDPbuffer[0x01] == GETCONFIG_RESPONSE)
    {
      // size - crc
      // byte 5 bis 8 ist die Anzahl der zu übertragenden bytes
      // byte 9 und 10 ist der crc-Wert
      // crc ist für uncompressed immer null
      //
      memcpy(&cs2FileSizes[f][0], &UDPbuffer[5], cntCS2Sizes);
      uint32_t cF;
      memcpy(&cF, &UDPbuffer[5], 4);
      return ntohl(cF);
    }
  }
  return 0;
}

// Anfordrung an den CANguru-Server zur Übertragung der lokomotive.cs2-Datei
void ask4CS2Data(byte lineNo, uint8_t f)
{
  //
  produceFrame(M_GETCONFIG2);
  // LoadCS2Data_R  0x57
  if (f == cntCS2Files)
    // gbs-X
    memcpy(&M_PATTERN[0x05], gbsFile, 8);
  else
    // sonst
    memcpy(&M_PATTERN[0x05], &cs2Files[f][0], shortnameLng2transfer);
  M_PATTERN[0x0A] = lineNo;
  M_PATTERN[0x0B] = httpBufferLength & 0x00FF;
  M_PATTERN[0x0C] = (httpBufferLength & 0xFF00) >> 8;
  // 5 6 7 8 9 A B C
  // 1 2 3 4 5 Y X X
  // Y - lineNo
  // XX - Pufferlänge
  // to Gateway
  sendOutGW(M_PATTERN, toGW);
}

// Übertragung der lokomotive.cs2-Datei vom CANguru-Server
// die CANguru-Bridge hält diese Daten und leitet sie bei Anfrage an
// den SYS weiter
void receiveLocFile(uint8_t f, bool cmprssd)
{
  // ruft Daten ab
  String fName;
  uint32_t cntFrame = getDataSize(f);
  if (cntFrame == 0)
  {
    locofileread = false;
    return;
  }
  if (f == cntCS2Files)
  {
    String fn(gbsFile);
    fName = "/" + fn;
  }
  else
    fName = "/" + cs2Files[f][1];
  if (cmprssd)
    fName += "cmprssd";
  if (SPIFFS.exists(fName))
  {
    if (!SPIFFS.remove(fName))
      Serial.println("Did NOT remove " + fName);
  }
  locofile = SPIFFS.open(fName, FILE_WRITE);
  // Configdaten abrufen
  uint16_t packetSize = 0;
  uint16_t inBufferCnt = 0;
  byte lineNo = 0;
  //
  locofileread = true;
  while (inBufferCnt < cntFrame)
  {
    ask4CS2Data(lineNo, f);
    packetSize = 0;
    while (packetSize == 0)
    {
      yield();
      packetSize = UdpINGW.parsePacket();
    }
    if (packetSize)
    {
      // read the packet into packetBuffer
      // from gateway via udp
      UdpINGW.read(httpBuffer, packetSize);
      uint16_t pOUT = 0;
      for (uint16_t pIN = 0; pIN < packetSize; pIN++)
      {
        if (httpBuffer[pIN] != 0x0D)
        {
          httpBuffer[pOUT] = httpBuffer[pIN];
          pOUT++;
        }
      }
      httpBuffer[pOUT] = 0x00;
      lineNo++;
      // write the packet to local file
      if (!locofile.write(httpBuffer, pOUT))
      {
        Serial.println(fName + " write failed");
        locofile.close();
        locofileread = false;
        return;
      }
      inBufferCnt += packetSize;
    }
  }
  locofile.close();
}

// die Routin antwortet auf die Anfrage des CANguru-Servers mit CMD 0x88;
// damit erhält er die IP-Adresse der CANguru-Bridge und kann dann
// damit eine Verbindung aufbauen
void proc_IP2GW()
{
  uint8_t UDPbuffer[CAN_FRAME_SIZE]; // buffer to hold incoming packet,
  if (telnetClient.getIsipBroadcastSet())
    return;
  yield();
  int packetSize = UdpINGW.parsePacket();
  // if there's data available, read a packet
  if (packetSize)
  {
    // read the packet into packetBuffer
    UdpINGW.read(UDPbuffer, CAN_FRAME_SIZE);
    // send received data via ETHERNET
    switch (UDPbuffer[0x1])
    {
    case CALL4CONNECT:
      ipGateway = telnetClient.setipBroadcast(UdpINGW.remoteIP());
      UDPbuffer[0x1]++;
      sendOutGW(UDPbuffer, toGW);
      break;
    }
  }
}

void sendTCPConfigData(uint8_t f, uint8_t *tcb)
{
  uint8_t TCPbuffer[] = {0x00, ConfigDataStream, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  uint8_t *TCPbufferLng = NULL;
  String fName;
  TCPbufferLng = new uint8_t[maxPackets * CAN_FRAME_SIZE]; // Allocate size ints and save ptr in buffer
  if (TCPbufferLng == NULL)
    Serial.println("No MEMORY!!!");
  if (f == cntCS2Files)
  {
    String fn(gbsFile);
    fName = "/" + fn;
  }
  else
    fName = "/" + cs2Files[f][1];
  fName += "cmprssd";
  locofile = SPIFFS.open(fName);
  if (!locofile)
  {
    Serial.println("Failed to open file for reading");
    return;
  }
  size_t size = locofile.size();
  uint8_t *filebuffer = NULL;     // Pointer to int, initialize to nothing.
  filebuffer = new uint8_t[size]; // Allocate size ints and save ptr in buffer
  while (locofile.available())
  {
    // write the file into the buffer
    locofile.read(filebuffer, size);
  }
  locofile.close();
  // TCPbuffer vorbereiten
  // zunächst die Antwort
  memcpy(&TCPbuffer[2], &tcb[2], 2);
  tcb[1] |= 1;
  memcpy(&TCPbufferLng[0], &tcb[0], CAN_FRAME_SIZE);
  // jetzt die Ankündigung zur Übertragung
  // CAN DLC is 6
  TCPbuffer[4] = 0x06;
  memcpy(&TCPbuffer[5], &cs2FileSizes[f][0], cntCS2Sizes);
  memcpy(&TCPbufferLng[CAN_FRAME_SIZE], &TCPbuffer[0], CAN_FRAME_SIZE);
  writeTCP(TCPbufferLng, 2 * CAN_FRAME_SIZE);
  // loop until all packets send
  // CAN DLC is always 8
  uint16_t src_ptr = 0;
  uint8_t loc_ptr = 0;
  uint8_t packet_nbr = 0;
  uint16_t dest_ptr = 0;
  TCPbuffer[0x04] = 0x08;
  do
  {
    do
    {
      // prepare TCPbuffer
      // delete old data
      loc_ptr = 0x05;
      memset(&TCPbuffer[5], 0x0, 8);
      // copy new data
      memcpy(&TCPbuffer[loc_ptr], &filebuffer[src_ptr], 8);
      // copy TCPbuffer into TCPbufferLng
      memcpy(&TCPbufferLng[dest_ptr], &TCPbuffer[0], CAN_FRAME_SIZE);
      dest_ptr += CAN_FRAME_SIZE;
      src_ptr += 8;
      packet_nbr++;
    } while ((src_ptr < size) && (packet_nbr < maxPackets));
    writeTCP(TCPbufferLng, dest_ptr);
    dest_ptr = 0;
    packet_nbr = 0;
  } while (src_ptr < size);
  delete[] filebuffer;   // Free memory allocated for the buffer array.
  filebuffer = NULL;     // Be sure the deallocated memory isn't used.
  delete[] TCPbufferLng; // Free memory allocated for the buffer array.
  TCPbufferLng = NULL;   // Be sure the deallocated memory isn't used.
}

void analyseTCP(uint8_t *buffer)
{
  char configName[shortnameLng];
  char frameName[shortnameLng];
  String fileName = "";
  bool found = false;

  // print_can_frame(2, buffer);

  switch (buffer[0x01])
  {
  case Lok_Speed:
  case Lok_Direction:
  case Lok_Function:
  case SWITCH_ACC:
    // send received data via wifi to clients
    proc2Clnts(buffer, fromTCP2Clnt);
    proc2CAN(buffer, fromTCP2CAN);
    sendOutUDPfromCAN(buffer);
    break;
  case ConfigDataCompressed: // config data
    uint8_t f;
    //    sendOutTCP(buffer);
    memset(frameName, 0x00, shortnameLng);
    uint8_t p0 = 0;
    while (buffer[5 + p0] != 0x00)
    {
      frameName[p0] = buffer[5 + p0];
      p0++;
    }
    if (buffer[8] == '-')
    {
      memcpy(gbsFile, &buffer[5], 8);
      f = cntCS2Files;
      fileName = String(frameName);
      found = true;
    }
    else
    {
      for (f = 0; f < cntCS2Files; f++)
      {
        memset(configName, 0x00, shortnameLng);
        uint8_t p1 = 0;
        while ((cs2Files[f][0][p1] != 0x20) && (p1 < shortnameLng))
        {
          configName[p1] = cs2Files[f][0][p1];
          p1++;
        }
        if (memcmp(frameName, configName, shortnameLng) == 0)
        {
          fileName = cs2Files[f][1];
          found = true;
          break;
        }
      }
    }
    if (found)
    {
      produceFrame(M_DOCOMPRESS);
      sendOutGW(M_PATTERN, toGW);
      receiveLocFile(f, true);
      sendTCPConfigData(f, buffer);
    }
    else
    {
      buffer[0x01]++;
      sendOutTCP(buffer);
      sendOutUDPfromCAN(buffer);
      proc2CAN(buffer, fromTCP2CAN);
    }
    break;
  }
}

void proc_startTCPServer()
{
  if (TCPclient)
    return;
  TCPclient = TCPINSYS.available();
  if (TCPclient)
  {
    produceFrame(M_CAN_PING);
    // to TCP
    sendOutTCP(M_PATTERN);
    proc2CAN(M_PATTERN, fromTCP2CAN);
    set_SYSseen(true);
  }
}

// Antwort auf die PING-Anfrage
void proc_PING()
{
  if (get_gotPINGmsg())
  {
    // der erste PING kommt von OnDataRecv nachdem alle Clients registriert sind
    set_gotPINGmsg(false);
    //  secs = 0;
    produceFrame(M_CAN_PING);
    //    proc2CAN(M_PATTERN, toCAN);
    proc2Clnts(M_PATTERN, toClnt);
    sendOutTCP(M_PATTERN);
  }
}

// Start des Telnet-Servers
void startTelnetserver()
{
  if (telnetClient.getTelnetHasConnected() == false)
  {
    if (telnetClient.startTelnetSrv())
    {
      delay(5);
      send_start_60113_frames();
      // erstes PING soll schnell kommen
      secs = wait_for_ping;
      set_time4Scanning(true);
    }
  }
}

// Meldung, dass SYS gestartet werden kann
void goSYS()
{
  printMSG(StartTrainApplication);
  drawCircle = true;
}

void proc_start_lokBuffer()
{
  if (get_sendLokBuffer())
  {
    produceFrame(M_CNTLOKBUFFER);
    sendOutGW(M_PATTERN, toGW);
  }
}

// standard-Hauptprogramm
void loop()
{
  // die folgenden Routinen werden ständig aufgerufen
  stillAliveBlinking();
  espNowProc();

  proc_IP2GW();
  if (drawCircle)
    // diese nur, wenn drawRect wahr ist (nicht zu Beginn des Programmes)
    fillTheCircle();
  if (getEthStatus() == true)
  {
    // diese nur nach Aufbau der ETHERNET-Verbindung
    proc_start_lokBuffer();
    startTelnetserver();
    proc_PING();
    proc_fromCAN2SYSandGW();
    proc_fromUDP2CAN();
    proc_startTCPServer();
    proc_fromTCPServer();
    proc_onTCP();
    proc_fromGW2CANandClnt();
    analyseHTTP();
  }
}
