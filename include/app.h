#ifndef SHORTEST_CYCLE_APP_H
#define SHORTEST_CYCLE_APP_H

#include "bfs.h"
#include "graph.h"
#include "raylib.h"

#define APP_VERTEX_LIMIT 64
#define APP_EXAMPLE_COUNT 7

typedef enum {
    EDIT_SELECT, EDIT_VERTEX, EDIT_EDGE, EDIT_DELETE_VERTEX, EDIT_DELETE_EDGE
} EditMode;

typedef enum {
    SIM_READY, SIM_RUNNING, SIM_PAUSED, SIM_FINISHED
} SimulationStatus;

typedef enum {
    EDU_PSEUDOCODE, EDU_VERTICES, EDU_COMPLEXITY
} EduTab;

typedef struct App {
    Graph graph;
    BFSState bfs;
    Vector2 positions[APP_VERTEX_LIMIT];
    Font body_font, bold_font, mono_font;
    EditMode edit_mode;
    SimulationStatus status;
    EduTab tab;
    int hovered_vertex, selected_vertex, edge_anchor;
    int hovered_edge_a, hovered_edge_b, selected_edge_a, selected_edge_b;
    int drag_vertex;
    Vector2 drag_offset;
    bool example_open, random_focused, random_replace, slider_drag;
    int example_index, table_scroll;
    char random_input[3], notice[160];
    float speed, notice_time, step_credit;
    bool notice_warning;
    Vector2 mouse;
} App;

extern const char *const app_example_names[APP_EXAMPLE_COUNT];

float app_speed_fraction(float speed);
void app_init(App *app);
void app_frame(App *app);
void app_destroy(App *app);

#endif
