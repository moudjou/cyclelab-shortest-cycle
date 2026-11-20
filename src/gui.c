#include "gui.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define ARROW "\xE2\x86\x92"
#define ELLIPSIS "\xE2\x80\xA6"
#define EM_DASH "\xE2\x80\x94"
#define INFINITY_GLYPH "\xE2\x88\x9E"
#define TIMES "\xC3\x97"

static const Color BACKGROUND = { 12, 17, 30, 255 };
static const Color PANEL = { 19, 26, 44, 255 };
static const Color CANVAS = { 14, 22, 39, 255 };
static const Color TILE = { 24, 34, 56, 255 };
static const Color LAID_IN = { 16, 24, 43, 255 };
static const Color HOVER = { 30, 43, 70, 255 };
static const Color STROKE = { 51, 67, 100, 255 };
static const Color TEXT = { 232, 239, 252, 255 };
static const Color MUTED = { 180, 196, 224, 255 };
static const Color SUBTLE = { 150, 171, 204, 255 };
static const Color DISABLED = { 125, 146, 179, 255 };
static const Color ACCENT_BLUE = { 102, 153, 255, 255 };
static const Color CYAN = { 79, 211, 241, 255 };
static const Color ACCENT_VIOLET = { 171, 141, 255, 255 };
static const Color ACCENT_YELLOW = { 252, 204, 104, 255 };
static const Color ACCENT_GREEN = { 77, 251, 177, 255 };
static const Color ROSE = { 255, 151, 174, 255 };
static const Color NODE_GRAY = { 126, 145, 178, 255 };

typedef enum Icon {
    ICON_NONE, ICON_POINTER, ICON_VERTEX, ICON_EDGE, ICON_REMOVE_VERTEX,
    ICON_REMOVE_EDGE, ICON_TRASH, ICON_SHUFFLE, ICON_CHEVRON,
    ICON_RESET, ICON_PLAY, ICON_PAUSE, ICON_NEXT, ICON_CHECK
} Icon;

static const char *const tool_labels[] = {
    "Select and drag", "Add vertex", "Add edge", "Delete vertex", "Delete edge"
};

static const Icon tool_icons[] = {
    ICON_POINTER, ICON_VERTEX, ICON_EDGE, ICON_REMOVE_VERTEX, ICON_REMOVE_EDGE
};

static Rectangle rect(float x, float y, float width, float height)
{
    return (Rectangle){ x, y, width, height };
}

static float clampf(float value, float lower, float upper)
{
    return fminf(fmaxf(value, lower), upper);
}

GuiLayout gui_layout(int width, int height)
{
    GuiLayout layout = {0};
    const float w = (float)width;
    const float h = (float)height;
    const float margin = clampf(w * 0.018f, 18.0f, 28.0f);
    const float gap = clampf(w * 0.011f, 12.0f, 18.0f);
    const float sidebar_width = clampf(w * 0.157f, 199.0f, 234.0f);
    const float inspector_width = clampf(w * 0.25f, 304.0f, 392.0f);
    const float body_y = margin + 63.0f;
    const float bottom_y = h - margin - 68.0f;
    const float body_height = bottom_y - gap - body_y;

    layout.header = rect(margin, margin, w - margin * 2.0f, 49.0f);
    layout.sidebar = rect(margin, body_y, sidebar_width, body_height);
    layout.inspector = rect(w - margin - inspector_width, body_y,
                            inspector_width, body_height);
    const float canvas_x = margin + sidebar_width + gap;
    layout.canvas = rect(canvas_x, body_y, layout.inspector.x - gap - canvas_x, body_height);
    layout.graph_area = rect(canvas_x + 1.0f, body_y + 56.0f,
                             layout.canvas.width - 2.0f, body_height - 104.0f);
    layout.transport = rect(margin, bottom_y, w - margin * 2.0f, 68.0f);

    const float sidebar_x = margin + 14.0f;
    const float control_width = sidebar_width - 28.0f;
    const float tool_height = clampf(body_height * 0.054f + 6.0f, 34.0f, 40.0f);
    float tool_y = body_y + 72.0f;
    for (int tool = 0; tool < 5; ++tool) {
        layout.tool[tool] = rect(sidebar_x, tool_y, control_width, tool_height);
        tool_y += tool_height + 5.0f;
    }
    layout.clear_graph = rect(sidebar_x, tool_y, control_width, tool_height);
    const float sources_y = tool_y + tool_height + 60.0f;
    layout.random_input = rect(sidebar_x, sources_y, 48.0f, tool_height);
    layout.random_graph = rect(sidebar_x + 55.0f, sources_y,
                               control_width - 55.0f, tool_height);
    layout.example = rect(sidebar_x, sources_y + tool_height + 35.0f,
                          control_width, tool_height);
    layout.reset_algorithm = rect(sidebar_x, layout.sidebar.y + layout.sidebar.height
                                  - tool_height - 14.0f, control_width, tool_height);
    const float menu_height = 32.0f * APP_EXAMPLE_COUNT;
    float menu_y = layout.example.y + tool_height + 7.0f;
    if (menu_y + menu_height > bottom_y - gap - 9.0f) {
        menu_y = layout.example.y - menu_height - 7.0f;
    }
    for (int example = 0; example < APP_EXAMPLE_COUNT; ++example) {
        layout.example_options[example] = rect(sidebar_x, menu_y + 32.0f * (float)example,
                                               control_width, 32.0f);
    }

    const float inspect_x = layout.inspector.x + 14.0f;
    const float inspect_width = inspector_width - 28.0f;
    const float tab_width = (inspect_width - 6.0f) / 3.0f;
    for (int tab = 0; tab < 3; ++tab) {
        layout.tabs[tab] = rect(inspect_x + (tab_width + 3.0f) * (float)tab,
                                body_y + 281.0f, tab_width, 32.0f);
    }
    layout.table = rect(inspect_x, body_y + 325.0f, inspect_width, body_height - 339.0f);
    layout.table_rows = (int)fmaxf(1.0f, floorf((layout.table.height - 29.0f) / 27.0f));

    const float controls_x = layout.transport.x + 13.0f;
    const float controls_y = bottom_y + 12.0f;
    layout.run = rect(controls_x, controls_y, 113.0f, 44.0f);
    layout.next = rect(layout.run.x + layout.run.width + 8.0f, controls_y, 105.0f, 44.0f);
    layout.play = rect(layout.next.x + layout.next.width + 8.0f, controls_y, 97.0f, 44.0f);
    layout.reset = rect(layout.play.x + layout.play.width + 8.0f, controls_y, 82.0f, 44.0f);
    const float speed_x = layout.reset.x + layout.reset.width + 23.0f;
    const float speed_width = clampf(w * 0.16f, 150.0f, 246.0f);
    layout.speed = rect(speed_x, bottom_y + 32.0f, speed_width, 24.0f);
    const float progress_x = speed_x + speed_width + 36.0f;
    layout.progress = rect(progress_x, bottom_y + 32.0f,
                           w - margin - 16.0f - progress_x, 24.0f);
    return layout;
}

static Rectangle vertex_bounds(const GuiLayout *layout)
{
    Rectangle area = layout->graph_area;
    return rect(area.x + 35.0f, area.y + 35.0f, area.width - 70.0f, area.height - 70.0f);
}

Vector2 gui_vertex_position(const GuiLayout *layout, Vector2 normalized)
{
    const Rectangle bounds = vertex_bounds(layout);
    return (Vector2){ bounds.x + normalized.x * bounds.width,
                      bounds.y + normalized.y * bounds.height };
}

