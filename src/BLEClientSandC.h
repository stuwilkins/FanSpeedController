#ifndef BLECLIENTSANDC_H_
#define BLECLIENTSANDC_H_

#include "bluefruit_common.h"
#include "BLEClientCharacteristic.h"
#include "BLEClientService.h"

#define SANDC_SPEED         0x01
#define SANDC_CADENCE       0x02

class BLEClientCharacteristicSandC : public BLEClientCharacteristic {
  public:
    BLEClientCharacteristicSandC(void);
    int process(uint8_t *data, uint16_t len);

  private:
    float _wheel_circ;
    bool _valid;
    uint32_t _wheel_revs;
    uint16_t _wheel_event_time;
    uint16_t _crank_revs;
    uint16_t _crank_event_time;
    uint32_t _last_wheel_revs;
    uint16_t _last_wheel_event_time;
    uint16_t _last_crank_revs;
    uint16_t _last_crank_event_time;
};

class BLEClientSandC : public BLEClientService
{
  public:
    BLEClientSandC(void);

    virtual bool  begin(void);
    virtual bool  discover(uint16_t conn_handle);

    uint8_t read(void);

    bool enableNotify(void);
    bool disableNotify(void);

    BLEClientCharacteristic* getSandC(void) {
      return &_sandc;
    }

  private:
    BLEClientCharacteristicSandC _sandc;
    static void _callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);
};

#endif /* BLECLIENTSANDC_H_ */