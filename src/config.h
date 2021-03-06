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

#ifndef SRC_CONFIG_H_
#define SRC_CONFIG_H_

#define WATCHDOG_TIMEOUT            2000
#define SERIAL_TIMEOUT              5000
#define CONFIG_JSON_SIZE            2048
#define CONFIG_FILENAME             "settings.json"

typedef struct {
    float speed_max;
    float speed_min;
    float speed_threshold;
    unsigned long triac_off_delay;
    unsigned long triac_on_delay;
    uint8_t bt_speed_sensor_id[6];
    uint8_t bt_power_sensor_id[6];
} config_data;

extern config_data config;

void config_print(void);
void config_set_defaults(void);

#endif  // SRC_CONFIG_H_
