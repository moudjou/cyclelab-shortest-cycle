#include "bfs.h"
#include "graph.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { REFERENCE_CAPACITY = 7, EXHAUSTIVE_LIMIT = 6 };

typedef struct {
    int vertex_count;
    bool adjacent[REFERENCE_CAPACITY][REFERENCE_CAPACITY];
} ReferenceGraph;

typedef struct {
    int from;
    int to;
} Edge;

#define COUNT(items) (sizeof(items) / sizeof((items)[0]))
#define CHECK(condition, context)                                            \
    do {                                                                      \
        if (!(condition)) {                                                   \
            fprintf(stderr, "%s, line %d: %s\n", (context), __LINE__, #condition); \
            exit(EXIT_FAILURE);                                               \
        }                                                                     \
    } while (0)

static void add_vertices(Graph *graph, int count, const char *context)
{
    for (int vertex = 0; vertex < count; ++vertex) {
        CHECK(graph_add_vertex(graph), context);
    }
    CHECK(graph->vertex_count == count, context);
}

static void add_edge(Graph *graph, ReferenceGraph *reference, Edge edge,
                     const char *context)
{
    CHECK(graph_add_edge(graph, edge.from, edge.to), context);
    reference->adjacent[edge.from][edge.to] = true;
    reference->adjacent[edge.to][edge.from] = true;
}

static void validate_result(const ReferenceGraph *graph, const CycleResult *cycle,
                            int expected_length, const char *context)
{
    if (cycle->length != expected_length) {
        fprintf(stderr, "%s: expected length %d, got %d\n",
                context, expected_length, cycle->length);
        exit(EXIT_FAILURE);
    }
    if (cycle->length == 0) {
        return;
    }

    CHECK(cycle->length >= 3 && cycle->length <= graph->vertex_count, context);
    CHECK(cycle->vertices != NULL, context);
    CHECK(cycle->vertices[0] == cycle->vertices[cycle->length], context);

    bool seen[REFERENCE_CAPACITY] = { false };
    for (int index = 0; index < cycle->length; ++index) {
        int from = cycle->vertices[index];
        int to = cycle->vertices[index + 1];
        CHECK(from >= 0 && from < graph->vertex_count, context);
        CHECK(to >= 0 && to < graph->vertex_count, context);
        CHECK(!seen[from], context);
        CHECK(graph->adjacent[from][to], context);
        seen[from] = true;
    }
}

static void search_simple_cycles(const ReferenceGraph *graph, int first, int last,
                                 uint32_t used, int path_length, int *shortest)
{
    for (int next = first; next < graph->vertex_count; ++next) {
        if (!graph->adjacent[last][next]) {
            continue;
        }
        if (next == first) {
            if (path_length >= 3 && path_length < *shortest) {
                *shortest = path_length;
            }
        } else if ((used & (UINT32_C(1) << next)) == 0 && path_length < *shortest) {
            search_simple_cycles(graph, first, next, used | (UINT32_C(1) << next),
                                 path_length + 1, shortest);
        }
    }
}

static int brute_force_shortest_cycle(const ReferenceGraph *graph)
{
    int shortest = graph->vertex_count + 1;
    for (int first = 0; first < graph->vertex_count && shortest > 3; ++first) {
        // Each simple cycle can be found from its smallest vertex.
        search_simple_cycles(graph, first, first, UINT32_C(1) << first, 1, &shortest);
    }
    return shortest <= graph->vertex_count ? shortest : 0;
}

static void test_shortest_cycle(const Graph *graph, const ReferenceGraph *reference,
                                int expected_length, const char *context)
{
    CycleResult result = { 0 };
    CHECK(bfs_shortest_cycle(graph, &result), context);
    validate_result(reference, &result, expected_length, context);
    cycle_result_destroy(&result);
}

static void test_fixture(const char *context, int vertex_count, const Edge *edges,
                         size_t edge_count, int expected_length)
{
    CHECK(vertex_count >= 0 && vertex_count <= REFERENCE_CAPACITY, context);
    Graph graph = { 0 };
    ReferenceGraph reference = { .vertex_count = vertex_count };
    add_vertices(&graph, vertex_count, context);
    for (size_t index = 0; index < edge_count; ++index) {
        add_edge(&graph, &reference, edges[index], context);
    }

    CHECK(graph.edge_count == (int)edge_count, context);
    CHECK(brute_force_shortest_cycle(&reference) == expected_length, context);
    test_shortest_cycle(&graph, &reference, expected_length, context);
    graph_destroy(&graph);
}

static void test_named_graphs(void)
{
    const Edge edge[] = { {0, 1} };
    const Edge triangle[] = { {0, 1}, {1, 2}, {2, 0} };
    const Edge square[] = { {0, 1}, {1, 2}, {2, 3}, {3, 0} };
    const Edge tree[] = { {0, 1}, {1, 2}, {1, 3}, {3, 4}, {3, 5} };
    const Edge multiple_cycles[] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0}, {2, 4}, {4, 5}, {5, 3}
    };
    const Edge disconnected_cycle[] = { {0, 1}, {1, 2}, {3, 4}, {4, 5}, {5, 3} };
    const Edge disconnected_forest[] = { {0, 1}, {1, 2}, {3, 4} };
    const Edge triangle_and_square[] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0}, {2, 4}, {4, 5}, {5, 2}
    };

    test_fixture("empty graph", 0, NULL, 0, 0);
    test_fixture("single vertex", 1, NULL, 0, 0);
    test_fixture("isolated vertices", 5, NULL, 0, 0);
    test_fixture("single edge", 2, edge, COUNT(edge), 0);
    test_fixture("triangle", 3, triangle, COUNT(triangle), 3);
    test_fixture("square", 4, square, COUNT(square), 4);
    test_fixture("tree", 6, tree, COUNT(tree), 0);
    test_fixture("multiple cycles tied at length four", 6, multiple_cycles,
                 COUNT(multiple_cycles), 4);
    test_fixture("disconnected graph with a cycle", 7, disconnected_cycle,
                 COUNT(disconnected_cycle), 3);
    test_fixture("disconnected forest", 6, disconnected_forest, COUNT(disconnected_forest), 0);
    test_fixture("triangle and square", 6, triangle_and_square, COUNT(triangle_and_square), 3);
}

