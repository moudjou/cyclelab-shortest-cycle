#include "app.h"

int main(void)
{
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(1440, 900, "CycleLab | Shortest Cycle Visualizer");
    if (!IsWindowReady()) return 1;

    SetWindowMinSize(1100, 700);
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    App app = {0};
    app_init(&app);
    while (!WindowShouldClose()) {
        app_frame(&app);
    }
    app_destroy(&app);
    CloseWindow();
    return 0;
}
