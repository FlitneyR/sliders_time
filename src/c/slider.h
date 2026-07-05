#ifndef SLIDER_H
#define SLIDER_H

#include <pebble.h>

typedef struct Slider Slider;

// draw the current + offset label into the provided rect
typedef void (*SliderDrawLabelFunc)( Slider* slider, GContext* ctx, GRect bounds, int offset );

typedef struct Ratio { int numerator, denominator; } Ratio;
typedef Ratio (*SliderGetProgressFunc)( Slider* slider );

typedef struct SliderCreateInfo {
    GRect frame;
    GColor8 backgroundColor;
    GColor8 foregroundColor;

    uint8_t showNeighbours;
    uint16_t labelWidth;

    void* context;
    SliderDrawLabelFunc label_func;
    SliderGetProgressFunc progress_func;
} SliderCreateInfo;

Slider* slider_create( const SliderCreateInfo* const create_info );
void slider_destroy( Slider* slider );

Layer* slider_get_layer( const Slider* slider );
GColor8 slider_get_background_color( const Slider* slider );
GColor8 slider_get_foreground_color( const Slider* slider );
void slider_set_background_color( Slider* slider, GColor8 color );
void slider_set_foreground_color( Slider* slider, GColor8 color );

void slider_set_context( Slider* slider, void* context );
void* slider_get_context( const Slider* slider );

// #pragma region Time slider

typedef struct FontAndPadding { GFont font; uint8_t bottomPadding; } FontAndPadding;

typedef struct TimeSliderData {
    tm time;

    // pebble fonts have some top padding baked in
    // vertically aligning them without this will not work
    // this cannot be extracted from the font, so you must provide it manually :/

    FontAndPadding yearFont;
    FontAndPadding monthFont;
    FontAndPadding dayFont;
    FontAndPadding hourFont;
    FontAndPadding minuteFont;
} TimeSliderData;

void time_slider_year_label( Slider* slider, GContext* ctx, GRect bounds, int offset );
void time_slider_month_label( Slider* slider, GContext* ctx, GRect bounds, int offset );
void time_slider_day_label( Slider* slider, GContext* ctx, GRect bounds, int offset );
void time_slider_hour_label( Slider* slider, GContext* ctx, GRect bounds, int offset );
void time_slider_minute_label( Slider* slider, GContext* ctx, GRect bounds, int offset );

Ratio time_slider_year_progress( Slider* slider );
Ratio time_slider_month_progress( Slider* slider );
Ratio time_slider_day_progress( Slider* slider );
Ratio time_slider_hour_progress( Slider* slider );
Ratio time_slider_minute_progress( Slider* slider );

// #pragma endregion Time slider

#endif // SLIDER_H