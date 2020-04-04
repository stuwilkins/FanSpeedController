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

BLEUart bleuart;
BLEClientSandC  clientSandC;
BLEClientPower  clientPower;

static void defaultCallback(const char *cmd, const int cmd_len, void * ctx) {}
bluetoothFuncPtr_t uart_usr_rx_callback = defaultCallback;
void* uart_usr_rx_callback_ptr = NULL;

void scan_callback(ble_gap_evt_adv_report_t* report) {
  // Serial.print("Found device with MAC ");
  // Serial.printBufferReverse(report->peer_addr.addr, 6, ':');
  // Serial.printf(" Signal %14s %d dBm\n", "RSSI", report->rssi);

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

void uart_connect_callback(uint16_t conn_handle) {
  (void) conn_handle;
  Serial.println("UART Connected");
}

void uart_disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  (void) reason;
  (void) conn_handle;

  Serial.println("UART Disconnected");
}

void uart_rx_callback(uint16_t conn_handle) {
  (void) conn_handle;

  // Forward data from Mobile to our peripheral
  char str[20+1] = { 0 };
  bleuart.read(str, 20);

  Serial.print("[Prph] RX: ");
  Serial.println(str);

  // Now do callback
  int str_len = 20;
  (*uart_usr_rx_callback)(str, str_len, uart_usr_rx_callback_ptr);
}

float calculate_bluetooth_speed(void) {
  return clientSandC.getSandC()->calculate();
}

void bluetooth_set_rx_callback(bluetoothFuncPtr_t func, void* ctx) {
  uart_usr_rx_callback = func;
  uart_usr_rx_callback_ptr = ctx;
}

void bluetooth_setup(void) {
  Bluefruit.begin(2, 2);
  Bluefruit.setTxPower(8);
  Bluefruit.setName(BT_NAME);

  // Set Connect / Disconnect Callbacks
  Bluefruit.Periph.setConnectCallback(uart_connect_callback);
  Bluefruit.Periph.setDisconnectCallback(uart_disconnect_callback);
  Bluefruit.Central.setConnectCallback(connect_callback);
  Bluefruit.Central.setDisconnectCallback(disconnect_callback);

  // Configure and Start BLE Uart Service
  bleuart.begin();
  bleuart.setRxCallback(uart_rx_callback);

  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);  // number of seconds in fast mode
  Bluefruit.Advertising.start(0);  // 0 = Don't stop advertising after n seconds

  Bluefruit.Scanner.setRxCallback(scan_callback);
  Bluefruit.Scanner.restartOnDisconnect(true);
  Bluefruit.Scanner.setInterval(160, 80);  // in unit of 0.625 ms
  Bluefruit.Scanner.useActiveScan(false);
  Bluefruit.Scanner.start(0);

  clientSandC.begin();
  clientPower.begin();

  Bluefruit.setConnLedInterval(250);
}
