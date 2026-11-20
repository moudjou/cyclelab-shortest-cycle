#include "app.h"
#include "gui.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *const app_example_names[APP_EXAMPLE_COUNT] = {
    "Triangle", "Square", "Acyclic Graph", "Multiple Cycles",
    "Disconnected Graph", "No Edges", "Single Vertex"
};

typedef struct {
    int count;
    int edge_count;
    int edges[7][2];
    Vector2 positions[7];
} Example;

static const Example examples[APP_EXAMPLE_COUNT] = {
    {3, 3, {{1,2}, {2,3}, {3,1}}, {{.50f,.18f}, {.83f,.72f}, {.17f,.72f}}},
    {4, 4, {{1,2}, {2,3}, {3,4}, {4,1}},
        {{.21f,.20f}, {.79f,.20f}, {.79f,.78f}, {.21f,.78f}}},
    {5, 4, {{1,2}, {2,3}, {3,4}, {4,5}},
        {{.09f,.54f}, {.29f,.28f}, {.49f,.53f}, {.71f,.30f}, {.91f,.55f}}},
    {6, 7, {{1,2}, {2,3}, {3,4}, {4,1}, {3,5}, {5,6}, {6,3}},
        {{.13f,.23f}, {.48f,.23f}, {.48f,.73f}, {.13f,.73f}, {.82f,.73f}, {.74f,.31f}}},
    {7, 5, {{1,2}, {2,3}, {4,5}, {5,6}, {6,4}},
        {{.10f,.27f}, {.28f,.45f}, {.13f,.69f}, {.73f,.17f}, {.89f,.58f}, {.56f,.58f}, {.70f,.88f}}},
    {5, 0, {{0,0}}, {{.50f,.20f}, {.80f,.40f}, {.68f,.77f}, {.32f,.77f}, {.20f,.40f}}},
    {1, 0, {{0,0}}, {{.50f,.48f}}}
};

static const float min_speed = 0.25f;
static const float speed_ratio = 128.0f;

static float clamp_unit(float value)
{
    return fmaxf(0.0f, fminf(1.0f, value));
}

float app_speed_fraction(float speed)
{
    return clamp_unit(logf(speed / min_speed) / logf(speed_ratio));
}

static void set_notice(App *app, const char *message, bool warning)
{
    snprintf(app->notice, sizeof app->notice, "%s", message);
    app->notice_warning = warning;
    app->notice_time = warning ? 5.0f : 3.0f;
}

static bool editing_locked(const App *app)
{
    return app->status == SIM_RUNNING || app->status == SIM_PAUSED;
}

static void clear_selection(App *app)
{
    app->selected_vertex = -1;
    app->selected_edge_a = -1;
    app->selected_edge_b = -1;
    app->edge_anchor = -1;
    app->drag_vertex = -1;
}

static void reset_simulation(App *app)
{
    bfs_destroy(&app->bfs);
    app->bfs.root = -1;
    app->bfs.current = -1;
    app->bfs.neighbor = -1;
    app->bfs.candidate_length = -1;
    app->bfs.candidate_score = -1;
    app->status = SIM_READY;
    app->step_credit = 0.0f;
}

static void graph_changed(App *app)
{
    reset_simulation(app);
    clear_selection(app);
    app->hovered_vertex = -1;
    app->hovered_edge_a = -1;
    app->hovered_edge_b = -1;
    app->example_index = -1;
    app->table_scroll = 0;
}

static void replace_graph(App *app, Graph graph)
{
    graph_changed(app);
    graph_destroy(&app->graph);
    app->graph = graph;
    app->edit_mode = EDIT_SELECT;
}

static void load_example(App *app, int index)
{
    const Example *example = &examples[index];
    Graph graph = {0};
    for (int vertex = 0; vertex < example->count; vertex++) {
        if (!graph_add_vertex(&graph)) goto failed;
    }
    for (int edge = 0; edge < example->edge_count; edge++) {
        if (!graph_add_edge(&graph, example->edges[edge][0] - 1,
                            example->edges[edge][1] - 1)) goto failed;
    }
    replace_graph(app, graph);
    memcpy(app->positions, example->positions, (size_t)example->count * sizeof(Vector2));
    app->example_index = index;
    app->notice_time = 0.0f;
    return;

failed:
    graph_destroy(&graph);
    set_notice(app, "Could not allocate this example graph.", true);
}

