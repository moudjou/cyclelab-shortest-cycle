#include "graph.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int reserve_capacity(int current, int minimum, size_t element_size)
{
    int capacity = current > INT_MAX / 2 ? INT_MAX : 2 * current;
    if (capacity < 4) capacity = 4;
    if (capacity < minimum) capacity = minimum;

    const size_t limit = SIZE_MAX / element_size;
    if ((size_t)capacity > limit) capacity = (int)limit;
    return capacity >= minimum ? capacity : 0;
}

static bool vertex_exists(const Graph *graph, int vertex)
{
    return graph && vertex >= 0 && vertex < graph->vertex_count;
}

static int neighbor_index(const Vertex *vertex, int neighbor)
{
    for (int i = 0; i < vertex->degree; ++i) {
        if (vertex->neighbors[i] == neighbor) return i;
    }
    return -1;
}

static bool reserve_neighbor(Vertex *vertex)
{
    if (vertex->degree < vertex->capacity) return true;

    const int capacity = reserve_capacity(vertex->capacity, vertex->degree + 1,
                                          sizeof *vertex->neighbors);
    if (capacity == 0) return false;

    int *neighbors = realloc(vertex->neighbors, (size_t)capacity * sizeof *neighbors);
    if (!neighbors) return false;
    vertex->neighbors = neighbors;
    vertex->capacity = capacity;
    return true;
}

static void shrink_neighbors(Vertex *vertex)
{
    if (vertex->degree == 0) {
        free(vertex->neighbors);
        *vertex = (Vertex){0};
    } else if (vertex->capacity > 4 && vertex->degree <= vertex->capacity / 4) {
        const int capacity = vertex->capacity / 2;
        int *neighbors = realloc(vertex->neighbors, (size_t)capacity * sizeof *neighbors);
        if (neighbors) {
            vertex->neighbors = neighbors;
            vertex->capacity = capacity;
        }
    }
}

static void remove_neighbor(Vertex *vertex, int index)
{
    --vertex->degree;
    for (int i = index; i < vertex->degree; ++i) {
        vertex->neighbors[i] = vertex->neighbors[i + 1];
    }
    shrink_neighbors(vertex);
}

bool graph_add_vertex(Graph *graph)
{
    if (!graph || graph->vertex_count == INT_MAX) return false;
    if (graph->vertex_count == graph->capacity) {
        const int capacity = reserve_capacity(graph->capacity, graph->vertex_count + 1,
                                              sizeof *graph->vertices);
        if (capacity == 0) return false;

        Vertex *vertices = realloc(graph->vertices, (size_t)capacity * sizeof *vertices);
        if (!vertices) return false;
        graph->vertices = vertices;
        graph->capacity = capacity;
    }
    graph->vertices[graph->vertex_count++] = (Vertex){0};
    return true;
}

bool graph_has_edge(const Graph *graph, int from, int to)
{
    return vertex_exists(graph, from) && vertex_exists(graph, to) &&
           neighbor_index(&graph->vertices[from], to) >= 0;
}

bool graph_add_edge(Graph *graph, int from, int to)
{
    if (!vertex_exists(graph, from) || !vertex_exists(graph, to) ||
        from == to || graph->edge_count == INT_MAX) return false;

    Vertex *source = &graph->vertices[from];
    Vertex *target = &graph->vertices[to];
    if (neighbor_index(source, to) >= 0 ||
        !reserve_neighbor(source) || !reserve_neighbor(target)) return false;

    source->neighbors[source->degree++] = to;
    target->neighbors[target->degree++] = from;
    ++graph->edge_count;
    return true;
}

bool graph_remove_edge(Graph *graph, int from, int to)
{
    if (!vertex_exists(graph, from) || !vertex_exists(graph, to) || from == to) {
        return false;
    }

    Vertex *source = &graph->vertices[from];
    Vertex *target = &graph->vertices[to];
    const int forward = neighbor_index(source, to);
    const int backward = neighbor_index(target, from);
    if (forward < 0 || backward < 0) return false;

    remove_neighbor(source, forward);
    remove_neighbor(target, backward);
    --graph->edge_count;
    return true;
}

bool graph_remove_vertex(Graph *graph, int vertex)
{
    if (!vertex_exists(graph, vertex)) return false;

    graph->edge_count -= graph->vertices[vertex].degree;
    free(graph->vertices[vertex].neighbors);
    --graph->vertex_count;
    memmove(&graph->vertices[vertex], &graph->vertices[vertex + 1],
            (size_t)(graph->vertex_count - vertex) * sizeof *graph->vertices);
    graph->vertices[graph->vertex_count] = (Vertex){0};

    for (int i = 0; i < graph->vertex_count; ++i) {
        Vertex *remaining = &graph->vertices[i];
        int write = 0;
        for (int j = 0; j < remaining->degree; ++j) {
            const int neighbor = remaining->neighbors[j];
            if (neighbor == vertex) continue;
            remaining->neighbors[write++] = neighbor > vertex ? neighbor - 1 : neighbor;
        }
        if (write != remaining->degree) {
            remaining->degree = write;
            shrink_neighbors(remaining);
        }
    }

    if (graph->vertex_count == 0) {
        graph_clear(graph);
    } else if (graph->capacity > 4 && graph->vertex_count <= graph->capacity / 4) {
        const int capacity = graph->capacity / 2;
        Vertex *vertices = realloc(graph->vertices, (size_t)capacity * sizeof *vertices);
        if (vertices) {
            graph->vertices = vertices;
            graph->capacity = capacity;
        }
    }
    return true;
}

void graph_clear(Graph *graph)
{
    if (!graph) return;
    for (int i = 0; i < graph->vertex_count; ++i) free(graph->vertices[i].neighbors);
    free(graph->vertices);
    *graph = (Graph){0};
}

void graph_destroy(Graph *graph)
{
    graph_clear(graph);
}
