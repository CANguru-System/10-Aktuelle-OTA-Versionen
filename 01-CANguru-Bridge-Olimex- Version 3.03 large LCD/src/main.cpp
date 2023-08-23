#include <Arduino.h>

#include "CANguruDefs.h"
#include "CANguru.h"
#include <CAN_const.h>
#include "driver/gpio.h"
#include "driver/twai.h"
#include "MOD-LCD.h"
#include <espnow.h>
#include <ETH.h>
#include <Adafruit_GFX.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"

enum enum_canguruStatus
{
  /*00*/ waiting4server,
  /*01*/ startTelnetServer,
  /*02*/ startGleisbox,
  /*03*/ startScanning,
  /*04*/ stopScanning,
  /*05*/ wait4slaves,
  /*06*/ wait4lokbuffer,
  /*07*/ wait4ping,
  systemIsRunning
};
enum_canguruStatus canguruStatus;

// buffer for receiving and sending data
uint8_t M_PATTERN[] = {0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t lastmfxUID[] = {0x00, 0x00, 0x00, 0x00};

#define httpBufferLength 0x03FF
uint8_t httpBuffer[httpBufferLength + 10];
File locofile;

String lokofileName = "lokomotive.cs2";
//    {"loks   ", "lokomotive.cs2"},
String lokofileNameShortName = "loks   ";
String geraetfileName = "geraet.vrs";
//    {"ger    ", "geraet.vrs"},
String geraetfileNameShortName = "ger    ";

uint8_t cntLoks;
struct LokBufferType
{
  uint8_t lastmfxUID[4];
  uint8_t adr;
};
LokBufferType *LokBuffer = NULL;

const uint8_t maxPackets = 30;
bool bLokDiscovery;

//% bool initialDataAlreadySent;
byte locid;
bool scanningFinished;
bool allLoksAreReported;
byte cvIndex;

canguruETHClient telnetClient;
const uint8_t indent = 10;

// Forward declarations
//
void sendToServer(uint8_t *buffer, CMD dest);
void sendToWDPfromCAN(uint8_t *buffer);
//

void printCANFrame(uint8_t *buffer, CMD dir)
{
  // Diese Prozedur könnte mehrere Male aufgerufen werden;
  // deshalb wird die Erhöhung begrenzt
  if (buffer[Framelng] <= 0x08)
    buffer[Framelng] += 0x0F;
  sendToServer(buffer, dir);
}

// sendet den CAN-Frame buffer über den CAN-Bus an die Gleisbox
void proc2CAN(uint8_t *buffer, CMD dir)
{
  twai_message_t Message2Send;
  // CAN uses (network) big endian format
  // Maerklin TCP/UDP Format: always 13 (CAN_FRAME_SIZE) bytes
  //   byte 0 - 3  CAN ID
  //   byte 4      DLC
  //   byte 5 - 12 CAN data
  //
  memset(&Message2Send, 0, CAN_FRAME_SIZE);
  Message2Send.rtr = CAN_MSG_FLAG_RTR_;
  Message2Send.ss = CAN_MSG_FLAG_SS_;
  Message2Send.extd = CAN_MSG_FLAG_EXTD_;
  memcpy(&Message2Send.identifier, buffer, 4);
  Message2Send.identifier = ntohl(Message2Send.identifier);
  // Anzahl Datenbytes
  Message2Send.data_length_code = buffer[4];
  // Datenbytes
  if (Message2Send.data_length_code > 0)
    memcpy(&Message2Send.data, &buffer[5], Message2Send.data_length_code);
  if (twai_transmit(&Message2Send, 3 * portTICK_PERIOD_MS) == ESP_FAIL)
    log_e("Failed to queue message for transmission\n");
  printCANFrame(buffer, dir);
}

void setup_can_driver()
{
  // start CAN Module
  // Initialize configuration structures using macro initializers
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_5, GPIO_NUM_35, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  // Install CAN driver
  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_FAIL)
    log_e("Failed to install driver\n");

  // Start CAN driver
  if (twai_start() == ESP_FAIL)
    log_e("Failed to start driver\n");
}

