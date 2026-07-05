#include "slider.h"
#include "slider_prv.h"
#include "macros.h"

// #pragma region Slider public interface

Slider* slider_create( const SliderCreateInfo* const create_info )
{
    // #define TRACE(fmt, expr) APP_LOG(APP_LOG_LEVEL_DEBUG, #expr " = " fmt, expr)
    #define TRACE(fmt, expr)
    TRACE("%p", time_slider_month_label);
    TRACE("%p", time_slider_day_label);
    TRACE("%p", time_slider_hour_label);
    TRACE("%p", time_slider_minute_label);
    TRACE("%p", slider_create);

    Layer* layer = NULL;
    Slider* slider = NULL;

    GUARD( create_info->label_func, goto give_up );
    GUARD( create_info->progress_func, goto give_up );

    GUARD( layer = layer_create_with_data( create_info->frame, sizeof( Slider ) ), goto give_up );
    GUARD( slider = layer_get_data( layer ), goto give_up );
    *slider = (Slider){
        .layer = layer,

        .backgroundColor = create_info->backgroundColor,
        .foregroundColor = create_info->foregroundColor,
        .showNeighbours = create_info->showNeighbours,
        .labelWidth = create_info->labelWidth,

        .context = create_info->context,
        .draw_label = create_info->label_func,
        .get_progress = create_info->progress_func,
    };

    if ( slider->labelWidth == 0 )
        slider->labelWidth = create_info->frame.size.w / ( 2 * slider->showNeighbours );

    layer_set_update_proc( layer, slider_layer_update_proc );

    return slider;

give_up:
    CHECKED_FREE( layer_destroy, layer );
    return NULL;
}

void slider_destroy( Slider* slider )
{
    CHECKED_FREE( layer_destroy, slider->layer );
}

Layer* slider_get_layer( const Slider* slider ) { return slider->layer; }
void* slider_get_context( const Slider* slider ) { return slider->context; }
GColor8 slider_get_background_color( const Slider* slider ) { return slider->backgroundColor; }
GColor8 slider_get_foreground_color( const Slider* slider ) { return slider->foregroundColor; }
void slider_set_background_color( Slider* slider, GColor8 color ) { slider->backgroundColor = color; }
void slider_set_foreground_color( Slider* slider, GColor8 color ) { slider->foregroundColor = color; }

void slider_set_context( Slider* slider, void* context )
{
    GUARD( slider, return );
    slider->context = context;
    layer_mark_dirty( slider->layer );
}

// #pragma endregion Slider public interface

// #pragma region Slider private functions

void slider_layer_update_proc( Layer* layer, GContext* ctx )
{
    Slider* const slider = layer_get_data( layer );
    GUARD( slider, return );

    slider->currentProgress = slider->get_progress( slider );

    const GSize layer_size = layer_get_frame( layer ).size;
    
    graphics_context_set_fill_color( ctx, slider->backgroundColor );
    graphics_context_set_stroke_color( ctx, slider->backgroundColor );
    graphics_fill_rect( ctx, (GRect){ .size=layer_size }, 0, GCornersAll );

    const GSize label_size = {
        .h=layer_size.h,
        .w=slider->labelWidth,
    };

    // show past neighbours, then future neighbours
    for ( int16_t neighbour = 1; neighbour <= slider->showNeighbours; ++neighbour )
    {
        slider->draw_label( slider, ctx, (GRect){
            .origin = {
                .x = layer_size.w / 2 - ( label_size.w * slider->currentProgress.numerator / slider->currentProgress.denominator ) - neighbour * label_size.w,
                .y = 0,
            },
            .size = label_size,
        }, - neighbour );

        slider->draw_label( slider, ctx, (GRect){
            .origin = {
                .x = layer_size.w / 2 - ( label_size.w * slider->currentProgress.numerator / slider->currentProgress.denominator ) + neighbour * label_size.w,
                .y = 0,
            },
            .size = label_size,
        }, + neighbour );
    }

    // show current
    slider->draw_label( slider, ctx, (GRect){
        .origin = {
            .x = layer_size.w / 2 - ( label_size.w * slider->currentProgress.numerator / slider->currentProgress.denominator ),
            .y = 0,
        },
        .size = label_size,
    }, 0 );

    graphics_context_set_stroke_color( ctx, slider->foregroundColor );
    graphics_context_set_stroke_width( ctx, 1 );

    graphics_context_set_stroke_width( ctx, 2 );
    graphics_context_set_stroke_color( ctx, slider->foregroundColor );
    graphics_context_set_fill_color( ctx, slider->foregroundColor );

    // graphics_fill_circle( ctx, (GPoint){ layer_size.w / 2, 0 }, 3 );
    // graphics_fill_circle( ctx, (GPoint){ layer_size.w / 2, layer_size.h }, 3 );

    graphics_draw_line( ctx, (GPoint){ layer_size.w / 2, 0 }, (GPoint){ layer_size.w / 2, layer_size.h / 10 } );
    graphics_draw_line( ctx, (GPoint){ layer_size.w / 2, layer_size.h }, (GPoint){ layer_size.w / 2, 9 * layer_size.h / 10 } );
}

