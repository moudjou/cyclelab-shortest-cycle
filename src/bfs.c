#include "bfs.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void reset_tree(BFSState *state)
{
    for (int i = 0; i < state->graph->vertex_count; ++i) {
        state->distance[i] = -1;
        state->parent[i] = -1;
    }
}

bool bfs_init(BFSState *state, const Graph *graph)
{
    if (!state || !graph || graph->vertex_count < 0) return false;

    BFSState initialized = {
        .graph = graph,
        .root = -1,
        .current = -1,
        .neighbor = -1,
        .candidate_score = -1,
        .candidate_length = -1
    };
    const size_t count = (size_t)graph->vertex_count;
    if (count > 0) {
        if (count >= SIZE_MAX / sizeof(int)) return false;

        initialized.distance = malloc(count * sizeof *initialized.distance);
        initialized.parent = malloc(count * sizeof *initialized.parent);
        initialized.queue.items = malloc(count * sizeof *initialized.queue.items);
        initialized.candidate_path = malloc((count + 1) * sizeof *initialized.candidate_path);
        initialized.best_path = malloc((count + 1) * sizeof *initialized.best_path);
        initialized.path_seen = malloc(count * sizeof *initialized.path_seen);
        if (!initialized.distance || !initialized.parent || !initialized.queue.items ||
            !initialized.candidate_path || !initialized.best_path || !initialized.path_seen) {
            bfs_destroy(&initialized);
            return false;
        }
        reset_tree(&initialized);
    }

    *state = initialized;
    return true;
}

static bool valid_cycle(BFSState *state, int length)
{
    const int count = state->graph->vertex_count;
    const int *path = state->candidate_path;
    if (length < 3 || length > count || path[0] != path[length]) return false;

    memset(state->path_seen, 0, (size_t)count * sizeof *state->path_seen);
    for (int i = 0; i < length; ++i) {
        const int vertex = path[i];
        if (vertex < 0 || vertex >= count || state->path_seen[vertex] ||
            !graph_has_edge(state->graph, vertex, path[i + 1])) return false;

        state->path_seen[vertex] = 1;
    }
    return true;
}

static int reconstruct_candidate(BFSState *state)
{
    const int from = state->current;
    const int to = state->neighbor;
    int ancestor = from;
    int other = to;
    while (state->distance[ancestor] > state->distance[other]) {
        ancestor = state->parent[ancestor];
    }
    while (state->distance[other] > state->distance[ancestor]) {
        other = state->parent[other];
    }
    while (ancestor != other) {
        ancestor = state->parent[ancestor];
        other = state->parent[other];
    }

    /* Follow the tree from the LCA to from, cross to to, and return to the LCA. */
    const int depth = state->distance[ancestor];
    const int first_branch = state->distance[from] - depth;
    const int length = first_branch + (state->distance[to] - depth) + 1;
    int *path = state->candidate_path;
    int index = first_branch;
    for (int vertex = from; vertex != ancestor; vertex = state->parent[vertex]) {
        path[index--] = vertex;
    }
    path[0] = ancestor;

    index = first_branch + 1;
    for (int vertex = to; vertex != ancestor; vertex = state->parent[vertex]) {
        path[index++] = vertex;
    }
    path[length] = ancestor;
    return valid_cycle(state, length) ? length : -1;
}

static BFSEvent start_root(BFSState *state)
{
    if (state->completed_roots == state->graph->vertex_count) {
        state->finished = true;
        return BFS_FINISH;
    }

    reset_tree(state);
    state->root = state->completed_roots;
    state->distance[state->root] = 0;
    state->queue.head = 0;
    state->queue.tail = 1;
    state->queue.items[0] = state->root;
    return BFS_ROOT;
}

static BFSEvent advance_scan(BFSState *state)
{
    state->candidate_score = -1;
    state->candidate_length = -1;
    if (state->current >= 0) {
        const Vertex *vertex = &state->graph->vertices[state->current];
        if (state->next_edge < vertex->degree) {
            state->neighbor = vertex->neighbors[state->next_edge++];
            return BFS_EXAMINE;
        }
    }

    state->neighbor = -1;
    if (state->queue.head < state->queue.tail) {
        state->current = state->queue.items[state->queue.head++];
        state->next_edge = 0;
        return BFS_DEQUEUE;
    }

    state->current = -1;
    ++state->completed_roots;
    return BFS_END_ROOT;
}

static BFSEvent process_edge(BFSState *state)
{
    const int from = state->current;
    const int to = state->neighbor;
    if (state->distance[to] < 0) {
        state->distance[to] = state->distance[from] + 1;
        state->parent[to] = from;
        state->queue.items[state->queue.tail++] = to;
        return BFS_DISCOVER;
    }

    /* Both directions are examined, but only one represents each non-tree edge. */
    if (from < to && state->parent[from] != to && state->parent[to] != from) {
        const int64_t score = (int64_t)state->distance[from] + state->distance[to] + 1;
        state->candidate_score = score > INT_MAX ? INT_MAX : (int)score;
        /* The score bounds the actual cycle length from above. A root on a
         * shortest cycle guarantees an edge whose score equals its length. */
        if (state->best.length == 0 || score < state->best.length) {
            state->candidate_length = reconstruct_candidate(state);
        }
        return BFS_CANDIDATE;
    }

    return advance_scan(state);
}

BFSEvent bfs_step(BFSState *state)
{
    if (!state || !state->graph) return BFS_IDLE;

    BFSEvent next = state->event;
    switch (state->event) {
        case BFS_IDLE:
        case BFS_END_ROOT:
            next = start_root(state);
            break;
        case BFS_ROOT:
        case BFS_DEQUEUE:
        case BFS_DISCOVER:
        case BFS_IMPROVE:
            next = advance_scan(state);
            break;
        case BFS_EXAMINE:
            next = process_edge(state);
            break;
        case BFS_CANDIDATE:
            if (state->candidate_length > 0) {
                const size_t size = ((size_t)state->candidate_length + 1) * sizeof *state->best_path;
                memcpy(state->best_path, state->candidate_path, size);
                state->best = (CycleResult){state->best_path, state->candidate_length};
                next = BFS_IMPROVE;
            } else {
                next = advance_scan(state);
            }
            break;
        case BFS_FINISH:
            break;
    }
    state->event = next;
    return next;
}

void bfs_destroy(BFSState *state)
{
    if (!state) return;
    free(state->distance);
    free(state->parent);
    free(state->queue.items);
    free(state->candidate_path);
    free(state->best_path);
    free(state->path_seen);
    *state = (BFSState){0};
}

bool bfs_shortest_cycle(const Graph *graph, CycleResult *result)
{
    BFSState state = {0};
    if (!result || !bfs_init(&state, graph)) return false;

    while (bfs_step(&state) != BFS_FINISH) {
    }
    *result = state.best;
    if (result->vertices) state.best_path = NULL;
    bfs_destroy(&state);
    return true;
}

void cycle_result_destroy(CycleResult *result)
{
    if (!result) return;
    free(result->vertices);
    *result = (CycleResult){0};
}
