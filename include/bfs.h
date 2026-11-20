#ifndef BFS_H
#define BFS_H

#include "graph.h"

typedef struct {
    int *vertices; /* Ordered zero-based indices, including the closing vertex. */
    int length;    /* Number of edges, or zero when no cycle exists. */
} CycleResult;

typedef struct {
    int *items;
    int head;
    int tail;
} BFSQueue;

typedef enum {
    BFS_IDLE,
    BFS_ROOT,
    BFS_DEQUEUE,
    BFS_EXAMINE,
    BFS_DISCOVER,
    BFS_CANDIDATE,
    BFS_IMPROVE,
    BFS_END_ROOT,
    BFS_FINISH
} BFSEvent;

typedef struct {
    const Graph *graph;
    BFSEvent event;
    int root;
    int current;
    int neighbor;
    int completed_roots;
    int *distance; /* A negative distance means the vertex is undiscovered. */
    int *parent;
    BFSQueue queue;
    CycleResult best;
    int candidate_score;  /* Walk length through the root, capped at INT_MAX. */
    int candidate_length; /* Actual validated length, or -1 if skipped. */
    bool finished;

    int next_edge;
    int *candidate_path;
    int *best_path;
    unsigned char *path_seen;
} BFSState;

/* Zero-initialize before use and destroy before reuse. Keep the graph unchanged
 * while stepping. The state owns its arrays and the best path it exposes. */
bool bfs_init(BFSState *state, const Graph *graph);
BFSEvent bfs_step(BFSState *state);
void bfs_destroy(BFSState *state);

/* On success result owns its path, if any. Destroy it before reuse. */
bool bfs_shortest_cycle(const Graph *graph, CycleResult *result);
void cycle_result_destroy(CycleResult *result);

#endif
