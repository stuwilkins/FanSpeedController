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

#include <bluefruit.h>
#include "BLEClient.h"
#include "bluetooth.h"

BLEClientSandC  clientSandC;
BLEClientPower  clientPower;

void scan_callback(ble_gap_evt_adv_report_t* report) {
  // Serial.print("Found device with MAC ");
  // Serial.printBufferReverse(report->peer_addr.addr, 6, ':');
  // Serial.println();
  // Serial.printf("Signal %14s %d dBm\n", "RSSI", report->rssi);

  if ( Bluefruit.Scanner.checkReportForService(report, clientSandC) ) {
    uint8_t mac[] = {0x99, 0xE8, 0x2C, 0xFB, 0x62, 0xFC};

    if (!memcmp(report->peer_addr.addr, mac, 6)) {
      Serial.println("Connecting to speed and cadence sensor");
      Serial.printf("Signal %14s %d dBm\n", "RSSI", report->rssi);
      Bluefruit.Central.connect(report);
      Bluefruit.Scanner.stop();
      return;
    } else {
      Serial.println("Skipping speed and cadence sensor, MAC does not match.");
    }
  }

  if ( Bluefruit.Scanner.checkReportForService(report, clientPower) ) {
    uint8_t mac[] = {0x99, 0xE8, 0x2C, 0xFB, 0x62, 0xFC};

    if (!memcmp(report->peer_addr.addr, mac, 6)) {
      Serial.println("Connecting to power sensor");
      Serial.printf("Signal %14s %d dBm\n", "RSSI", report->rssi);
      Bluefruit.Central.connect(report);
      Bluefruit.Scanner.stop();
      return;
    } else {
      Serial.println("Skipping power sensor, MAC does not match.");
    }
  }

  Bluefruit.Scanner.resume();
}

void connect_callback(uint16_t conn_handle) {
  if ( !clientSandC.discover(conn_handle) ) {
    Bluefruit.disconnect(conn_handle);
    Serial.println("Unable to discover SandC device.");
    return;
  }

  if ( !clientSandC.enableNotify() ) {
    Serial.println("Couldn't enable notify for SandC measurement.");
    return;
  }

  if ( !clientPower.discover(conn_handle) ) {
    Bluefruit.disconnect(conn_handle);
    Serial.println("Unable to discover power device.");
    return;
  }

  if ( !clientPower.enableNotify() ) {
    Serial.println("Couldn't enable notify for Power measurement.");
    return;
  }

  // indicator.setPixelColor(0, INDICATOR_CONNECTED);
  // indicator.show();
}

void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  (void) conn_handle;

  Serial.print("Disconnected, reason = 0x");
  Serial.println(reason, HEX);
  // indicator.setPixelColor(0, INDICATOR_OK);
  // indicator.show();
}

void setup_bluetooth(void) {
  Bluefruit.begin(0, 1);
  Bluefruit.setName(BT_NAME);
  clientSandC.begin();
  clientPower.begin();

  Bluefruit.setConnLedInterval(250);

  Bluefruit.Central.setConnectCallback(connect_callback);
  Bluefruit.Central.setDisconnectCallback(disconnect_callback);

  Bluefruit.Scanner.setRxCallback(scan_callback);
  Bluefruit.Scanner.restartOnDisconnect(true);
  Bluefruit.Scanner.setInterval(160, 80);  // in unit of 0.625 ms
  Bluefruit.Scanner.useActiveScan(false);
  Bluefruit.Scanner.start(0);
}

float calculate_bluetooth_speed(void) {
  return clientSandC.getSandC()->calculate();
}