void printMSG(uint8_t no)
{
  uint8_t MSG[] = {0x00, no, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  sendToServer(MSG, MSGfromBridge);
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
  case M_GETCONFIG:
    M_PATTERN[1] = LoadCS2Data;
    M_PATTERN[4] = 0x08;
    M_PATTERN[5] = 0x6C;
    M_PATTERN[6] = 0x6F;
    M_PATTERN[7] = 0x6B;
    M_PATTERN[8] = 0x73;
    break;
  case M_GETCONFIG_R:
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
  case M_CALL4CONNECTISDONE:
    M_PATTERN[1] = CALL4CONNECT + 1;
    M_PATTERN[4] = 0x00;
    break;
  }
}

#include <utils.h>

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
  {
    sendTheData(ss, buffer, CAN_FRAME_SIZE);
  }
}

void sendOutClnt(uint8_t *buffer, CMD dir)
{
  log_d("sendOutClnt0: %X", buffer[0x01]);
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

void sendToServer(uint8_t *buffer, CMD dest)
{
  uint8_t RCVDbuffer[] = {0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  uint16_t packetSize = 0;
  uint8_t cmd = buffer[1];
  log_d("sendToServer: %x", cmd);
  buffer[0] = dest;
  UDPToServer.beginPacket(ipGateway, localPortToServer);
  UDPToServer.write(buffer, CAN_FRAME_SIZE);
  UDPToServer.endPacket();
  buffer[0] = 0x00;
  // auf Quittung warten
  if (dest == toServer)
  {
    while (packetSize == 0)
    {
      yield();
      packetSize = UDPFromServer.parsePacket();
    }
    if (packetSize)
    {
      // read the packet into packetBuffer
      UDPFromServer.read(RCVDbuffer, CAN_FRAME_SIZE);
      if (RCVDbuffer[CANcmd] != 0xFF || RCVDbuffer[data0] != cmd)
      {
        log_e("ERROR! Incorrect Msg RCPT: %X NEW: %X OLD: %X", RCVDbuffer[CANcmd], RCVDbuffer[data0], cmd);
      }
    }
  }
}

void msgStartScanning()
{
  printMSG(startScanningDecoder);
  log_d("Start scanning");
}

// Anfrage, wieviele Daten sind zu übertragen
uint32_t getDataSize(uint8_t f)
{
  uint8_t UDPbuffer[] = {0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  //
  produceFrame(M_GETCONFIG);
  switch (f)
  {
  case 0:
    memcpy(&M_PATTERN[0x05], &lokofileNameShortName, lokofileNameShortName.length());
    break;
  case 1:
    memcpy(&M_PATTERN[0x05], &geraetfileNameShortName, geraetfileNameShortName.length());
    break;
  }
  // to Gateway
  sendToServer(M_PATTERN, toUDP);
  delay(wait_time_small);
  uint16_t packetSize = 0;
  // maximal 2 Sekunden warten
  while (packetSize == 0)
  {
    yield();
    packetSize = UDPFromServer.parsePacket();
  }
  if (packetSize)
  {
    // read the packet into packetBufffer
    UDPFromServer.read(UDPbuffer, CAN_FRAME_SIZE);
    if (UDPbuffer[0x01] == GETCONFIG_RESPONSE)
    {
      // size - crc
      // byte 5 bis 8 ist die Anzahl der zu übertragenden bytes
      // byte 9 und 10 ist der crc-Wert
      // crc ist für uncompressed immer null
      //
      uint32_t cF = 0;
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
  produceFrame(M_GETCONFIG_R);
  switch (f)
  {
  case 0:
    memcpy(&M_PATTERN[0x05], &lokofileNameShortName, lokofileNameShortName.length());
    break;
  case 1:
    memcpy(&M_PATTERN[0x05], &geraetfileNameShortName, geraetfileNameShortName.length());
    break;
  }
  // LoadCS2Data_R  0x57
  M_PATTERN[0x0A] = lineNo;
  M_PATTERN[0x0B] = httpBufferLength & 0x00FF;
  M_PATTERN[0x0C] = (httpBufferLength & 0xFF00) >> 8;
  // 5 6 7 8 9 A B C
  // 1 2 3 4 5 Y X X
  // Y - lineNo
  // XX - Pufferlänge
  // to Gateway
  sendToServer(M_PATTERN, toUDP);
}

// Übertragung der lokomotive.cs2-Datei vom CANguru-Server
// die CANguru-Bridge hält diese Daten und leitet sie bei Anfrage an
// den WDP weiter
bool loadCS2LocFile(uint8_t f)
{
  String fName = "";
  switch (f)
  {
  case 0:
    fName = "/" + lokofileName;
    break;
  case 1:
    fName = "/" + geraetfileName;
    break;
  }
  produceFrame(M_DONOTCOMPRESS);
  sendToServer(M_PATTERN, toUDP);
  // ruft Daten ab
  uint32_t cntFrame = getDataSize(f);
  if (cntFrame == 0)
  {
    return false;
  }
  if (LittleFS.exists(fName))
  {
    log_i("LittleFS exists: %s", fName);
    return true;
    if (!LittleFS.remove(fName))
      log_d("Did NOT remove %s", fName);
  }
  locofile = LittleFS.open(fName, FILE_WRITE);
  // Configdaten abrufen
  uint16_t packetSize = 0;
  uint16_t inBufferCnt = 0;
  byte lineNo = 0;
  //
  while (inBufferCnt < cntFrame)
  {
    ask4CS2Data(lineNo, f);
    packetSize = 0;
    while (packetSize == 0)
    {
      yield();
      packetSize = UDPFromServer.parsePacket();
    }
    if (packetSize)
    {
      // read the packet into packetBuffer
      // from gateway via udp
      UDPFromServer.read(httpBuffer, packetSize);
      uint16_t pOUT = 0;
      for (uint16_t pIN = 0; pIN < packetSize; pIN++)
      {
        if (httpBuffer[pIN] != 0x0D)
        {
          httpBuffer[pOUT] = httpBuffer[pIN];
          pOUT++;
        }
      }
      httpBuffer[packetSize] = 0x00;
      lineNo++;
      // write the packet to local file
      if (!locofile.write(httpBuffer, pOUT))
      {
        log_d("%s write failed", fName);
        locofile.close();
        return false;
      }
      inBufferCnt += packetSize;
    }
  }
  locofile.close();
  log_i("LittleFS read: %s", fName);
  return true;
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

void analyseHTTP()
{
  if (cntURLUsed == 0)
    return;
  bool urlfound;
  bool ffound;
  uint8_t fNmbr = 255;
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
  log_d("http: %s", currentLine);
  currentLine.toLowerCase();
  if (currentLine.indexOf(lokofileName) > 0)
  {
    ffound = true;
    fNmbr = 0;
  }
  else if (currentLine.indexOf(geraetfileName) > 0)
  {
    ffound = true;
    fNmbr = 1;
  }
  if (ffound)
  {
    loadCS2LocFile(fNmbr);
    String fName = "";
    switch (fNmbr)
    {
    case 0:
      fName = "/" + lokofileName;
      break;
    case 1:
      fName = "/" + geraetfileName;
      break;
    }
    telnetClient.printTelnet(true, "Sendet per HTTP: " + request->url());
    request->send(LittleFS, fName, "text/html");
    return;
  }
  // Handle Unknown Request
  request->send(404, "text/plain", "Not found: " + currentLine);
  telnetClient.printTelnet(true, "Not found: " + currentLine, indent);
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

// sendet CAN-Frames vom  CAN (Gleisbox) zum SYS
void proc_fromCAN2WDPandServer()
{
  twai_message_t MessageReceived;
  if (twai_receive(&MessageReceived, 3 * portTICK_PERIOD_MS) == ESP_OK)
  {
    // read a packet from CAN
    uint8_t UDPbuffer[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    MessageReceived.identifier &= CAN_EFF_MASK;
    MessageReceived.identifier = htonl(MessageReceived.identifier);
    memcpy(UDPbuffer, &MessageReceived.identifier, 4);
    UDPbuffer[4] = MessageReceived.data_length_code;
    memcpy(&UDPbuffer[5], MessageReceived.data, MessageReceived.data_length_code);
    // now dispatch
    log_d("proc_fromCAN2WDPandServer: %X", UDPbuffer[0x01]);
    switch (UDPbuffer[0x01])
    {
    case PING: // PING
      sendToServer(UDPbuffer, fromCAN);
      break;
    case PING_R: // PING
                 // wenn alle slaves zu Beginn des Programmes gezählt werden, ist damit die Gleisbox auch dabei
      incSlavesAreReadyToZero();
      sendToServer(UDPbuffer, fromCAN);
      sendToWDPfromCAN(UDPbuffer);
      //      sendOutTCPfromCAN(UDPbuffer);
      delay(100);
      if (UDPbuffer[12] == DEVTYPE_GB && get_slaveCnt() == 0)
      {
        printMSG(NoSlaves);
        displayLCD(" -- No Slaves!");
        goSYS();
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
      sendToServer(M_PATTERN, fromCAN);
      cvIndex = readConfig(0);
      break;
    case ReadConfig_R:
      // Rückmeldungen von config
      sendToServer(UDPbuffer, fromCAN);
      sendToWDPfromCAN(UDPbuffer);
      //      sendOutTCPfromCAN(UDPbuffer);
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
          sendToServer(M_PATTERN, fromCAN);
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
      sendToServer(UDPbuffer, fromCAN);
      sendToWDPfromCAN(UDPbuffer);
      //      sendOutTCPfromCAN(UDPbuffer);

      break;
    case WriteConfig_R:
      sendToServer(UDPbuffer, fromCAN);
      sendToWDPfromCAN(UDPbuffer);
      //      sendOutTCPfromCAN(UDPbuffer);
      break;
    default:
      sendToWDPfromCAN(UDPbuffer);
      //      sendOutTCPfromCAN(UDPbuffer);
      break;
    }
  }
}

//////////////// Ausgaberoutinen

void sendToWDP(uint8_t *buffer)
{
  UDPToWDP.beginPacket(ipGateway, localPortToWDP);
  UDPToWDP.write(buffer, CAN_FRAME_SIZE);
  UDPToWDP.endPacket();
  printCANFrame(buffer, toUDP);
}

void sendToWDPfromCAN(uint8_t *buffer)
{
  UDPToWDP.beginPacket(ipGateway, localPortToWDP);
  UDPToWDP.write(buffer, CAN_FRAME_SIZE);
  UDPToWDP.endPacket();
}

//////////////// Empfangsroutinen

// Behandlung der Kommandos, die der CANguru-Server aussendet
void proc_fromServer2CANandClnt()
{
  uint8_t Lokno;
  uint8_t UDPbuffer[CAN_FRAME_SIZE]; // buffer to hold incoming packet,
  int packetSize = UDPFromServer.parsePacket();
  // if there's data available, read a packet
  if (packetSize)
  {
    // read the packet into packetBufffer
    UDPFromServer.read(UDPbuffer, CAN_FRAME_SIZE);
    log_d("fromGW2CANandClnt: %X", UDPbuffer[0x01]);
    // send received data via ESPNOW and CAN
    switch (UDPbuffer[0x1])
    {
    case SYS_CMD:
    case 0x36:
      if (UDPbuffer[0x09] != START_OTA)
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
      sendToServer(M_PATTERN, fromCAN);
      break;
    case MfxProc_R:
      // there is a new file lokomotive.cs2 to send
      produceFrame(M_SIGNAL);
      sendToServer(M_PATTERN, fromCAN);
      log_d("fromGW2CANandClnt: loadCS2LocFile");
      loadCS2LocFile(0);
      break;
    // PING
    case PING:
      log_d("M_CAN_PING");
      produceFrame(M_CAN_PING);
      proc2CAN(M_PATTERN, fromGW2CAN);
      break;
    case sendCntLokBuffer_R:
      // Anzahl der Loks, die der Server kennt, werden gemeldet
      log_d("sendCntLokBuffer_R %x", UDPbuffer[0x05]);
      cntLoks = UDPbuffer[0x05];
      // die Anzahl an gemeldeten Loks im Server ausgeben
      char cntBuffer[25];
      sprintf(cntBuffer, "%d Lok(s) im System", cntLoks);
      telnetClient.printTelnet(true, cntBuffer, 0);
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
        sendToServer(M_PATTERN, toServer);
      }
      break;
    case sendLokBuffer_R:
      Lokno = UDPbuffer[0x05];
      log_d("sendLokBuffer_R: %x", Lokno);
      if (LokBuffer != NULL)
      {
        //        saveFrame(UDPbuffer);
        LokBuffer[Lokno].adr = UDPbuffer[0x06];
        for (uint8_t b = 0; b < 4; b++)
        {
          LokBuffer[Lokno].lastmfxUID[b] = UDPbuffer[0x07 + b];
        }
        Lokno++;
        allLoksAreReported = cntLoks == Lokno;
        if (cntLoks > Lokno)
        {
          // nächste Lok abrufen
          produceFrame(M_SENDLOKBUFFER);
          M_PATTERN[5] = Lokno;
          sendToServer(M_PATTERN, toServer);
          // das war die letzte Lok
          // der Anmeldeprozess kann weitergehen
        }
      }
      break;
    case restartBridge:
      proc2Clnts(UDPbuffer, fromGW2Clnt);
      ESP.restart();
      break;
    }
  }
}

// sendet CAN-Frames vom SYS zum CAN (Gleisbox)
void proc_fromWDP2CAN()
{
  uint8_t UDPbuffer[CAN_FRAME_SIZE]; // buffer to hold incoming packet
  uint8_t M_PING_RESPONSEx[] = {0x00, 0x30, 0x00, 0x00, 0x00};
  int packetSize = UDPFromWDP.parsePacket();
  // if there's data available, read a packet
  if (packetSize)
  {
    // read the packet into packetBufffer
    UDPFromWDP.read(UDPbuffer, CAN_FRAME_SIZE);
    // send received data via usb and CAN
    if (UDPbuffer[0x01] == SYS_CMD && UDPbuffer[0x09] == SYS_GO)
    {
      // Meldung an die Clients, dass WDP gestartet wurde
      produceFrame(M_CALL4CONNECTISDONE);
      proc2Clnts(M_PATTERN, fromGW2Clnt);
      set_SYSseen(true);
      // Schienenspannung einschalten
      produceFrame(M_GO);
      proc2Clnts(M_PATTERN, fromGW2Clnt);
    }
    proc2CAN(UDPbuffer, fromWDP2CAN);
    log_d("fromServer2CAN: %X", UDPbuffer[0x01]);
    switch (UDPbuffer[0x01])
    {
    case PING:
      produceFrame(M_CAN_PING_CS2); //% M_CAN_PING_CS2_2
      sendToWDP(M_PATTERN);
      log_d("PING");
      set_SYSseen(true);
/*      if (initialDataAlreadySent == false)
      {
        initialDataAlreadySent = true;
        uint8_t no_slv = get_slaveCnt();
        for (uint8_t slv = 0; slv < no_slv; slv++)
        {
          set_initialData2send(slv);
        }
      }*/
      //      produceFrame(M_CAN_PING_CS2_2);
      //      sendToWDP(M_PATTERN);
      set_SYSseen(true);
      break;
    case PING_R:
      if ((UDPbuffer[11] == 0xEE) && (UDPbuffer[12] == 0xEE))
      {
        delay(wait_time_small);
        memcpy(&UDPbuffer, &M_PING_RESPONSEx, 5);
        sendToWDP(UDPbuffer);
        //        proc2CAN(UDPbuffer, fromServer2CAN);
        delay(100);
      }
      produceFrame(M_CAN_PING_CS2_1);
      sendToWDP(M_PATTERN);
      produceFrame(M_CAN_PING_CS2_2);
      sendToWDP(M_PATTERN);
      break;
    case Lok_Speed:
    case Lok_Direction:
    case Lok_Function:
      // send received data via wifi to clients
      proc2Clnts(UDPbuffer, fromWDP2CAN);
      break;
    case SWITCH_ACC:
      // send received data via wifi to clients
      proc2Clnts(UDPbuffer, toClnt);
      break;
    case S88_Polling:
      UDPbuffer[0x01]++;
      UDPbuffer[0x04] = 7;
      sendToWDPfromCAN(UDPbuffer);
      break;
    case S88_EVENT:
      UDPbuffer[0x01]++;
      UDPbuffer[0x09] = 0x00; // is free
      UDPbuffer[0x04] = 8;
      sendToWDPfromCAN(UDPbuffer);
      break;
    }
  }
}

void setup()
{
#if defined ARDUINO_ESP32_EVB
  delay(350);
#endif
  Serial.begin(bdrMonitor);
  delay(500);
  log_i("\r\n\r\nC A N g u r u - B r i d g e - %s", CgVersionnmbr);
  log_i("\n on %s", ARDUINO_BOARD);
  log_i("CPU Frequency = %d Mhz", F_CPU / 1000000);
  //  log_e("ERROR!");
  //  log_d("VERBOSE");
  //  log_w("WARNING");
  //  log_i("INFO");
  drawCircle = false;
  // das Display wird initalisiert
  initDisplayLCD28();
  // der Timer für das Blinken wird initialisiert
  stillAliveBlinkSetup();
  // start the CAN bus at 250 kbps
  setup_can_driver();
  // ESPNow wird initialisiert
  // Variablen werden auf Anfangswerte gesetzt und die Routinen für das Senden
  // die Routine für die Statusmeldungen des WiFi wird registriert
  wifi_event_id_t inet_evt_hnd = WiFi.onEvent(iNetEvtCB);
  // das gleiche mit ETHERNET
  ETH.begin();
  // das Zugprogramm (WDP) wurde noch nicht erkannt
  set_SYSseen(false);

  // Variablen werden gesetzt
  bLokDiscovery = false;
//%  initialDataAlreadySent = false;
  locid = 1;
  canguruStatus = waiting4server;
  scanningFinished = false;
  allLoksAreReported = false;
  setallSlavesAreReadyToZero();
  // start the telnetClient
  telnetClient.telnetInit();
  // This initializes udp and transfer buffer
  if (UDPFromWDP.begin(localPortFromWDP) == 0)
  {
    displayLCD("ERROR From WDP");
  }
  if (UDPFromServer.begin(localPortFromServer) == 0)
  {
    displayLCD("ERROR To WDP");
  }
  if (UDPToServer.begin(localPortToServer) == 0)
  {
    displayLCD("ERROR To Server");
  }
  if (UDPToWDP.begin(localPortToWDP) == 0)
  {
    displayLCD("ERROR From Server");
  }
  // start the TCP-server
  TCPINSYS.begin();
  //
  if (!LittleFS.begin(true))
  {
    log_e("An Error has occurred while mounting LittleFS");
    return;
  }
  if (!LittleFS.format())
  {
    log_e("An Error has occurred while formatting LittleFS");
    return;
  }
  //
  // Catch-All Handlers
  // Any request that can not find a Handler that canHandle it
  // ends in the callbacks below.
  AsyncWebSrvr.onNotFound(onRequest);
  // start the http-server
  AsyncWebSrvr.begin();
}

// Meldung, dass SYS gestartet werden kann
void goSYS()
{
  printMSG(Decodergelesen);
  printMSG(StartTrainApplication);
  drawCircle = true;
  log_d("Server has started");
}

// die Routin antwortet auf die Anfrage des CANguru-Servers mit CMD 0x88;
// damit erhält er die IP-Adresse der CANguru-Bridge und kann dann
// damit eine Verbindung aufbauen
bool proc_wait4Server()
{
  bool result;
  uint8_t UDPbuffer[CAN_FRAME_SIZE]; // buffer to hold incoming packet,
  yield();
  result = false;
  int packetSize = UDPFromServer.parsePacket();
  // if there's data available, read a packet
  if (packetSize)
  {
    // read the packet into packetBuffer
    UDPFromServer.read(UDPbuffer, CAN_FRAME_SIZE);
    // send received data via ETHERNET
    switch (UDPbuffer[0x1])
    {
    case CALL4CONNECT:
      setServerStatus(true);
      ipGateway = telnetClient.setipBroadcast(UDPFromServer.remoteIP());
      UDPbuffer[0x1]++;
      sendToServer(UDPbuffer, toServer);
      result = true;
      break;
    }
  }
  return result;
}

void proc_start_lokBuffer()
{
  produceFrame(M_CNTLOKBUFFER);
  sendToServer(M_PATTERN, toServer);
}

// damit wird die Gleisbox zum Leben erweckt
void send_start_60113_frames()
{
  produceFrame(M_GLEISBOX_MAGIC_START_SEQUENCE);
  proc2CAN(M_PATTERN, toCAN);
  produceFrame(M_GLEISBOX_ALL_PROTO_ENABLE);
  proc2CAN(M_PATTERN, toCAN);
}

void loop()
{
  // die folgenden Routinen werden ständig aufgerufen
  switch (canguruStatus)
  {
  case waiting4server:
    //  log_d("waiting4server");
    if (proc_wait4Server() == true)
      canguruStatus = startTelnetServer;
    break;
  case startTelnetServer:
    log_d("startTelnetServer");
    if (telnetClient.startTelnetSrv())
      canguruStatus = startGleisbox;
    break;
  case startGleisbox:
    log_d("startGleisbox");
    delay(5);
    send_start_60113_frames();
    // Empfangen über ESNOW werden registriert
    espInit();
    delay(200);
    // erstes PING soll schnell kommen
    secs = wait_for_ping;
    canguruStatus = startScanning;
    break;
  case startScanning:
    log_d("startScanning");
    Scan4Slaves();
    canguruStatus = stopScanning;
    break;
  case stopScanning:
    yield();
    if (scanningFinished == true)
    {
      log_d("stopScanning");
      registerSlaves();
      //      sendOutTCP(M_PATTERN);
      canguruStatus = wait4lokbuffer;
    }
    break;
  case wait4lokbuffer:
    log_d("wait4lokbuffer");
    proc_start_lokBuffer();
    canguruStatus = wait4ping;
    break;
  case wait4ping:
    log_d("wait4ping");
    proc_fromServer2CANandClnt();
    if (allLoksAreReported == true)
    {
      delay(100);
      if (loadCS2LocFile(0))
        telnetClient.printTelnet(true, "lokomotive.cs2 gelesen");
      else
        telnetClient.printTelnet(true, "Konnte lokomotive.cs2 nicht lesen");
      telnetClient.printTelnet(true, "");
      if (loadCS2LocFile(1))
        telnetClient.printTelnet(true, "geraete.vrs gelesen");
      else
        telnetClient.printTelnet(true, "Konnte geraete.vrs nicht lesen");
      telnetClient.printTelnet(true, "");
      delay(200);
      telnetClient.printTelnet(true, "Rufe die Decoder auf:");
      produceFrame(M_CAN_PING);
      proc2CAN(M_PATTERN, toCAN);
      proc2Clnts(M_PATTERN, toClnt);
      canguruStatus = wait4slaves;
    }
    break;
  case wait4slaves:
    log_d("wait4slaves");
    proc_fromCAN2WDPandServer();
    if (getallSlavesAreReady() == (get_slaveCnt() + 1))
    {
      canguruStatus = systemIsRunning;
    }
    break;
  case systemIsRunning:
    stillAliveBlinking();
    proc_fromServer2CANandClnt();
    proc_fromCAN2WDPandServer();
    proc_fromWDP2CAN();
    fillTheCircle();
    analyseHTTP();
    break;
  default:
    break;
  }
}
