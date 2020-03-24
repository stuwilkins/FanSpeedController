#include "bluefruit.h"
#include "BLEClientSandC.h"

BLEClientCharacteristicSandC::BLEClientCharacteristicSandC(void)
    : BLEClientCharacteristic(UUID16_CHR_CSC_MEASUREMENT) {
    _valid = 0;

    _wheel_revs = 0;
    _wheel_event_time = 0;
    _crank_revs = 0;
    _crank_event_time = 0;

    _last_wheel_revs = 0;
    _last_wheel_event_time = 0;
    _last_crank_revs = 0;
    _last_crank_event_time = 0;
}

int BLEClientCharacteristicSandC::process(uint8_t *data, uint16_t len) {
    // First set the valid flag to zero

    Serial.print("Data = ");
    Serial.printBuffer(data, len, ':');
    Serial.println();

    _valid = 0;

    if (len < 1) {
        return -127;
    }

    uint8_t flags = data[0];
    int doff = 1;

    if (flags & SANDC_SPEED) {
        // We have wheel rev data
        // Check length

        if(len < (doff + 6)) {
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

        if(len < (doff + 4)) {
            return -127;
        }

        _crank_revs = data[doff++];
        _crank_revs |= data[doff++] << 8;

        _crank_event_time = data[doff++];
        _crank_event_time |= data[doff++];
    }

    // Print some debug
    if (flags & SANDC_SPEED) {
        Serial.printf("Speed : revs %d : event_time %d\n", _wheel_revs, _wheel_event_time);
    }
    if(flags & SANDC_CADENCE) {
        Serial.printf("Cadence : revs %d : event_time %d\n", _crank_revs, _crank_event_time);
    }

    _valid = flags;
    return 0;
}

BLEClientSandC::BLEClientSandC(void)
  : BLEClientService(UUID16_SVC_CYCLING_SPEED_AND_CADENCE)
{

}

bool BLEClientSandC::begin(void)
{
  // Setup callback for measurement
  _sandc.setNotifyCallback(this->_callback, true);

  // Invoke base class begin()
  BLEClientService::begin();

  _sandc.begin(this);

  return true;
}

bool BLEClientSandC::discover(uint16_t conn_handle)
{
  // Call BLECentralService discover
  VERIFY( BLEClientService::discover(conn_handle) );
  _conn_hdl = BLE_CONN_HANDLE_INVALID; // make as invalid until we found all chars

  // Discover TXD, RXD characteristics
  VERIFY( 1 == Bluefruit.Discovery.discoverCharacteristic(conn_handle, _sandc) );

  _conn_hdl = conn_handle;
  return true;
}

uint8_t BLEClientSandC::read(void)
{
  return _sandc.read8();
}

bool BLEClientSandC::enableNotify(void)
{
  return _sandc.enableNotify();
}

bool BLEClientSandC::disableNotify(void)
{
  return _sandc.disableNotify();
}

void BLEClientSandC::_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len) {
    ((BLEClientCharacteristicSandC*)chr)->process(data, len);
}