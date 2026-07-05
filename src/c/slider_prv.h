#ifndef SLIDER_PRV_H
#define SLIDER_PRV_H

#include "slider.h"

struct Slider {
    Layer* layer;
    GColor8 backgroundColor;
    GColor8 foregroundColor;

    void* context;
    Ratio currentProgress;
    uint8_t showNeighbours;
    uint16_t labelWidth;

    SliderDrawLabelFunc draw_label;
    SliderGetProgressFunc get_progress;
};

void slider_layer_update_proc( Layer* layer, GContext* ctx );

#endif // SLIDER_PRV_H