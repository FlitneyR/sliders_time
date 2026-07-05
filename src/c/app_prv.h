#ifndef APP_PRV_H
#define APP_PRV_H

#include <pebble.h>
#include "app.h"
#include "slider.h"

typedef enum AnimationMode {
    // modes
    AnimationMode_PerMinute = 0,
    AnimationMode_PerSecond,
    AnimationMode_AnimatedPerMinute,
    AnimationMode_ModeMask = AnimationMode_PerMinute
                           | AnimationMode_PerSecond
                           | AnimationMode_AnimatedPerMinute,

    // flags
    AnimationMode_BacklightPeekFlag = 0b1000,
    AnimationMode_FlagMask = AnimationMode_BacklightPeekFlag,

    // combinations
    AnimationMode_PerMinuteBacklightPeek = AnimationMode_PerMinute | AnimationMode_BacklightPeekFlag,
    AnimationMode_AnimatedPerMinuteBacklightPeek = AnimationMode_AnimatedPerMinute | AnimationMode_BacklightPeekFlag,
} AnimationMode;

struct App {
    Window* window;

    TimeSliderData timeSliderData;
    AnimationMode animationMode;

    Slider* monthSlider;
    Slider* daySlider;
    Slider* hourSlider;
    Slider* minuteSlider;

    Animation* animation;
};

bool app_init( App app[ static 1 ] );
void app_deinit( App app[ static 1 ] );

void app_init_layout( App app[ static 1 ] );
void app_destroy_layout( App app[ static 1 ] );

TimeUnits app_get_tick_rate( App app[ static 1 ] );
void app_on_tick( App app[ static 1 ], const tm time[ static 1 ], TimeUnits units, bool real );
void app_on_backlight_state_changed( App app[ static 1 ] );

#endif // APP_PRV_H
