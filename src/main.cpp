//
// MIT License
//
// Copyright (c) 2020 Stuart Wilkins
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include <Arduino.h>
#include <bluefruit.h>
#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include "BLEClientSandC.h"
#include "inttimer.h"

#define PIN_MAINS_CLOCK       6
#define PIN_FAN_1             5
#define PIN_FAN_2             9

#define BT_NAME         "FAN_CONTROLLER"

#define SPIWIFI       SPI  // The SPI port
#define SPIWIFI_SS    13   // Chip select pin
#define ESP32_RESETN  12   // Reset pin
#define SPIWIFI_ACK   11   // a.k.a BUSY or READY pin
#define ESP32_GPIO0   -1

//
// User Interface
//

Adafruit_NeoPixel indicator(1, PIN_NEOPIXEL, NEO_GRB);
#define INDICATOR_BOOT        indicator.Color(255, 255, 0)
#define INDICATOR_OK          indicator.Color(0, 255, 0)
#define INDICATOR_CONNECTED   indicator.Color(0, 0, 255)

//
// Bluetooth Setup
//

BLEClientSandC  clientSandC;

//
// ISR / Timer
//

// TimerHandle_t softtimer;
TimerClass timer1(4, 0);
// TimerClass timer2(4, 0);

volatile int zero_cross_clock = 0;
unsigned long zero_cross_last_clock = 0;
volatile bool zero_cross_trigger_1 = false;
volatile bool zero_cross_trigger_2 = false;
volatile long int zero_cross_micros = 0;
unsigned long fan1_delay = 0;
unsigned long fan2_delay = 0;
unsigned long hardtimer_count = 0;

void zero_crossing_isr(void) {
  zero_cross_clock++;
  zero_cross_trigger_1 = true;
  zero_cross_trigger_2 = true;
  zero_cross_micros = micros();
}

void hardtimer_callback(void) {
  // if (fan1_delay > 0) {
  //   if (zero_cross_trigger_1) {
  //     if ((micros() - zero_cross_micros) > fan1_delay) {
  //       digitalWrite(PIN_FAN_1, HIGH);
  //       delayMicroseconds(50);
  //       digitalWrite(PIN_FAN_1, LOW);
  //       zero_cross_trigger_1 = false;
  //     }
  //   }
  // } else {
  //   zero_cross_trigger_1 = false;
  // }

  if (zero_cross_trigger_2) {
    if ((micros() - zero_cross_micros) > fan2_delay)
    {
      digitalWrite(PIN_FAN_2, HIGH);
      delayMicroseconds(50);
      digitalWrite(PIN_FAN_2, LOW);
      zero_cross_trigger_2 = false;
    }
  }

  hardtimer_count++;
  timer1.attachInterrupt(5);
}

float calc_mains_freq(void) {
  float _freq = zero_cross_clock;
  _freq /= ((float)millis() - (float)zero_cross_last_clock);
  _freq *= 500;  // Convert to seconds (and 2 per cycle)

  zero_cross_last_clock = millis();
  zero_cross_clock = 0;

  return _freq;
}

void scan_callback(ble_gap_evt_adv_report_t* report) {
  if ( Bluefruit.Scanner.checkReportForService(report, clientSandC) ) {
    Serial.print("Found device with MAC ");
    Serial.printBufferReverse(report->peer_addr.addr, 6, ':');
    Serial.println();
    Serial.printf("Signal %14s %d dBm\n", "RSSI", report->rssi);
    Bluefruit.Central.connect(report);
  } else {
    Bluefruit.Scanner.resume();
  }
}

void connect_callback(uint16_t conn_handle) {
  if ( !clientSandC.discover(conn_handle) ) {
    Bluefruit.disconnect(conn_handle);
    Serial.println("Unable to discover device.");
    return;
  }

  if ( !clientSandC.enableNotify() ) {
    Serial.println("Couldn't enable notify for SandC Measurement.");
    return;
  }

  indicator.setPixelColor(0, INDICATOR_CONNECTED);
  indicator.show();
}

