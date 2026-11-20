#ifndef GRAPH_H
#define GRAPH_H

#include <stdbool.h>

typedef struct {
    int *neighbors;
    int degree;
    int capacity;
} Vertex;

typedef struct {
    Vertex *vertices;
    int vertex_count;
    int edge_count;
    int capacity;
} Graph;

/* Zero-initialize before first use. Vertex indices are zero-based and compact. */
bool graph_add_vertex(Graph *graph);
bool graph_add_edge(Graph *graph, int from, int to);
bool graph_remove_vertex(Graph *graph, int vertex);
bool graph_remove_edge(Graph *graph, int from, int to);
bool graph_has_edge(const Graph *graph, int from, int to);

void graph_clear(Graph *graph);
void graph_destroy(Graph *graph);

#endif
