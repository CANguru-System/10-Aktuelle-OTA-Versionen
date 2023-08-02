
/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <CANguru-Buch@web.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gustav Wostrack
 * ----------------------------------------------------------------------------
 */

#include <Arduino.h>
#include <espnow.h>
#include <CANguruDefs.h>
#include <CAN_const.h>
#include "MOD-LCD.h"
#include <telnet.h>
#include "CANguru.h"

uint8_t slaveCnt;
uint8_t slaveCurr;
decoderStruct gate;
uint8_t nbrSlavesAreReady;

// willkürlich festgelegte MAC-Adresse
const uint8_t masterCustomMac[] = {0x30, 0xAE, 0xA4, 0x89, 0x92, 0x71};

slaveInfoStruct slaveInfo[maxSlaves];
slaveInfoStruct tmpSlaveInfo;
esp_now_peer_info_t cand;
String ssidSLV = "CNgrSLV";

bool SYSseen;
uint8_t cntConfig;

// identifiziert einen Slave anhand seiner UID
uint8_t matchUID(uint8_t *buffer)
{
  uint8_t slaveCnt = get_slaveCnt();
  for (uint8_t s = 0; s < slaveCnt; s++)
  {
    uint32_t uid = UID_BASE + s;
    if (
        (buffer[5] == (uint8_t)(uid >> 24)) &&
        (buffer[6] == (uint8_t)(uid >> 16)) &&
        (buffer[7] == (uint8_t)(uid >> 8)) &&
        (buffer[8] == (uint8_t)uid))
      return s;
  }
  return 0xFF;
}

// kopiert die Struktur slaveInfoStruct von source nach dest
void cpySlaveInfo(slaveInfoStruct dest, slaveInfoStruct source)
{
  dest.slave = source.slave;
  dest.peer = source.peer;
  dest.initialData2send = source.initialData2send;
  dest.no = source.no;
  dest.decoderType = source.decoderType;
}

// ESPNow wird initialisiert
void espInit()
{
  cntConfig = 0;
  slaveCnt = 0;
  slaveCurr = 0;
  gate.isType = false;
  initVariant();
  if (esp_now_init() == ESP_OK)
  {
    displayLCD("ESPNow started!");
  }
  else
  {
    displayLCD("ESP_Now INIT FAILED....");
  }
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
}

// gibt die Anzahl der gefundenen Slaves zurück
uint8_t get_slaveCnt()
{
  return slaveCnt;
}

// gibt an, ob SYS gestartet ist
bool get_SYSseen()
{
  return SYSseen;
}

// setzt, dass SYS gestartet ist
void set_SYSseen(bool SYS)
{
  SYSseen = SYS;
}

// fordert einen Slave dazu auf, Anfangsdaten bekannt zu geben
void set_initialData2send(uint8_t slave)
{
  slaveInfo[slave].initialData2send = true;
}

// quittiert, dass Anfangsdaten übertragen wurden
void reset_initialData2send(uint8_t slave)
{
  slaveInfo[slave].initialData2send = false;
}

// gibt zurück, ob noch Anfangsdaten von einem Slave zu liefern sind
bool get_initialData2send(uint8_t slave)
{
  return slaveInfo[slave].initialData2send;
}
// gibt zurück, um welchen Decodertypen es sich handelt
uint8_t get_decoder_type(uint8_t no)
{
  return slaveInfo[no].decoderType;
}

// liefert die Struktur slaveInfo zurück
slaveInfoStruct get_slaveInfo(uint8_t slave)
{
  return slaveInfo[slave];
}

// gibt eine MAC-Adresse aus
void printMac(uint8_t m[macLen])
{
  char macStr[18] = {0};
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
  displayLCD(macStr);
}

// vergleicht zwei MAC-Adressen
bool macIsEqual(const uint8_t m0[macLen], const uint8_t m1[macLen])
{
  for (uint8_t ii = 0; ii < macLen; ++ii)
  {
    if (m0[ii] != m1[ii])
    {
      return false;
    }
  }
  return true;
}