static void position_random_graph(App *app, int count)
{
    const float turn = 6.28318530718f;
    if (count == 1) {
        app->positions[0] = (Vector2){.5f, .5f};
    } else if (count <= 9) {
        for (int vertex = 0; vertex < count; vertex++) {
            float angle = turn * ((float)vertex / (float)count - (count == 2 ? 0.0f : .25f));
            app->positions[vertex] = (Vector2){.5f + .38f * cosf(angle), .5f + .37f * sinf(angle)};
        }
    } else {
        int columns = (int)ceilf(sqrtf((float)count));
        int rows = (count + columns - 1) / columns;
        for (int vertex = 0; vertex < count; vertex++) {
            int row = vertex / columns;
            int column = vertex % columns;
            int row_length = row == rows - 1 ? count - row * columns : columns;
            float center_offset = (float)(columns - row_length) * .5f;
            float x = ((float)column + center_offset + .5f) / (float)columns;
            float y = ((float)row + .5f) / (float)rows;
            app->positions[vertex] = (Vector2){x, y};
        }
    }
}

static void random_graph(App *app)
{
    int count = atoi(app->random_input);
    if (count < 1 || count > APP_VERTEX_LIMIT) {
        set_notice(app, "Enter a vertex count from 1 to 64.", true);
        return;
    }

    Graph graph = {0};
    for (int vertex = 0; vertex < count; vertex++) {
        if (!graph_add_vertex(&graph)) goto failed;
    }
    int edge_chance = count <= 8 ? 44 : 380 / (count - 1);
    for (int a = 0; a < count; a++) {
        for (int b = a + 1; b < count; b++) {
            if (GetRandomValue(1, 100) <= edge_chance && !graph_add_edge(&graph, a, b)) {
                goto failed;
            }
        }
    }
    replace_graph(app, graph);
    position_random_graph(app, count);
    set_notice(app, "Random graph generated.", false);
    return;

failed:
    graph_destroy(&graph);
    set_notice(app, "Could not allocate a new graph.", true);
}

static void delete_vertex(App *app, int vertex)
{
    if (!graph_remove_vertex(&app->graph, vertex)) return;
    int tail = app->graph.vertex_count - vertex;
    if (tail > 0) {
        memmove(&app->positions[vertex], &app->positions[vertex + 1],
                (size_t)tail * sizeof(Vector2));
    }
    graph_changed(app);
    set_notice(app, "Vertex and its incident edges removed.", false);
}

static void delete_edge(App *app, int a, int b)
{
    if (!graph_remove_edge(&app->graph, a, b)) return;
    graph_changed(app);
    set_notice(app, "Edge removed.", false);
}

static float distance_squared(Vector2 a, Vector2 b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return dx * dx + dy * dy;
}

static void add_vertex(App *app, const GuiLayout *layout)
{
    int vertex = app->graph.vertex_count;
    if (vertex == APP_VERTEX_LIMIT) {
        set_notice(app, "This canvas supports up to 64 vertices.", true);
        return;
    }
    Vector2 normalized = gui_normalized_position(layout, app->mouse);
    Vector2 position = gui_vertex_position(layout, normalized);
    float spacing = gui_node_radius(app, layout) * 2.0f;
    for (int i = 0; i < vertex; i++) {
        Vector2 existing = gui_vertex_position(layout, app->positions[i]);
        if (distance_squared(position, existing) < spacing * spacing) {
            set_notice(app, "Leave room between the vertices.", true);
            return;
        }
    }
    if (!graph_add_vertex(&app->graph)) {
        set_notice(app, "Could not allocate a new vertex.", true);
        return;
    }
    graph_changed(app);
    app->positions[vertex] = normalized;
    app->selected_vertex = vertex;
    app->notice_time = 0.0f;
}

