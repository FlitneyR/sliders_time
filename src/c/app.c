#include "app_prv.h"
#include "macros.h"

// #pragma region App public interface

App* app_create()
{
    App* app = malloc( sizeof( *app ) );
    GUARD( app, return NULL );

    *app = (App){ 0 };

    if ( !app_init( app ) )
        CHECKED_FREE( app_destroy, app );

    return app;
}

void app_destroy( App* app )
{
    if ( !app ) return;

    app_deinit( app );
    free( app );
}

// #pragma endregion App public interface

// #pragma region App private functions

// #pragma region Init / Deinit

static void on_tick( tm* time, TimeUnits units ) { app_on_tick( window_get_user_data( window_stack_get_top_window() ), time, units ); }
static void on_main_window_load( Window* window ) { app_init_layout( window_get_user_data( window ) ); }
static void on_main_window_appear( Window* window ) { SUBSCRIBE_TICK_SERVER( on_tick, SECOND_UNIT ); }
static void on_main_window_disappear( Window* window ) { tick_timer_service_unsubscribe(); }
static void on_main_window_unload( Window* window ) { app_destroy_layout( window_get_user_data( window ) ); }

bool app_init( App app[ static 1 ] )
{
    *app = (App){ 0 };

    app_message_open( 1024, 0 );

    GUARD( app->window = window_create(), return false );
    window_set_user_data( app->window, app );
    window_set_window_handlers( app->window, (WindowHandlers){
        .load=on_main_window_load,
        .appear=on_main_window_appear,
        .disappear=on_main_window_disappear,
        .unload=on_main_window_unload,
    });

    window_stack_push( app->window, false );

    return true;
}

void app_deinit( App app[ static 1 ] )
{
    app_destroy_layout( app );
}

void _time_slider_year_label( Slider* slider, GContext* ctx, GRect bounds, int offset ) { time_slider_year_label( slider, ctx, bounds, offset ); }
void _time_slider_month_label( Slider* slider, GContext* ctx, GRect bounds, int offset ) { time_slider_month_label( slider, ctx, bounds, offset ); }
void _time_slider_day_label( Slider* slider, GContext* ctx, GRect bounds, int offset ) { time_slider_day_label( slider, ctx, bounds, offset ); }
void _time_slider_hour_label( Slider* slider, GContext* ctx, GRect bounds, int offset ) { time_slider_hour_label( slider, ctx, bounds, offset ); }
void _time_slider_minute_label( Slider* slider, GContext* ctx, GRect bounds, int offset ) { time_slider_minute_label( slider, ctx, bounds, offset ); }

uint8_t _time_slider_year_progress( Slider* slider ) { return time_slider_year_progress( slider ); }
uint8_t _time_slider_month_progress( Slider* slider ) { return time_slider_month_progress( slider ); }
uint8_t _time_slider_day_progress( Slider* slider ) { return time_slider_day_progress( slider ); }
uint8_t _time_slider_hour_progress( Slider* slider ) { return time_slider_hour_progress( slider ); }
uint8_t _time_slider_minute_progress( Slider* slider ) { return time_slider_minute_progress( slider ); }

typedef struct ColorScheme { GColor8 fg, bg, fg_alt, bg_alt; } ColorScheme;

void received_callback( DictionaryIterator* iterator, void* context )
{
    // APP_LOG( APP_LOG_LEVEL_DEBUG, "Received a message" );
    
    GUARD( iterator, return );
    GUARD( context, return );
    App* const app = (App*)context;
    
    const Tuple* const bg_color = dict_find( iterator, MESSAGE_KEY_PrimaryBackgroundColour );
    const Tuple* const fg_color = dict_find( iterator, MESSAGE_KEY_PrimaryForegroundColour );
    const Tuple* const bg_color_alt = dict_find( iterator, MESSAGE_KEY_SecondaryBackgroundColour );
    const Tuple* const fg_color_alt = dict_find( iterator, MESSAGE_KEY_SecondaryForegroundColour );

    if (bg_color)
    {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Recieved background color: %d", bg_color->value->uint8);
        slider_set_background_color( app->daySlider, (GColor8){ .argb=bg_color->value->uint8 } );
        slider_set_background_color( app->minuteSlider, (GColor8){ .argb=bg_color->value->uint8 } );
    }

    if (fg_color)
    {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Recieved foreground color: %d", fg_color->value->uint8);
        slider_set_foreground_color( app->daySlider, (GColor8){ .argb=fg_color->value->uint8 } );
        slider_set_foreground_color( app->minuteSlider, (GColor8){ .argb=fg_color->value->uint8 } );
    }

    if (bg_color_alt)
    {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Recieved background color: %d", bg_color_alt->value->uint8);
        slider_set_background_color( app->monthSlider, (GColor8){ .argb=bg_color_alt->value->uint8 } );
        slider_set_background_color( app->hourSlider, (GColor8){ .argb=bg_color_alt->value->uint8 } );
    }

    if (fg_color_alt)
    {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Recieved foreground color: %d", fg_color_alt->value->uint8);
        slider_set_foreground_color( app->monthSlider, (GColor8){ .argb=fg_color_alt->value->uint8 } );
        slider_set_foreground_color( app->hourSlider, (GColor8){ .argb=fg_color_alt->value->uint8 } );
    }
}

