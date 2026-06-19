#ifndef MACROS_H
#define MACROS_H

void trap( void );

// crash trap
#define TRAP *(char*)0 = 0

// breakpoint-able trap
// #define TRAP trap()

// no trap
// #define TRAP

#define GUARD( cond, ... ) \
    if ( !( cond ) ) { \
        APP_LOG( APP_LOG_LEVEL_ERROR, "!! " #cond ); \
        TRAP; \
        __VA_ARGS__; \
    }

#define CHECKED_FREE( func, ptr, ... ) if ( ptr ) { func( ptr __VA_OPT__( , __VA_ARGS__ ) ); ptr = NULL; }

#define FORMAT_BUFFER( buffer, fmt, ... ) snprintf( buffer, ARRAY_LENGTH( buffer ), fmt, ##__VA_ARGS__ )

#define DECLARE_COMMAND_LIST_ITER_FUNC( func_name, ... ) struct func_name##_args { __VA_ARGS__ }; bool func_name( GDrawCommand* command, uint32_t index, void* context );
#define DEFINE_COMMAND_LIST_ITER_FUNC( func_name ) bool func_name( GDrawCommand* command, uint32_t index, void* context )
#define BEGIN_COMMAND_LIST_ITER_FUNC( func_name ) const struct func_name##_args* const args = context
#define COMMAND_LIST_ITERATE( func_name, command_list, ... ) gdraw_command_list_iterate( command_list, func_name, &(struct func_name##_args){ __VA_ARGS__ } )
#define IMAGE_ITERATE( func_name, image, ... ) COMMAND_LIST_ITERATE( func_name, gdraw_command_image_get_command_list( image ), ##__VA_ARGS__ )

#define MIN( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )
#define MAX( a, b ) ( ( a ) > ( b ) ? ( a ) : ( b ) )

#endif // MACROS_H