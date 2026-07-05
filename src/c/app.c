#include "app_prv.h"
#include "macros.h"
#include "persistant_data_codes.h"

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

static void on_main_window_load( Window* window )
{
    app_init_layout( window_get_user_data( window ) );
}

static void on_tick( tm* time, TimeUnits units )
{
    app_on_tick( window_get_user_data( window_stack_get_top_window() ), time, units, true );
}

static void app_update_tick_rate( App app[ static 1 ] )
{
    tick_timer_service_unsubscribe();
    tick_timer_service_subscribe( app_get_tick_rate( app ), on_tick );
    app_on_tick( app, localtime( &(time_t){ time( NULL ) } ), ~0, false );
}

static void on_backlight_state_changed( bool is_on )
{
    Window* window = window_stack_get_top_window();
    App* app = window_get_user_data( window );
    app_on_backlight_state_changed( app );
}

static void on_main_window_appear( Window* window )
{
    App* app = window_get_user_data( window );
    app_update_tick_rate( app );
    backlight_service_subscribe( on_backlight_state_changed );
}

static void on_main_window_disappear( Window* window )
{
    tick_timer_service_unsubscribe();
    backlight_service_unsubscribe();
}

static void on_main_window_unload( Window* window )
{
    app_destroy_layout( window_get_user_data( window ) );
}

bool app_init( App app[ static 1 ] )
{
    *app = (App){ 0 };
    app->animationMode = persist_exists( ANIMATION_MODE )
        ? persist_read_int( ANIMATION_MODE )
        : AnimationMode_AnimatedPerMinuteBacklightPeek;

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

typedef struct ColorScheme { GColor8 fg, bg, fg_alt, bg_alt; } ColorScheme;

static AnimationMode get_animation_mode( const char* name )
{
    if ( !strcmp( name, "per_minute" ) ) return AnimationMode_PerMinute;
    if ( !strcmp( name, "per_second" ) ) return AnimationMode_PerSecond;
    if ( !strcmp( name, "animated_per_minute" ) ) return AnimationMode_AnimatedPerMinute;
    if ( !strcmp( name, "per_minute_backlight" ) ) return AnimationMode_PerMinuteBacklightPeek;
    if ( !strcmp( name, "animated_per_minute_backlight" ) ) return AnimationMode_AnimatedPerMinuteBacklightPeek;
    return AnimationMode_AnimatedPerMinuteBacklightPeek;
}

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
    const Tuple* const animation_mode = dict_find( iterator, MESSAGE_KEY_AnimationMode );

    if ( bg_color )
    {
        // APP_LOG(APP_LOG_LEVEL_DEBUG, "Recieved background color: %d", bg_color->value->uint32);
        persist_write_int( PRIMARY_BACKGROUND_COLOR, bg_color->value->int32 );
        slider_set_background_color( app->daySlider, GColorFromHEX( bg_color->value->int32 ) );
        slider_set_background_color( app->minuteSlider, GColorFromHEX( bg_color->value->int32 ) );
    }

    if ( fg_color )
    {
        // APP_LOG(APP_LOG_LEVEL_DEBUG, "Recieved foreground color: %d", fg_color->value->uint32);
        persist_write_int( PRIMARY_FOREGROUND_COLOR, fg_color->value->int32 );
        slider_set_foreground_color( app->daySlider, GColorFromHEX( fg_color->value->int32 ) );
        slider_set_foreground_color( app->minuteSlider, GColorFromHEX( fg_color->value->int32 ) );
    }

    if ( bg_color_alt )
    {
        // APP_LOG(APP_LOG_LEVEL_DEBUG, "Recieved alt background color: %d", bg_color_alt->value->uint32);
        persist_write_int( SECONDARY_BACKGROUND_COLOR, bg_color_alt->value->int32 );
        slider_set_background_color( app->monthSlider, GColorFromHEX( bg_color_alt->value->int32 ) );
        slider_set_background_color( app->hourSlider, GColorFromHEX( bg_color_alt->value->int32 ) );
    }

    if ( fg_color_alt )
    {
        // APP_LOG(APP_LOG_LEVEL_DEBUG, "Recieved alt foreground color: %d", fg_color_alt->value->uint32);
        persist_write_int( SECONDARY_FOREGROUND_COLOR, fg_color_alt->value->int32 );
        slider_set_foreground_color( app->monthSlider, GColorFromHEX( fg_color_alt->value->int32 ) );
        slider_set_foreground_color( app->hourSlider, GColorFromHEX( fg_color_alt->value->int32 ) );
    }

    if ( animation_mode )
    {
        app->animationMode = get_animation_mode( animation_mode->value->cstring );
        persist_write_int( ANIMATION_MODE, app->animationMode );
        on_main_window_appear( app->window );
    }
}