Vector2 gui_normalized_position(const GuiLayout *layout, Vector2 screen)
{
    const Rectangle bounds = vertex_bounds(layout);
    return (Vector2){ clampf((screen.x - bounds.x) / bounds.width, 0.0f, 1.0f),
                      clampf((screen.y - bounds.y) / bounds.height, 0.0f, 1.0f) };
}

float gui_node_radius(const App *app, const GuiLayout *layout)
{
    const float canvas_scale = fminf(layout->graph_area.width, layout->graph_area.height);
    const float density = fmaxf(0.0f, (float)app->graph.vertex_count - 11.0f);
    return clampf(canvas_scale * 0.039f - density * 0.18f, 11.0f, 22.0f);
}

static float roundness(Rectangle bounds, float radius)
{
    return clampf(radius * 2.0f / fminf(bounds.width, bounds.height), 0.0f, 1.0f);
}

static void fill_round(Rectangle bounds, float radius, Color fill)
{
    DrawRectangleRounded(bounds, roundness(bounds, radius), 8, fill);
}

static void fill_outline(Rectangle bounds, float radius, Color fill, Color border)
{
    fill_round(bounds, radius, border);
    const Rectangle inside = rect(bounds.x + 1.0f, bounds.y + 1.0f,
                                  bounds.width - 2.0f, bounds.height - 2.0f);
    fill_round(inside, radius - 1.0f, fill);
}

static void panel(Rectangle bounds, Color fill)
{
    const Rectangle shadow = rect(bounds.x + 1.0f, bounds.y + 4.0f,
                                  bounds.width - 2.0f, bounds.height);
    fill_round(shadow, 15.0f, (Color){ 5, 9, 19, 122 });
    fill_outline(bounds, 15.0f, fill, STROKE);
}

static void label(Font font, const char *value, float x, float y, float size, Color color)
{
    DrawTextEx(font, value, (Vector2){ x, y }, size, 0.24f, color);
}

static float text_width(Font font, const char *value, float size)
{
    return MeasureTextEx(font, value, size, 0.24f).x;
}

static void label_right(Font font, const char *value, float right, float y, float size, Color color)
{
    label(font, value, right - text_width(font, value, size), y, size, color);
}

static void label_center(Font font, const char *value, Rectangle bounds, float size, Color color)
{
    const Vector2 measured = MeasureTextEx(font, value, size, 0.24f);
    label(font, value, bounds.x + (bounds.width - measured.x) * 0.5f,
          bounds.y + (bounds.height - measured.y) * 0.5f, size, color);
}

static bool has_unicode(Font font)
{
    return font.texture.id != GetFontDefault().texture.id;
}

static const char *ellipsis(Font font)
{
    return has_unicode(font) ? ELLIPSIS : "...";
}

static const char *dash(Font font)
{
    return has_unicode(font) ? EM_DASH : "-";
}

static void line(float x1, float y1, float x2, float y2, float width, Color color)
{
    DrawLineEx((Vector2){ x1, y1 }, (Vector2){ x2, y2 }, width, color);
}

static void rule(float x, float y, float width)
{
    DrawRectangleRec(rect(x, y, width, 1.0f), STROKE);
}

static void draw_icon(Icon icon, float x, float y, Color color)
{
    switch (icon) {
        case ICON_POINTER:
            line(x - 4, y - 6, x - 4, y + 5, 1.6f, color);
            line(x - 4, y - 6, x + 5, y + 1, 1.6f, color);
            line(x - 4, y + 5, x, y + 2, 1.6f, color);
            line(x, y + 2, x + 3, y + 6, 1.6f, color);
            line(x, y + 2, x + 5, y + 1, 1.6f, color);
            break;
        case ICON_VERTEX:
        case ICON_REMOVE_VERTEX:
            DrawRing((Vector2){ x, y }, 5.0f, 6.5f, 0, 360, 24, color);
            line(x - 2.5f, y, x + 2.5f, y, 1.35f, color);
            if (icon == ICON_VERTEX) {
                line(x, y - 2.5f, x, y + 2.5f, 1.35f, color);
            }
            break;
        case ICON_EDGE:
        case ICON_REMOVE_EDGE:
            DrawCircleV((Vector2){ x - 4.5f, y + 3.5f }, 2.5f, color);
            DrawCircleV((Vector2){ x + 4.5f, y - 3.5f }, 2.5f, color);
            line(x - 4.5f, y + 3.5f, x + 4.5f, y - 3.5f, 1.55f, color);
            if (icon == ICON_REMOVE_EDGE) {
                line(x - 3.5f, y - 5.5f, x + 3.5f, y + 5.5f, 1.65f, color);
            }
            break;
        case ICON_TRASH:
            line(x - 6, y - 4, x + 6, y - 4, 1.6f, color);
            line(x - 2.5f, y - 6, x + 2.5f, y - 6, 1.5f, color);
            line(x - 4.5f, y - 3, x - 3.5f, y + 6, 1.5f, color);
            line(x + 4.5f, y - 3, x + 3.5f, y + 6, 1.5f, color);
            line(x - 3.5f, y + 6, x + 3.5f, y + 6, 1.5f, color);
            break;
        case ICON_SHUFFLE:
            line(x - 6, y - 4, x - 3, y - 4, 1.55f, color);
            line(x - 3, y - 4, x + 2, y + 4, 1.55f, color);
            line(x + 2, y + 4, x + 6, y + 4, 1.55f, color);
            line(x - 6, y + 4, x - 3, y + 4, 1.55f, color);
            line(x - 3, y + 4, x + 2, y - 4, 1.55f, color);
            line(x + 2, y - 4, x + 6, y - 4, 1.55f, color);
            line(x + 3.5f, y - 6.5f, x + 6, y - 4, 1.55f, color);
            line(x + 3.5f, y + 6.5f, x + 6, y + 4, 1.55f, color);
            break;
        case ICON_CHEVRON:
            line(x - 4, y - 2, x, y + 2, 1.6f, color);
            line(x, y + 2, x + 4, y - 2, 1.6f, color);
            break;
        case ICON_RESET:
            DrawRing((Vector2){ x, y }, 4.7f, 6.2f, -65, 220, 24, color);
            line(x - 5.7f, y - 5.2f, x - 5.7f, y - 0.7f, 1.5f, color);
            line(x - 5.7f, y - 0.7f, x - 1.2f, y - 0.7f, 1.5f, color);
            break;
        case ICON_PLAY:
        case ICON_NEXT:
            DrawTriangle((Vector2){ x - 3.5f, y - 5.5f }, (Vector2){ x - 3.5f, y + 5.5f },
                         (Vector2){ x + 5, y }, color);
            if (icon == ICON_NEXT) {
                line(x + 6, y - 5, x + 6, y + 5, 1.6f, color);
            }
            break;
        case ICON_PAUSE:
            line(x - 2.5f, y - 5.5f, x - 2.5f, y + 5.5f, 2.3f, color);
            line(x + 2.5f, y - 5.5f, x + 2.5f, y + 5.5f, 2.3f, color);
            break;
        case ICON_CHECK:
            line(x - 4, y, x - 1, y + 3, 1.7f, color);
            line(x - 1, y + 3, x + 5, y - 4, 1.7f, color);
            break;
        case ICON_NONE:
            break;
    }
}

static bool over(const App *app, Rectangle bounds)
{
    return CheckCollisionPointRec(app->mouse, bounds);
}

