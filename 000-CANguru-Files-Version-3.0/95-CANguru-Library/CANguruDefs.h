
/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <CANguru-Buch@web.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gustav Wostrack
 * ----------------------------------------------------------------------------
 */

#ifndef CANguruDefs
#define CANguruDefs

#define CAN_FRAME_SIZE 13 /* maximum datagram size */

// allgemein
#define wait_time_long 500
#define wait_time_medium 50
#define wait_time_small 10
#define wait_time 125
#define max_char 50
#define min_char 2
#define min_char1 3
#define bdrMonitor 115200

#define CANcmd   1
#define hash0 2
#define hash1 3
#define Framelng   4
#define data0 5
#define data1 6
#define data2 7
#define data3 8
#define data4 9
#define data5 10
#define data6 11
#define data7 12

#define setup_done 0x47

/*
 *  Gerätetypen
 */
/*
  Prio	2+2 Bit Message Prio			28 .. 25
										XXXX
  Priorität (Prio)
  Bestimmt die Priorisierung der Meldung auf dem CAN Bus:
  Prio 1: Stopp / Go / Kurzschluss-Meldung
  --> Prio 2: Rückmeldungen				0001
  Prio 3: Lok anhalten (?)
  --> Prio 4: Lok / Zubehörbefehle		0100
                                       Gleisbox
                                       0100 0111  0100 0100  0001 1001  0001 1000
                                          4    7     4    4     1    9     1    8
                                       Weichendecoder
                                          4    5     0    0     9    1     9    1

  Rest Frei								1111
*/

#define UID_BASE  0x45009195ULL // CAN-UID
#define maxdevice 99

#define DEVTYPE_GFP 0x0000
#define DEVTYPE_GB 0x0010
#define DEVTYPE_CONNECT 0x0020
#define DEVTYPE_MS2 0x0030
#define DEVTYPE_WDEV 0x00E0
#define DEVTYPE_CS2 0x00FF
#define DEVTYPE_FirstCANguru 0x004F
#define DEVTYPE_BASE 0x0050
#define DEVTYPE_TRAFFICLIGHT 0x0051
#define DEVTYPE_SERVO 0x0053
#define DEVTYPE_RM 0x0054
#define DEVTYPE_LIGHT 0x0055
#define DEVTYPE_SIGNAL 0x0056
#define DEVTYPE_LEDSIGNAL 0x0057
#define DEVTYPE_CANBOOSTER 0x0058
#define DEVTYPE_GATE 0x0059
#define DEVTYPE_CAR_CAR 0x005A
#define DEVTYPE_CAR_RM 0x005B
#define DEVTYPE_CAR_SERVO 0x005C
#define DEVTYPE_LastCANguru 0x005F

#define BASE_Offset 0x01
#define DECODER_Offset 0x02

/*
 * Adressbereiche:
*/
#define MM_ACC 0x3000    //Magnetartikel Motorola
#define DCC_ACC 0x3800   //Magbetartikel NRMA_DCC
#define MM_TRACK 0x0000  //Gleissignal Motorola
#define DCC_TRACK 0xC000 //Gleissignal NRMA_DCC

/*
 * CAN-Befehle (Märklin)
*/
#define SYS_CMD 0x00    //Systembefehle
#define SYS_CMD_R SYS_CMD+1    //Systembefehle Antwort
#define SYS_STOPP		          0x00	//System - Stopp
#define SYS_GO		          0x01	//System - Go
#define LokDiscovery 0x02
#define LokDiscovery_R LokDiscovery+1
#define MFXVerify 0x07
#define MFXVerify_R MFXVerify+1
#define Lok_Speed           0x08
#define Lok_Direction       0x0A
#define Lok_Function        0x0C
#define SYS_STAT 0x0B   //System - Status (sendet geänderte Konfiguration)
#define ReadConfig   0x0E
#define ReadConfig_R   ReadConfig+1
#define WriteConfig   0x10
#define WriteConfig_R   WriteConfig+1
#define SWITCH_ACC 0x16 //Magnetartikel schalten
#define SWITCH_ACC_R 0x17 //Magnetartikel schalten
#define S88_Polling 0x20
#define S88_EVENT 0x22            //Rückmelde-Event
#define S88_EVENT_R S88_EVENT + 1 // Rück-Rückmelde-Event
#define PING 0x30                 //CAN-Teilnehmer anpingen
#define PING_R PING + 1           //CAN-Teilnehmer anpingen
#define CONFIG_Status 0x3A
#define CONFIG_Status_R CONFIG_Status + 1
#define ConfigDataCompressed 0x40
#define ConfigDataCompressed_R ConfigDataCompressed + 0x01
#define ConfigDataStream 0x42

#define stopp 0x00
#define halt 0x02
#define overload 0x0A

/*
 * CAN-Befehle (eigene)
*/
#define ConfigData 0x40
#define ConfigData_R 0x41
#define MfxProc 0x50
#define MfxProc_R 0x51
#define LoadCS2Data 0x56
#define LoadCS2Data_R LoadCS2Data + 0x01            // 0x57
#define GETCONFIG_RESPONSE LoadCS2Data_R + 0x01     // 0x58
#define DoCompress GETCONFIG_RESPONSE + 0x02        // 0x5A
#define DoNotCompress DoCompress + 0x01             // 0x5B
#define BlinkAlive 0x60
#define BlinkAlive_R BlinkAlive + 0x01
#define restartBridge 0x62
#define SEND_IP 0x64
#define SEND_IP_R SEND_IP + 1
#define INIT_COMPLETE 0x66
#define CALL4CONNECT 0x88
#define sendCntLokBuffer 0x90
#define sendCntLokBuffer_R 0x91
#define sendLokBuffer 0x92
#define sendLokBuffer_R 0x93
#define sendCurrAmp 0x94
#define RESET_MEM 0xFE
#define START_OTA 0xFF

// damit 1024 (eigentlich 1023) Artikel adressiert werden können,
// bedarf es 1024/4 Decoder; das 256 bzw. bei 255 entspr. 0xFF
const int16_t minadr = 0x01;
const int16_t maxadr = 0xFF;
const uint8_t uid_num = 4;
const uint8_t num_accessory = 4;

// Funktion stellt sicher, dass keine unerlaubten 8-Bit-Werte geladen werden können
uint8_t readValfromEEPROM(uint16_t adr, uint8_t val, uint8_t min, uint8_t max);

// Funktion stellt sicher, dass keine unerlaubten 16-Bit-Werte geladen werden können
uint16_t readValfromEEPROM16(uint16_t adr, uint16_t val, uint16_t min, uint16_t max);

// Mit testMinMax wird festgestellt, ob ein Wert innerhalb der
// Grenzen von min und max liegt
bool testMinMax(uint16_t oldval, uint16_t val, uint16_t min, uint16_t max);

// converts highbyte of integer to char
char highbyte2char(int num);
// converts lowbyte of integer to char
char lowbyte2char(int num);
// returns one char of an int
uint8_t oneChar(uint16_t val, uint8_t no);

uint8_t hex2dec(uint8_t h);

#endif