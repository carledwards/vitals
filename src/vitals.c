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
#include "string.h"
#include "stdlib.h"

const GPathInfo MINUTE_HAND_POINTS =
{
  4,
  (GPoint []) {
    { -3, 12 },
    { 4, 12 },
    { 4, -63 },
    { -3, -63 }
  }
};

const GPathInfo HOUR_HAND_POINTS = {
  4, (GPoint []){
    {-4, 12},
    {4, 12},
    {4, -43},
    {-4, -43}
  }
};

VitalsApplication app;

#if defined(PBL_RECT)
#define SCREEN_WIDTH        144
#define SCREEN_HEIGHT       168
#define TOP_BOTTOM_MARGIN     1
#define LEFT_RIGHT_MARGIN     0
#elif defined(PBL_ROUND)
#define SCREEN_WIDTH        180
#define SCREEN_HEIGHT       180
#define TOP_BOTTOM_MARGIN    12
#define LEFT_RIGHT_MARGIN     5
#endif

DateLocation get_obstructed_location_from_minutes(int minutes) {
    // this is tuned for the size of the date layer (dow + day)
    if (minutes >= 54) {
        return Top;
    }
    if (minutes >= 37) {
        return Left;
    }
    if (minutes >= 24) {
        return Bottom;
    }
    if (minutes >= 7) {
        return Right;
    }
    return Top;
}

DateLocation get_available_date_location(struct tm *t) {
    // convert the hour hand into a location on the minutes scale
    int32_t hour_as_minutes = ((((t->tm_hour % 12) * 6.) + (t->tm_min / 10.)) / (12. * 6.)) * 60.;

    // both hands on top half
    if (t->tm_min > 45 || t->tm_min <= 15) {
        if (hour_as_minutes > 45 || hour_as_minutes <= 15) {
            return Bottom;
        }
    }

    // both hands on the bottom half
    if (t->tm_min <= 45 && t->tm_min > 15) {
        if (hour_as_minutes <= 45 && hour_as_minutes > 15) {
            return Top;
        }
    }
    
    // both hands on the right side
    if (t->tm_min <= 30 && hour_as_minutes <= 30) {
        return Left;
    }
    
    // both hands on the left side
    if (t->tm_min > 30 && hour_as_minutes > 30) {
        return Right;
    }

    // the hands are split into opposite quadrants
    int hour_location = get_obstructed_location_from_minutes((int)hour_as_minutes);
    int minute_location = get_obstructed_location_from_minutes(t->tm_min);
    
    // try to put on the top
    if (hour_location != Top && minute_location != Top) {
        return Top;
    }

    // next, try the bottom
    if (hour_location != Bottom && minute_location != Bottom) {
        return Bottom;
    }

    // next, try the right
    if (hour_location != Right && minute_location != Right) {
        return Right;
    }

    return Left;
}

