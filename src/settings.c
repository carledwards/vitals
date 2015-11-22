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

#include "vitals.h"
#include "settings.h"
#include "pebble.h"

#define TIMEOUT_SETTINGS_KEY 1
#define DELAY_SETTINGS_KEY 2
#define VIBRATE_SETTINGS_KEY 3
#define SECONDS_HAND_SETTINGS_KEY 4

#define TIMEOUT_DEFAULT 30
#define DELAY_DEFAULT 5
#define VIBRATE_DEFAULT true
#define SECONDS_HAND_DEFAULT true

typedef enum {
    VitalsMenuTimeout = 0,
    VitalsMenuDelay,
    VitalsMenuVibrate,
    VitalsMenuSecondsHand,
    VitalsMenuItemCount
} VitalsMenuId; // Aliases for each menu item by index

static SimpleMenuLayer *settings_menu_layer;
static SimpleMenuItem settings_menu_items[VitalsMenuItemCount];
static SimpleMenuSection settings_menu_section_root;
static SimpleMenuSection settings_menu_section_all[1];

void vitals_update_menus() {
    SimpleMenuItem *m;
    static char beatTimeString[30];
    static char startDelayString[30];
    static char vibrationString[30];
    static char secondsHandString[30];
    for (VitalsMenuId id = 0; id < VitalsMenuItemCount; id++) {
        m = &settings_menu_items[id];
        switch (id) {
            case VitalsMenuTimeout:
                m->title = "HB Count Time";
                snprintf(beatTimeString, ARRAY_LENGTH(beatTimeString), "%d seconds", app.settings.timeout);
                m->subtitle = beatTimeString;
                break;
                
            case VitalsMenuDelay:
                m->title = "Start Delay";
                snprintf(startDelayString, ARRAY_LENGTH(startDelayString), "%d seconds", app.settings.delay);
                m->subtitle = startDelayString;
                break;
                
            case VitalsMenuVibrate:
                m->title = "Vibration";
                snprintf(vibrationString, ARRAY_LENGTH(vibrationString), "%s", app.settings.vibrate ? "ON" : "OFF");
                m->subtitle = vibrationString;
                break;
                
            case VitalsMenuSecondsHand:
                m->title = "Seconds Hand";
                snprintf(secondsHandString, ARRAY_LENGTH(secondsHandString), "%s", app.settings.seconds_hand ? "ON" : "OFF");
                m->subtitle = secondsHandString;
                break;
                
            default:
                break;
        }
    }
    menu_layer_reload_data((MenuLayer *)settings_menu_layer);
}

void set_timeout(int timeout) {
    app.settings.timeout = timeout;
    persist_write_int(TIMEOUT_SETTINGS_KEY, timeout);
}

void set_delay(int delay) {
    app.settings.delay = delay;
    persist_write_int(DELAY_SETTINGS_KEY, delay);
}

void set_vibrate(bool vibrate) {
    app.settings.vibrate = vibrate;
    persist_write_int(VIBRATE_SETTINGS_KEY, vibrate);
}

void set_seconds_hand(bool enabled) {
    app.settings.seconds_hand = enabled;
    persist_write_int(SECONDS_HAND_SETTINGS_KEY, enabled);
}

void settings_menu_select(int index, void *context) {
    VitalsSettings *s = &app.settings;
    VitalsMenuId id = index;
    switch (id) {
        case VitalsMenuTimeout:
            if (s->timeout == 15) {
                set_timeout(30);
            }
            else if (s->timeout == 30) {
                set_timeout(60);
            }
            else {
                set_timeout(15);
            }
            break;
        
        case VitalsMenuDelay:
            if (s->delay == 3) {
                set_delay(5);
            }
            else if (s->delay == 5) {
                set_delay(10);
            }
            else {
                set_delay(3);
            }
            break;

        case VitalsMenuVibrate:
            set_vibrate(!s->vibrate);
            break;

        case VitalsMenuSecondsHand:
            set_seconds_hand(!s->seconds_hand);
            break;
            
        default:
            return;
    }
    vitals_update_menus();
}

void load_settings_from_storage() {
    if (persist_exists(TIMEOUT_SETTINGS_KEY) == false) {
        set_timeout(TIMEOUT_DEFAULT);
    }
    else {
        app.settings.timeout = persist_read_int(TIMEOUT_SETTINGS_KEY);
    }
    
    if (persist_exists(DELAY_SETTINGS_KEY) == false) {
        set_delay(DELAY_DEFAULT);
    }
    else {
        app.settings.delay = persist_read_int(DELAY_SETTINGS_KEY);
    }
    
    if (persist_exists(VIBRATE_SETTINGS_KEY) == false) {
        set_vibrate(VIBRATE_DEFAULT);
    }
    else {
        app.settings.vibrate = persist_read_bool(VIBRATE_SETTINGS_KEY);
    }

    if (persist_exists(SECONDS_HAND_SETTINGS_KEY) == false) {
        set_seconds_hand(SECONDS_HAND_DEFAULT);
    }
    else {
        app.settings.seconds_hand = persist_read_bool(SECONDS_HAND_SETTINGS_KEY);
    }
}

void settings_window_load(Window *window) {
    vitals_update_menus();
}

void settings_window_unload(Window *window) {
    app_set_state(VitalsStateWatch);
}

void settings_init() {
    load_settings_from_storage();
    
    app.settings_window = window_create();

#ifdef PBL_SDK_2
    window_set_fullscreen(app.settings_window, true);
#endif

    window_set_background_color(app.settings_window, GColorWhite);
    window_set_window_handlers(app.settings_window, (WindowHandlers){
        .load = settings_window_load,
        .unload = settings_window_unload,
    });
    
    for (VitalsMenuId id = 0; id < VitalsMenuItemCount; id++) {
        settings_menu_items[id].callback = settings_menu_select;
    }
    settings_menu_section_root.items = settings_menu_items;
    settings_menu_section_root.num_items = VitalsMenuItemCount;
#if PBL_ROUND
    settings_menu_section_root.title = "     SETTINGS";
#else
    settings_menu_section_root.title = "SETTINGS";
#endif
    settings_menu_section_all[0] = settings_menu_section_root;
    
    // setup menu layer
    settings_menu_layer = simple_menu_layer_create(
        layer_get_frame(window_get_root_layer(app.settings_window)),
        app.settings_window,
        settings_menu_section_all,
        1,
        NULL);
    layer_add_child(window_get_root_layer(app.settings_window), simple_menu_layer_get_layer(settings_menu_layer));
}
