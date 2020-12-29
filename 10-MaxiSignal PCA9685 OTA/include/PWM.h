
/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <CANguru-Buch@web.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gustav Wostrack
 * ----------------------------------------------------------------------------
 */

#pragma once

#include "Arduino.h"
#include <Adafruit_PWMServoDriver.h>

enum colorLED
{
  red,
  green
};

/* Märklin Dokumentation:
Bit 0,1: Stellung:
00: Aus, Rund, Rot, Rechts, HP0
01: Ein, Grün, Gerade, HP1
10: Gelb, Links, HP2
11: Weiss, SH0
*/
//

const int LEDfreq = 1600;
const int Formfreq = 50;

const uint16_t MIN_PULSE_WIDTH = 80;
const uint16_t MAX_PULSE_WIDTH = 600;

const uint16_t MIN_ANGLE_WIDTH = 0;
const uint16_t MAX_ANGLE_WIDTH = 180;

// Verzögerungen
const uint8_t minLEDsignaldelay = 2;
const uint8_t maxLEDsignaldelay = 99;
const uint8_t stdLEDsignaldelay = (minLEDsignaldelay + maxLEDsignaldelay - 1) / 2;

const uint8_t minFormsignaldelay = 200;
const uint8_t maxFormsignaldelay = 255;
const uint8_t stdFormsignaldelay = (minFormsignaldelay + maxFormsignaldelay) / 2;

//
const uint8_t stdStartAngle = 70;
const uint8_t stdStopAngle = 100;

// Anzahl der Magnetartikel
// 1. PCA9685
const uint8_t num_LEDSignals = 8; //
// 2. PCA9685
const uint8_t num_FormSignals = 16; //

const uint8_t minendAngle = 2;
const uint8_t maxendAngle = 10;
const uint8_t stdendAngle = (minendAngle + maxendAngle) / 4;

void initPWM_Form();
void initPWM_LED();

class BaseSignalClass
{
public:
  // Voreinstellungen, Servonummer wird physikalisch mit
  // einem Signal verbunden;
  void Attach(uint8_t ch);
  // Zielfarbe ist Grün
  virtual void GoGreen(){};
  // Zielfarbe ist Rot
  virtual void GoRed(){};
  // Setzt die Zielfarbe
  void SetcolorLED();
  // Setzt die Zielfarbe
  void SetLightDest(colorLED c);
  // Liefert die Zielfarbe
  colorLED GetLightDest();
  // Liefert die aktuelle Farbe
  colorLED GetLightCurr();
  // Setzt die aktuelle Farbe
  void SetLightCurr(colorLED c);
  // Damit werden unnötige Farbänderungen vermieden
  bool ColorChg();
  // Setzt den Wert für die Verzögerung
  void SetDelay(int d);
  // Setzt die Adresse eines Signals
  void Set_to_address(uint16_t _to_address);
  // Liefert die Adresse eines Signals
  uint16_t Get_to_address();

  unsigned long updateInterval; // interval between updates
  unsigned long lastUpdate;     // last update of colorLED
  uint16_t acc__to_address;
  colorLED acc_light_dest;
  colorLED acc_light_curr;
  colorLED wakeupdir;
  uint16_t delayFactor;
};

class LEDSignalClass : public BaseSignalClass
{
public:
  // einem Signal verbunden;
  void Attach(uint8_t ch);
  // Zielfarbe ist Grün
  void GoGreen();
  // Zielfarbe ist Rot
  void GoRed();
  // Überprüft periodisch, ob die Zielfarbe erreicht ist
  void Update();

private:
  uint8_t pinGREEN, pinRED;
  uint8_t green_dutyCycle_dest, green_dutyCycle_curr;
  uint8_t red_dutyCycle_dest, red_dutyCycle_curr;
  uint8_t green_channel, red_channel;
  int8_t green_increment, red_increment;
  uint8_t dutyCycle_max, dutyCycle_min;
};

class FormSignalClass : public BaseSignalClass
{
public:
  // einem Signal verbunden;
  void Attach(uint8_t ch);
  // Setzt den Startwinkel für das Servo
  // zu den Drehbewegungen des Servos:
  // man legt das Servo so, dass der Arm bei Rot
  // nach rechts oben zeigt und beim Umschalten auf
  // Grün nach links läuft;
  // dann ist null Grad rechts unten und 180 Grad
  // links unten. Weil der Startwinkel kleiner als
  // der Stoppwinkel ist, liegt der Startwinkel bei Rot
  // und der Stoppwinkel bei Grün.
  void SetStartAngle(uint8_t angle)
  {
    startAngle = angle;
  };
  // Setzt den Stoppwinkel für das Servo
  void SetStopAngle(uint8_t angle)
  {
    stopAngle = angle;
  };
  // Setzt den Überschwingwinkel für das Servo
  void SetEndAngle(uint8_t angle)
  {
    endAngle = angle;
  };
  // Zielfarbe ist Grün
  void GoGreen();
  // Zielfarbe ist Rot
  void GoRed();
  // Überprüft periodisch, ob die Zielfarbe erreicht ist
  void Update();

private:
  uint8_t pin;
  long dutyCycle_dest, dutyCycle_curr;
  uint8_t channel;
//  int8_t increment;
  uint8_t startAngle, stopAngle;
  int8_t curr_endAngle, endAngle;
  bool firstTime;
};
