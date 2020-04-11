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

#include "config.h"
#include "debug.h"

void config_print(void) {
  DEBUG_PRINT("speed_max              = %f\n", config.speed_max);
  DEBUG_PRINT("speed_min              = %f\n", config.speed_min);
  DEBUG_PRINT("speed_threshold        = %f\n", config.speed_threshold);
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
}