static void select_edge_end(App *app, int vertex)
{
    int anchor = app->edge_anchor;
    if (anchor < 0) {
        clear_selection(app);
        app->edge_anchor = vertex;
        app->notice_time = 0.0f;
    } else if (anchor == vertex) {
        set_notice(app, "Choose a different vertex. Self-loops are excluded.", true);
    } else if (graph_has_edge(&app->graph, anchor, vertex)) {
        set_notice(app, "Those vertices are already connected.", true);
    } else if (graph_add_edge(&app->graph, anchor, vertex)) {
        graph_changed(app);
        app->notice_time = 0.0f;
    } else {
        set_notice(app, "Could not allocate a new edge.", true);
    }
}

static float segment_distance_squared(Vector2 point, Vector2 a, Vector2 b)
{
    float length_squared = distance_squared(a, b);
    if (length_squared <= FLT_EPSILON) return distance_squared(point, a);
    float t = ((point.x - a.x) * (b.x - a.x) + (point.y - a.y) * (b.y - a.y)) / length_squared;
    t = clamp_unit(t);
    return distance_squared(point, (Vector2){a.x + t * (b.x - a.x), a.y + t * (b.y - a.y)});
}

static bool hits(Vector2 point, Rectangle rect)
{
    return CheckCollisionPointRec(point, rect);
}

static void update_hover(App *app, const GuiLayout *layout)
{
    app->hovered_vertex = -1;
    app->hovered_edge_a = -1;
    app->hovered_edge_b = -1;
    if (editing_locked(app) || app->example_open || !hits(app->mouse, layout->graph_area)) return;

    float radius = gui_node_radius(app, layout) + 4.0f;
    float nearest = radius * radius;
    for (int vertex = 0; vertex < app->graph.vertex_count; vertex++) {
        Vector2 position = gui_vertex_position(layout, app->positions[vertex]);
        float distance = distance_squared(app->mouse, position);
        if (distance <= nearest) {
            app->hovered_vertex = vertex;
            nearest = distance;
        }
    }
    if (app->hovered_vertex >= 0) return;

    nearest = 100.0f;
    for (int a = 0; a < app->graph.vertex_count; a++) {
        const Vertex *vertex = &app->graph.vertices[a];
        Vector2 start = gui_vertex_position(layout, app->positions[a]);
        for (int i = 0; i < vertex->degree; i++) {
            int b = vertex->neighbors[i];
            if (b <= a) continue;
            Vector2 end = gui_vertex_position(layout, app->positions[b]);
            float distance = segment_distance_squared(app->mouse, start, end);
            if (distance <= nearest) {
                nearest = distance;
                app->hovered_edge_a = a;
                app->hovered_edge_b = b;
            }
        }
    }
}

static void click_canvas(App *app, const GuiLayout *layout)
{
    int vertex = app->hovered_vertex;
    int a = app->hovered_edge_a;
    int b = app->hovered_edge_b;
    switch (app->edit_mode) {
        case EDIT_SELECT:
            clear_selection(app);
            if (vertex >= 0) {
                Vector2 position = gui_vertex_position(layout, app->positions[vertex]);
                app->selected_vertex = vertex;
                app->drag_vertex = vertex;
                app->drag_offset = (Vector2){position.x - app->mouse.x, position.y - app->mouse.y};
            } else {
                app->selected_edge_a = a;
                app->selected_edge_b = b;
            }
            break;
        case EDIT_VERTEX:
            if (vertex < 0 && a < 0) add_vertex(app, layout);
            break;
        case EDIT_EDGE:
            if (vertex >= 0) select_edge_end(app, vertex);
            break;
        case EDIT_DELETE_VERTEX:
            if (vertex >= 0) delete_vertex(app, vertex);
            break;
        case EDIT_DELETE_EDGE:
            if (a >= 0) delete_edge(app, a, b);
            break;
    }
}

static void advance_step(App *app)
{
    bfs_step(&app->bfs);
    if (app->bfs.finished) {
        app->status = SIM_FINISHED;
        app->step_credit = 0.0f;
    }
}