void update_app_layout( App app[ static 1 ] )
{
    const GSize window_size = layer_get_unobstructed_bounds( window_get_root_layer( app->window ) ).size;

    uint16_t month_height, day_height, hour_height, minute_height;
    month_height = day_height = hour_height = minute_height = 0;

    if ( preferred_content_size() >= PreferredContentSizeLarge )
    {
        month_height = day_height = window_size.h / 5;
        hour_height = minute_height = ( window_size.h - month_height - day_height ) / 2;
    }
    else
    {
        month_height = day_height = window_size.h / 6;
        hour_height = minute_height = ( window_size.h - 2 * month_height ) / 2;
    }

    layer_set_frame( slider_get_layer( app->monthSlider ), (GRect){
        .origin={ 0, 0 },
        .size={ window_size.w, month_height },
    });

    layer_set_frame( slider_get_layer( app->daySlider ), (GRect){
        .origin={ 0, month_height },
        .size={ window_size.w, day_height },
    });

    layer_set_frame( slider_get_layer( app->hourSlider ), (GRect){
        .origin={ 0, month_height + day_height },
        .size={ window_size.w, hour_height },
    });

    layer_set_frame( slider_get_layer( app->minuteSlider ), (GRect){
        .origin={ 0, month_height + day_height + hour_height },
        .size={ window_size.w, minute_height },
    });
}

void on_unobstructed_area_change( AnimationProgress progress, void* context )
{
    update_app_layout( (App*)context );
}