void layout_date_layer(struct tm *t) {
    DateLocation location = get_available_date_location(t);
    
    // don't do anything if the location hasn't changed
    if (location == app.date_location) {
        return;
    }
    app.date_location = location;

    switch(app.date_location) {
    case Right:
        layer_set_frame((Layer *)app.day_label, GRect(SCREEN_WIDTH-62-LEFT_RIGHT_MARGIN, SCREEN_HEIGHT/2-28, 40, 30));
        text_layer_set_text_alignment(app.day_label, GTextAlignmentCenter);
        layer_set_frame((Layer *)app.num_label, GRect(SCREEN_WIDTH-62-LEFT_RIGHT_MARGIN, SCREEN_HEIGHT/2-7, 40, 30));
        text_layer_set_text_alignment(app.num_label, GTextAlignmentCenter);
        break;
    case Bottom:
        layer_set_frame((Layer *)app.day_label, GRect(SCREEN_WIDTH/2 - 37, SCREEN_HEIGHT-59-TOP_BOTTOM_MARGIN, 40, 30));
        text_layer_set_text_alignment(app.day_label, GTextAlignmentRight);
        layer_set_frame((Layer *)app.num_label, GRect(SCREEN_WIDTH/2 + 9, SCREEN_HEIGHT-59-TOP_BOTTOM_MARGIN, 40, 30));
        text_layer_set_text_alignment(app.num_label, GTextAlignmentLeft);
        break;
    case Left:
        layer_set_frame((Layer *)app.day_label, GRect(25+LEFT_RIGHT_MARGIN, SCREEN_HEIGHT/2-28, 40, 30));
        text_layer_set_text_alignment(app.day_label, GTextAlignmentCenter);
        layer_set_frame((Layer *)app.num_label, GRect(25+LEFT_RIGHT_MARGIN, SCREEN_HEIGHT/2-7, 40, 30));
        text_layer_set_text_alignment(app.num_label, GTextAlignmentCenter);
        break;
    default:
        layer_set_frame((Layer *)app.day_label, GRect(SCREEN_WIDTH/2 - 37, 25+TOP_BOTTOM_MARGIN, 40, 30));
        text_layer_set_text_alignment(app.day_label, GTextAlignmentRight);
        layer_set_frame((Layer *)app.num_label, GRect(SCREEN_WIDTH/2 + 9, 25+TOP_BOTTOM_MARGIN, 40, 30));
        text_layer_set_text_alignment(app.num_label, GTextAlignmentLeft);
    }
}