void app_init_layout( App app[ static 1 ] )
{
    const GSize window_size = layer_get_unobstructed_bounds( window_get_root_layer( app->window ) ).size;

    // app->timeSliderData.bigFont = fonts_get_system_font( FONT_KEY_GOTHIC_24_BOLD );
    // app->timeSliderData.bigFontBottomPadding = 10;

    // app->timeSliderData.bigFont = fonts_get_system_font( FONT_KEY_GOTHIC_28_BOLD );
    // app->timeSliderData.bigFontBottomPadding = 11;
    
    // app->timeSliderData.smallFont = fonts_get_system_font( FONT_KEY_GOTHIC_14_BOLD );
    // app->timeSliderData.smallFontBottomPadding = 6;

    uint16_t month_height, day_height, hour_height, minute_height;
    month_height = day_height = hour_height = minute_height = 0;

    if ( preferred_content_size() >= PreferredContentSizeLarge )
    {
        app->timeSliderData.hourFont = app->timeSliderData.minuteFont = (FontAndPadding){
            .font = fonts_get_system_font( FONT_KEY_BITHAM_30_BLACK ),
            .bottomPadding = 10,
        };
        
        app->timeSliderData.dayFont = app->timeSliderData.monthFont = (FontAndPadding){
            .font = fonts_get_system_font( FONT_KEY_GOTHIC_28_BOLD ),
            .bottomPadding = 11,
        };

        month_height = day_height = window_size.h / 5;
        hour_height = minute_height = ( window_size.h - month_height - day_height ) / 2;
    }
    else
    {
        app->timeSliderData.dayFont = app->timeSliderData.hourFont = app->timeSliderData.minuteFont = (FontAndPadding){
            .font = fonts_get_system_font( FONT_KEY_GOTHIC_28_BOLD ),
            .bottomPadding = 12,
        };
        
        app->timeSliderData.monthFont = (FontAndPadding){
            .font = fonts_get_system_font( FONT_KEY_GOTHIC_14_BOLD ),
            .bottomPadding = 6,
        };

        month_height = window_size.h / 6;
        day_height = hour_height = minute_height = ( window_size.h - month_height ) / 3;
    }

    // GUARD( app->yearSlider = slider_create( &(SliderCreateInfo){
    //     .frame={
    //         .origin={ 0, 0 * ( window_size.h / 5 ) },
    //         .size={ window_size.w, window_size.h / 5 + 1 }
    //     },

    //     .showNeighbours = PBL_IF_ROUND_ELSE( 2, 1 ),
    //     .backgroundColor = PBL_IF_COLOR_ELSE( GColorRed, GColorBlack ),
    //     .foregroundColor = PBL_IF_COLOR_ELSE( GColorWhite, GColorWhite ),

    //     .label_func=time_slider_year_label,
    //     .progress_func=time_slider_year_progress,
    // } ), goto give_up );

    // #define TRACE(fmt, expr) APP_LOG(APP_LOG_LEVEL_DEBUG, #expr " = " fmt, expr)
    #define TRACE(fmt, expr)
    TRACE("%p", time_slider_month_label);
    TRACE("%p", time_slider_day_label);
    TRACE("%p", time_slider_hour_label);
    TRACE("%p", time_slider_minute_label);
    TRACE("%p", slider_create);

    const ColorScheme cs = {
        // .bg=GColorBlack, .fg=GColorWhite, .bg_alt=GColorBlack, .fg_alt=GColorWhite, // black and white
        .bg=GColorRed, .fg=GColorWhite, .bg_alt=GColorDarkCandyAppleRed, .fg_alt=GColorWhite, // red
        // .bg=GColorDukeBlue, .fg=GColorWhite, .bg_alt=GColorOxfordBlue, .fg_alt=GColorWhite, // blue 
        // .bg=GColorRed, .fg=GColorWhite, .bg_alt=GColorBlue, .fg_alt=GColorWhite, // red and blue
        // .bg=GColorRed, .fg=GColorWhite, .bg_alt=GColorDarkGreen, .fg_alt=GColorWhite, // red and green
        // .bg=GColorLightGray, .fg=GColorWhite, .bg_alt=GColorDarkGray, .fg_alt=GColorWhite, // grey scale
    };

    SliderCreateInfo month_slider_create_info = {
         .frame={
            // .origin={ 0, 1 * ( window_size.h / 5 ) + 1 },
            // .size={ window_size.w, window_size.h / 5 },
            .origin={ 0, 0 },
            .size={ window_size.w, month_height },
        },

        .showNeighbours = PBL_IF_ROUND_ELSE( 2, 1 ),
        .backgroundColor = PBL_IF_COLOR_ELSE( cs.bg, GColorBlack ),
        .foregroundColor = PBL_IF_COLOR_ELSE( cs.fg, GColorWhite ),

        .context = NULL,
        .label_func=_time_slider_month_label,
        .progress_func=_time_slider_month_progress,       
    };
     
    TRACE("%p", month_slider_create_info.label_func);
    GUARD( app->monthSlider = slider_create( &month_slider_create_info ), goto give_up );

    const SliderCreateInfo day_slider_create_info = {
        .frame={
            // .origin={ 0, 2 * ( window_size.h / 5 ) + 1 },
            // .size={ window_size.w, window_size.h / 5 },
            .origin={ 0, month_height },
            .size={ window_size.w, day_height },
        },

        .showNeighbours = 1,
        .backgroundColor = PBL_IF_COLOR_ELSE( cs.bg_alt, GColorBlack ),
        .foregroundColor = PBL_IF_COLOR_ELSE( cs.fg_alt, GColorWhite ),

        .context = NULL,
        .label_func=_time_slider_day_label,
        .progress_func=_time_slider_day_progress,
    };

    TRACE("%p", day_slider_create_info.label_func);
    GUARD( app->daySlider = slider_create( &day_slider_create_info ), goto give_up );

    const SliderCreateInfo hour_slider_create_info = {
        .frame={
            // .origin={ 0, 3 * ( window_size.h / 5 ) + 1 },
            // .size={ window_size.w, window_size.h / 5 },
            .origin={ 0, month_height + day_height },
            .size={ window_size.w, hour_height },
        },

        .showNeighbours = 2,
        .backgroundColor = PBL_IF_COLOR_ELSE( cs.bg, GColorBlack ),
        .foregroundColor = PBL_IF_COLOR_ELSE( cs.fg, GColorWhite ),

        .context = NULL,
        .label_func=_time_slider_hour_label,
        .progress_func=_time_slider_hour_progress,
    };

    TRACE("%p", hour_slider_create_info.label_func);
    GUARD( app->hourSlider = slider_create( &hour_slider_create_info ), goto give_up );

    SliderCreateInfo minute_slider_create_info = {
        .frame={
            // .origin={ 0, 4 * ( window_size.h / 5 ) + 1 },
            // .size={ window_size.w, window_size.h / 5 + 1 },
            .origin={ 0, month_height + day_height + hour_height },
            .size={ window_size.w, minute_height },
        },

        .showNeighbours = 2,
        .backgroundColor = PBL_IF_COLOR_ELSE( cs.bg_alt, GColorBlack ),
        .foregroundColor = PBL_IF_COLOR_ELSE( cs.fg_alt, GColorWhite ),

        .context = NULL,
        .label_func=_time_slider_minute_label,
        .progress_func=_time_slider_minute_progress,
    };

    minute_slider_create_info.frame.size.h = window_size.h - ( minute_slider_create_info.frame.origin.y );

    TRACE("%p", minute_slider_create_info.label_func);
    GUARD( app->minuteSlider = slider_create( &minute_slider_create_info ), goto give_up );

    // layer_add_child( window_get_root_layer( app->window ), slider_get_layer( app->yearSlider ) );
    layer_add_child( window_get_root_layer( app->window ), slider_get_layer( app->monthSlider ) );
    layer_add_child( window_get_root_layer( app->window ), slider_get_layer( app->daySlider ) );
    layer_add_child( window_get_root_layer( app->window ), slider_get_layer( app->hourSlider ) );
    layer_add_child( window_get_root_layer( app->window ), slider_get_layer( app->minuteSlider ) );

    app_message_set_context( app );
    app_message_register_inbox_received( received_callback );

    return;
    
give_up:
    app_destroy( app );
}