static void test_graph_editing(void)
{
    const char *context = "graph editing and compaction";
    Graph graph = { 0 };
    add_vertices(&graph, 35, context);
    for (int vertex = 0; vertex < graph.vertex_count; ++vertex) {
        if (vertex != 1) {
            CHECK(graph_add_edge(&graph, 1, vertex), context);
        }
    }
    CHECK(graph_has_edge(&graph, 0, 1) && graph_has_edge(&graph, 1, 0), context);
    CHECK(!graph_add_edge(&graph, 0, 1), context);
    CHECK(!graph_add_edge(&graph, 2, 2), context);
    CHECK(!graph_add_edge(&graph, -1, 0), context);
    CHECK(!graph_add_edge(&graph, 1, 35), context);
    CHECK(graph.edge_count == 34, context);

    CHECK(graph_add_edge(&graph, 0, 34), context);
    CHECK(graph_remove_edge(&graph, 34, 1), context);
    CHECK(!graph_has_edge(&graph, 1, 34) && !graph_has_edge(&graph, 34, 1), context);
    CHECK(!graph_remove_edge(&graph, 1, 34), context);
    CHECK(!graph_remove_vertex(&graph, 35), context);
    CHECK(graph_remove_vertex(&graph, 1), context);
    CHECK(graph.vertex_count == 34 && graph.edge_count == 1, context);
    CHECK(graph_has_edge(&graph, 0, 33) && graph_has_edge(&graph, 33, 0), context);

    graph_clear(&graph);
    CHECK(graph.vertex_count == 0 && graph.edge_count == 0, context);
    add_vertices(&graph, 2, context);
    CHECK(graph_add_edge(&graph, 0, 1), context);
    graph_destroy(&graph);
}

static void validate_dequeue(const BFSState *state, const char *context)
{
    CHECK(state->current >= 0 && state->current < state->graph->vertex_count, context);
    CHECK(state->queue.head > 0 && state->queue.items[state->queue.head - 1] == state->current,
          context);
    CHECK(state->distance[state->current] >= 0, context);
}

static void validate_root(const BFSState *state, int previous_roots, const char *context)
{
    CHECK(state->root == previous_roots, context);
    CHECK(state->queue.head == 0 && state->queue.tail == 1, context);
    CHECK(state->queue.items[0] == state->root, context);
    for (int vertex = 0; vertex < state->graph->vertex_count; ++vertex) {
        int expected_distance = vertex == state->root ? 0 : -1;
        CHECK(state->distance[vertex] == expected_distance && state->parent[vertex] == -1,
              context);
    }
}