void hands_update_proc(Layer *layer, GContext *ctx) {
    if (app.state != VitalsStateWatch && app.state != VitalsStateCountPulses) {
        return;
    }

    GRect bounds = layer_get_bounds(layer);
    const GPoint center = grect_center_point(&bounds);
    const int16_t secondHandLength = (bounds.size.w / 2) - 2;

    GPoint secondHand;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    int32_t second_angle = TRIG_MAX_ANGLE * (app.state == VitalsStateCountPulses ? app.timer_seconds++ : t->tm_sec) / 60;

    // second hand
    if (app.settings.seconds_hand || app.state == VitalsStateCountPulses) {
        secondHand.y = (int16_t)(-cos_lookup(second_angle) * (int32_t)(secondHandLength-2) / TRIG_MAX_RATIO) + center.y;
        secondHand.x = (int16_t)(sin_lookup(second_angle) * (int32_t)(secondHandLength-2) / TRIG_MAX_RATIO) + center.x;
        graphics_context_set_stroke_color(ctx, GColorWhite);
        graphics_draw_line(ctx, secondHand, center);
    }

    if (app.state == VitalsStateWatch) {
        graphics_context_set_fill_color(ctx, GColorWhite);
        graphics_context_set_stroke_color(ctx, GColorBlack);

        // hour hand
        gpath_rotate_to(app.hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
        gpath_draw_filled(ctx, app.hour_arrow);
        gpath_draw_outline(ctx, app.hour_arrow);

        // minute hand
        gpath_rotate_to(app.minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
        gpath_draw_filled(ctx, app.minute_arrow);
        gpath_draw_outline(ctx, app.minute_arrow);

        // update the text in the date layer
        if (t->tm_yday != app.last_tm_yday) {
            strftime(app.day_buffer, sizeof(app.day_buffer), "%a", t);
            text_layer_set_text(app.day_label, app.day_buffer);

            strftime(app.num_buffer, sizeof(app.num_buffer), "%d", t);
            text_layer_set_text(app.num_label, app.num_buffer);
            app.last_tm_yday = t->tm_yday;
        }
        if (t->tm_hour != app.last_tm_hour || t->tm_min != app.last_tm_min) {
            layout_date_layer(t);
            app.last_tm_hour = t->tm_hour;
            app.last_tm_min = t->tm_min;
        }
        // draw the dot in the middle
        graphics_context_set_fill_color(ctx, GColorWhite);
        graphics_fill_circle(ctx, center, 4);
        graphics_context_set_stroke_color(ctx, GColorBlack);
        graphics_draw_circle(ctx, center, 4);
    }
}

void handle_timer_tick(struct tm *tick_time, TimeUnits units_changed) {
    layer_mark_dirty(app.hands_layer);
}

void subscribe_tick_timer() {
    tick_timer_service_unsubscribe();
    tick_timer_service_subscribe(
        app.settings.seconds_hand || app.state == VitalsStateCountPulses ? SECOND_UNIT : MINUTE_UNIT, 
        handle_timer_tick);
}

void timeout_timer_callback(void *data) {
    app.timeout_timer = (AppTimer *)NULL;
    if (app.settings.vibrate) {
        vibes_double_pulse();
    }
    light_enable(false);
    app_set_state(VitalsStateWatch);
}

void delay_timer_callback(void *data) {
    app.delay_timer = (AppTimer *)NULL;
    if (app.settings.vibrate) {
        vibes_double_pulse();
    }
    light_enable(true);
    app.timeout_timer = app_timer_register(
        app.settings.timeout * 1000, timeout_timer_callback, (void *)0);
}

void switch_mode_handler(ClickRecognizerRef recognizer, void *context) {
    if (app.state == VitalsStateCountPulses) {
        app_set_state(VitalsStateWatch);
    }
    else if (app.state == VitalsStateWatch) {
        app_set_state(VitalsStateCountPulses);
    }
}

void back_button_handler(ClickRecognizerRef recognizer, void *context) {
    if (app.state == VitalsStateCountPulses) {
        app_set_state(VitalsStateWatch);
    }
    else if (app.state == VitalsStateWatch) {
        // exit the app
        window_stack_pop_all(true);
    }
}

void app_set_state(VitalsState new_state) {
    VitalsState old_state = app.state;
    bool state_changed = old_state != new_state;
    
    if (state_changed == false) {
        return;
    }
    app.state = new_state;

    if (old_state == VitalsStateCountPulses) {
        if (app.delay_timer) {
            app_timer_cancel(app.delay_timer);
            app.delay_timer = (AppTimer *)0;
        }
        if (app.timeout_timer) {
            app_timer_cancel(app.timeout_timer);
            app.timeout_timer = (AppTimer *)0;
        }
        light_enable(false);
    }

    subscribe_tick_timer();
    
    switch(app.state) {
    case VitalsStateWatch:
        layer_set_hidden(bitmap_layer_get_layer(app.heart_image_layer), true);
        layer_set_hidden(app.date_layer, false);
        break;
    case VitalsStateCountPulses:
        layer_set_hidden(bitmap_layer_get_layer(app.heart_image_layer), false);
        layer_set_hidden(app.date_layer, true);
        app.delay_timer = app_timer_register(
            app.settings.delay * 1000, delay_timer_callback, (void *)0);
        app.timer_seconds = 60-app.settings.delay < 60 ? 60-app.settings.delay : 0;
        break;
    case VitalsStateSettings:
        if (window_stack_contains_window(app.settings_window)) {
            APP_LOG(APP_LOG_LEVEL_WARNING, "Window already in window stack");
            return;
        }
        window_stack_push(app.settings_window, true);
        break;
    }
}

void back_button_long_click_handler(ClickRecognizerRef recognizer, void *context) {
    app_set_state(VitalsStateSettings);
}

void window_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler)switch_mode_handler);
    window_single_click_subscribe(BUTTON_ID_BACK, (ClickHandler)back_button_handler);
    window_long_click_subscribe(BUTTON_ID_SELECT, 0, (ClickHandler)back_button_long_click_handler, (ClickHandler)NULL);
}