void app_destroy_layout( App app[ static 1 ] )
{
    app_message_set_context( NULL );
    app_message_deregister_callbacks();

    // CHECKED_FREE( slider_destroy, app->yearSlider );
    CHECKED_FREE( slider_destroy, app->monthSlider );
    CHECKED_FREE( slider_destroy, app->daySlider );
    CHECKED_FREE( slider_destroy, app->hourSlider );
    CHECKED_FREE( slider_destroy, app->minuteSlider );

    CHECKED_FREE( window_destroy, app->window );
}

// #pragma endregion Init / Deinit

void app_on_tick( App app[ static 1 ], const tm time[ static 1 ], TimeUnits units )
{
    app->timeSliderData.time = *time;

    // if ( units & MONTH_UNIT  ) slider_set_context( app->yearSlider,   &app->timeSliderData );
    /* if ( units & DAY_UNIT    ) */ slider_set_context( app->monthSlider,  &app->timeSliderData );
    /* if ( units & HOUR_UNIT   ) */ slider_set_context( app->daySlider,    &app->timeSliderData );
    /* if ( units & MINUTE_UNIT ) */ slider_set_context( app->hourSlider,   &app->timeSliderData );
    /* if ( units & SECOND_UNIT ) */ slider_set_context( app->minuteSlider, &app->timeSliderData );
}

// #pragma endregion App private functions