static void start_run(App *app, bool play)
{
    app->status = play ? SIM_RUNNING : SIM_PAUSED;
    app->step_credit = 0.0f;
    if (app->bfs.graph == NULL || app->bfs.finished) {
        reset_simulation(app);
        if (!bfs_init(&app->bfs, &app->graph)) {
            set_notice(app, "Could not allocate BFS working memory.", true);
            return;
        }
        app->status = play ? SIM_RUNNING : SIM_PAUSED;
        clear_selection(app);
        app->notice_time = 0.0f;
        advance_step(app);
    } else if (!play) {
        advance_step(app);
    }
}

static void toggle_play(App *app)
{
    if (app->status == SIM_RUNNING) {
        app->status = SIM_PAUSED;
        app->step_credit = 0.0f;
    } else {
        start_run(app, true);
    }
}

static void update_random_input(App *app)
{
    for (int code = GetCharPressed(); code != 0; code = GetCharPressed()) {
        if (code < '0' || code > '9') continue;
        if (app->random_replace) app->random_input[0] = '\0';
        app->random_replace = false;
        size_t length = strlen(app->random_input);
        if (length < sizeof app->random_input - 1) {
            app->random_input[length] = (char)code;
            app->random_input[length + 1] = '\0';
        }
    }
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_DELETE)) {
        size_t length = strlen(app->random_input);
        if (length > 0) app->random_input[app->random_replace ? 0 : length - 1] = '\0';
        app->random_replace = false;
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
        app->random_focused = false;
        random_graph(app);
    }
}

static void process_keys(App *app)
{
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (app->example_open) app->example_open = false;
        else if (app->random_focused) app->random_focused = false;
        else {
            clear_selection(app);
            app->edit_mode = EDIT_SELECT;
        }
        return;
    }
    if (app->example_open) return;
    if (app->random_focused) {
        update_random_input(app);
        return;
    }
    if (IsKeyPressed(KEY_SPACE)) toggle_play(app);
    else if (IsKeyPressed(KEY_N)) start_run(app, false);
    else if (IsKeyPressed(KEY_R)) reset_simulation(app);
    else if (IsKeyPressed(KEY_DELETE) && !editing_locked(app)) {
        if (app->selected_vertex >= 0) delete_vertex(app, app->selected_vertex);
        else if (app->selected_edge_a >= 0) delete_edge(app, app->selected_edge_a, app->selected_edge_b);
    }
}

static void process_click(App *app, const GuiLayout *layout)
{
    Vector2 mouse = app->mouse;
    if (app->example_open) {
        app->example_open = false;
        for (int index = 0; index < APP_EXAMPLE_COUNT; index++) {
            if (hits(mouse, layout->example_options[index])) {
                load_example(app, index);
                break;
            }
        }
        return;
    }

    app->random_focused = false;
    if (hits(mouse, layout->run)) {
        if (app->status != SIM_RUNNING) start_run(app, true);
    } else if (hits(mouse, layout->next)) start_run(app, false);
    else if (hits(mouse, layout->play)) toggle_play(app);
    else if (hits(mouse, layout->reset) || hits(mouse, layout->reset_algorithm)) reset_simulation(app);
    else if (hits(mouse, layout->speed)) app->slider_drag = true;
    else {
        for (int tab = EDU_PSEUDOCODE; tab <= EDU_COMPLEXITY; tab++) {
            if (hits(mouse, layout->tabs[tab])) {
                app->tab = (EduTab)tab;
                return;
            }
        }
        if (editing_locked(app)) return;
        for (int mode = EDIT_SELECT; mode <= EDIT_DELETE_EDGE; mode++) {
            if (hits(mouse, layout->tool[mode])) {
                app->edit_mode = (EditMode)mode;
                clear_selection(app);
                app->notice_time = 0.0f;
                return;
            }
        }
        if (hits(mouse, layout->random_input)) {
            app->random_focused = true;
            app->random_replace = true;
        } else if (hits(mouse, layout->random_graph)) random_graph(app);
        else if (hits(mouse, layout->example)) app->example_open = true;
        else if (hits(mouse, layout->clear_graph)) {
            replace_graph(app, (Graph){0});
            set_notice(app, "Graph cleared.", false);
        } else if (hits(mouse, layout->graph_area)) click_canvas(app, layout);
    }
}

