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

#ifndef SRC_BLECLIENT_H_
#define SRC_BLECLIENT_H_

#include "bluefruit_common.h"
#include "BLEClientCharacteristic.h"
#include "BLEClientService.h"

#define SANDC_SPEED         0x01
#define SANDC_CADENCE       0x02

class BLEClientCharacteristicPower : public BLEClientCharacteristic {
 public:
  BLEClientCharacteristicPower(void);
  int process(uint8_t *data, uint16_t len);
 private:
  float _wheel_circ;
  float _wheel_speed;
  float _crank_speed;

  int16_t _inst_power;

  uint32_t _wheel_revs;
  uint16_t _wheel_event_time;
  uint16_t _crank_revs;
  uint16_t _crank_event_time;
  uint32_t _last_wheel_revs;
  uint16_t _last_wheel_event_time;
  uint16_t _last_crank_revs;
  uint16_t _last_crank_event_time;
};

class BLEClientCharacteristicSandC : public BLEClientCharacteristic {
 public:
  BLEClientCharacteristicSandC(void);
  int process(uint8_t *data, uint16_t len);
  float calculate(void);

 private:
  bool _valid;

  float _wheel_circ;
  float _wheel_speed;
  float _crank_speed;

  uint32_t _wheel_revs;
  uint16_t _wheel_event_time;
  uint16_t _crank_revs;
  uint16_t _crank_event_time;
  uint32_t _last_wheel_revs;
  uint16_t _last_wheel_event_time;
  uint16_t _last_crank_revs;
  uint16_t _last_crank_event_time;
};

class BLEClientPower : public BLEClientService {
 public:
  BLEClientPower(void);

  virtual bool begin(void);
  virtual bool discover(uint16_t conn_handle);

  uint8_t read(void);
  bool enableNotify(void);
  bool disableNotify(void);

 private:
  BLEClientCharacteristicPower _power;
  static void _callback(BLEClientCharacteristic* chr,
    uint8_t* data, uint16_t len);
};

class BLEClientSandC : public BLEClientService {
 public:
  BLEClientSandC(void);

  virtual bool begin(void);
  virtual bool discover(uint16_t conn_handle);

  uint8_t read(void);
  bool enableNotify(void);
  bool disableNotify(void);

  BLEClientCharacteristicSandC* getSandC(void) {
    return &_sandc;
  }

 private:
  BLEClientCharacteristicSandC _sandc;
  static void _callback(BLEClientCharacteristic* chr,
    uint8_t* data, uint16_t len);
};

#endif  // SRC_BLECLIENT_H_
