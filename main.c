#include "DarkMatter/dm.h"

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 360

int main(void)
{
    dm_context context = { 0 };

    if(!dm_init(&context, SCREEN_WIDTH, SCREEN_HEIGHT, "Test", 0, DM_MEGABYTE))
    {
        LOG_FATAL("Initializing DarkMatter failed");
        return 1;
    }

    // main loop
    while(dm_is_running(context))
    {
        dm_update(&context);

        // begin frame
        dm_begin_render(&context);
        dm_end_render(&context);
    }

    dm_shutdown(&context);

    return 0;
}
