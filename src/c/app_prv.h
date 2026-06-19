#ifndef APP_PRV_H
#define APP_PRV_H

#include <pebble.h>
#include "app.h"
#include "slider.h"

typedef enum AnimationMode {
    AnimationMode_PerMinute,
    AnimationMode_PerSecond,
    AnimationMode_AnimatedPerMinute,
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

#endif // APP_PRV_H
