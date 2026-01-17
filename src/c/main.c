#include <pebble.h>

#include "app.h"
#include "macros.h"

int main( void )
{
    App* app = app_create();
    GUARD( app );

    if ( app )
        app_event_loop();

    app_destroy( app );
}