static void button(const App *app, Rectangle bounds, const char *title, Icon icon,
                   bool active, bool enabled, bool primary, bool centered, Color accent)
{
    const bool hovered = enabled && over(app, bounds);
    Color fill = hovered ? HOVER : TILE;
    Color border = STROKE;
    Color foreground = enabled ? TEXT : DISABLED;
    if (primary && enabled) {
        fill = hovered ? (Color){ 69, 113, 218, 255 } : (Color){ 58, 94, 195, 255 };
        border = hovered ? ACCENT_BLUE : (Color){ 103, 151, 255, 255 };
        accent = TEXT;
    } else if (active && enabled) {
        fill = hovered ? (Color){ 53, 49, 92, 255 } : (Color){ 45, 40, 81, 255 };
        border = hovered ? ACCENT_VIOLET : (Color){ 106, 86, 173, 255 };
    }
    if (!enabled) {
        fill = LAID_IN;
        accent = DISABLED;
    }
    fill_outline(bounds, 8.0f, fill, border);
    const Font font = primary || active ? app->bold_font : app->body_font;
    const float icon_width = icon == ICON_NONE ? 0.0f : 20.0f;
    const float preferred_size = 16.0f;
    const float size = clampf(preferred_size * (bounds.width - icon_width - 14.0f)
                             / text_width(font, title, preferred_size), 12.0f, preferred_size);
    const float combined_width = text_width(font, title, size) + icon_width;
    const float x = centered ? bounds.x + (bounds.width - combined_width) * 0.5f
                             : bounds.x + 11.0f;
    if (icon != ICON_NONE) {
        draw_icon(icon, x + 6.0f, bounds.y + bounds.height * 0.5f, accent);
    }
    label(font, title, x + icon_width, bounds.y + (bounds.height - size) * 0.5f,
          size, foreground);
}

static void draw_header(const App *app, const GuiLayout *layout)
{
    const Rectangle bounds = layout->header;
    const Rectangle logo = rect(bounds.x, bounds.y, 49.0f, 49.0f);
    fill_outline(logo, 13.0f, (Color){ 37, 38, 80, 255 }, (Color){ 91, 89, 155, 255 });
    const Vector2 a = { logo.x + 24.5f, logo.y + 12.5f };
    const Vector2 b = { logo.x + 13.0f, logo.y + 33.5f };
    const Vector2 c = { logo.x + 36.0f, logo.y + 33.5f };
    DrawLineEx(a, b, 1.75f, ACCENT_VIOLET);
    DrawLineEx(b, c, 1.75f, ACCENT_VIOLET);
    DrawLineEx(c, a, 1.75f, ACCENT_VIOLET);
    DrawCircleV(a, 3.7f, CYAN);
    DrawCircleV(b, 3.7f, ACCENT_VIOLET);
    DrawCircleV(c, 3.7f, ACCENT_BLUE);
    label(app->bold_font, "Shortest Cycle Visualizer", bounds.x + 63.0f, bounds.y - 1.0f,
          30.0f, TEXT);
    label(app->body_font, "Undirected graphs  /  Breadth-first search from every vertex",
          bounds.x + 64.0f, bounds.y + 33.0f, 16.0f, MUTED);

    const float x = bounds.x + bounds.width - 182.0f;
    const Rectangle key = rect(x, bounds.y + 12.0f, 54.0f, 25.0f);
    fill_outline(key, 5.0f, TILE, STROKE);
    label_center(app->mono_font, "Space", key, 13.0f, MUTED);
    label(app->body_font, "Play / pause BFS", x + 64.0f, bounds.y + 17.0f, 15.0f, SUBTLE);
}

static bool editing_enabled(const App *app)
{
    return app->status != SIM_RUNNING && app->status != SIM_PAUSED;
}

static void draw_sidebar(const App *app, const GuiLayout *layout)
{
    const Rectangle bounds = layout->sidebar;
    const bool editable = editing_enabled(app);
    panel(bounds, PANEL);
    label(app->bold_font, "Graph controls", bounds.x + 16.0f, bounds.y + 15.0f, 22.0f, TEXT);
    label(app->bold_font, "EDIT GRAPH", bounds.x + 15.0f, layout->tool[0].y - 20.0f,
          12.0f, SUBTLE);
    for (int tool = 0; tool < 5; ++tool) {
        button(app, layout->tool[tool], tool_labels[tool], tool_icons[tool],
               (int)app->edit_mode == tool, editable, false, false,
               tool >= EDIT_DELETE_VERTEX ? ROSE : ACCENT_VIOLET);
    }
    button(app, layout->clear_graph, "Clear graph", ICON_TRASH, false,
           editable && app->graph.vertex_count > 0, false, false, ROSE);

    rule(bounds.x + 14.0f, layout->random_input.y - 48.0f, bounds.width - 28.0f);
    label(app->bold_font, "GRAPH SOURCES", bounds.x + 15.0f, layout->random_input.y - 36.0f,
          12.0f, SUBTLE);
    label(app->body_font, "Vertices", layout->random_input.x, layout->random_input.y - 15.0f,
          13.0f, MUTED);
    const Color input_border = editable && app->random_focused ? ACCENT_BLUE : STROKE;
    fill_outline(layout->random_input, 8.0f, LAID_IN, input_border);
    label_center(app->mono_font, app->random_input, layout->random_input, 15.0f,
                 editable ? TEXT : DISABLED);
    if (editable && app->random_focused && fmod(GetTime(), 1.0) < 0.62) {
        const Rectangle input = layout->random_input;
        const float x = input.x + (input.width + text_width(app->mono_font, app->random_input,
                                                          15.0f)) * 0.5f + 2.0f;
        line(x, input.y + input.height * 0.5f - 8.0f,
             x, input.y + input.height * 0.5f + 8.0f, 1.1f, ACCENT_BLUE);
    }
    button(app, layout->random_graph, "Generate random", ICON_SHUFFLE,
           false, editable, false, true, ACCENT_BLUE);

    label(app->body_font, "Load example graph", layout->example.x, layout->example.y - 17.0f,
          14.0f, MUTED);
    const Color example_fill = editable && over(app, layout->example) ? HOVER : TILE;
    fill_outline(layout->example, 8.0f, example_fill, app->example_open ? ACCENT_BLUE : STROKE);
    const char *example = "Choose an example";
    if (app->example_index >= 0 && app->example_index < APP_EXAMPLE_COUNT) {
        example = app_example_names[app->example_index];
    }
    label(app->body_font, example, layout->example.x + 11.0f,
          layout->example.y + (layout->example.height - 16.0f) * 0.5f,
          16.0f, editable ? TEXT : DISABLED);
    draw_icon(ICON_CHEVRON, layout->example.x + layout->example.width - 15.0f,
              layout->example.y + layout->example.height * 0.5f, editable ? MUTED : DISABLED);

    if (!editable && layout->reset_algorithm.y - (layout->example.y + layout->example.height) > 35.0f) {
        label(app->body_font, "Reset to edit the graph", bounds.x + 15.0f,
              layout->reset_algorithm.y - 22.0f, 14.0f, SUBTLE);
    }
    button(app, layout->reset_algorithm, "Reset algorithm", ICON_RESET,
           false, true, false, false, MUTED);
}

static void draw_example_menu(const App *app, const GuiLayout *layout)
{
    if (!app->example_open) {
        return;
    }
    const Rectangle first = layout->example_options[0];
    const Rectangle frame = rect(first.x - 4.0f, first.y - 4.0f, first.width + 8.0f,
                                 first.height * APP_EXAMPLE_COUNT + 8.0f);
    const Rectangle shadow = rect(frame.x - 2.0f, frame.y + 4.0f, frame.width + 4.0f,
                                  frame.height + 4.0f);
    fill_round(shadow, 11.0f, (Color){ 4, 8, 18, 205 });
    fill_outline(frame, 10.0f, (Color){ 25, 35, 60, 255 }, (Color){ 70, 86, 130, 255 });
    for (int index = 0; index < APP_EXAMPLE_COUNT; ++index) {
        const Rectangle item = layout->example_options[index];
        if (over(app, item) || index == app->example_index) {
            fill_round(item, 6.0f, HOVER);
        }
        label(app->body_font, app_example_names[index], item.x + 10.0f,
              item.y + 8.0f, 16.0f, index == app->example_index ? ACCENT_BLUE : TEXT);
        if (index == app->example_index) {
            draw_icon(ICON_CHECK, item.x + item.width - 14.0f, item.y + 16.0f, ACCENT_BLUE);
        }
    }
}

