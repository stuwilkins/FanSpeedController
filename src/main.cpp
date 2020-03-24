#include <Arduino.h>
#include <bluefruit.h>
#include <Adafruit_NeoPixel.h>
#include "BLEClientSandC.h"

#define BT_NAME         "FAN_CONTROLLER"

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

TimerHandle_t timer1;
TimerHandle_t timer2;

void timer1_callback(TimerHandle_t timer) {
  (void) timer;
  // digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void timer2_callback(TimerHandle_t timer) {
  (void) timer;
  clientSandC.getSandC()->calculate();
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void scan_callback(ble_gap_evt_adv_report_t* report)
{
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

void connect_callback(uint16_t conn_handle)
{
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

void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  (void) conn_handle;

  Serial.print("Disconnected, reason = 0x");
  Serial.println(reason, HEX);
  indicator.setPixelColor(0, INDICATOR_OK);
  indicator.show();
}

void setup() {
  // Setup Input / Output

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_NEOPIXEL, INPUT_PULLUP);

  // Start, set indicator to RED

  indicator.begin();
  indicator.setBrightness(10);
  indicator.setPixelColor(0, INDICATOR_BOOT);
  indicator.show();

  // Setup Serial Monitor

  Serial.begin(115200);
  while ( !Serial ) delay(10);   // for nrf52840 with native usb

  // Setup Bluetooth

  Bluefruit.begin(0, 1);
  Bluefruit.setName(BT_NAME);
  // Setup timer

  digitalWrite(LED_BUILTIN, 1);
  //  timer.begin(1, flash_led);
  // timer.start();

  timer1 = xTimerCreate("TRIAC Timer", pdMS_TO_TICKS(10),
    pdTRUE, 0, timer1_callback);
  timer2 = xTimerCreate("Data Timer", pdMS_TO_TICKS(5000),
    pdTRUE, 0, timer2_callback);

  if (timer1 == NULL) {
    Serial.println("Timer can not be created");
  } else {
    xTimerStart(timer1, 0);
  }

  // Configure Speed and Cadence Client
  clientSandC.begin();

  // Increase Blink rate to different from PrPh advertising mode
  Bluefruit.setConnLedInterval(500);

  Bluefruit.Central.setConnectCallback(connect_callback);
  Bluefruit.Central.setDisconnectCallback(disconnect_callback);

  Bluefruit.Scanner.setRxCallback(scan_callback);
  Bluefruit.Scanner.restartOnDisconnect(true);
  Bluefruit.Scanner.setInterval(160, 80); // in unit of 0.625 ms
  Bluefruit.Scanner.useActiveScan(false);
  Bluefruit.Scanner.start(0);

  indicator.setPixelColor(0, INDICATOR_OK);
  indicator.show();
}

void loop() {
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

  delay(100);
}