// Wir suchen nach Slaves
// Scannt nach Slaves
void Scan4Slaves()
{
  displayLCD("Suche Slaves ...");
  msgStartScanning();
  int8_t scanResults = WiFi.scanNetworks();
  if (scanResults == 0)
  {
    displayLCD("Noch kein WiFi Gerät im AP Modus gefunden");
  }
  else
  {
    for (int i = 0; i < scanResults; ++i)
    {
      // Print SSID and RSSI for each device found
      String SSID = WiFi.SSID(i);
      String BSSIDstr = WiFi.BSSIDstr(i);

      // Prüfen ob der Netzname mit `Slave` beginnt
      if (SSID.indexOf(ssidSLV) == 0)
      {
        // Ja, dann haben wir einen Slave gefunden
        // MAC-Adresse aus der BSSID ses Slaves ermitteln und in der Slave info struktur speichern
        int mac[macLen];
        if (macLen == sscanf(BSSIDstr.c_str(), "%x:%x:%x:%x:%x:%x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]))
        {
          for (int ii = 0; ii < macLen; ++ii)
          {
            cand.peer_addr[ii] = (uint8_t)mac[ii];
          }
        }
        bool notAlreadyFound = true;
        for (uint8_t s = 0; s < slaveCnt; s++)
        {
          if (macIsEqual(slaveInfo[s].slave.peer_addr, cand.peer_addr) == true)
          {
            notAlreadyFound = false;
            break;
          }
        }
        if (notAlreadyFound == true)
        {
          memcpy(&slaveInfo[slaveCnt].slave.peer_addr, &cand.peer_addr, macLen);
          slaveInfo[slaveCnt].slave.channel = WIFI_CHANNEL;
          slaveInfo[slaveCnt].slave.encrypt = 0;
          slaveInfo[slaveCnt].peer = &slaveInfo[slaveCnt].slave;
          slaveInfo[slaveCnt].initialData2send = false;
          slaveInfo[slaveCnt].no = slaveCnt;
          slaveCnt++;
        }
      }
    }
  }
  // clean up ram
  WiFi.scanDelete();
}

// setzt die vorgegebene MAC-Adresse des Masters
void initVariant()
{
  WiFi.mode(WIFI_MODE_STA);
  esp_err_t setMacResult = esp_wifi_set_mac((wifi_interface_t)ESP_IF_WIFI_STA, &masterCustomMac[0]);
  if (setMacResult == ESP_OK)
    log_i("Init Variant ok!");
  else
    log_e("Init Variant failed!");
  WiFi.disconnect();
}

// addiert und registriert gefundene Slaves
void addSlaves()
{
  uint8_t Clntbuffer[CAN_FRAME_SIZE]; // buffer to hold incoming packet,
  macAddressesstruct macAddresses[maxSlaves];
  for (uint8_t s = 0; s < slaveCnt; s++)
  {
    if (esp_now_add_peer(slaveInfo[s].peer) == ESP_OK)
    {
      for (uint8_t i = 0; i < macLen; i++)
      {
        macAddresses[s].peer_addr[i] = slaveInfo[s].slave.peer_addr[i];
      }
      macAddresses[s].mac = 0;
      for (uint8_t m = 0; m < macLen - 1; m++)
      {
        macAddresses[s].mac += macAddresses[s].peer_addr[m];
        macAddresses[s].mac = macAddresses[s].mac << 8;
      }
      macAddresses[s].mac += macAddresses[s].peer_addr[macLen - 1];
    }
  }
  // bubblesort
  uint8_t peer_addr[macLen];
  for (uint8_t s = 0; s < (slaveCnt - 1); s++)
  {
    for (int o = 0; o < (slaveCnt - (s + 1)); o++)
    {
      if (macAddresses[o].mac > macAddresses[o + 1].mac)
      {
        //
        uint64_t t = macAddresses[o].mac;
        memcpy(peer_addr, macAddresses[o].peer_addr, macLen);
        cpySlaveInfo(tmpSlaveInfo, slaveInfo[o]);
        //
        macAddresses[o].mac = macAddresses[o + 1].mac;
        memcpy(macAddresses[o].peer_addr, macAddresses[o + 1].peer_addr, macLen);
        cpySlaveInfo(slaveInfo[o], slaveInfo[o + 1]);
        //
        macAddresses[o + 1].mac = t;
        memcpy(macAddresses[o + 1].peer_addr, peer_addr, macLen);
        cpySlaveInfo(slaveInfo[o], tmpSlaveInfo);
      }
    }
  }
  for (uint8_t s = 0; s < slaveCnt; s++)
  {
    memcpy(&Clntbuffer, slaveInfo[s].slave.peer_addr, macLen);
    // device-Nummer übermitteln
    Clntbuffer[macLen] = s;
    sendTheData(s, Clntbuffer, macLen + 1);
    printMac(slaveInfo[s].slave.peer_addr);
    char chs[30];
    sprintf(chs, " -- Added Slave %d\r\n", s + 1);
    displayLCD(chs);
  }
}

// steuert den Registrierungsprozess der Slaves
void registerSlaves()
{
  if (slaveCnt > 0)
  {
    // add slaves
    addSlaves();
    delay(50);
    // alle slaves sind bekannt
    char chs[30];
    int sc = slaveCnt;
    sprintf(chs, "%d slave(s) found!\r\n", sc);
    displayLCD(chs);
    delay(500);
    clearDisplay();
  }
}

// Fehlermeldungen
void printESPNowError(esp_err_t Result)
{
  if (Result == ESP_ERR_ESPNOW_NOT_INIT)
  {
    // How did we get so far!!
    displayLCD("ESPNOW not Init.");
  }
  else if (Result == ESP_ERR_ESPNOW_ARG)
  {
    displayLCD("Invalid Argument");
  }
  else if (Result == ESP_ERR_ESPNOW_INTERNAL)
  {
    displayLCD("Internal Error");
  }
  else if (Result == ESP_ERR_ESPNOW_NO_MEM)
  {
    displayLCD("ESP_ERR_ESPNOW_NO_MEM");
  }
  else if (Result == ESP_ERR_ESPNOW_NOT_FOUND)
  {
    if (slaveCnt > 0)
      displayLCD("Peer not found!");
  }
  else if (Result == ESP_ERR_ESPNOW_IF)
  {
    displayLCD("Interface Error.");
  }
  else
  {
    int res = Result;
    char chs[30];
    sprintf(chs, "\r\nNot sure what happened\t%d", res);
    displayLCD(chs);
  }
}

// sendet daten über ESPNow
// der slave wird mit der nummer angesprochen, die sich durch die Reihenfolge beim Erkennen (scannen) ergibt
void sendTheData(uint8_t slave, const uint8_t *data, size_t len)
{
  esp_err_t sendResult = esp_now_send(slaveInfo[slave].slave.peer_addr, data, len);
  if (sendResult != ESP_OK)
  {
    printESPNowError(sendResult);
  }
}

void setallSlavesAreReadyToZero()
{
  nbrSlavesAreReady = 0;
}

void incSlavesAreReadyToZero()
{
  nbrSlavesAreReady++;
}

uint8_t getallSlavesAreReady()
{
  return nbrSlavesAreReady;
}

// nach dem Versand von Meldungen können hier Fehlermeldungen abgerufen werden
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
}

// empfängt Daten über ESPNow
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
  uint8_t Clntbuffer[CAN_FRAME_SIZE]; // buffer to hold incoming packet,
  memcpy(Clntbuffer, data, data_len);
  log_d("Clntbuffer: %x", data_len);
  if (data_len == macLen)
  {
  // Rückmeldung der slaves, nachdem sie ihre UID festgelegt haben
  // mit nbrSlavesAreReady wird die Anzahl der Rückmeldungen gezählt
    nbrSlavesAreReady++;
    return;
  }
  log_d("OnDataRecv0: %x", data[0x01]);
  switch (data[0x01])
  {
  case PING_R:
    sendToServer(Clntbuffer, fromClnt);
    for (uint8_t s = 0; s < slaveCnt; s++)
    {
      uint8_t m[macLen];
      for (uint8_t cnt = 0; cnt < macLen; cnt++)
      {
        m[cnt] = mac_addr[cnt];
      }
      if (macIsEqual(slaveInfo[s].slave.peer_addr, m))
      {
        slaveInfo[s].decoderType = data[12];
        if (data[12] == DEVTYPE_GATE)
        {
          gate.isType = true;
          gate.decoder_no = s;
          break;
        }
      }
    }
    break;
  case CONFIG_Status_R:
    sendToServer(Clntbuffer, fromClnt);
    break;
  case SEND_IP_R:
    sendToServer(Clntbuffer, fromClnt);
    if (!SYSseen)
    {
      cntConfig++;
      if (cntConfig == slaveCnt)
        goSYS();
    }
    break;
  case S88_EVENT_R:
    // Meldungen vom Gleisbesetztmelder
    // an das Gateway
    sendToServer(Clntbuffer, fromClnt);
    // nur wenn Win-DigiPet gestartet ist
    if (SYSseen)
    {
      // an SYS
      sendToWDP(Clntbuffer);
      // bei Schranken auch an diesen Decoder
      if (gate.isType)
      {
        sendTheData(gate.decoder_no, Clntbuffer, CAN_FRAME_SIZE);
      }
    }
    break;
  case sendCurrAmp:
    for (uint8_t i = 0x05; i < 0x05 + 0x08; i++)
    {
      setAmpere(i - 0x05, data[i]);
    }
    break;
  default:
    // send received data via Ethernet to GW and evtl to SYS
    sendToServer(Clntbuffer, fromClnt);
  log_d("OnDataRecv1: %x", data[0x01]);
    if (SYSseen)
    {
      sendToWDP(Clntbuffer);
    }
    break;
  }
}
