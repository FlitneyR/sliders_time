#include <pebble.h>

#include "app.h"
#include "macros.h"

// If this variable doesn't exist or is optimised out, we cannot receive any configuration messages
int _pointless_global_variable;

int main( void )
{
    _pointless_global_variable++;

    App* app = app_create();
    GUARD( app );

    if ( app )
        app_event_loop();

    app_destroy( app );
}
