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

#define PWM_MIN 900
#define PWM_CENTER 1500
#define PWM_MAX 2100

#define CENTER_BAND_SIZE 300 // 300 means center is from 1350-1650

#define LED_VALUE_OFF 0
#define LED_VALUE_ON 100
#define LED_VALUE_HIGH 255

// GPIO PINS - check here: https://randomnerdtutorials.com/esp32-pinout-reference-gpios/

const int InputPin_Steering = 34;
const int InputPin_Throttle = 35;
const int InputPin_Aux = 36;
const int InputPin_Aux2 = 39;

// set the second value to -1 if you only need one pin for that channel
const int LedPins[NUM_CHANNELS][2] = {
    {21, 25}, // Headlights
    {23, 33}, // Brake lights
    {18, 27}, // Backup lights
    {26, 32}, // Left signals
    {19, 22}  // Right signals
};

int LedValues[NUM_CHANNELS] = {
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

// Globals

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting");

  // configure LED PWM channels & LedPins

  for (unsigned int channel = 0; channel < NUM_CHANNELS; channel++)
  {

    ledcSetup(channel, freq, resolution);
    for (unsigned int i = 0; i < 2; i++)
      if (LedPins[channel][i] > -1)
      {
        ledcAttachPin(LedPins[channel][i], channel);
      }
  }

  pinMode(InputPin_Steering, INPUT);

  xTaskCreate(inputMonitorTaskHandler, "InputMonitorTask", 1000, NULL, 1, NULL);
  // xTaskCreate(ledTaskHandler, "LedTask", 1000, NULL, 1, NULL);
}

void loop() 
{
  delay(1000);
}

void inputMonitorTaskHandler(void *parameter)
{
  unsigned long duration;

  while (1) // loop()
  {
    // Assume off, the turn on what's needed
    int brakeLights = LED_VALUE_OFF;
    int leftSignals = LED_VALUE_OFF;
    int rightSignals = LED_VALUE_OFF;

    // Read the current value
    duration = pulseIn(InputPin_Steering, HIGH);

    if (duration < 800) // No signal
    {
      leftSignals = LED_VALUE_OFF;
      brakeLights = LED_VALUE_OFF;
      rightSignals = LED_VALUE_OFF;
    }
    else if (duration > 1750) // HIGH/RIGHT
    {
      leftSignals = LED_VALUE_OFF;
      brakeLights = LED_VALUE_OFF;
      rightSignals = LED_VALUE_HIGH;
    }
    else if (duration < 1350) // LOW/LEFT
    {
      leftSignals = LED_VALUE_HIGH;
      brakeLights = LED_VALUE_OFF;
      rightSignals = LED_VALUE_OFF;
    }
    else // CENTER
    {
      leftSignals = LED_VALUE_OFF;
      brakeLights = LED_VALUE_HIGH;
      rightSignals = LED_VALUE_OFF;
    }

    ledcWrite(CHANNEL_BRAKE_LIGHTS, brakeLights);
    ledcWrite(CHANNEL_LEFT_SIGNAL, leftSignals);
    ledcWrite(CHANNEL_RIGHT_SIGNAL, rightSignals);

    Serial.printf("%lu - (%d:%d:%d)\n", duration, leftSignals, brakeLights, rightSignals);

    vTaskDelay(500);
  }
}

void ledTaskHandler(void *parameter)
{
  while (1) // loop()
  {
    for (int channel = 0; channel < NUM_CHANNELS; channel++)
    {
      // increase the LED brightness
      for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle++)
      {
        // changing the LED brightness with PWM
        ledcWrite(channel, dutyCycle);
        vTaskDelay(15);
      }

      // decrease the LED brightness
      for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle--)
      {
        // changing the LED brightness with PWM
        ledcWrite(channel, dutyCycle);
        vTaskDelay(15);
      }
    }
  }
}