// #pragma endregion Slider private functions

// #pragma region Time slider funcs

static void graphics_draw_text_vertically_centred(
    GContext *ctx, const char *text, const GFont font, GRect box,
    const GTextOverflowMode overflow_mode,
    const GTextAlignment alignment,
    GTextAttributes *text_attributes,
    uint8_t bottom_padding
) {
    GSize text_size = graphics_text_layout_get_content_size_with_attributes( text, font, box, overflow_mode, alignment, text_attributes );
    text_size.h += bottom_padding;

    box.origin.y = box.origin.y + box.size.h / 2 - text_size.h / 2;
    box.origin.x = box.origin.x + box.size.w / 2 - text_size.w / 2;
    box.size = text_size;

    // graphics_draw_rect( ctx, box );
    graphics_draw_text( ctx, text, font, box, overflow_mode, alignment, NULL );
}

static void draw_slider_label( Slider* slider, GContext* ctx, GRect bounds, char* str, GFont font, uint8_t bottom_padding )
{
    graphics_context_set_text_color( ctx, slider_get_foreground_color( slider ) );
    graphics_context_set_stroke_color( ctx, GColorWhite );
    graphics_draw_text_vertically_centred( ctx, str, font, bounds, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL, bottom_padding );

    graphics_context_set_fill_color( ctx, slider_get_foreground_color( slider ) );
    graphics_context_set_stroke_color( ctx, slider_get_foreground_color( slider ) );
    graphics_context_set_stroke_width( ctx, 1 );

    graphics_draw_line( ctx,
        (GPoint){ .x=bounds.origin.x + bounds.size.w, .y=bounds.origin.y + 1 * bounds.size.h / 4 },
        (GPoint){ .x=bounds.origin.x + bounds.size.w, .y=bounds.origin.y + 3 * bounds.size.h / 4 }
    );
}

void time_slider_year_label( Slider* slider, GContext* ctx, GRect bounds, int offset )
{
    const TimeSliderData* const data = slider_get_context( slider );
    GUARD( data, return );

    const uint16_t year = 1900 + data->time.tm_year + offset;
    
    static char buffer[] = "____";
    FORMAT_BUFFER( buffer, "%d", year );
    draw_slider_label( slider, ctx, bounds, buffer, data->yearFont.font, data->yearFont.bottomPadding );
}

void time_slider_month_label( Slider* slider, GContext* ctx, GRect bounds, int offset )
{
    const TimeSliderData* const data = slider_get_context( slider );
    GUARD( data, return );

    tm offset_time = data->time;
    offset_time.tm_mon += offset;
    memcpy( &offset_time, localtime( &(time_t){ mktime( &offset_time ) } ), sizeof( offset_time ) );

    const uint16_t year = 1900 + offset_time.tm_year;
    const uint8_t month = offset_time.tm_mon;

    static const char* const month_names[] = {
        "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
    };

    static char buffer[] = "___ ____";
    FORMAT_BUFFER( buffer, "%s %d", month_names[ month ], year );
    draw_slider_label( slider, ctx, bounds, buffer, data->monthFont.font, data->monthFont.bottomPadding );
}

