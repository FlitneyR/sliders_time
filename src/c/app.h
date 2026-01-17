#ifndef APP_H
#define APP_H

typedef struct App App;

App* app_create();
void app_destroy( App* app );

#endif // APP_H