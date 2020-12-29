
/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <CANguru-Buch@web.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gustav Wostrack
 * ----------------------------------------------------------------------------
 */
#include <PWM.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm_Form = Adafruit_PWMServoDriver(0x40);
Adafruit_PWMServoDriver pwm_LED = Adafruit_PWMServoDriver(0x41);

void initPWM_Form()
{
  pwm_Form.begin();
  pwm_Form.setPWMFreq(Formfreq);
}

void initPWM_LED()
{
  pwm_LED.begin();
  pwm_LED.setPWMFreq(LEDfreq);
}

// Voreinstellungen, Servonummer wird physikalisch mit
// einem Signal verbunden;
void BaseSignalClass::Attach(uint8_t ch)
{
  wakeupdir = red;
  delayFactor = 100;
}

// Setzt die Zielfarbe
void BaseSignalClass::SetcolorLED()
{
  lastUpdate = micros();
  acc_light_curr = acc_light_dest;
  switch (acc_light_dest)
  {
  case green:
  {
    GoGreen();
  }
  break;
  case red:
  {
    GoRed();
  }
  break;
  }
}

// Setzt die Zielfarbe
void BaseSignalClass::SetLightDest(colorLED c)
{
  acc_light_dest = c;
}

// Liefert die Zielfarbe
colorLED BaseSignalClass::GetLightDest()
{
  return acc_light_dest;
}

// Liefert die aktuelle Farbe
colorLED BaseSignalClass::GetLightCurr()
{
  return acc_light_curr;
}

// Setzt die aktuelle Farbe
void BaseSignalClass::SetLightCurr(colorLED c)
{
  acc_light_curr = c;
}

// Damit werden unnötige Farbänderungen vermieden
bool BaseSignalClass::ColorChg()
{
  return acc_light_dest != acc_light_curr;
}

// Setzt den Wert für die Verzögerung
void BaseSignalClass::SetDelay(int d)
{
  updateInterval = d * delayFactor;
}

// Setzt die Adresse eines Signals
void BaseSignalClass::Set_to_address(uint16_t _to_address)
{
  acc__to_address = _to_address;
}

// Liefert die Adresse eines Signals
uint16_t BaseSignalClass::Get_to_address()
{
  return acc__to_address;
}

//*************************************************************//

// Voreinstellungen, Servonummer wird physikalisch mit
// einem Signal verbunden;
void LEDSignalClass::Attach(uint8_t ch)
{
  BaseSignalClass::Attach(ch);
  // called this way, it uses the default address 0x40
  dutyCycle_max = 255;
  dutyCycle_min = 0;
  // configure LED PWM functionalitites
  green_channel = ch * 2;
  red_channel = ch * 2 + 1;
}

// Zielfarbe ist Grün
void LEDSignalClass::GoGreen()
{
  // es ist ROT
  // jetzt umschalten auf GRÜN
  BaseSignalClass::GoGreen();
  green_dutyCycle_dest = dutyCycle_max;
  green_dutyCycle_curr = dutyCycle_min;
  green_increment = 1;
  red_dutyCycle_dest = dutyCycle_min;
  red_dutyCycle_curr = dutyCycle_max;
  red_increment = -1;
}

// Zielfarbe ist Rot
void LEDSignalClass::GoRed()
{
  // es ist GRÜN
  // jetzt umschalten auf ROT
  BaseSignalClass::GoRed();
  red_dutyCycle_dest = dutyCycle_max;
  red_dutyCycle_curr = dutyCycle_min;
  red_increment = 1;
  green_dutyCycle_dest = dutyCycle_min;
  green_dutyCycle_curr = dutyCycle_max;
  green_increment = -1;
}

// Überprüft periodisch, ob die Zielfarbe erreicht ist
void LEDSignalClass::Update()
{
  if ((micros() - lastUpdate) > updateInterval)
  { // time to update
    // GRÜN
    if (green_dutyCycle_curr != green_dutyCycle_dest)
    {
      green_dutyCycle_curr += green_increment;
      pwm_LED.setPWM(green_channel, 0, green_dutyCycle_curr);
    }
    // ROT
    if (red_dutyCycle_curr != red_dutyCycle_dest)
    {
      red_dutyCycle_curr += red_increment;
      pwm_LED.setPWM(red_channel, 0, red_dutyCycle_curr);
    }
    lastUpdate = micros();
  }
}

//*************************************************************//

// Voreinstellungen, Servonummer wird physikalisch mit
// einem Signal verbunden;
void FormSignalClass::Attach(uint8_t ch)
{
  BaseSignalClass::Attach(ch);
  firstTime = true;
  delayFactor = 500;
  // configure LED PWM functionalitites
  // ch = 0    green = 8
  // ch = 1    green = 9
  // ch = 2    green = 10
  // ch = 3    green = 11
  channel = ch;
}

// Zielfarbe ist Rot
void FormSignalClass::GoRed()
{
  // es ist ROT
  // jetzt umschalten auf GRÜN
  // dutyCycle_curr läuft von 70 auf 100
  // unnötige Bewegungen vermeiden
  // nur stellen beim ersten Aufruf
  if (firstTime == true)
  {
    dutyCycle_curr = stopAngle;
    dutyCycle_dest = dutyCycle_curr;
    firstTime = false;
  }
  else
  {
    dutyCycle_curr = startAngle;
    dutyCycle_dest = stopAngle;
  }
  curr_endAngle = endAngle;
}

// Zielfarbe ist Grün
void FormSignalClass::GoGreen()
{
  // es ist GRÜN
  // jetzt umschalten auf ROT
  // dutyCycle_curr läuft von 100 auf 70
  // unnötige Bewegungen vermeiden
  // nur stellen beim ersten Aufruf
  if (firstTime == true)
  {
    dutyCycle_curr = startAngle;
    dutyCycle_dest = dutyCycle_curr;
    firstTime = false;
  }
  else
  {
    dutyCycle_curr = stopAngle;
    dutyCycle_dest = startAngle;
  }
  curr_endAngle = -endAngle;
}

// Überprüft periodisch, ob die Zielfarbe erreicht ist
void FormSignalClass::Update()
{
  if ((micros() - lastUpdate) > updateInterval)
  { // time to update
    // GRÜN
    if (dutyCycle_curr != (dutyCycle_dest + curr_endAngle))
    {
      if ((dutyCycle_dest + curr_endAngle) > dutyCycle_curr)
        dutyCycle_curr++;
      else
        dutyCycle_curr--;
      int pwm_dc = map(dutyCycle_curr, MIN_ANGLE_WIDTH, MAX_ANGLE_WIDTH, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
      // Write to PCA9685
      pwm_Form.setPWM(channel, 0, pwm_dc);
    }
    // Überschwingen
    if ((curr_endAngle != 0) && (dutyCycle_curr == (dutyCycle_dest + curr_endAngle)))
    {
      if (curr_endAngle > 0)
        curr_endAngle--;
      else
        curr_endAngle++;
      curr_endAngle *= -1;
    }
    lastUpdate = micros();
  }
}
