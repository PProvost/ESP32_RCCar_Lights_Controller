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

// Defined in input.cpp
void inputMonitorTaskHandler(void *parameter);
void handleHeadlightsPwm(int *states);
void handleSteeringPwm(int *states);
void handleThrottlePwm(int *states);
// Defined in output.cpp
void ledTaskHandler(void *parameter);

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
        ledcAttachPin(LedPins[channel][i], channel);
  }

  // Configure input pins for PWM, buttons, etc.
  pinMode(InputPin_Steering, INPUT);
  pinMode(InputPin_Throttle, INPUT);
  pinMode(InputPin_Headlights, INPUT);

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