static bool same_edge(int a, int b, int x, int y)
{
    return (a == x && b == y) || (a == y && b == x);
}

static bool tree_edge(const App *app, int a, int b)
{
    return app->bfs.root >= 0 && app->bfs.parent
        && (app->bfs.parent[a] == b || app->bfs.parent[b] == a);
}

static bool examined_edge(const App *app, int a, int b)
{
    const BFSEvent event = app->bfs.event;
    return (event == BFS_EXAMINE || event == BFS_CANDIDATE || event == BFS_IMPROVE)
        && same_edge(a, b, app->bfs.current, app->bfs.neighbor);
}

static void draw_graph_edges(const App *app, const Vector2 *points, const int *cycle_next)
{
    const Graph *graph = &app->graph;
    for (int pass = 0; pass < 2; ++pass) {
        for (int a = 0; a < graph->vertex_count; ++a) {
            for (int adj = 0; adj < graph->vertices[a].degree; ++adj) {
                const int b = graph->vertices[a].neighbors[adj];
                if (a >= b) {
                    continue;
                }
                const bool cycle = cycle_next[a] == b || cycle_next[b] == a;
                const bool examined = examined_edge(app, a, b);
                const bool hovered = same_edge(a, b, app->hovered_edge_a, app->hovered_edge_b);
                const bool selected = same_edge(a, b, app->selected_edge_a, app->selected_edge_b);
                const bool accent = cycle || examined || selected || hovered;
                if (accent != (pass == 1)) {
                    continue;
                }

                Color color = (Color){ 86, 108, 149, 255 };
                float width = 2.2f;
                if (tree_edge(app, a, b)) {
                    color = ACCENT_VIOLET;
                    width = 2.65f;
                }
                if (selected || hovered) {
                    color = app->edit_mode == EDIT_DELETE_EDGE ? ROSE : ACCENT_BLUE;
                    width = 3.4f;
                }
                if (cycle) {
                    DrawLineEx(points[a], points[b], 14.0f, Fade(ACCENT_GREEN, 0.08f));
                    DrawLineEx(points[a], points[b], 8.0f, Fade(ACCENT_GREEN, 0.20f));
                    color = ACCENT_GREEN;
                    width = 3.7f;
                }
                if (examined) {
                    DrawLineEx(points[a], points[b], 10.0f, Fade(ACCENT_YELLOW, 0.16f));
                    color = ACCENT_YELLOW;
                    width = 3.8f;
                }
                DrawLineEx(points[a], points[b], width, color);
            }
        }
    }
}

static void draw_graph_vertices(const App *app, const GuiLayout *layout,
                                const Vector2 *points, const int *cycle_next)
{
    const float radius = gui_node_radius(app, layout);
    const float pulse = app->status == SIM_RUNNING
        ? (1.0f + sinf((float)GetTime() * 6.5f)) * 0.5f
        : 0.4f;
    for (int v = 0; v < app->graph.vertex_count; ++v) {
        const Vector2 point = points[v];
        const bool current = app->bfs.current == v && app->status != SIM_FINISHED;
        const bool root = app->bfs.root == v && app->status != SIM_FINISHED;
        const bool discovered = app->bfs.root >= 0 && app->bfs.distance
            && app->bfs.distance[v] >= 0;
        const bool cycle = cycle_next[v] >= 0;
        const bool hovered = v == app->hovered_vertex;
        const bool selected = v == app->selected_vertex || v == app->edge_anchor;
        Color border = NODE_GRAY;
        Color fill = (Color){ 40, 52, 76, 255 };
        if (discovered) {
            border = CYAN;
            fill = (Color){ 22, 67, 89, 255 };
        }
        if (current) {
            border = ACCENT_BLUE;
            fill = (Color){ 47, 91, 197, 255 };
        }
        if (cycle && app->status == SIM_FINISHED) {
            border = ACCENT_GREEN;
            fill = (Color){ 17, 82, 69, 255 };
        }
        if (editing_enabled(app) && (hovered || selected)) {
            border = app->edit_mode == EDIT_DELETE_VERTEX ? ROSE : ACCENT_VIOLET;
            fill = (Color){ 61, 54, 102, 255 };
        }

        DrawCircleV((Vector2){ point.x + 1.0f, point.y + 3.0f }, radius + 3.0f,
                    (Color){ 4, 9, 20, 165 });
        if (cycle) {
            DrawCircleV(point, radius + 12.0f, Fade(ACCENT_GREEN, 0.07f));
            DrawCircleV(point, radius + 8.0f, Fade(ACCENT_GREEN, 0.13f));
            DrawRing(point, radius + 3.0f, radius + 4.6f, 0, 360, 44, ACCENT_GREEN);
        }
        if (current || root) {
            DrawRing(point, radius + (cycle ? 8.0f : 4.0f),
                     radius + (cycle ? 9.3f : 5.4f), 0, 360, 44,
                     Fade(ACCENT_BLUE, 0.52f + pulse * 0.35f));
        }
        if ((selected || hovered) && editing_enabled(app)) {
            DrawCircleV(point, radius + 6.0f, Fade(border, selected ? 0.22f : 0.12f));
        }
        DrawCircleV(point, radius, fill);
        DrawRing(point, radius - 1.7f, radius, 0, 360, 48, border);
        char node[12];
        snprintf(node, sizeof(node), "%d", v + 1);
        const float number_size = clampf(radius * 0.92f, 12.0f, 20.0f);
        label_center(app->bold_font, node, rect(point.x - radius, point.y - radius + 0.5f,
                                               radius * 2.0f, radius * 2.0f),
                     number_size, TEXT);
    }
}

static void draw_legend(const App *app, const GuiLayout *layout)
{
    static const char *const states[] = {
        "Unvisited", "Current", "Discovered", "Examined", "Tree edge", "Cycle"
    };
    const Color colors[] = { NODE_GRAY, ACCENT_BLUE, CYAN, ACCENT_YELLOW, ACCENT_VIOLET, ACCENT_GREEN };
    const Rectangle bounds = layout->canvas;
    const float y = bounds.y + bounds.height - 48.0f;
    rule(bounds.x + 1.0f, y, bounds.width - 2.0f);

    const float font_size = bounds.width < 560.0f ? 14.0f : 15.0f;
    const float gap = bounds.width < 560.0f ? 12.0f : 20.0f;
    float width = -gap;
    for (int state = 0; state < 6; ++state) {
        width += 15.0f + text_width(app->body_font, states[state], font_size) + gap;
    }
    float x = bounds.x + (bounds.width - width) * 0.5f;
    for (int state = 0; state < 6; ++state) {
        if (state >= 3) {
            line(x + 1.0f, y + 23.0f, x + 11.0f, y + 23.0f, 2.5f, colors[state]);
        } else {
            DrawCircleV((Vector2){ x + 5.0f, y + 23.0f }, 3.8f, colors[state]);
        }
        label(app->body_font, states[state], x + 15.0f, y + 23.0f - font_size * 0.5f,
              font_size, MUTED);
        x += 15.0f + text_width(app->body_font, states[state], font_size) + gap;
    }
}

