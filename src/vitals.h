/***
    Copyright 2014 Carl Edwards

    Licensed to the Apache Software Foundation (ASF) under one
    or more contributor license agreements.  See the NOTICE file
    distributed with this work for additional information
    regarding copyright ownership.  The ASF licenses this file
    to you under the Apache License, Version 2.0 (the
    "License"); you may not use this file except in compliance
    with the License.  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing,
    software distributed under the License is distributed on an
    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
    KIND, either express or implied.  See the License for the
    specific language governing permissions and limitations
    under the License.
*/

#pragma once

#include "pebble.h"

typedef enum {
    Top,
    Right,
    Bottom,
    Left
} DateLocation;

typedef enum {
    VitalsStateWatch,
    VitalsStateCountPulses,
    VitalsStateSettings
} VitalsState;

typedef struct {
    int timeout;
    int delay;
    bool vibrate;
    bool seconds_hand;
} VitalsSettings;

typedef struct {
    Window *window;
    Layer *date_layer;
    Layer *hands_layer;
    TextLayer *day_label;
    TextLayer *num_label;
    AppTimer *timeout_timer;
    AppTimer *delay_timer;
    DateLocation date_location;
    GPath *minute_arrow;
    GPath *hour_arrow;
    GBitmap *watchface_background_image;
    BitmapLayer *watchface_image_layer;
    GBitmap *heart_image;
    BitmapLayer *heart_image_layer;

    char day_buffer[6];
    char num_buffer[4];

    int timer_seconds;

    int last_tm_yday;
    int last_tm_hour;
    int last_tm_min;

    VitalsState state;
    
    VitalsSettings settings;

    Window *settings_window;

} VitalsApplication;

void app_set_state(VitalsState new_state);

extern VitalsApplication app;