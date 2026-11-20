#ifndef GUI_H
#define GUI_H

#include "app.h"

typedef struct GuiLayout {
    Rectangle header, sidebar, canvas, inspector, transport;
    Rectangle graph_area;
    Rectangle tool[5];
    Rectangle clear_graph, random_input, random_graph;
    Rectangle example, example_options[APP_EXAMPLE_COUNT];
    Rectangle reset_algorithm;
    Rectangle run, next, play, reset, speed, progress;
    Rectangle tabs[3], table;
    int table_rows;
} GuiLayout;

GuiLayout gui_layout(int width, int height);
Vector2 gui_vertex_position(const GuiLayout *layout, Vector2 normalized);
Vector2 gui_normalized_position(const GuiLayout *layout, Vector2 screen);
float gui_node_radius(const App *app, const GuiLayout *layout);
void gui_draw(const App *app, const GuiLayout *layout);

#endif