static const char *graph_hint(const App *app)
{
    if (!editing_enabled(app)) {
        return "Graph editing is locked during BFS";
    }
    switch (app->edit_mode) {
        case EDIT_VERTEX: return "Click the canvas to add a vertex";
        case EDIT_EDGE: return app->edge_anchor >= 0
            ? "Select the second endpoint" : "Select the first endpoint";
        case EDIT_DELETE_VERTEX: return "Click a vertex to delete it";
        case EDIT_DELETE_EDGE: return "Click an edge to delete it";
        case EDIT_SELECT: return "Drag vertices to reposition them";
    }
    return "";
}

static void draw_canvas(const App *app, const GuiLayout *layout)
{
    const Rectangle bounds = layout->canvas;
    const Rectangle graph_area = layout->graph_area;
    panel(bounds, PANEL);
    label(app->bold_font, "Graph workspace", bounds.x + 17.0f, bounds.y + 16.0f, 22.0f, TEXT);

    const char *hint = graph_hint(app);
    const float hint_width = text_width(app->body_font, hint, 14.0f) + 25.0f;
    const Rectangle hint_box = rect(bounds.x + bounds.width - hint_width - 13.0f,
                                   bounds.y + 16.0f, hint_width, 25.0f);
    if (hint_box.x > bounds.x + 174.0f) {
        fill_outline(hint_box, 12.0f, LAID_IN, STROKE);
        label_center(app->body_font, hint, hint_box, 14.0f, MUTED);
    }
    rule(bounds.x + 1.0f, graph_area.y - 1.0f, bounds.width - 2.0f);
    DrawRectangleGradientV((int)graph_area.x, (int)graph_area.y,
                          (int)ceilf(graph_area.width), (int)ceilf(graph_area.height),
                          (Color){ 17, 26, 45, 255 }, CANVAS);
    const int dots_x = (int)((graph_area.width - 29.0f) / 22.0f);
    const int dots_y = (int)((graph_area.height - 29.0f) / 22.0f);
    const float dot_x = graph_area.x + (graph_area.width - (float)dots_x * 22.0f) * 0.5f;
    const float dot_y = graph_area.y + (graph_area.height - (float)dots_y * 22.0f) * 0.5f;
    for (int y = 0; y <= dots_y; ++y) {
        for (int x = 0; x <= dots_x; ++x) {
            DrawCircleV((Vector2){ dot_x + 22.0f * (float)x, dot_y + 22.0f * (float)y },
                        0.9f, (Color){ 43, 59, 89, 220 });
        }
    }

    if (app->graph.vertex_count == 0) {
        const float middle_y = graph_area.y + graph_area.height * 0.5f;
        label_center(app->bold_font, "The graph is empty",
                     rect(graph_area.x, middle_y - 26.0f, graph_area.width, 27.0f),
                     24.0f, TEXT);
        label_center(app->body_font, "Add a vertex or load an example graph.",
                     rect(graph_area.x, middle_y + 7.0f, graph_area.width, 25.0f),
                     16.0f, MUTED);
    }

    Vector2 points[APP_VERTEX_LIMIT];
    int cycle_next[APP_VERTEX_LIMIT];
    for (int v = 0; v < app->graph.vertex_count; ++v) {
        points[v] = gui_vertex_position(layout, app->positions[v]);
        cycle_next[v] = -1;
    }
    for (int i = 0; i < app->bfs.best.length; ++i) {
        cycle_next[app->bfs.best.vertices[i]] = app->bfs.best.vertices[i + 1];
    }
    draw_graph_edges(app, points, cycle_next);
    if (app->edit_mode == EDIT_EDGE && app->edge_anchor >= 0 && editing_enabled(app)
        && over(app, graph_area) && app->hovered_vertex != app->edge_anchor) {
        const Vector2 end = app->hovered_vertex >= 0 ? points[app->hovered_vertex] : app->mouse;
        DrawLineEx(points[app->edge_anchor], end, 2.0f, Fade(ACCENT_VIOLET, 0.75f));
    }
    draw_graph_vertices(app, layout, points, cycle_next);

    if (app->notice_time > 0.0f && app->notice[0] != '\0') {
        const float notice_width = fminf(text_width(app->body_font, app->notice, 15.0f) + 25.0f,
                                       graph_area.width - 24.0f);
        const Rectangle notice_box = rect(graph_area.x + (graph_area.width - notice_width) * 0.5f,
                                         graph_area.y + graph_area.height - 42.0f,
                                         notice_width, 30.0f);
        const Color accent = app->notice_warning ? ACCENT_YELLOW : ACCENT_BLUE;
        fill_outline(notice_box, 9.0f, TILE, Fade(accent, 0.62f));
        label_center(app->body_font, app->notice, notice_box, 15.0f,
                     app->notice_warning ? ACCENT_YELLOW : TEXT);
    }
    draw_legend(app, layout);
}

static const char *vertex_text(Font font, int vertex, char buffer[12])
{
    if (vertex < 0) {
        return dash(font);
    }
    snprintf(buffer, 12, "%d", vertex + 1);
    return buffer;
}

static const char *number_text(Font font, int value, char buffer[12])
{
    if (value < 0) {
        return dash(font);
    }
    snprintf(buffer, 12, "%d", value);
    return buffer;
}

static void vertical_metric(const App *app, Rectangle bounds,
                            const char *title, const char *value, Color accent)
{
    fill_outline(bounds, 7.0f, LAID_IN, STROKE);
    label(app->body_font, title, bounds.x + 8.0f, bounds.y + 5.0f, 14.0f, MUTED);
    label(app->mono_font, value, bounds.x + 8.0f, bounds.y + 23.0f, 18.0f, accent);
}

static void count_metric(const App *app, Rectangle bounds,
                         const char *title, int value, Color accent)
{
    char count[12];
    snprintf(count, sizeof(count), "%d", value);
    fill_outline(bounds, 8.0f, TILE, STROKE);
    label(app->body_font, title, bounds.x + 10.0f, bounds.y + 12.0f, 15.0f, MUTED);
    label_right(app->mono_font, count, bounds.x + bounds.width - 10.0f,
                bounds.y + 8.0f, 22.0f, accent);
}

static void status_pill(const App *app, Rectangle panel_bounds)
{
    static const char *const labels[] = { "Ready", "Running", "Paused", "Finished" };
    const Color colors[] = { MUTED, CYAN, ACCENT_YELLOW, ACCENT_GREEN };
    const Color color = colors[app->status];
    const char *value = labels[app->status];
    const float width = text_width(app->bold_font, value, 14.0f) + 30.0f;
    const Rectangle bounds = rect(panel_bounds.x + panel_bounds.width - width - 14.0f,
                                  panel_bounds.y + 17.0f, width, 24.0f);
    fill_outline(bounds, 11.0f, LAID_IN, Fade(color, 0.43f));
    float opacity = 1.0f;
    if (app->status == SIM_RUNNING) {
        opacity = 0.65f + 0.35f * (0.5f + 0.5f * sinf((float)GetTime() * 6.0f));
    }
    DrawCircleV((Vector2){ bounds.x + 11.0f, bounds.y + 12.0f }, 3.6f, Fade(color, opacity));
    label(app->bold_font, value, bounds.x + 20.0f, bounds.y + 5.0f, 14.0f, color);
}