static void update_pointer(App *app, const GuiLayout *layout)
{
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) process_click(app, layout);
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        app->slider_drag = false;
        app->drag_vertex = -1;
    }
    if (app->slider_drag) {
        float fraction = (app->mouse.x - layout->speed.x) / layout->speed.width;
        app->speed = min_speed * powf(speed_ratio, clamp_unit(fraction));
    }
    if (app->drag_vertex >= 0 && !editing_locked(app)) {
        Vector2 point = {app->mouse.x + app->drag_offset.x, app->mouse.y + app->drag_offset.y};
        app->positions[app->drag_vertex] = gui_normalized_position(layout, point);
    }
    if (app->tab == EDU_VERTICES && hits(app->mouse, layout->table) && !app->example_open) {
        app->table_scroll -= (int)GetMouseWheelMove();
    }
    int max_scroll = app->graph.vertex_count - layout->table_rows;
    if (app->table_scroll > max_scroll) app->table_scroll = max_scroll;
    if (app->table_scroll < 0) app->table_scroll = 0;
}

static Font load_ui_font(const char *filename)
{
    Font fallback = GetFontDefault();
#ifdef _WIN32
    char path[1024] = {0};
    const char *windows_dir = "C:/Windows";
    #ifdef _MSC_VER
    char directory[768] = {0};
    size_t required = 0;
    if (getenv_s(&required, directory, sizeof directory, "WINDIR") == 0 && required > 1) {
        windows_dir = directory;
    }
    #else
    const char *environment = getenv("WINDIR");
    if (environment && *environment) windows_dir = environment;
    #endif
    int size = snprintf(path, sizeof path, "%s/Fonts/%s", windows_dir, filename);
    if (size > 0 && (size_t)size < sizeof path && FileExists(path)) {
        int points[128];
        int count = 0;
        for (int code = 32; code <= 126; code++) points[count++] = code;
        const int extra[] = {0x00b7, 0x00d7, 0x2013, 0x2014, 0x2022, 0x2026,
                             0x2190, 0x2191, 0x2192, 0x2193, 0x2212, 0x221e,
                             0x2260, 0x2264, 0x2265};
        for (size_t i = 0; i < sizeof extra / sizeof extra[0]; i++) points[count++] = extra[i];
        Font font = LoadFontEx(path, 48, points, count);
        if (font.texture.id != 0 && font.texture.id != fallback.texture.id) {
            SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
            return font;
        }
        if (IsFontValid(font) && font.glyphs != fallback.glyphs) UnloadFont(font);
    }
#else
    (void)filename;
#endif
    return fallback;
}

void app_init(App *app)
{
    app->body_font = load_ui_font("segoeui.ttf");
    app->bold_font = load_ui_font("seguisb.ttf");
    app->mono_font = load_ui_font("consola.ttf");
    app->speed = 1.0f;
    memcpy(app->random_input, "8", sizeof "8");
    graph_changed(app);
    load_example(app, 0);
}

void app_frame(App *app)
{
    float elapsed = fminf(GetFrameTime(), 0.1f);
    GuiLayout layout = gui_layout(GetScreenWidth(), GetScreenHeight());
    app->mouse = GetMousePosition();
    app->notice_time = fmaxf(0.0f, app->notice_time - elapsed);

    update_hover(app, &layout);
    process_keys(app);
    update_pointer(app, &layout);
    update_hover(app, &layout);

    if (app->status == SIM_RUNNING) {
        app->step_credit += elapsed * app->speed * 8.0f;
        while (app->status == SIM_RUNNING && app->step_credit >= 1.0f) {
            app->step_credit -= 1.0f;
            advance_step(app);
        }
    }

    BeginDrawing();
    gui_draw(app, &layout);
    EndDrawing();
}

void app_destroy(App *app)
{
    bfs_destroy(&app->bfs);
    graph_destroy(&app->graph);
    Font fallback = GetFontDefault();
    const Font fonts[] = {app->body_font, app->bold_font, app->mono_font};
    for (size_t i = 0; i < sizeof fonts / sizeof fonts[0]; i++) {
        if (fonts[i].texture.id != fallback.texture.id && fonts[i].texture.id != 0) UnloadFont(fonts[i]);
    }
}