void app_init_layout( App app[ static 1 ] )
{
    const GSize window_size = layer_get_unobstructed_bounds( window_get_root_layer( app->window ) ).size;

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
        app->timeSliderData.hourFont = app->timeSliderData.minuteFont = (FontAndPadding){
            .font = fonts_get_system_font( FONT_KEY_GOTHIC_28_BOLD ),
            .bottomPadding = 12,
        };
        
        app->timeSliderData.dayFont = app->timeSliderData.monthFont = (FontAndPadding){
            .font = fonts_get_system_font( FONT_KEY_GOTHIC_14_BOLD ),
            .bottomPadding = 6,
        };

        month_height = day_height = window_size.h / 6;
        hour_height = minute_height = ( window_size.h - 2 * month_height ) / 2;
    }

    #define READ_PERSISTENT_COLOR( identifier, default )\
        !persist_exists( identifier ) ? default : GColorFromHEX( persist_read_int( identifier ) )

    const ColorScheme cs = {
        .bg=READ_PERSISTENT_COLOR( PRIMARY_BACKGROUND_COLOR, GColorBlack ),
        .fg=READ_PERSISTENT_COLOR( PRIMARY_FOREGROUND_COLOR, GColorWhite ),
        .bg_alt=READ_PERSISTENT_COLOR( SECONDARY_BACKGROUND_COLOR, GColorWhite ),
        .fg_alt=READ_PERSISTENT_COLOR( SECONDARY_FOREGROUND_COLOR, GColorBlack ),
    };

    #undef READ_PERSISTENT_COLOR

    SliderCreateInfo month_slider_create_info = {
         .frame={
            .origin={ 0, 0 },
            .size={ window_size.w, month_height },
        },

        .showNeighbours = PBL_IF_ROUND_ELSE( 2, 1 ),
        .backgroundColor = cs.bg,
        .foregroundColor = cs.fg,

        .context = NULL,
        .label_func=time_slider_month_label,
        .progress_func=time_slider_month_progress,       
    };
     
    GUARD( app->monthSlider = slider_create( &month_slider_create_info ), goto give_up );

    const SliderCreateInfo day_slider_create_info = {
        .frame={
            .origin={ 0, month_height },
            .size={ window_size.w, day_height },
        },

        .showNeighbours = 1,
        .backgroundColor = cs.bg_alt,
        .foregroundColor = cs.fg_alt,

        .context = NULL,
        .label_func=time_slider_day_label,
        .progress_func=time_slider_day_progress,
    };

    GUARD( app->daySlider = slider_create( &day_slider_create_info ), goto give_up );

    const SliderCreateInfo hour_slider_create_info = {
        .frame={
            .origin={ 0, month_height + day_height },
            .size={ window_size.w, hour_height },
        },

        .showNeighbours = 2,
        .labelWidth = 60,
        .backgroundColor = cs.bg,
        .foregroundColor = cs.fg,

        .context = NULL,
        .label_func=time_slider_hour_label,
        .progress_func=time_slider_hour_progress,
    };

    GUARD( app->hourSlider = slider_create( &hour_slider_create_info ), goto give_up );

    SliderCreateInfo minute_slider_create_info = {
        .frame={
            .origin={ 0, month_height + day_height + hour_height },
            .size={ window_size.w, minute_height },
        },

        .showNeighbours = 2,
        .labelWidth = 60,
        .backgroundColor = cs.bg_alt,
        .foregroundColor = cs.fg_alt,

        .context = NULL,
        .label_func=time_slider_minute_label,
        .progress_func=time_slider_minute_progress,
    };

    minute_slider_create_info.frame.size.h = window_size.h - ( minute_slider_create_info.frame.origin.y );

    GUARD( app->minuteSlider = slider_create( &minute_slider_create_info ), goto give_up );

    unobstructed_area_service_subscribe( (UnobstructedAreaHandlers){ .change=on_unobstructed_area_change }, app );

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

    CHECKED_FREE( animation_destroy, app->animation );

    CHECKED_FREE( slider_destroy, app->monthSlider );
    CHECKED_FREE( slider_destroy, app->daySlider );
    CHECKED_FREE( slider_destroy, app->hourSlider );
    CHECKED_FREE( slider_destroy, app->minuteSlider );

    CHECKED_FREE( window_destroy, app->window );
}

// #pragma endregion Init / Deinit

TimeUnits app_get_tick_rate( App app[ static 1 ] )
{
    if ( ( app->animationMode & AnimationMode_BacklightPeekFlag ) && light_is_on() )
        return SECOND_UNIT;

    switch ( app->animationMode & AnimationMode_ModeMask )
    {
    case AnimationMode_PerSecond:
        return SECOND_UNIT;
    case AnimationMode_PerMinute:
    case AnimationMode_AnimatedPerMinute:
        return MINUTE_UNIT;
    default:
        GUARD(!"invalid app->animationMode", return SECOND_UNIT);
    }
}

typedef struct GenericTimeTransitionAnimationContext {
    App* app;
    time_t startTime;
    time_t endTime;
} GenericTimeTransitionAnimationContext;

void generic_time_transition_implementation_update( Animation* animation, AnimationProgress progress )
{
    GenericTimeTransitionAnimationContext* context = animation_get_context( animation );
    time_t time = context->startTime + ( ( context->endTime - context->startTime ) * progress ) / ANIMATION_NORMALIZED_MAX;
    context->app->timeSliderData.time = *(tm*)localtime( &time );

    slider_set_context( context->app->monthSlider, &context->app->timeSliderData );
    slider_set_context( context->app->daySlider, &context->app->timeSliderData );
    slider_set_context( context->app->hourSlider, &context->app->timeSliderData );
    slider_set_context( context->app->minuteSlider, &context->app->timeSliderData );
}

void minute_tick_animation_implementation_teardown( Animation* animation )
{
    GenericTimeTransitionAnimationContext* context = animation_get_context( animation );
    context->app->animation = NULL; // make sure the app doesn't try to destroy this animation as well
    free( context );
}

AnimationImplementation g_minuteTickAnimationImplementation = {
    .update=generic_time_transition_implementation_update,
    .teardown=minute_tick_animation_implementation_teardown,
};