void time_slider_day_label( Slider* slider, GContext* ctx, GRect bounds, int offset )
{
    const TimeSliderData* const data = slider_get_context( slider );
    GUARD( data, return );

    tm offset_time = data->time;
    offset_time.tm_wday += offset;
    offset_time.tm_mday += offset;
    offset_time.tm_yday += offset;
    memcpy( &offset_time, localtime( &(time_t){ mktime( &offset_time ) } ), sizeof( offset_time ) );

    const uint8_t mday = offset_time.tm_mday;
    const uint8_t wday = offset_time.tm_wday;

    const char* const wday_names[] = {
        "SUN", "MON", "TUE", "WED", "THUR", "FRI", "SAT"
    };

    static char buffer[] = "____ __";
    FORMAT_BUFFER( buffer, "%s %02d", wday_names[ wday ], mday );
    draw_slider_label( slider, ctx, bounds, buffer, data->dayFont.font, data->dayFont.bottomPadding );
}

void time_slider_hour_label( Slider* slider, GContext* ctx, GRect bounds, int offset )
{
    const TimeSliderData* const data = slider_get_context( slider );
    GUARD( data, return );

    tm offset_time = data->time;
    offset_time.tm_hour += offset;
    memcpy( &offset_time, localtime( &(time_t){ mktime( &offset_time ) } ), sizeof( offset_time ) );

    uint8_t hour = offset_time.tm_hour;
    if ( !clock_is_24h_style() )
    {
        if ( hour >= 12 )
            hour -= 12;
        if ( hour == 0 )
            hour = 12;
    }

    static char buffer[] = "__";
    FORMAT_BUFFER( buffer, "%02d", hour );
    draw_slider_label( slider, ctx, bounds, buffer, data->hourFont.font, data->hourFont.bottomPadding );
}

void time_slider_minute_label( Slider* slider, GContext* ctx, GRect bounds, int offset )
{
    const TimeSliderData* const data = slider_get_context( slider );
    GUARD( data, return );

    tm offset_time = data->time;
    offset_time.tm_min += offset;
    memcpy( &offset_time, localtime( &(time_t){ mktime( &offset_time ) } ), sizeof( offset_time ) );

    const uint8_t min = offset_time.tm_min;

    static char buffer[] = "__";
    FORMAT_BUFFER( buffer, "%02d", min );
    draw_slider_label( slider, ctx, bounds, buffer, data->minuteFont.font, data->minuteFont.bottomPadding );
}

Ratio get_progress( time_t from, time_t to, time_t sample )
{
    return (Ratio){ sample - from, to - from };
}

#define PROGRESS_FUNC( aspect ) \
    const TimeSliderData* const data = slider_get_context( slider );\
    GUARD( data, return (Ratio){ 0, 1 } );\
    tm start, now, end;\
    start = now = end = data->time;\
    memset(&start, 0, __offsetof(tm, aspect));\
    memset(&end, 0, __offsetof(tm, aspect));\
    end.aspect += 1;\
    return get_progress( mktime( &start ), mktime( &end ), mktime( &now ) );\

Ratio time_slider_year_progress( Slider* slider )
{
    PROGRESS_FUNC( tm_year );
}

Ratio time_slider_month_progress( Slider* slider )
{
    PROGRESS_FUNC( tm_mon );
}

Ratio time_slider_day_progress( Slider* slider )
{
    PROGRESS_FUNC( tm_mday );
}

Ratio time_slider_hour_progress( Slider* slider )
{
    PROGRESS_FUNC( tm_hour );
}

Ratio time_slider_minute_progress( Slider* slider )
{
    PROGRESS_FUNC( tm_min );
}

// #pragma endregion Time slider funcs