static bool draw_queue(const App *app, float x, float y, float width)
{
    label(app->bold_font, "BFS queue", x, y - 1.0f, 14.0f, MUTED);
    const BFSQueue *queue = &app->bfs.queue;
    const float start = x;
    const float right = start + width;
    y += 17.0f;
    if (queue->tail <= queue->head) {
        label(app->body_font, "Empty", x, y + 5.0f, 15.0f, SUBTLE);
        return false;
    }
    for (int i = queue->head; i < queue->tail; ++i) {
        const bool first = i == queue->head;
        char buffer[12];
        const char *value = vertex_text(app->mono_font, queue->items[i], buffer);
        const float chip_width = fmaxf(text_width(app->mono_font, value, 15.0f) + 12.0f, 24.0f);
        if (x + chip_width + (i + 1 < queue->tail ? 33.0f : 0.0f) > right) {
            snprintf(buffer, sizeof(buffer), "+%d", queue->tail - i);
            label(app->mono_font, buffer, x + 2.0f, y + 5.0f, 15.0f, MUTED);
            return true;
        }
        const Rectangle chip = rect(x, y, chip_width, 25.0f);
        fill_outline(chip, 6.0f, first ? (Color){ 23, 58, 78, 255 } : TILE,
                     first ? (Color){ 58, 113, 149, 255 } : STROKE);
        label_center(app->mono_font, value, chip, 15.0f, first ? CYAN : TEXT);
        x += chip_width + 5.0f;
    }
    return false;
}

static void format_cycle(const App *app, char *buffer, size_t capacity)
{
    size_t used = 0;
    const char *arrow = has_unicode(app->mono_font) ? " " ARROW " " : " -> ";
    buffer[0] = '\0';
    if (app->bfs.best.length < 3) {
        return;
    }
    for (int i = 0; i <= app->bfs.best.length; ++i) {
        const char *separator = i == 0 ? "" : arrow;
        const int written = snprintf(buffer + used, capacity - used, "%s%d", separator,
                                     app->bfs.best.vertices[i] + 1);
        if (written < 0 || (size_t)written >= capacity - used) {
            return;
        }
        used += (size_t)written;
    }
}

static void draw_cycle_preview(const App *app, const char *route, const Rectangle bounds)
{
    char preview[APP_VERTEX_LIMIT * 10 + 16];
    const float size = 17.0f;
    const float available = bounds.width - 20.0f;
    const float x = bounds.x + 10.0f;
    const float y = bounds.y + 27.0f;
    const Font font = app->mono_font;
    if (text_width(font, route, size) <= available) {
        label(font, route, x, y, size, ACCENT_GREEN);
        return;
    }

    size_t fit = 0;
    for (const char *end = strchr(route, ' '); end; end = strchr(end + 1, ' ')) {
        if (end[-1] >= '0' && end[-1] <= '9') {
            const int length = (int)(end - route);
            snprintf(preview, sizeof(preview), "%.*s %s", length, route, ellipsis(font));
            if (text_width(font, preview, size) > available - 17.0f) {
                break;
            }
            fit = (size_t)length;
        }
    }
    snprintf(preview, sizeof(preview), "%.*s %s", (int)fit, route, ellipsis(font));
    label(font, preview, x, y, size, ACCENT_GREEN);
    draw_icon(ICON_CHEVRON, bounds.x + bounds.width - 15.0f, y + 7.0f, ACCENT_GREEN);
}

static void draw_result(const App *app, Rectangle bounds, const char *route)
{
    const bool finished = app->status == SIM_FINISHED;
    const bool has_cycle = route[0] != '\0';
    const Color border = has_cycle ? (Color){ 51, 111, 97, 255 } : STROKE;
    const Color fill = has_cycle ? (Color){ 18, 45, 49, 255 } : LAID_IN;
    fill_outline(bounds, 9.0f, fill, border);
    const float x = bounds.x + 10.0f;
    if (has_cycle) {
        label(app->bold_font, finished ? "Shortest cycle" : "Best cycle so far", x,
              bounds.y + 7.0f, 14.0f, ACCENT_GREEN);
        draw_cycle_preview(app, route, bounds);
    } else if (finished) {
        label(app->bold_font, "Shortest cycle", x, bounds.y + 7.0f, 14.0f, MUTED);
        const char *result = has_unicode(app->body_font)
            ? "No cycle found " EM_DASH " the graph is acyclic."
            : "No cycle found - the graph is acyclic.";
        const float size = fminf(15.0f, 15.0f * (bounds.width - 20.0f)
                                 / text_width(app->body_font, result, 15.0f));
        label(app->body_font, result, x, bounds.y + 27.0f, size, TEXT);
    } else {
        label(app->bold_font, "Shortest cycle", x, bounds.y + 7.0f, 14.0f, MUTED);
        label(app->body_font, app->status == SIM_READY ? "Run BFS to find a cycle." : "No cycle found yet.",
              x, bounds.y + 27.0f, 15.0f, SUBTLE);
    }
}

static float wrap_text(Font font, const char *value, Rectangle bounds,
                       float size, float line_height, Color color)
{
    char row[384];
    const char *start = value;
    float y = bounds.y;
    while (*start != '\0') {
        const char *word = start;
        const char *fit = start;
        while (*word != '\0') {
            const char *end = word + strcspn(word, " ");
            const size_t length = (size_t)(end - start);
            if (length >= sizeof(row)) {
                break;
            }
            memcpy(row, start, length);
            row[length] = '\0';
            if (fit > start && text_width(font, row, size) > bounds.width) {
                break;
            }
            fit = end;
            word = *end == ' ' ? end + 1 : end;
        }
        const size_t length = (size_t)(fit - start);
        if (!length) {
            break;
        }
        memcpy(row, start, length);
        row[length] = '\0';
        if (y + size > bounds.y + bounds.height) {
            break;
        }
        if (color.a != 0) {
            label(font, row, bounds.x, y, size, color);
        }
        y += line_height;
        start = *fit == ' ' ? fit + 1 : fit;
    }
    return y;
}

static int active_instruction(BFSEvent event)
{
    switch (event) {
        case BFS_ROOT: return 1;
        case BFS_DEQUEUE: return 2;
        case BFS_EXAMINE: return 3;
        case BFS_DISCOVER: return 4;
        case BFS_CANDIDATE: return 5;
        case BFS_IMPROVE: return 6;
        case BFS_END_ROOT: return 0;
        case BFS_FINISH: return 7;
        case BFS_IDLE: return -1;
    }
    return -1;
}

static void draw_pseudocode(const App *app, Rectangle bounds)
{
    static const char *const instructions[] = {
        "for each root s in V",
        "  mark and enqueue s",
        "  u = dequeue()",
        "  for each neighbor v of u",
        "    if new: visit and enqueue v",
        "    if non-tree: score the edge",
        "    if score is lower: use LCA",
        "return the shortest cycle"
    };
    const int active = active_instruction(app->bfs.event);
    const float line_height = clampf((bounds.height - 42.0f) / 8.0f, 17.0f, 25.0f);
    const float font_size = clampf(bounds.width * 0.048f, 12.0f, 15.5f);
    fill_outline(rect(bounds.x, bounds.y, bounds.width, line_height * 8.0f + 3.0f),
                 8.0f, LAID_IN, STROKE);
    for (int i = 0; i < 8; ++i) {
        const Rectangle row = rect(bounds.x + 2.0f, bounds.y + 2.0f + (float)i * line_height,
                                  bounds.width - 4.0f, line_height);
        if (i == active) {
            fill_round(row, 5.0f, (Color){ 47, 49, 91, 255 });
            fill_round(rect(row.x, row.y + 2.0f, 2.0f, line_height - 4.0f), 1.0f, ACCENT_VIOLET);
        }
        char count[8];
        snprintf(count, sizeof(count), "%02d", i + 1);
        const float y = row.y + (line_height - font_size) * 0.5f;
        label(app->mono_font, count, row.x + 8.0f, y, font_size, i == active ? ACCENT_VIOLET : SUBTLE);
        label(app->mono_font, instructions[i], row.x + 33.0f, y, font_size,
              i == active ? TEXT : MUTED);
    }

    const char *description = "Each vertex gets its own BFS tree.";
    char message[112];
    switch (app->bfs.event) {
        case BFS_IDLE:
            break;
        case BFS_ROOT:
            description = "The new root starts at distance 0.";
            break;
        case BFS_DEQUEUE:
            description = "Inspect the next vertex in the queue.";
            break;
        case BFS_EXAMINE:
            description = "Check whether the neighbor is discovered.";
            break;
        case BFS_DISCOVER:
            description = "Record the parent and distance, then enqueue.";
            break;
        case BFS_CANDIDATE:
            snprintf(message, sizeof(message), "Candidate score %d = d[u] + d[v] + 1.",
                     app->bfs.candidate_score);
            description = message;
            break;
        case BFS_IMPROVE:
            snprintf(message, sizeof(message), "%d-edge cycle rebuilt via the lowest common ancestor.",
                     app->bfs.best.length);
            description = message;
            break;
        case BFS_END_ROOT:
            description = "The root's BFS queue is empty.";
            break;
        case BFS_FINISH:
            description = "All roots have been explored.";
            break;
    }
    const float explanation_y = bounds.y + line_height * 8.0f + 11.0f;
    const float explanation_height = bounds.y + bounds.height - explanation_y + 1.0f;
    wrap_text(app->body_font, description,
              rect(bounds.x + 2.0f, explanation_y, bounds.width - 4.0f, explanation_height),
              14.0f, 16.0f, MUTED);
}

