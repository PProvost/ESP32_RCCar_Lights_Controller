#include <Arduino.h>
#include "configuration.h"

extern int channelStates[NUM_CHANNELS];

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
