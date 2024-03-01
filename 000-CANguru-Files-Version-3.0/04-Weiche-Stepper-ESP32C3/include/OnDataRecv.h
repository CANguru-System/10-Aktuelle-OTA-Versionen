
/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <CANguru-Buch@web.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gustav Wostrack
 * ----------------------------------------------------------------------------
 */

#ifndef ON_DATA_RECEIVE_H
#define ON_DATA_RECEIVE_H

#include <Arduino.h>

// alle Meldungen von der CANguru-Bridge kommen hier rein
// und werden hier ausgewertet und evtl. weiter geleitet
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
  memcpy(opFrame, data, data_len);
  if (data_len == macLen + 1)
  {
    // nur beim handshaking; es werden die macaddress
    // und die devicenummer übermittelt und die macaddress zurückgeschickt
    esp_err_t sendResult = esp_now_send(master.peer_addr, opFrame, macLen);
    if (sendResult != ESP_OK)
    {
      printESPNowError(sendResult);
    }
    generateHash(opFrame[macLen]);
    return;
  }
  switch (opFrame[0x01])
  {
    // bewirkt das Aufblitzen der LED
  case BlinkAlive:
    if (secs < 10)
      secs = 10;
    bDecoderIsAlive = true;
    got1CANmsg = true;
    break;
  case PING:
    statusPING = true;
    got1CANmsg = true;
    // alles Weitere wird in loop erledigt
    break;
  case CONFIG_Status:
  {
    CONFIG_Status_Request = true;
    CONFIGURATION_Status_Index = (Kanals)opFrame[9];
    if (CONFIGURATION_Status_Index > 0 && secs < 100)
      secs = 100;
    got1CANmsg = true;
  }
  break;
  // IP-Adresse rückmelden
  case SEND_IP:
    SEND_IP_Request = true;
    got1CANmsg = true;
    // alles Weitere wird in loop erledigt
    break;
  // config
  case SYS_CMD:
  {
    switch (opFrame[9])
    {
    case SYS_STAT:
      SYS_CMD_Request = true;
      // alles Weitere wird in loop erledigt
      got1CANmsg = true;
      break;
    // Modul wird neu gestartet und wartet anschließend auf neue Software (OTA)
    case RESET_MEM:
      preferences.putUChar("setup_done", setup_NOT_done);
      preferences.putUChar("receiveTheData", false);
      ESP.restart();
      break;
    case START_OTA:
      preferences.putUChar("ota", startWithOTA);
      //
      ESP.restart();
      break;
    }
  }
  break;
  case SWITCH_ACC:
  {
    // Umsetzung nur bei gültiger Weichenadresse
    uint8_t _to_address = (uint16_t)((opFrame[7] << 8) | opFrame[8]) - MM_ACC + 1;
    log_i("SWITCH_ACC: %d", _to_address);
    // Auf benutzte Adresse überprüfen
    if (_to_address == stepper.Get_to_address())
    {
      stepper.SetPosDest((position)opFrame[9]);
      // muss Artikel geändert werden?
      if (stepper.PosChg())
      {
        switchAcc();
      }
      break;
    }
  }
  break;
  }
}

#endif