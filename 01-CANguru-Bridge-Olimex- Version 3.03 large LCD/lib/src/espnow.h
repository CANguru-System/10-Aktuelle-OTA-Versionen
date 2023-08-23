
/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <CANguru-Buch@web.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gustav Wostrack
 * ----------------------------------------------------------------------------
 */

#ifndef ESPNOW_H
#define ESPNOW_H

/*
  - The SLAVE Node needs to be initialized as a WIFI_AP you will need to get its MAC via WiFi.softAPmacAddress()
  - THE MASTER Node needs to know the SLAVE Nodes WIFI_SOFT_AP MAC address (from the above comment)
  - You will need the MASTER nodes WIFI_STA MAC address via WiFi.macAddress()
  - The SLAVE needs the MASTER nodes WIFI_STA MAC address (from above line)
 */

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Ticker.h>
#include <telnet.h>

/*
            M A S T E R
*/

const uint8_t WIFI_CHANNEL = 0;
const uint8_t maxSlaves = 20;
const uint8_t macLen = 6;

// Infos über ESPNow
struct slaveInfoStruct
{
  esp_now_peer_info_t slave;
  esp_now_peer_info_t *peer;
  uint8_t no;
};

// zur Verarbeitung der MAC-Adressen
struct macAddressesstruct
{
  uint8_t peer_addr[macLen];
  uint64_t mac;
};

// Befehle, die benutzt werden
enum patterns
{
  M_GLEISBOX_MAGIC_START_SEQUENCE,
  M_GLEISBOX_ALL_PROTO_ENABLE,
  M_STOP,
  M_GO,
  M_BIND,
  M_VERIFY,
  M_FUNCTION,
  M_CAN_PING,
  M_PING_RESPONSE,
  M_CAN_PING_CS2,
  M_CAN_PING_CS2_1,
  M_CAN_PING_CS2_2,
  M_READCONFIG,
  M_STARTCONFIG,
  M_FINISHCONFIG,
  M_DOCOMPRESS,
  M_DONOTCOMPRESS,
  M_GETCONFIG,
  M_GETCONFIG_R,
  M_SIGNAL,
  M_CNTLOKBUFFER,
  M_SENDLOKBUFFER,
  M_CALL4CONNECTISDONE
};

// Kommunikationslinien
enum CMD
{
  /*00*/ toCAN,
  toClnt,
  toUDP,
  toTCP,
  fromCAN,
  /*05*/ fromClnt,
  fromServer,
  fromTCP,
  fromWDP2CAN,
  fromTCP2CAN,
  /*10*/ fromTCP2Clnt,
  fromCAN2UDP,
  fromCAN2TCP,
  fromGW2Clnt,
  /*14*/ fromGW2CAN,
  toServer,
  X0,
  MSGfromBridge
};

enum msg
{
  /*00*/ receivedCANping,
  /*01*/ CANmagic60113start,
  /*02*/ CANenabledalllocoprotos,
  /*03*/ StartTrainApplication,
  /*04*/ Decodergelesen,
  /*05*/ NoSlaves,
  /*06*/ repliedCANpingfakemember,
  /*07*/ startScanningDecoder,
  /*08*/ Meldepunkt0,
  /*09*/ Meldepunkt1,
  /*10*/ Meldepunkt2
};

// speichert die Stromstaerke in das array amperes
void setAmpere(uint8_t i, uint8_t d);
// identifiziert einen Slave anhand seiner UID
uint8_t matchUID(uint8_t *buffer);
// ESPNow wird initialisiert
void espInit();
// Kommunikationsroutine zu den CAN-Komponenten (momentan nur die Gleisbox)
void proc2CAN(uint8_t *buffer, CMD dir);
// Laden der lokomotive.cs2-Datei
void receiveLocFile();
// Übertragung von Frames zum CANguru-Server zur dortigen Ausgabe
void printUDPCAN(uint8_t *buffer, CMD dir);
// Ausgaberoutine
void sendToWDP(uint8_t *buffer);
// Ausgaberoutine
void sendOutTCP(uint8_t *buffer);
// Ausgaberoutine
void sendToServer(uint8_t *buffer, CMD cmd);
// Zeigt an, dass die Suche nach Decodern begonnen hat
void msgStartScanning();
// Anzeige, dass SYS gestartet werden kabb
void goSYS();
// gibt die Anzahl der gefundenen Slaves zurück
uint8_t get_slaveCnt();
// gibt an, ob SYS gestartet ist
bool get_SYSseen();
// setzt, dass SYS gestartet ist
void set_SYSseen(bool SYS);
// liefert die Struktur slaveInfo zurück
slaveInfoStruct get_slaveInfo(uint8_t slave);
// gibt eine MAC-Adresse aus
void printMac(uint8_t m[macLen]);
// vergleicht zwei MAC-Adressen
bool macIsEqual(const uint8_t m0[macLen], const uint8_t m1[macLen]);
// setzt die Variable nbrSlavesAreReady auf NULL
void setallSlavesAreReadyToZero();
// gibt den Wert der Variablen nbrSlavesAreReady zurück
uint8_t getallSlavesAreReady();
// erhöht SlavesAreReady um 1
void incSlavesAreReadyToZero();
// Scannt nach Slaves
void Scan4Slaves();
// setzt die vorgegebene MAC-Adresse des Masters
void initVariant();
// addiert und registriert gefundene Slaves
void addSlaves();
// steuert den Registrierungsprozess der Slaves
void registerSlaves();
// sendet daten über ESPNow
void sendTheData(const uint8_t slave, const uint8_t *data, size_t len);
// nach dem Versand von Meldungen können hier Fehlermeldungen abgerufen werden
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
// empfängt Daten über ESPNow
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len);

#endif
