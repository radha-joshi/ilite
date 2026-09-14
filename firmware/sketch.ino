#include "config.h"

// Hardware Pins
const int ledPin = 2;  // Your Streetlight
const int pirPin = 14; // Your Motion Sensor
const int ldrPin = 34; // Assuming you used 34 for Analog. Change if needed!

// The ESP32 reads analog values from 0 to 4095 (unlike Uno's 0 to 1023)
const int lightThreshold = 2000;

// Link to your Adafruit IO Feeds (Names must match exactly)
AdafruitIO_Feed *lightFeed = io.feed("light-status");
AdafruitIO_Feed *motionFeed = io.feed("motion-status");
AdafruitIO_Feed *logFeed = io.feed("system-logs");

void setup()
{
  Serial.begin(115200);

  pinMode(pirPin, INPUT_PULLDOWN);
  pinMode(ledPin, OUTPUT);

  // Connect to the Cloud
  Serial.print("Connecting to I-LITE Dashboard");
  io.connect();

  // Wait for the Wokwi-GUEST wifi to connect
  while (io.status() < AIO_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nSystem Status: ONLINE!");
}

void loop()
{
  // Required to keep the MQTT connection alive
  io.run();

  bool isNight = analogRead(ldrPin) < lightThreshold;
  bool motion = digitalRead(pirPin);
  Serial.println(motion);

  // State Machine: Only send data to Adafruit when the environment actually changes
  static int lastState = -1;
  int currentState = 0;

  if (isNight && motion)
  {
    analogWrite(ledPin, 255);
    currentState = 2;
  }
  else if (isNight && !motion)
  {
    analogWrite(ledPin, 51);
    currentState = 1;
  }
  else
  {
    analogWrite(ledPin, 0);
    currentState = 0;
  }

  // If a change happened, push the new data to the cloud
  if (currentState != lastState)
  {
    if (currentState == 2)
    {
      lightFeed->save(100);
      motionFeed->save("DETECTED");
      logFeed->save("Trigger: NIGHT + Motion -> LED 100%");
      Serial.println("Trigger: NIGHT + Motion -> LED 100%");
    }
    else if (currentState == 1)
    {
      lightFeed->save(20);
      motionFeed->save("CLEAR");
      logFeed->save("Trigger: NIGHT + Clear -> LED 20%");
      Serial.println("Trigger: NIGHT + Clear -> LED 20%");
    }
    else
    {
      lightFeed->save(0);
      motionFeed->save("STANDBY");
      logFeed->save("Trigger: DAY -> LED OFF");
      Serial.println("Trigger: DAY -> LED OFF");
    }
    lastState = currentState;
  }

  delay(200);
}
