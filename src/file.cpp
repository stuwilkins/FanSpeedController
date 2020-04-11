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

#include <inttypes.h>
#include <SdFat.h>
#include <Adafruit_SPIFlash.h>
#include <Adafruit_TinyUSB.h>
#include <ArduinoJson.h>
#include "data.h"
#include "debug.h"
#include "file.h"

Adafruit_FlashTransport_QSPI flashTransport;
Adafruit_SPIFlash flash(&flashTransport);
FatFileSystem fatfs;

// USB Mass Storage object
Adafruit_USBD_MSC usb_msc;

bool changed = false;

// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and
// return number of copied bytes (must be multiple of block size)
int32_t msc_read_cb(uint32_t lba, void* buffer, uint32_t bufsize) {
  // Note: SPIFLash Bock API: readBlocks/writeBlocks/syncBlocks
  // already include 4K sector caching internally.
  // We don't need to cache it, yahhhh!!
  return flash.readBlocks(lba, reinterpret_cast<uint8_t*>(buffer),
    bufsize/512) ? bufsize : -1;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and
// return number of written bytes (must be multiple of block size)
int32_t msc_write_cb(uint32_t lba, uint8_t* buffer, uint32_t bufsize) {
  digitalWrite(LED_BUILTIN, HIGH);

  // Note: SPIFLash Bock API: readBlocks/writeBlocks/syncBlocks
  // already include 4K sector caching internally.
  // We don't need to cache it, yahhhh!!
  return flash.writeBlocks(lba, buffer, bufsize/512) ? bufsize : -1;
}

// Callback invoked when WRITE10 command is completed
// (status received and accepted by host).
// used to flush any pending cache.
void msc_flush_cb(void) {
  // sync with flash
  flash.syncBlocks();

  // clear file system's cache to force refresh
  fatfs.cacheClear();

  digitalWrite(LED_BUILTIN, LOW);
  changed = true;
}

bool file_data_changed(void) {
  bool rtn = changed;
  changed = false;
  return rtn;
}

int file_write_config(const char *filename) {
  DEBUG_PRINT("Writing config file [%s]\n", filename);
  File myFile = fatfs.open(filename, O_RDWR | O_CREAT);
  if (!myFile) {
    DEBUG_COMMENT("Failed to open file.\n");
    return -127;
  }

  StaticJsonDocument<CONFIG_JSON_SIZE> doc;
  serializeJson(doc, myFile);
  myFile.println();
  myFile.close();

  return 0;
}

int file_read_config(const char* filename) {
  // Allocate
  StaticJsonDocument<CONFIG_JSON_SIZE> doc;

  DEBUG_PRINT("Reading config file [%s]\n", filename);
  File file = fatfs.open(filename, O_RDONLY);
  if (file) {
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
      DEBUG_COMMENT("Failed to parse JSON\n");
      DEBUG_PRINT("Error = %s\n", error.c_str());
      return -127;
    }

    // Now load the structs

    config.max_speed = doc["speed"]["max_speed"] | 15.0;
    config.min_speed = doc["speed"]["min_speed"] | 5.0;
    config.triac_off_delay = doc["triac"]["off_delay"] | 0L;
    config.triac_on_delay = doc["triac"]["on_delay"] | 0L;

    // Process mac addresses
    int mac[6];
    sscanf(doc["speed"]["sensor_id"].as<char *>(),
      "%X:%X:%X:%X:%X:%X",
      &mac[5], &mac[4], &mac[3], &mac[2], &mac[1], &mac[0]);
    for (int i=0; i < 6; i++) {
      config.bt_speed_sensor_id[i] = static_cast<uint8_t>(mac[i]);
    }
    sscanf(doc["power"]["sensor_id"].as<char *>(),
      "%X:%X:%X:%X:%X:%X",
      &mac[5], &mac[4], &mac[3], &mac[2], &mac[1], &mac[0]);
    for (int i=0; i < 6; i++) {
      config.bt_power_sensor_id[i] = static_cast<uint8_t>(mac[i]);
    }

    // Print out config

    DEBUG_PRINT("max_speed              = %f\n", config.max_speed);
    DEBUG_PRINT("min_speed              = %f\n", config.min_speed);
    DEBUG_PRINT("triac_on_delay         = %ld\n", config.triac_on_delay);
    DEBUG_PRINT("triac_off_delay        = %ld\n", config.triac_off_delay);
    DEBUG_PRINT("bt_speed_sensor_id     = %02X:%02X:%02X:%02X:%02X:%02X\n",
      config.bt_speed_sensor_id[5], config.bt_speed_sensor_id[4],
      config.bt_speed_sensor_id[3], config.bt_speed_sensor_id[2],
      config.bt_speed_sensor_id[1], config.bt_speed_sensor_id[0]);
    DEBUG_PRINT("bt_power_sensor_id     = %02X:%02X:%02X:%02X:%02X:%02X\n",
      config.bt_power_sensor_id[5], config.bt_power_sensor_id[4],
      config.bt_power_sensor_id[3], config.bt_power_sensor_id[2],
      config.bt_power_sensor_id[1], config.bt_power_sensor_id[0]);

    return 0;
  }

  DEBUG_COMMENT("Failed to open file.\n");
  return 0;
}

void file_loop(void) {
  if (file_data_changed()) {
    DEBUG_COMMENT("Filesystem Changed\n");
    file_read_config(CONFIG_FILENAME);
  }
}

FatFileSystem file_setup(void) {
  DEBUG_COMMENT("Setting up flash\n");

  flash.begin();

  // Set disk vendor id, product id and revision with string
  // up to 8, 16, 4 characters respectively
  usb_msc.setID("Wilkins", "External Flash", "1.0");

  // Set callback
  usb_msc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);

  // Set disk size, block size should be 512 regardless of spi flash page size
  usb_msc.setCapacity(flash.pageSize()*flash.numPages()/512, 512);

  // MSC is ready for read/write
  usb_msc.setUnitReady(true);

  usb_msc.begin();

  // Init file system on the flash
  fatfs.begin(&flash);

  DEBUG_PRINT("JEDEC ID: 0x%X\n", flash.getJEDECID());
  DEBUG_PRINT("Flash size: %d\n", flash.size());

  changed = true;  // Trigger on setup

  StaticJsonDocument<200> doc;
  doc["sensor"] = "gps";
  doc["time"] = 1351824120;

  return fatfs;
}
