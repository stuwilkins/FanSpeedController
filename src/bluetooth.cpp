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
#include "debug.h"
#include "bluetooth.h"
#include "indicator.h"

BLEUart bleuart;
BLEClientSandC  clientSandC;
BLEClientPower  clientPower;

bool power_connected = false;
bool sandc_connected = false;

uint8_t bluetooth_mac_whitelist[][6] = {{0x99, 0xE8, 0x2C, 0xFB, 0x62, 0xFC},
                                        // {0x94, 0x4F, 0xD9, 0xDA, 0x39, 0xDD},
                                        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

static void defaultCallback(const char *cmd, const int cmd_len, void * ctx) {}
bluetoothFuncPtr_t uart_usr_rx_callback = defaultCallback;
void* uart_usr_rx_callback_ptr = NULL;

bool check_mac_whitelist(uint8_t *mac) {
  int i = 0;
  uint8_t zero[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  while (memcmp(bluetooth_mac_whitelist[i], zero, 6)) {
    if (!memcmp(bluetooth_mac_whitelist[i], mac, 6)) {
      DEBUG_PRINT("MAC %02X:%02X:%02X:%02X:%02X:%02X"
                  " is in whitelist\n",
                  mac[5], mac[4], mac[3],
                  mac[2], mac[1], mac[0]);
      return true;
    }
    i++;
  }

  DEBUG_PRINT("MAC %02X:%02X:%02X:%02X:%02X:%02X"
              " is NOT whitelist\n",
              mac[5], mac[4], mac[3],
              mac[2], mac[1], mac[0]);
  return false;
}

void scan_callback(ble_gap_evt_adv_report_t* report) {
  // DEBUG_PRINT("Found device with MAC %02X:%02X:%02X:%02X:%02X:%02X"
  //   " Signal = %d dBm\n",
  //   report->peer_addr.addr[5], report->peer_addr.addr[4],
  //   report->peer_addr.addr[3], report->peer_addr.addr[2],
  //   report->peer_addr.addr[1], report->peer_addr.addr[0],
  //   report->rssi);

  if (check_mac_whitelist(report->peer_addr.addr)) {
    DEBUG_PRINT("Connecting to device with MAC %02X:%02X:%02X:%02X:%02X:%02X"
      " Signal = %d dBm\n",
      report->peer_addr.addr[5], report->peer_addr.addr[4],
      report->peer_addr.addr[3], report->peer_addr.addr[2],
      report->peer_addr.addr[1], report->peer_addr.addr[0],
      report->rssi);
    Bluefruit.Central.connect(report);
  }

  Bluefruit.Scanner.resume();
}

void connect_callback(uint16_t conn_handle) {
  BLEConnection* connection = Bluefruit.Connection(conn_handle);
  char peer_name[32] = { 0 };
  connection->getPeerName(peer_name, sizeof(peer_name));
  DEBUG_PRINT("Connected to [%s]\n", peer_name);

  bool notify = false;
  if (clientSandC.discover(conn_handle)) {
    if ( !clientSandC.enableNotify() ) {
      DEBUG_COMMENT("Couldn't enable notify for SandC measurement.\n");
      Bluefruit.disconnect(conn_handle);
    } else {
      DEBUG_COMMENT("Enabled notify on sandc\n");
      notify = true;
    }
  }

  if (clientPower.discover(conn_handle)) {
    if ( !clientPower.enableNotify() ) {
      DEBUG_COMMENT("Couldn't enable notify for Power measurement.\n");
      Bluefruit.disconnect(conn_handle);
    } else {
      DEBUG_COMMENT("Enabled notify on power\n");
      notify = true;
    }
  }

  if (!notify) {
      Bluefruit.disconnect(conn_handle);
  }
}

void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  (void) conn_handle;

  DEBUG_PRINT("Disconnected, reason = 0x%02X\n", reason);
}

void uart_connect_callback(uint16_t conn_handle) {
  (void) conn_handle;
  DEBUG_COMMENT("Device connected to UART\n");
}

void uart_disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  (void) reason;
  (void) conn_handle;

  DEBUG_COMMENT("UART Disconnected\n");
}

void uart_rx_callback(uint16_t conn_handle) {
  (void) conn_handle;

  // Forward data from Mobile to our peripheral
  char str[20+1] = { 0 };
  bleuart.read(str, 20);

  DEBUG_PRINT("Recieved : %s\n", str);

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
  Bluefruit.begin(1, 2);
  Bluefruit.setTxPower(4);
  Bluefruit.setName(BT_NAME);

  // Set Connect / Disconnect Callbacks
  Bluefruit.Periph.setConnectCallback(uart_connect_callback);
  Bluefruit.Periph.setDisconnectCallback(uart_disconnect_callback);
  Bluefruit.Central.setConnectCallback(connect_callback);
  Bluefruit.Central.setDisconnectCallback(disconnect_callback);

  clientSandC.begin();
  clientPower.begin();

  // Configure and Start BLE Uart Service
  bleuart.begin();
  bleuart.setRxCallback(uart_rx_callback);

  Bluefruit.Scanner.setRxCallback(scan_callback);
  Bluefruit.Scanner.restartOnDisconnect(true);
  Bluefruit.Scanner.filterUuid(clientSandC.uuid, clientPower.uuid);
  Bluefruit.Scanner.setInterval(160, 80);  // in unit of 0.625 ms
  Bluefruit.Scanner.useActiveScan(false);
  Bluefruit.Scanner.start(0);

  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);  // number of seconds in fast mode
  Bluefruit.Advertising.start(0);  // 0 = Don't stop advertising after n seconds

  Bluefruit.setConnLedInterval(250);
}
