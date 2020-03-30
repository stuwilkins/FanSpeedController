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
#include "BLEClientSandC.h"
#include "bluetooth.h"

BLEClientSandC  clientSandC;

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

  // Increase Blink rate to different from PrPh advertising mode
  Bluefruit.setConnLedInterval(250);

  Bluefruit.Central.setConnectCallback(connect_callback);
  Bluefruit.Central.setDisconnectCallback(disconnect_callback);

  Bluefruit.Scanner.setRxCallback(scan_callback);
  Bluefruit.Scanner.restartOnDisconnect(true);
  Bluefruit.Scanner.setInterval(160, 80);  // in unit of 0.625 ms
  Bluefruit.Scanner.useActiveScan(false);
  Bluefruit.Scanner.start(0);
}

void calculate_bluetooth(void) {
  clientSandC.getSandC()->calculate();
}