static void test_stepper(const char *context, int vertex_count, const Edge *edges,
                         size_t edge_count, int first_root_best)
{
    CHECK(vertex_count > 0 && vertex_count <= REFERENCE_CAPACITY, context);
    Graph graph = { 0 };
    ReferenceGraph reference = { .vertex_count = vertex_count };
    add_vertices(&graph, vertex_count, context);
    for (size_t index = 0; index < edge_count; ++index) {
        add_edge(&graph, &reference, edges[index], context);
    }

    BFSState state = { 0 };
    CHECK(bfs_init(&state, &graph), context);
    int root_count = 0;
    int previous_best = vertex_count + 1;
    int dequeued = -1;
    int examined = -1;
    bool examined_edges[REFERENCE_CAPACITY][REFERENCE_CAPACITY] = { { false } };
    size_t max_steps = (size_t)vertex_count * ((size_t)vertex_count + edge_count * 8 + 5);
    for (size_t step = 0; !state.finished; ++step) {
        CHECK(step < max_steps, context);
        BFSEvent previous_event = state.event;
        BFSEvent event = bfs_step(&state);
        CHECK(state.event == event, context);
        CHECK(state.queue.head >= 0 && state.queue.head <= state.queue.tail, context);
        CHECK(state.queue.tail <= vertex_count, context);

        switch (event) {
        case BFS_ROOT:
            validate_root(&state, root_count, context);
            dequeued = -1;
            examined = -1;
            memset(examined_edges, 0, sizeof examined_edges);
            break;
        case BFS_DEQUEUE:
            validate_dequeue(&state, context);
            dequeued = state.current;
            examined = -1;
            break;
        case BFS_EXAMINE:
            CHECK(dequeued >= 0 && state.current == dequeued, context);
            CHECK(state.neighbor >= 0 && state.neighbor < vertex_count, context);
            CHECK(reference.adjacent[dequeued][state.neighbor], context);
            examined = state.neighbor;
            CHECK(!examined_edges[dequeued][examined], context);
            examined_edges[dequeued][examined] = true;
            break;
        case BFS_DISCOVER:
            CHECK(dequeued >= 0 && examined >= 0, context);
            CHECK(state.current == dequeued && state.neighbor == examined, context);
            CHECK(state.parent[examined] == dequeued, context);
            CHECK(state.distance[examined] == state.distance[dequeued] + 1, context);
            CHECK(state.queue.tail > 0 && state.queue.items[state.queue.tail - 1] == examined,
                  context);
            break;
        case BFS_CANDIDATE:
            CHECK(dequeued >= 0 && examined >= 0, context);
            CHECK(state.current == dequeued && state.neighbor == examined, context);
            CHECK(state.distance[dequeued] >= 0 && state.distance[examined] >= 0, context);
            CHECK(state.parent[dequeued] != examined && state.parent[examined] != dequeued,
                  context);
            CHECK(state.candidate_score == state.distance[dequeued] + state.distance[examined] + 1,
                  context);
            break;
        case BFS_IMPROVE:
            CHECK(previous_event == BFS_CANDIDATE, context);
            CHECK(state.best.length == state.candidate_length, context);
            CHECK(state.best.length <= state.candidate_score, context);
            CHECK(state.best.length >= 3 && state.best.length < previous_best, context);
            validate_result(&reference, &state.best, state.best.length, context);
            previous_best = state.best.length;
            break;
        case BFS_END_ROOT:
            CHECK(state.completed_roots == root_count + 1, context);
            CHECK(state.queue.head == state.queue.tail, context);
            for (int from = 0; from < vertex_count; ++from) {
                for (int to = 0; to < REFERENCE_CAPACITY; ++to) {
                    bool expected = state.distance[from] >= 0 && reference.adjacent[from][to];
                    CHECK(examined_edges[from][to] == expected, context);
                }
            }
            if (root_count == 0) {
                validate_result(&reference, &state.best, first_root_best, context);
            }
            ++root_count;
            break;
        case BFS_FINISH:
            CHECK(state.finished && root_count == vertex_count, context);
            break;
        case BFS_IDLE:
            CHECK(false, context);
            break;
        }
    }

    CHECK(root_count == vertex_count && previous_best == 3, context);
    validate_result(&reference, &state.best, 3, context);
    test_shortest_cycle(&graph, &reference, 3, context);
    bfs_destroy(&state);
    graph_destroy(&graph);
}

static void test_exhaustive_tiny_graphs(void)
{
    unsigned checked = 0;
    for (int count = 0; count <= EXHAUSTIVE_LIMIT; ++count) {
        int possible_edges = count * (count - 1) / 2;
        for (uint32_t mask = 0; mask < (UINT32_C(1) << possible_edges); ++mask) {
            char context[80];
            snprintf(context, sizeof(context), "exhaustive graph with %d vertices, mask %lu",
                     count, (unsigned long)mask);
            Graph graph = { 0 };
            ReferenceGraph reference = { .vertex_count = count };
            add_vertices(&graph, count, context);

            uint32_t edge_bit = 1;
            for (int from = 0; from < count; ++from) {
                for (int to = from + 1; to < count; ++to) {
                    if ((mask & edge_bit) != 0) {
                        add_edge(&graph, &reference, (Edge){ from, to }, context);
                    }
                    edge_bit <<= 1;
                }
            }

            test_shortest_cycle(&graph, &reference, brute_force_shortest_cycle(&reference), context);
            graph_destroy(&graph);
            ++checked;
        }
    }
    printf("Compared BFS with brute force on %u simple graphs.\n", checked);
}

int main(void)
{
    const Edge later_root[] = {
        {0, 1}, {0, 2}, {0, 3}, {1, 4}, {2, 5}, {3, 6}, {4, 5}, {5, 6}, {6, 4}
    };
    const Edge shared_parent_path[] = { {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 2} };

    test_graph_editing();
    test_named_graphs();
    test_stepper("a later BFS root finds the shortest cycle", 7, later_root,
                 COUNT(later_root), 5);
    test_stepper("the LCA trims the shared parent path", 5, shared_parent_path,
                 COUNT(shared_parent_path), 3);
    test_exhaustive_tiny_graphs();
    puts("All shortest-cycle tests passed.");
    return EXIT_SUCCESS;
}
