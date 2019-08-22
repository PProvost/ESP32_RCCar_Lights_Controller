#include <Arduino.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <freertos/FreeRTOS.h>
// #include <freertos/task.h>

// CHANNELS
#define NUM_CHANNELS 5
#define CHANNEL_HEAD_LIGHTS 0
#define CHANNEL_BRAKE_LIGHTS 1
#define CHANNEL_BACKUP_LIGHTS 2
#define CHANNEL_LEFT_SIGNAL 3
#define CHANNEL_RIGHT_SIGNAL 4

#define BLINK_PERIOD_MS 500
#define TASK_DELAY 100 // used for debugging
#define PULSEIN_TIMOUT 25000UL // micro-seconds - smaller is more responsive, too small and it won't read

#define PWM_MIN 900
#define PWM_CENTER 1500
#define PWM_MAX 2100
#define PWM_BAND_SEP_LOW 1350
#define PWM_BAND_SEP_HIGH 1650

#define CENTER_BAND_SIZE 300 // 300 means center is from 1350-1650

// GPIO PINS - check here: https://randomnerdtutorials.com/esp32-pinout-reference-gpios/

const int InputPin_Headlights = 34;
const int InputPin_Steering = 35;
const int InputPin_Throttle = 37;
const int InputPin_Aux2 = 39;

// DEFINE THE OUTPUT LED PINS WE WILL USE FOR LIGHTS
// Note: Set the second value to -1 if you only need one pin for that channel
const int LedPins[NUM_CHANNELS][2] = {
    {21, 25}, // Headlights
    {23, 33}, // Brake lights
    {18, 27}, // Backup lights
    {26, 32}, // Left signals
    {19, 22}  // Right signals
};

// DEFINE THE CHANNEL STATES
// Note: Adjust the constants below (not the mask) to change the intensities for the different states
#define CHANNEL_STATE_OFF 0
#define CHANNEL_STATE_ON 100
#define CHANNEL_STATE_HIGH 0xFF           // 255
#define CHANNEL_STATE_MASK_BLINK (1 << 8) // You can Bitwise-OR this with the value to make it blink

int ChannelStates[NUM_CHANNELS] = {
    0, // CHANNEL_HEAD_LIGHTS
    0, // CHANNEL_BRAKE_LIGHTS
    0, // CHANNEL_BACKUP_LIGHTS
    0, // CHANNEL_LEFT_SIGNAL
    0, // CHANNEL_RIGHT_SIGNAL
};

// Output PWM properties for LED channels
const int freq = 5000;
const int resolution = 8;

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

  Serial.printf("setup() running on core %d\n", xPortGetCoreID());

  // Configure Output LED PWM channels & LedPins
  for (unsigned int channel = 0; channel < NUM_CHANNELS; channel++)
  {

    ledcSetup(channel, freq, resolution);
    for (unsigned int i = 0; i < 2; i++)
      if (LedPins[channel][i] > -1)
      {
        ledcAttachPin(LedPins[channel][i], channel);
      }
  }

  // Configure input pins for PWM, buttons, etc.
  pinMode(InputPin_Headlights, INPUT);
  pinMode(InputPin_Steering, INPUT);
  pinMode(InputPin_Throttle, INPUT);
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

  // Atomic update of state
  // if (xSemaphoreTake(channelStatesSemaphore, (TickType_t)100) == pdTRUE)
  {
    for (channel = 0; channel < NUM_CHANNELS; channel++)
      ChannelStates[channel] = channelStatesTemp[channel];
    // xSemaphoreGive(channelStatesSemaphore);
  }

  // vTaskDelay(TASK_DELAY); // TODO - make 100 or smaller after debugging
  yield();
  // delay(500); // TODO - make 100 or smaller after debugging
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
    states[CHANNEL_HEAD_LIGHTS] = CHANNEL_STATE_OFF;
    break;
  }
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
    break;
  case PWMSTATE_CENTER:
    states[CHANNEL_BRAKE_LIGHTS] = CHANNEL_STATE_ON;
    break;
  case PWMSTATE_HIGH:
  case PWMSTATE_LOW:
  default:
    states[CHANNEL_BRAKE_LIGHTS] = CHANNEL_STATE_OFF;
    break;
  }

  Serial.printf("Throttle PWM: %lu (%d) -> (0x%x)\n", throttlePwm, throttlePwmState, states[CHANNEL_BRAKE_LIGHTS]);
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

void inputMonitorTaskHandler(void *parameter)
{
  int channelStatesTemp[NUM_CHANNELS];

  // Semaphore must exist or the task can't run
  // if (channelStatesSemaphore == NULL)
  //   return;

  while (1) // loop()
  {
    // Default if no rule fires is for the LED to be off.
    for (int i = 0; i < NUM_CHANNELS; i++)
      channelStatesTemp[i] = CHANNEL_STATE_OFF;

    // Input processing rules (order matters!)
    handleHeadlightsPwm(channelStatesTemp);
    handleSteeringPwm(channelStatesTemp);
    handleThrottlePwm(channelStatesTemp);

    // Atomic update of state
    // if (xSemaphoreTake(channelStatesSemaphore, (TickType_t)100) == pdTRUE)
    {
      for (int channel = 0; channel < NUM_CHANNELS; channel++)
        ChannelStates[channel] = channelStatesTemp[channel];
      // xSemaphoreGive(channelStatesSemaphore);
    }

    vTaskDelay(TASK_DELAY); // TODO - make 100 or smaller after debugging
  }
}

void ledTaskHandler(void *parameter)
{
  bool isBlinking = false, lastBlinkOn = false;
  int channel, channelState;
  int channelStatesTemp[NUM_CHANNELS];
  TickType_t currentTicks = 0, lastBlinkTicks = 0;

  Serial.printf("ledTaskHandler() running on core %d\n", xPortGetCoreID());

  // Semaphore must exist or the task can't run
  // if (channelStatesSemaphore == NULL)
  //   return;

  while (1) // loop()
  {
    // Copy the shared state into local and release the semaphore
    // if (xSemaphoreTake(channelStatesSemaphore, (TickType_t)100) == pdTRUE)
    {
      for (int channel = 0; channel < NUM_CHANNELS; channel++)
        channelStatesTemp[channel] = ChannelStates[channel];
      // xSemaphoreGive(channelStatesSemaphore);
    }
    // else
    // continue;

    for (channel = 0; channel < NUM_CHANNELS; channel++)
    {
      channelState = channelStatesTemp[channel];

      isBlinking = ((channelState & CHANNEL_STATE_MASK_BLINK) >> 8) == 1;
      channelState &= 0xFF; // remove the mask

      if (isBlinking == false)
        ledcWrite(channel, channelState);
      else
      {
        currentTicks = xTaskGetTickCount() / portTICK_PERIOD_MS;

        // Serial.printf("Blinking: %d, %d - ", currentTicks, lastBlinkTicks);

        // if 500 ms have passed since turning it on, turn it off
        if ((currentTicks - lastBlinkTicks) > BLINK_PERIOD_MS) // 0.5s have passed since we set the blinker on or off
        {
          // Serial.printf("[TOGGLE]\n");
          if (lastBlinkOn)
          {
            ledcWrite(channel, CHANNEL_STATE_OFF);
            lastBlinkOn = false;
          }
          else
          {
            ledcWrite(channel, channelState);
            lastBlinkOn = true;
          }
          lastBlinkTicks = currentTicks;
        }
      }
    }
    vTaskDelay(TASK_DELAY); // TODO - make 100 or smaller after debugging
  }
}
