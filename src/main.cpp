/*
   Copyright 2019 Peter Provost <peter@provost.org>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <Arduino.h>
#include "configuration.h"

// Shared state to store each LED output value (including a bitmask for blink)
int channelStates[NUM_CHANNELS] = {
    0, // CHANNEL_HEAD_LIGHTS
    0, // CHANNEL_BRAKE_LIGHTS
    0, // CHANNEL_BACKUP_LIGHTS
    0, // CHANNEL_LEFT_SIGNAL
    0, // CHANNEL_RIGHT_SIGNAL
};

// Forward declarations
void ledTaskHandler(void *parameter);
void inputMonitorTaskHandler(void *parameter);
void handleHeadlightsPwm(int *states);
void handleSteeringPwm(int *states);
void handleThrottlePwm(int *states);

// Globals
void setup()
{
  Serial.begin(115200);
  Serial.println("Starting");
  Serial.printf("setup() and loop() running on core %d\n", xPortGetCoreID());

  // Configure Output LED PWM channels & LedPins
  for (unsigned int channel = 0; channel < NUM_CHANNELS; channel++)
  {

    ledcSetup(channel, OUTPUT_PWM_FREQ, OUTPUT_PWM_RES);
    for (unsigned int i = 0; i < 2; i++)
      if (LedPins[channel][i] > -1)
      {
        ledcAttachPin(LedPins[channel][i], channel);
      }
  }

  // Configure input pins for PWM, buttons, etc.
  pinMode(InputPin_Steering, INPUT);
  pinMode(InputPin_Throttle, INPUT);
  pinMode(InputPin_Headlights, INPUT);
  // pinMode(InputPin_Aux2, INPUT);

  // Start a tasks for the LED update code
  xTaskCreate(ledTaskHandler, "LedTask", 10000, NULL, 1, NULL);
}

void loop()
{
  int channelStatesTemp[NUM_CHANNELS];
  int channel;

  // Default if no rule fires is for the LED to be off.
  for (channel = 0; channel < NUM_CHANNELS; channel++)
    channelStatesTemp[channel] = CHANNEL_STATE_OFF;

  // Input processing rules (order matters!)
  handleHeadlightsPwm(channelStatesTemp);
  handleSteeringPwm(channelStatesTemp);
  handleThrottlePwm(channelStatesTemp);

  for (channel = 0; channel < NUM_CHANNELS; channel++)
    channelStates[channel] = channelStatesTemp[channel];

  yield();
}

typedef enum
{
  PWMSTATE_NO_SIGNAL,
  PWMSTATE_LOW,
  PWMSTATE_CENTER,
  PWMSTATE_HIGH
} PwmState;

PwmState GetPwmState(unsigned long pwmValue)
{
  if ((pwmValue < PWM_MIN) || (pwmValue > PWM_MAX))
    return PWMSTATE_NO_SIGNAL;
  else if (pwmValue > PWM_BAND_SEP_HIGH)
    return PWMSTATE_HIGH;
  else if (pwmValue < PWM_BAND_SEP_LOW)
    return PWMSTATE_LOW;
  else
    return PWMSTATE_CENTER;
}

void handleHeadlightsPwm(int *states)
{
  unsigned long headlightsPwm;
  headlightsPwm = pulseIn(InputPin_Headlights, HIGH, PULSEIN_TIMOUT);
  PwmState headlightsPwmState = GetPwmState(headlightsPwm);

  switch (headlightsPwmState)
  {
  case PWMSTATE_NO_SIGNAL:
    states[CHANNEL_HEAD_LIGHTS] = CHANNEL_STATE_ON | CHANNEL_STATE_MASK_BLINK;
    break;
  case PWMSTATE_HIGH:
    states[CHANNEL_HEAD_LIGHTS] = CHANNEL_STATE_ON;
    break;
  default:
    break;
  }

  // Serial.printf("Headlights PWM: %lu (%d) -> (0x%x)\n", headlightsPwm, headlightsPwmState, states[CHANNEL_BRAKE_LIGHTS]);
}

void handleThrottlePwm(int *states)
{
  unsigned long throttlePwm;
  throttlePwm = pulseIn(InputPin_Throttle, HIGH, PULSEIN_TIMOUT);

  PwmState throttlePwmState = GetPwmState(throttlePwm);
  switch (throttlePwmState)
  {
  case PWMSTATE_NO_SIGNAL:
    states[CHANNEL_BRAKE_LIGHTS] = CHANNEL_STATE_ON | CHANNEL_STATE_MASK_BLINK;
    states[CHANNEL_BACKUP_LIGHTS] = CHANNEL_STATE_ON | CHANNEL_STATE_MASK_BLINK;
    break;
  case PWMSTATE_CENTER:
    states[CHANNEL_BRAKE_LIGHTS] = CHANNEL_STATE_ON;
    break;
  case PWMSTATE_HIGH:
    break;
  case PWMSTATE_LOW:
    states[CHANNEL_BACKUP_LIGHTS] = CHANNEL_STATE_ON;
    break;
  default:
    break;
  }

  // Serial.printf("Throttle PWM: %lu (%d) -> (0x%x)\n", throttlePwm, throttlePwmState, states[CHANNEL_BRAKE_LIGHTS]);
}

void handleSteeringPwm(int *states)
{
  unsigned long steeringPwm;
  steeringPwm = pulseIn(InputPin_Steering, HIGH, PULSEIN_TIMOUT);

  PwmState steeringPwmState = GetPwmState(steeringPwm);
  switch (steeringPwmState)
  {
  case PWMSTATE_NO_SIGNAL:
    states[CHANNEL_LEFT_SIGNAL] = CHANNEL_STATE_ON | CHANNEL_STATE_MASK_BLINK;
    states[CHANNEL_RIGHT_SIGNAL] = CHANNEL_STATE_ON | CHANNEL_STATE_MASK_BLINK;
    break;
  case PWMSTATE_LOW: // Left
    states[CHANNEL_LEFT_SIGNAL] = CHANNEL_STATE_ON | CHANNEL_STATE_MASK_BLINK;
    break;
  case PWMSTATE_HIGH: // Right
    states[CHANNEL_RIGHT_SIGNAL] = CHANNEL_STATE_ON | CHANNEL_STATE_MASK_BLINK;
    break;
  case PWMSTATE_CENTER:
  default:
    break;
  }

  // Serial.printf("Steering PWM: %lu -> (0x%x:0x%x)\n", steeringPwm, states[CHANNEL_LEFT_SIGNAL], states[CHANNEL_RIGHT_SIGNAL]);
}

void ledTaskHandler(void *parameter)
{
  bool isBlinking = false, lastBlinkOn = false;
  int channel, channelState, blinkState = CHANNEL_STATE_OFF;
  // int channelStatesTemp[NUM_CHANNELS];
  TickType_t currentTicks = 0, lastBlinkTicks = 0;

  Serial.printf("ledTaskHandler() running on core %d\n", xPortGetCoreID());

  while (1) // equivalent of loop() for this task
  {
    // Figure out the blinking state (on or off) based on time
    // Any channel with the blink flag will use this to control the blinking
    currentTicks = xTaskGetTickCount() / portTICK_PERIOD_MS;
    if ((currentTicks - lastBlinkTicks) > BLINK_PERIOD_MS) // Time to swap the blink state
    {
      if (lastBlinkOn)
      {
        blinkState = CHANNEL_STATE_OFF;
        lastBlinkOn = false;
      }
      else
      {
        blinkState = CHANNEL_STATE_ON;
        lastBlinkOn = true;
      }
      lastBlinkTicks = currentTicks;
    }

    // Process each output channel as required
    for (channel = 0; channel < NUM_CHANNELS; channel++)
    {
      channelState = channelStates[channel];

      isBlinking = ((channelState & CHANNEL_STATE_MASK_BLINK) >> 8) == 1;
      channelState &= 0xFF; // remove the mask

      if (isBlinking)
        ledcWrite(channel, (blinkState == CHANNEL_STATE_ON ? channelState : blinkState));
      else
        ledcWrite(channel, channelState);
    }
    
    vTaskDelay(TASK_DELAY);
  }
}
