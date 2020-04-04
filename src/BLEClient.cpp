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

#include "bluefruit.h"
#include "debug.h"
#include "BLEClient.h"

BLEClientCharacteristicPower::BLEClientCharacteristicPower(void)
    : BLEClientCharacteristic(UUID16_CHR_CYCLING_POWER_MEASUREMENT) {
}

int BLEClientCharacteristicPower::process(uint8_t *data, uint16_t len) {
    uint16_t flags = data[0];
    flags |= data[1] << 8;


    _inst_power = data[2];
    _inst_power |= data[3] << 8;

    DEBUG_PRINT("Power flags = 0x%X : Power = %d W\n", flags, _inst_power);
    return 0;
}

BLEClientPower::BLEClientPower(void)
  : BLEClientService(UUID16_SVC_CYCLING_POWER) {
}

bool BLEClientPower::begin(void) {
  // Setup callback for measurement
  _power.setNotifyCallback(this->_callback, true);

  // Invoke base class begin()
  BLEClientService::begin();

  _power.begin(this);

  return true;
}

bool BLEClientPower::discover(uint16_t conn_handle) {
  // Call BLECentralService discover
  VERIFY(BLEClientService::discover(conn_handle));
  _conn_hdl = BLE_CONN_HANDLE_INVALID;  // make as invalid

  // Discover TXD, RXD characteristics
  VERIFY(1 == Bluefruit.Discovery.discoverCharacteristic(
      conn_handle, _power));

  _conn_hdl = conn_handle;
  return true;
}

uint8_t BLEClientPower::read(void) {
  return _power.read8();
}

bool BLEClientPower::enableNotify(void) {
  return _power.enableNotify();
}

bool BLEClientPower::disableNotify(void) {
  return _power.disableNotify();
}

void BLEClientPower::_callback(BLEClientCharacteristic* chr,
  uint8_t* data, uint16_t len) {
    reinterpret_cast<BLEClientCharacteristicPower*>(chr)->process(data, len);
}

BLEClientCharacteristicSandC::BLEClientCharacteristicSandC(void)
    : BLEClientCharacteristic(UUID16_CHR_CSC_MEASUREMENT) {
    _valid = 0;
    _wheel_circ = 67;

    _wheel_revs = 0;
    _wheel_event_time = 0;
    _crank_revs = 0;
    _crank_event_time = 0;

    _last_wheel_revs = 0;
    _last_wheel_event_time = 0;
    _last_crank_revs = 0;
    _last_crank_event_time = 0;

    _wheel_speed = 0;
    _crank_speed = 0;
}

float BLEClientCharacteristicSandC::calculate(void) {
  // This routine calculates the speed based
  uint32_t _revs = _wheel_revs - _last_wheel_revs;
  uint16_t _time = _wheel_event_time - _last_wheel_event_time;

  _last_wheel_revs = _wheel_revs;
  _last_wheel_event_time = _wheel_event_time;

  if (_time != 0) {
    // Check if we can calculate
    _wheel_speed = static_cast<float>(_revs)  * _wheel_circ;
    _wheel_speed /= static_cast<float>(_time) / 1024;
    _wheel_speed *= 0.00223694;  // mm.s^-1 to mph
  } else {
    return 0.0;
  }

  DEBUG_PRINT("Wheel speed = %f\n", _wheel_speed);
  return _wheel_speed;
}

int BLEClientCharacteristicSandC::process(uint8_t *data, uint16_t len) {
    // First set the valid flag to zero
    _valid = 0;

    if (len < 1) {
        return -127;
    }

    uint8_t flags = data[0];
    int doff = 1;

    if (flags & SANDC_SPEED) {
        // We have wheel rev data
        // Check length

        if (len < (doff + 6)) {
            return -127;
        }

        _wheel_revs = data[doff++];
        _wheel_revs |= data[doff++] << 8;
        _wheel_revs |= data[doff++] << 16;
        _wheel_revs |= data[doff++] << 24;

        _wheel_event_time = data[doff++];
        _wheel_event_time |= data[doff++] << 8;
    }

    if (flags & SANDC_CADENCE) {
        // We have crank rev data (cadence)
        // Check length

        if (len < (doff + 4)) {
            return -127;
        }

        _crank_revs = data[doff++];
        _crank_revs |= data[doff++] << 8;

        _crank_event_time = data[doff++];
        _crank_event_time |= data[doff++];
    }

    if (flags & SANDC_SPEED) {
        DEBUG_PRINT("Speed : revs %d : event_time %d\n",
          _wheel_revs, _wheel_event_time);
    }

    if (flags & SANDC_CADENCE) {
        DEBUG_PRINT("Cadence : revs %d : event_time %d\n",
          _crank_revs, _crank_event_time);
    }

    _valid = flags;
    return 0;
}

BLEClientSandC::BLEClientSandC(void)
  : BLEClientService(UUID16_SVC_CYCLING_SPEED_AND_CADENCE) {
}

bool BLEClientSandC::begin(void) {
  // Setup callback for measurement
  _sandc.setNotifyCallback(this->_callback, true);

  // Invoke base class begin()
  BLEClientService::begin();

  _sandc.begin(this);

  return true;
}

bool BLEClientSandC::discover(uint16_t conn_handle) {
  // Call BLECentralService discover
  VERIFY(BLEClientService::discover(conn_handle));
  _conn_hdl = BLE_CONN_HANDLE_INVALID;  // make as invalid

  // Discover TXD, RXD characteristics
  VERIFY(1 == Bluefruit.Discovery.discoverCharacteristic(
      conn_handle, _sandc));

  _conn_hdl = conn_handle;
  return true;
}

uint8_t BLEClientSandC::read(void) {
  return _sandc.read8();
}

bool BLEClientSandC::enableNotify(void) {
  return _sandc.enableNotify();
}

bool BLEClientSandC::disableNotify(void) {
  return _sandc.disableNotify();
}

void BLEClientSandC::_callback(BLEClientCharacteristic* chr,
  uint8_t* data, uint16_t len) {
    reinterpret_cast<BLEClientCharacteristicSandC*>(chr)->process(data, len);
}
