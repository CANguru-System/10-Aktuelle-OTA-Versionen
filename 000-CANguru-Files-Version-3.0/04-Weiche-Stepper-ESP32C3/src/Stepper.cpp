
/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <CANguru-Buch@web.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gustav Wostrack
 * ----------------------------------------------------------------------------
 */

#include <Stepper.h>

// Voreinstellungen, steppernummer wird physikalisch mit
// einem stepper verbunden
void Stepper::Attach()
{
  // setup the pins on the microcontroller:
  pinMode(A_plus, OUTPUT);
  pinMode(A_minus, OUTPUT);
  pinMode(B_plus, OUTPUT);
  pinMode(B_minus, OUTPUT);
  btn_Stepper.begin(btn_Step_pin);
  btn_Correction.begin(btn_Corr_pin);
  stopStepper();
  last_step_time = 0;
  direction_delay = 250; // 20 * step_delay/1000;
  phase = phase0;
  readyToStep = stepsToSwitch != 0;
  set_stepsToSwitch = false;
  no_correction = true;
  rightpos = 0;
  leftpos = stepsToSwitch;
  switch (acc_pos_curr)
  {
  case right:
    currpos = rightpos;
    break;
  case left:
    currpos = leftpos;
    break;
  }
}

void Stepper::oneStep()
{
  switch (step)
  {
    // Bipolare Ansteuerung Vollschritt
    // 1a 1b 2a 2b
    // 1  0  0  1
    // 0  1  0  1
    // 0  1  1  0
    // 1  0  1  0

  case 0: // 1  0  0  1
    digitalWrite(A_plus, HIGH);
    digitalWrite(A_minus, LOW);
    digitalWrite(B_plus, LOW);
    digitalWrite(B_minus, HIGH);
    break;
  case 1: // 0  1  0  1
    digitalWrite(A_plus, LOW);
    digitalWrite(A_minus, HIGH);
    digitalWrite(B_plus, LOW);
    digitalWrite(B_minus, HIGH);
    break;
  case 2: // 0  1  1  0
    digitalWrite(A_plus, LOW);
    digitalWrite(A_minus, HIGH);
    digitalWrite(B_plus, HIGH);
    digitalWrite(B_minus, LOW);
    break;
  case 3: // 1  0  1  0
    digitalWrite(A_plus, HIGH);
    digitalWrite(A_minus, LOW);
    digitalWrite(B_plus, HIGH);
    digitalWrite(B_minus, LOW);
    break;
  }
  switch (direction)
  {
  case forward:
    step++;
    if (step >= steps)
      step = 0;
    break;
  case reverse:
    step--;
    if (step < 0)
      step = steps - 1;
    break;
  }
}

void Stepper::stopStepper()
{
  digitalWrite(A_plus, LOW);
  digitalWrite(A_minus, LOW);
  digitalWrite(B_plus, LOW);
  digitalWrite(B_minus, LOW);
}

// Setzt die Zielposition
void Stepper::SetPosition()
{
  last_step_time = micros();
  acc_pos_dest = acc_pos_curr;
  switch (acc_pos_dest)
  {
  case left:
  {
    GoLeft();
  }
  break;
  case right:
  {
    GoRight();
  }
  break;
  }
}

// Zielposition ist links
void Stepper::GoLeft()
{
  log_i("going left");
  destpos = leftpos;
  // von currpos < 74 bis 74, des halb increment positiv
  increment = 1;
  step = 0;
  direction = reverse;
  /*  endpos = -maxendpos; // +1
    way = longway;*/
}

// Zielposition ist rechts
void Stepper::GoRight()
{
  log_i("going right");
  destpos = rightpos; // 1
                      // von currpos > 1 bis 1, des halb increment negativ
  increment = -1;
  step = steps - 1;
  direction = forward;
  /*  endpos = maxendpos; // -1
    way = longway;*/
}