static AnimationProgress bounce_curve( AnimationProgress linear )
{
    if ( 2 * linear < ANIMATION_NORMALIZED_MAX )
    {
        AnimationProgress value = linear;
        value *= 2;
        value *= value;
        value /= ANIMATION_NORMALIZED_MAX;
        return value;
    }
    else
    {
        AnimationProgress value = linear;
        value -= 3 * ANIMATION_NORMALIZED_MAX / 4;
        value *= value;
        value /= ANIMATION_NORMALIZED_MAX;
        value *= 8;
        value = ANIMATION_NORMALIZED_MAX / 2 + value;
        return value;
    }
}

void app_on_tick( App app[ static 1 ], const tm time[ static 1 ], TimeUnits units, bool real )
{
    AnimationMode animation_mode = app->animationMode & AnimationMode_ModeMask;

    if ( ( app->animationMode & AnimationMode_BacklightPeekFlag ) && light_is_on() )
        animation_mode = AnimationMode_PerSecond;

    if ( ( animation_mode == AnimationMode_AnimatedPerMinute ) && !real )
        animation_mode = AnimationMode_PerMinute;

    switch ( animation_mode )
    {
    case AnimationMode_PerSecond:
    case AnimationMode_PerMinute:
    {
        app->timeSliderData.time = *time;
        if ( animation_mode == AnimationMode_PerMinute )
            app->timeSliderData.time.tm_sec = 30;

        slider_set_context( app->monthSlider,  &app->timeSliderData );
        slider_set_context( app->daySlider,    &app->timeSliderData );
        slider_set_context( app->hourSlider,   &app->timeSliderData );
        slider_set_context( app->minuteSlider, &app->timeSliderData );
        break;
    }
    case AnimationMode_AnimatedPerMinute:
    {
        CHECKED_FREE( animation_destroy, app->animation );

        tm end_time = *time;
        end_time.tm_sec = 30;

        GenericTimeTransitionAnimationContext* animation_context = calloc( 1, sizeof *animation_context );
        animation_context->app = app;
        animation_context->startTime = mktime( &app->timeSliderData.time );
        animation_context->endTime = mktime( &end_time );

        app->animation = animation_create();
        animation_set_duration( app->animation, 1000 );
        animation_set_custom_curve( app->animation, bounce_curve );
        animation_set_handlers( app->animation, (AnimationHandlers){}, animation_context );
        animation_set_implementation( app->animation, &g_minuteTickAnimationImplementation );
        animation_schedule( app->animation );
        break;
    }
    default: break;
    }
}

void tick_rate_transition_animation_implementation_setup( Animation* animation )
{
    tick_timer_service_unsubscribe();
}

void tick_rate_transition_animation_implementation_teardown( Animation* animation )
{
    GenericTimeTransitionAnimationContext* ctx = animation_get_context( animation );
    ctx->app->animation = NULL;
    app_update_tick_rate( ctx->app );
    free( ctx );
}

AnimationImplementation g_tickRateTransitionAnimationImplementation = {
    .setup=tick_rate_transition_animation_implementation_setup,
    .update=generic_time_transition_implementation_update,
    .teardown=tick_rate_transition_animation_implementation_teardown,
};

void app_on_backlight_state_changed( App app[ static 1 ] )
{
    if ( !( app->animationMode & AnimationMode_BacklightPeekFlag ) )
        return;

    CHECKED_FREE( animation_destroy, app->animation );

    GenericTimeTransitionAnimationContext* animation_context = calloc( 1, sizeof *animation_context );
    animation_context->app = app;

    {
        animation_context->startTime = mktime( &app->timeSliderData.time );
        
        time_t end_time = time( NULL );
        if ( !light_is_on() )
        {
            tm temp = *(tm*)localtime( &end_time );
            temp.tm_sec = 30;
            end_time = mktime( &temp );
        }

        animation_context->endTime = end_time;
    }

    app->animation = animation_create();
    animation_set_duration( app->animation, 250 );
    animation_set_curve( app->animation, AnimationCurveEaseOut );
    animation_set_handlers( app->animation, (AnimationHandlers){}, animation_context );
    animation_set_implementation( app->animation, &g_tickRateTransitionAnimationImplementation );
    animation_schedule( app->animation );
}

// #pragma endregion App private functions