void window_load(Window *window) {
    window_set_click_config_provider(window, (ClickConfigProvider) window_config_provider);

    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    // set the watchface background image
    app.watchface_background_image = gbitmap_create_with_resource(RESOURCE_ID_WATCHFACE_BACKGROUND);
    app.watchface_image_layer = bitmap_layer_create(bounds);
    bitmap_layer_set_bitmap(app.watchface_image_layer, app.watchface_background_image);
    bitmap_layer_set_alignment(app.watchface_image_layer, GAlignCenter);
    layer_add_child(window_layer, bitmap_layer_get_layer(app.watchface_image_layer));

    // load the heart image
    app.heart_image = gbitmap_create_with_resource(RESOURCE_ID_HEART);
    app.heart_image_layer = bitmap_layer_create(bounds);
    bitmap_layer_set_bitmap(app.heart_image_layer, app.heart_image);
    bitmap_layer_set_alignment(app.heart_image_layer, GAlignCenter);
    layer_set_hidden(bitmap_layer_get_layer(app.heart_image_layer), true);
    layer_add_child(window_layer, bitmap_layer_get_layer(app.heart_image_layer));

    // create the date layer
    app.date_layer = layer_create(bounds);
    layer_add_child(window_layer, app.date_layer);

    // init day
    app.day_label = text_layer_create(GRect(0, 0, 40, 32));
    text_layer_set_text(app.day_label, "");
    text_layer_set_background_color(app.day_label, GColorClear);
    text_layer_set_text_color(app.day_label, GColorWhite);
    text_layer_set_text_alignment(app.day_label, GTextAlignmentCenter);
    GFont norm18 = fonts_get_system_font(FONT_KEY_GOTHIC_24);
    text_layer_set_font(app.day_label, norm18);
    layer_add_child(app.date_layer, text_layer_get_layer(app.day_label));

    // init num
    app.num_label = text_layer_create(GRect(0, 22, 40, 32));
    text_layer_set_text(app.num_label, "");
    text_layer_set_background_color(app.num_label, GColorClear);
    text_layer_set_text_color(app.num_label, GColorWhite);
    GFont bold18 = fonts_get_system_font(FONT_KEY_GOTHIC_24);
    text_layer_set_text_alignment(app.num_label, GTextAlignmentCenter);
    text_layer_set_font(app.num_label, bold18);
    layer_add_child(app.date_layer, text_layer_get_layer(app.num_label));

    // init hands
    app.hands_layer = layer_create(bounds);
    layer_set_update_proc(app.hands_layer, hands_update_proc);
    layer_add_child(window_layer, app.hands_layer);
}

void window_unload(Window *window) {
    gbitmap_destroy(app.watchface_background_image);
    bitmap_layer_destroy(app.watchface_image_layer);
    gbitmap_destroy(app.heart_image);
    bitmap_layer_destroy(app.heart_image_layer);
    layer_destroy(app.hands_layer);
    layer_destroy(app.date_layer);
    text_layer_destroy(app.day_label);
    text_layer_destroy(app.num_label);
}

void app_init(void) {
    settings_init();
    
    app.delay_timer = (AppTimer *)0;
    app.timeout_timer = (AppTimer *)0;
    app.date_location = -1;
    app.last_tm_yday = -1;
    app.last_tm_hour = -1;
    app.last_tm_min = -1;
    app_set_state(VitalsStateWatch);

    app.window = window_create();

#ifdef PBL_SDK_2
    window_set_fullscreen(app.window, true);
#endif

    window_set_window_handlers(app.window, (WindowHandlers) {
        .load = window_load,
         .unload = window_unload,
    });

    // init hand paths
    app.minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
    app.hour_arrow = gpath_create(&HOUR_HAND_POINTS);

    Layer *window_layer = window_get_root_layer(app.window);
    GRect bounds = layer_get_bounds(window_layer);
    GPoint center = grect_center_point(&bounds);
    
#ifdef PBL_SDK_2
    center.y = center.y+8;
#endif

    gpath_move_to(app.minute_arrow, center);
    gpath_move_to(app.hour_arrow, center);

    // Push the window onto the stack
    const bool animated = true;
    window_stack_push(app.window, animated);

    subscribe_tick_timer();
}

void app_deinit(void) {
    gpath_destroy(app.minute_arrow);
    gpath_destroy(app.hour_arrow);

    tick_timer_service_unsubscribe();
    window_destroy(app.window);
}

int main(void) {
    app_init();
    app_event_loop();
    app_deinit();
}