// Überprüft periodisch, ob die Zielposition erreicht wird
void Stepper::Update()
{
  if (readyToStep && (destpos != currpos))
  {
    if (micros() - last_step_time >= step_delay)
    {
      // get the timeStamp of when you stepped:
      last_step_time = micros();
      oneStep();
      currpos += increment;
      if (destpos == currpos)
        stopStepper();
    }
    return;
  }
  switch (phase)
  {
  case phase0:
    // waiting for initial button
    if (btn_Stepper.debounce())
    {
      // step one revolution in one direction:
      log_i("going to phase1");
      readyToStep = false;
      phase = phase1;
      direction = reverse;
      step = steps - 1;
    }
    if (btn_Correction.debounce())
    {
      // step one revolution in one direction:
      log_i("going to Correction");
      phase = phase4;
      no_correction = false;
    }
    break;

  case phase1:
    // run to one end till button is pressed
    // step one revolution in one direction:
    now_micros = micros();
    // move only if the appropriate delay has passed:
    if (now_micros - last_step_time >= step_delay)
    {
      // get the timeStamp of when you stepped:
      last_step_time = now_micros;
      oneStep();
    }
    if (btn_Stepper.debounce())
    {
      // step one revolution in one direction:
      log_i("going to phase2");
      phase = phase2;
      direction = forward;
      step = 0;
      stepsToSwitch = 0;
      delay(direction_delay);
      stopStepper();
    }
    break;

  case phase2:
    // run to the opposite end till button is pressed
    // step one revolution in one direction:
    now_micros = micros();
    // move only if the appropriate delay has passed:
    if (now_micros - last_step_time >= step_delay)
    {
      // get the timeStamp of when you stepped:
      last_step_time = now_micros;
      oneStep();
      stepsToSwitch++;
    }
    if (btn_Stepper.debounce())
    {
      // step one revolution in one direction:
      log_i("going to phase3");
      phase = phase3;
    }
    break;

  case phase3:
    stopStepper();
    readyToStep = true;
    leftpos = stepsToSwitch;
    phase = phase0;
    acc_pos_curr = left;
    GoLeft();
    currpos = 0;
    set_stepsToSwitch = true;
    break;

  case 4:
    if (btn_Stepper.debounce())
    {
      // step one revolution in one direction:
      log_i("going to phase5");
      phase = phase5;
      direction = forward;
    }
    if (btn_Correction.debounce())
    {
      // step one revolution in one direction:
      log_i("going to phase5");
      phase = phase5;
      direction = reverse;
    }
    break;

  case 5:
    // run to the opposite end till button is pressed
    // step one revolution in one direction:
    now_micros = micros();
    // move only if the appropriate delay has passed:
    if (now_micros - last_step_time >= step_delay)
    {
      // get the timeStamp of when you stepped:
      last_step_time = now_micros;
      oneStep();
    }
    if (btn_Stepper.debounce() ||
        btn_Correction.debounce())
    {
      // step one revolution in one direction:
      log_i("finish correction");
      phase = phase0;
      stopStepper();
      readyToStep = false;
      set_stepsToSwitch = false;
      no_correction = true;
    }
    break;
  }

  /*
    // Überprüfung, ob die Zielposition bereits erreicht wurde
    if (pos != (destpos + endpos))
    {
      // wenn die Zeit updateInterval vorüber ist, wird
      // wird das stepper ggf. neu gestellt
      if ((micros() - lastUpdate) > updateInterval)
      {
        // time to update
        currpos += increment;
  //      ledcWrite(channel, pos);
        lastUpdate = micros();
      }
    }
    else
    {
      // Zielposition wurde erreicht
      if (way == longway)
      {
        // Überschwingpunkt jetzt rückwärts zum Zielpunkt laufen
        increment *= -1;
        // kein Überschwingen mehr
        endpos = 0;
        // Umschalten auf noway
        way = noway;
      }
    }*/
}