void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  (void) conn_handle;

  Serial.print("Disconnected, reason = 0x");
  Serial.println(reason, HEX);
  indicator.setPixelColor(0, INDICATOR_OK);
  indicator.show();
}

void setup() {
  // Setup Input / Output

  pinMode(LED_BUILTIN,      OUTPUT);
  pinMode(PIN_NEOPIXEL,     INPUT_PULLUP);
  pinMode(PIN_MAINS_CLOCK,  INPUT_PULLUP);
  pinMode(PIN_FAN_1,        OUTPUT);
  pinMode(PIN_FAN_2,        OUTPUT);
  digitalWrite(PIN_FAN_1,   LOW);
  digitalWrite(PIN_FAN_2,   LOW);

  // Start, set indicator to RED

  indicator.begin();
  indicator.setBrightness(10);
  indicator.setPixelColor(0, INDICATOR_BOOT);
  indicator.show();

  // Setup Serial Monitor

  Serial.begin(115200);
  while (!Serial) delay(10);   // for nrf52840 with native usb

  // Setup Bluetooth

  Bluefruit.begin(0, 1);
  Bluefruit.setName(BT_NAME);
  // Setup timer

  timer1.setCallback(hardtimer_callback);
  timer1.init();
  timer1.attachInterrupt(5);
  digitalWrite(LED_BUILTIN, LOW);
  // timer2.setCallback(timer2_callback);

  // softtimer = xTimerCreate("Data Timer", pdMS_TO_TICKS(3000),
  //   pdTRUE, 0, softtimer_callback);

  // if (softtimer == NULL) {
  //   Serial.println("Timer 1 can not be created");
  // } else {
  //   xTimerStart(softtimer, 0);
  // }

  // Configure Speed and Cadence Client
  clientSandC.begin();

  // Increase Blink rate to different from PrPh advertising mode
  Bluefruit.setConnLedInterval(500);

  Bluefruit.Central.setConnectCallback(connect_callback);
  Bluefruit.Central.setDisconnectCallback(disconnect_callback);

  Bluefruit.Scanner.setRxCallback(scan_callback);
  Bluefruit.Scanner.restartOnDisconnect(true);
  Bluefruit.Scanner.setInterval(160, 80);  // in unit of 0.625 ms
  Bluefruit.Scanner.useActiveScan(false);
  Bluefruit.Scanner.start(0);

  // Setup WiFi
  WiFi.setPins(SPIWIFI_SS, SPIWIFI_ACK, ESP32_RESETN, ESP32_GPIO0, &SPIWIFI);
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }
  Serial.print("Wifi firmware :");
  Serial.println(WiFi.firmwareVersion());

  indicator.setPixelColor(0, INDICATOR_OK);
  indicator.show();

  fan1_delay = 0;
  fan2_delay = 1;

  attachInterrupt(digitalPinToInterrupt(PIN_MAINS_CLOCK), zero_crossing_isr, FALLING);
}

void loop() {
  Serial.print("Mains Frequency = ");
  Serial.println(calc_mains_freq());
  delay(250);
  Serial.print("Fan 1 period = ");
  Serial.println(fan1_delay);
  delay(250);
  Serial.print("Fan 2 period = ");
  Serial.println(fan2_delay);
  delay(250);
  Serial.print("Hardtimer count = ");
  Serial.println(hardtimer_count);
  // if ( Bluefruit.Central.connected() )
  // {
  //   // Not discovered yet
  //   if ( clientSandC.discovered() )
  //   {
  //     // Discovered means in working state
  //     // Get Serial input and send to Peripheral
  //     //if ( Serial.available() )
  //     //{
  //     //  delay(2); // delay a bit for all characters to arrive

  //     //  char str[20+1] = { 0 };
  //     //  Serial.readBytes(str, 20);

  //     //  clientUart.print( str );
  //     //}
  //   }
  // }
  // // put your main code here, to run repeatedly:

  delay(2000);
}