static void draw_vertex_table(const App *app, const GuiLayout *layout)
{
    const Rectangle bounds = layout->table;
    const float height = 27.0f;
    const float columns[] = { 0.0f, 0.24f, 0.45f, 0.72f, 1.0f };
    const char *const names[] = { "Vertex", "Visited", "Distance", "Parent" };
    fill_outline(bounds, 8.0f, LAID_IN, STROKE);
    const Rectangle header = rect(bounds.x + 1.0f, bounds.y + 1.0f,
                                   bounds.width - 2.0f, height);
    fill_round(header, 6.0f, TILE);
    for (int col = 0; col < 4; ++col) {
        const Rectangle cell = rect(bounds.x + columns[col] * bounds.width, header.y,
                                     (columns[col + 1] - columns[col]) * bounds.width,
                                     height);
        label_center(app->bold_font, names[col], cell, 14.0f, MUTED);
    }
    if (app->graph.vertex_count == 0) {
        label_center(app->body_font, "Add vertices to populate the table.",
                     rect(bounds.x, bounds.y + 28.0f, bounds.width, height * 2.0f),
                     15.0f, SUBTLE);
        return;
    }

    for (int row = 0; row < layout->table_rows; ++row) {
        const int index = app->table_scroll + row;
        if (index >= app->graph.vertex_count) {
            break;
        }
        const float y = header.y + (float)(row + 1) * height;
        const bool current = app->bfs.current == index;
        const bool tracked = app->bfs.root >= 0 && app->bfs.distance;
        const bool visited = tracked && app->bfs.distance[index] >= 0;
        if (current || row % 2 == 0) {
            DrawRectangleRec(rect(bounds.x + 2.0f, y, bounds.width - 4.0f, height),
                             current ? (Color){ 32, 56, 95, 255 } : PANEL);
        }
        char number[12];
        char distance[12];
        char parent[12];
        const Font mono = app->mono_font;
        const char *inf = has_unicode(mono) ? INFINITY_GLYPH : "INF";
        const char *const values[] = {
            vertex_text(mono, index, number),
            visited ? "Yes" : "No",
            visited ? number_text(mono, app->bfs.distance[index], distance) : inf,
            vertex_text(mono, visited ? app->bfs.parent[index] : -1, parent)
        };
        for (int col = 0; col < 4; ++col) {
            const Rectangle cell = rect(bounds.x + columns[col] * bounds.width, y,
                                         (columns[col + 1] - columns[col]) * bounds.width,
                                         height);
            const Color color = col == 1 ? (visited ? CYAN : MUTED) : TEXT;
            label_center(mono, values[col], cell, 15.0f, color);
        }
    }

    if (app->graph.vertex_count > layout->table_rows) {
        const float track_y = header.y + height + 4.0f;
        const float track_height = (float)layout->table_rows * height - 8.0f;
        const float offset = (float)app->table_scroll / (float)app->graph.vertex_count;
        const float ratio = (float)layout->table_rows / (float)app->graph.vertex_count;
        const float bar_x = bounds.x + bounds.width - 6.0f;
        fill_round(rect(bar_x, track_y, 3.0f, track_height), 1.5f, STROKE);
        fill_round(rect(bar_x, track_y + track_height * offset, 3.0f, track_height * ratio),
                   1.5f, ACCENT_BLUE);
    }
}

static void draw_complexity(const App *app, Rectangle bounds)
{
    label(app->bold_font, "Complexity Analysis", bounds.x, bounds.y + 1.0f, 19.0f, TEXT);
    const float time_width = bounds.width * 0.56f - 3.0f;
    const Rectangle time = rect(bounds.x, bounds.y + 28.0f, time_width, 48.0f);
    const Rectangle space = rect(bounds.x + time_width + 7.0f, time.y,
                                  bounds.width - time_width - 7.0f, 48.0f);
    fill_outline(time, 7.0f, TILE, STROKE);
    fill_outline(space, 7.0f, TILE, STROKE);
    label(app->bold_font, "Time", time.x + 8.0f, time.y + 5.0f, 14.0f, MUTED);
    label(app->bold_font, "Space", space.x + 8.0f, space.y + 5.0f, 14.0f, MUTED);
    const char *time_bound = has_unicode(app->mono_font) ? "O(V " TIMES " (V + E))"
                                                        : "O(V * (V + E))";
    label(app->mono_font, time_bound, time.x + 8.0f, time.y + 24.0f, 14.0f, ACCENT_BLUE);
    label(app->mono_font, "O(V + E)", space.x + 8.0f, space.y + 24.0f, 14.0f, ACCENT_VIOLET);

    const float text_y = bounds.y + 86.0f;
    const float bottom = bounds.y + bounds.height;
    const float next_y = wrap_text(app->body_font,
                                  "An adjacency-list BFS takes O(V + E). "
                                  "Running it from every vertex multiplies that cost by V.",
                                  rect(bounds.x, text_y, bounds.width, bottom - text_y),
                                  15.0f, 17.0f, MUTED);
    wrap_text(app->body_font,
              "The adjacency list, queue and working arrays require O(V + E) space.",
              rect(bounds.x, next_y + 5.0f, bounds.width, bottom - next_y - 5.0f),
              15.0f, 17.0f, MUTED);
}

static void draw_detail(const App *app, Rectangle parent, float y, const char *title,
                        const char *sequence, Color accent)
{
    const float size = 14.0f;
    const float line_height = 17.0f;
    const float text_y = y + 28.0f;
    const float width = parent.width - 22.0f;
    const float max_height = 190.0f;
    // A two-pass wrap makes the overlay only as tall as its ordered contents.
    const Rectangle contents = rect(parent.x + 11.0f, text_y, width, max_height);
    const float line_end = wrap_text(app->mono_font, sequence, contents, size, line_height, BLANK);
    const Rectangle frame = rect(parent.x, y, parent.width, line_end - y + 9.0f);
    panel(frame, TILE);
    label(app->bold_font, title, frame.x + 11.0f, frame.y + 7.0f, 15.0f, TEXT);
    wrap_text(app->mono_font, sequence, contents, size, line_height, accent);
}

