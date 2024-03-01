
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
#include "button.h"

#undef left2right

// mögliche Positionen des steppers
enum position
{
  right,
  left
};

/* Märklin Dokumentation:
Bit 0,1: Stellung:
00: Aus, Rund, Rot, Rechts, HP0
01: Ein, Grün, Gerade, HP1
10: Gelb, Links, HP2
11: Weiss, SH0
*/
//
enum enumway
{
  longway,  // von rechts nach links oder umgekehrt
  shortway, // der kleine Überschwinger beim Signal
  noway     // stepper steht
};

enum directions
{
  forward,
  reverse
};

enum setupPhases
{
  phase0,
  phase1,
  phase2,
  phase3,
  phase4,
  phase5
};

const int steps = 4; // how many pins are in use.

// MX1508 Motor Driver Pins
/*
// mit diesen Werten fährt der Stepper in die umgekehrte Richtung*/

#ifdef left2right
const uint8_t A_plus = GPIO_NUM_5;
const uint8_t A_minus = GPIO_NUM_6;
const uint8_t B_plus = GPIO_NUM_7;
const uint8_t B_minus = GPIO_NUM_10;
#else
const uint8_t A_plus = GPIO_NUM_5;
const uint8_t A_minus = GPIO_NUM_6;
const uint8_t B_plus = GPIO_NUM_7;
const uint8_t B_minus = GPIO_NUM_10;
#endif

const uint8_t btn_Step_pin = GPIO_NUM_9;
const uint8_t btn_Corr_pin = GPIO_NUM_21;

// Verzögerungen
const uint8_t maxstepperdelay = 10;
const uint8_t minstepperdelay = maxstepperdelay / 5;
const uint8_t stdstepperdelay = maxstepperdelay / 2;
const unsigned long step_delay_max = 1200; 


class Stepper
{
public:
  // Voreinstellungen, steppernummer wird physikalisch mit
  // einem stepper verbunden;
  void Attach();
  // Setzt die Zielposition
  void SetPosition();
  // Zielposition ist links
  void GoLeft();
  // Zielposition ist rechts
  void GoRight();
  // Überprüft periodisch, ob die Zielposition erreicht wird
  void Update();
  // Setzt die Zielposition
  void SetPosDest(position p)
  {
    acc_pos_dest = p;
  }
  // Liefert die Zielposition
  position GetPosDest()
  {
    return acc_pos_dest;
  }
  // Setzt die aktuelle Position
  void SetPosCurr(position p)
  {
    acc_pos_curr = p;
  }
  // Liefert die aktuelle Position
  position GetPosCurr()
  {
    return acc_pos_curr;
  }
  // Hat sich die Position des steppers verändert?
  // Damit werden unnötige Positionsänderungen vermieden
  bool PosChg()
  {
    return acc_pos_dest != acc_pos_curr;
  }
  // Setzt den Wert für die Verzögerung
  void SetDelay(uint8_t delay_factor)
  {
    step_delay = step_delay_max * delay_factor;    // 60L * 1000L / stepsPerRevolution / whatSpeed;
  }
  // Setzt die Adresse eines steppers
  void Set_to_address(uint8_t _to_address)
  {
    log_i("Set_to_address: %d", _to_address);
    acc__to_address = _to_address;
  }
  // Liefert die Adresse eines steppers
  uint8_t Get_to_address()
  {
    log_i("Get_to_address: %d", acc__to_address);
    return acc__to_address;
  }
  // Setzt die Gesamtumdrehungen eines steppers
  void Set_stepsToSwitch(uint16_t steps)
  {
    stepsToSwitch = steps;
  }
  // Liefert die Gesamtumdrehungen eines steppers
  uint16_t Get_stepsToSwitch()
  {
    return stepsToSwitch;
  }
  bool Get_set_stepsToSwitch()
  {
    return set_stepsToSwitch;
  }
  void Reset_stepsToSwitch() {
    set_stepsToSwitch = false;
  }
  bool is_no_Correction()
  {
    return no_correction;
  }

private:
  // entprellt den Taster
  Button btn_Stepper;
  Button btn_Correction;
  void oneStep();
  void stopStepper();
  int currpos;   // current stepper position
  int destpos;   // stepper position, where to go
  int increment; // increment to move for each interval
  int endpos;
  unsigned long now_micros; // interval between updates
  uint8_t acc__to_address;
  position acc_pos_dest;
  position acc_pos_curr;
  int leftpos;   // 74 je groesser desto weiter nach links
  int rightpos;  // 5 je kleiner desto weiter nach rechts
  int maxendpos; // * grdinmillis;
  enumway way;
  bool readyToStep;
  bool set_stepsToSwitch;
  bool no_correction;
  int8_t step;
  unsigned long last_step_time;  // timestamp in us of when the last step was taken
  unsigned long step_delay;      // delay between steps, in us, based on speed
  unsigned long direction_delay; // delay between steps, in us, based on speed
  directions direction;
  setupPhases phase;
  int16_t stepsToSwitch;
};