static void draw_inspector(const App *app, const GuiLayout *layout)
{
    static const char *const tabs[] = { "Pseudocode", "Vertices", "Complexity" };
    const Rectangle bounds = layout->inspector;
    const float x = bounds.x + 14.0f;
    const float width = bounds.width - 28.0f;
    panel(bounds, PANEL);
    label(app->bold_font, "Algorithm state", x, bounds.y + 15.0f, 22.0f, TEXT);
    status_pill(app, bounds);

    const float count_width = (width - 7.0f) * 0.5f;
    count_metric(app, rect(x, bounds.y + 53.0f, count_width, 38.0f),
                 "Vertices |V|", app->graph.vertex_count, ACCENT_BLUE);
    count_metric(app, rect(x + count_width + 7.0f, bounds.y + 53.0f, count_width, 38.0f),
                 "Edges |E|", app->graph.edge_count, ACCENT_VIOLET);

    const int distance = app->bfs.current >= 0 && app->bfs.distance
        ? app->bfs.distance[app->bfs.current] : -1;
    char root_value[12], current_value[12], distance_value[12], best_value[12];
    const Font mono = app->mono_font;
    const char *const names[] = { "BFS root", "Current", "Distance", "Best cycle" };
    const char *const values[] = {
        vertex_text(mono, app->bfs.root, root_value),
        vertex_text(mono, app->bfs.current, current_value),
        number_text(mono, distance, distance_value),
        number_text(mono, app->bfs.best.length > 0 ? app->bfs.best.length : -1, best_value)
    };
    const Color colors[] = { ACCENT_BLUE, ACCENT_BLUE, CYAN, app->bfs.best.length > 0 ? ACCENT_GREEN : MUTED };
    const float metric_width = (width - 15.0f) * 0.25f;
    for (int i = 0; i < 4; ++i) {
        vertical_metric(app, rect(x + (float)i * (metric_width + 5.0f), bounds.y + 100.0f,
                                  metric_width, 47.0f), names[i], values[i], colors[i]);
    }

    const bool queue_overflow = draw_queue(app, x, bounds.y + 159.0f, width);
    const Rectangle result = rect(x, bounds.y + 211.0f, width, 55.0f);
    char route[APP_VERTEX_LIMIT * 10 + 16];
    format_cycle(app, route, sizeof(route));
    draw_result(app, result, route);
    rule(x, bounds.y + 273.0f, width);
    for (int tab = 0; tab < 3; ++tab) {
        const bool selected = tab == (int)app->tab;
        const Rectangle box = layout->tabs[tab];
        const Color fill = selected ? (Color){ 42, 44, 79, 255 }
            : (over(app, box) ? HOVER : LAID_IN);
        fill_outline(box, 7.0f, fill, selected ? (Color){ 95, 82, 152, 255 } : STROKE);
        label_center(selected ? app->bold_font : app->body_font, tabs[tab], box, 15.0f,
                     selected ? TEXT : MUTED);
    }
    switch (app->tab) {
        case EDU_PSEUDOCODE: draw_pseudocode(app, layout->table); break;
        case EDU_VERTICES: draw_vertex_table(app, layout); break;
        case EDU_COMPLEXITY: draw_complexity(app, layout->table); break;
    }

    const Rectangle queue = rect(x, bounds.y + 157.0f, width, 44.0f);
    if (queue_overflow && over(app, queue)) {
        char list[APP_VERTEX_LIMIT * 8 + 16];
        size_t used = 0;
        list[0] = '\0';
        for (int i = app->bfs.queue.head; i < app->bfs.queue.tail; ++i) {
            const int written = snprintf(list + used, sizeof(list) - used, "%s%d",
                i == app->bfs.queue.head ? "" : ", ", app->bfs.queue.items[i] + 1);
            if (written < 0 || (size_t)written >= sizeof(list) - used) {
                break;
            }
            used += (size_t)written;
        }
        draw_detail(app, queue, queue.y + queue.height + 4.0f,
                    "BFS queue, front to back", list, CYAN);
    } else if (over(app, result) && text_width(mono, route, 17.0f) > result.width - 20.0f) {
        draw_detail(app, result, result.y + result.height + 4.0f,
                    "Ordered cycle", route, ACCENT_GREEN);
    }
}

static void draw_slider(const App *app, Rectangle slider)
{
    const float position = clampf(app_speed_fraction(app->speed), 0.0f, 1.0f);
    const float y = slider.y + slider.height * 0.5f;
    const float x = slider.x + slider.width * position;
    char title[24];
    const char *times = has_unicode(app->mono_font) ? TIMES : "x";
    snprintf(title, sizeof(title), "%.2g%s", (double)app->speed, times);
    label(app->bold_font, "Animation speed", slider.x, slider.y - 18.0f, 15.0f, MUTED);
    label_right(app->mono_font, title, slider.x + slider.width, slider.y - 18.0f, 15.0f, ACCENT_BLUE);
    fill_round(rect(slider.x, y - 2.0f, slider.width, 4.0f), 2.0f, STROKE);
    if (position > 0.0f) {
        fill_round(rect(slider.x, y - 2.0f, slider.width * position, 4.0f), 2.0f, ACCENT_BLUE);
    }
    DrawCircleV((Vector2){ x, y }, 10.0f, Fade(ACCENT_BLUE, app->slider_drag ? 0.38f : 0.20f));
    DrawCircleV((Vector2){ x, y }, 6.3f, app->slider_drag || over(app, slider) ? TEXT : ACCENT_BLUE);
}

static void draw_progress(const App *app, Rectangle progress)
{
    const int completed = app->bfs.completed_roots;
    const int count = app->graph.vertex_count;
    const float value = app->status == SIM_FINISHED ? 1.0f
        : (count > 0 ? (float)completed / (float)count : 0.0f);
    char summary[48];
    snprintf(summary, sizeof(summary), "%d / %d roots", completed, count);
    label(app->bold_font, "BFS progress", progress.x, progress.y - 18.0f, 15.0f, MUTED);
    label_right(app->mono_font, summary, progress.x + progress.width, progress.y - 18.0f,
                15.0f, app->status == SIM_FINISHED ? ACCENT_GREEN : MUTED);
    const float y = progress.y + progress.height * 0.5f - 3.0f;
    fill_round(rect(progress.x, y, progress.width, 6.0f), 3.0f, STROKE);
    if (value > 0.0f) {
        fill_round(rect(progress.x, y, progress.width * value, 6.0f), 3.0f,
                   app->status == SIM_FINISHED ? ACCENT_GREEN : ACCENT_VIOLET);
    }
}

static void draw_transport(const App *app, const GuiLayout *layout)
{
    panel(layout->transport, PANEL);
    const bool running = app->status == SIM_RUNNING;
    button(app, layout->run, "Run BFS", ICON_PLAY, false, !running, true, true, TEXT);
    button(app, layout->next, "Next step", ICON_NEXT, false, true, false, true, ACCENT_VIOLET);
    button(app, layout->play, running ? "Pause" : "Play", running ? ICON_PAUSE : ICON_PLAY,
           false, true, false, true, ACCENT_BLUE);
    button(app, layout->reset, "Reset", ICON_RESET, false, true, false, true, MUTED);
    const float x = layout->reset.x + layout->reset.width + 11.0f;
    const float y = layout->transport.y + 13.0f;
    line(x, y, x, y + 42.0f, 1.0f, STROKE);
    draw_slider(app, layout->speed);
    draw_progress(app, layout->progress);
}

void gui_draw(const App *app, const GuiLayout *layout)
{
    ClearBackground(BACKGROUND);
    draw_header(app, layout);
    draw_sidebar(app, layout);
    draw_canvas(app, layout);
    draw_inspector(app, layout);
    draw_transport(app, layout);
    draw_example_menu(app, layout);
}
