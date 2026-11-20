# CycleLab

CycleLab is a C11 and raylib desktop visualizer for **Exercise 6 — The Shortest Cycle in an Undirected Graph**. It finds the shortest cycle in a simple undirected graph by running breadth-first search (BFS) from every vertex. The animation uses the same event stream as the standalone solver.

## Build and run on Windows 10 or 11

1. Install MSYS2. Open **MSYS2 UCRT64** from the Start menu and update it. [MSYS2 installer](https://www.msys2.org/)

   ```bash
   pacman -Syu
   ```

   If MSYS2 asks you to close the terminal, reopen **MSYS2 UCRT64** and run `pacman -Syu` again.

2. Install MinGW-w64 GCC, CMake, and Ninja. CMake 3.24 or newer is required. [Building with CMake in MSYS2](https://www.msys2.org/docs/cmake/)

   ```bash
   pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
   ```

3. In the same terminal, change to the folder containing `CMakeLists.txt`. Configure and build the project. The first configuration downloads the pinned raylib 5.5 source archive.

   ```bash
   cd '/c/path/to/ShortestCycleVisualizer'
   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
   cmake --build build --parallel
   ```

4. Launch the desktop application and run the automated tests from the project folder.

   ```bash
   ./build/shortest_cycle.exe
   ctest --test-dir build --output-on-failure
   ```

### PowerShell and Visual Studio Code

In VS Code, open the folder containing `CMakeLists.txt` and use either the UCRT64 steps above or the integrated PowerShell terminal. For the PowerShell commands below, MSYS2 users can install `mingw-w64-ucrt-x86_64-make` with `pacman -S --needed mingw-w64-ucrt-x86_64-make` in UCRT64. If `cmake`, `gcc`, and `mingw32-make` are already on your `PATH`, start with the `cmake` command. [MSYS2 MinGW make package](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-make)

```powershell
$env:Path = "C:\msys64\ucrt64\bin;$env:Path"
cmake -S . -B build-mingw -G 'MinGW Makefiles' -DCMAKE_BUILD_TYPE=Release
cmake --build build-mingw --parallel
ctest --test-dir build-mingw --output-on-failure
.\build-mingw\shortest_cycle.exe
```

### Build options

To use an existing raylib 5.5-compatible CMake installation, add `-DUSE_SYSTEM_RAYLIB=ON -DCMAKE_PREFIX_PATH=C:/path/to/raylib` to the configure command. Use a package built for the same compiler and architecture. To build the solver and tests without raylib or a network download, use a separate build directory.

```bash
cmake -S . -B build-core -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_VISUALIZER=OFF
cmake --build build-core --parallel
ctest --test-dir build-core --output-on-failure
```

## Use the visualizer

CycleLab opens with the Triangle example. It starts at 1440 × 900 and resizes down to 1100 × 700. The editor supports up to 64 vertices. The standalone solver uses dynamically allocated adjacency lists. On Windows the interface uses Segoe UI and Consolas where available, with raylib's built-in font as a fallback.

| Control | Action |
| --- | --- |
| Select and drag | Click a node or the middle of an edge to select it. Drag a node to rearrange the graph. |
| Add vertex | Click empty space on the canvas to place a vertex. |
| Add edge | Click two distinct vertices to connect them. Existing edges and self-loops are rejected. |
| Delete vertex / Delete edge | Choose a delete tool and click its target. `Delete` removes the current selection. |
| Clear graph | Empty the graph and reset the simulation. |
| Generate random | Enter a count from 1 to 64 and generate a simple undirected graph. Pressing `Enter` in the input field also generates it. |
| Load example graph | Open the example menu and choose a graph to replace the current one. |
| Run BFS / Play | Start the animation, resume a paused run, or start again after it finishes. `Space` toggles playback. |
| Next Step | Pause playback and advance one event. `N` is the shortcut. |
| Reset | Return the simulation to Ready. `R` is the shortcut. |
| Speed | Set the animation speed from 0.25× to 32×. |

`Esc` closes an open menu, leaves the vertex-count field, or returns to Select and drag mode. Reset unlocks editing during a running or paused simulation. Changing vertices or edges after a finished simulation clears its old result.

The inspector shows the active BFS root, the dequeued vertex, its level, the queue, and the current best cycle. Hover over an abbreviated queue or cycle to see its full order. The **Pseudocode** tab highlights the active instruction. **Vertices** lists the visited state, distance, and parent for every vertex, with mouse-wheel scrolling over the table. **Complexity** explains the time and space costs.

Gray nodes are unvisited, blue marks the current vertex, cyan marks discovered nodes, yellow marks the examined edge, violet marks tree edges, and green marks the shortest cycle. A cycle path repeats its first vertex at the end, as in `1 → 2 → 3 → 1`.

### Built-in examples

| Example | Vertices | Shortest cycle length |
| --- | ---: | ---: |
| Triangle | 3 | 3 |
| Square | 4 | 4 |
| Acyclic Graph | 5 | No cycle |
| Multiple Cycles | 6 | 3 |
| Disconnected Graph | 7 | 3 |
| No Edges | 5 | No cycle |
| Single Vertex | 1 | No cycle |

## How the solver works

For each starting vertex `s`, initialize the distances to `−1` and parents to `−1`. Give `s` distance `0` and enqueue it. Each time BFS dequeues `u`, it examines the neighbors of `u`. A newly discovered `v` gets parent `u` and distance `d[u] + 1`, then joins the queue. An edge between already discovered vertices is a cycle candidate when neither endpoint is the other’s parent.

For a candidate edge `(u, v)`, define its score as `d[u] + d[v] + 1`. The two parent paths meet at their lowest common ancestor `a`. Walking from `a` to one endpoint, crossing the candidate edge, and following the other branch back to `a` gives a simple cycle of length `d[u] + d[v] − 2d[a] + 1`. This length is at most the score. Cycle validation checks that the path closes, its vertices are distinct before the return to the start, and every consecutive vertex pair is an edge.

Let `g` be the length of a shortest cycle and choose a BFS root on that cycle. BFS distances are at most the distances obtained by following the cycle from the root. For adjacent vertices on a length-`g` cycle, these two cycle distances sum to at most `g − 1`. At least one edge of the cycle lies outside the BFS tree, so its score is at most `g`. Consequently, if the current best is longer than `g`, that edge will pass the reconstruction cutoff and yield a cycle of length `g`. If the search never sees a non-tree edge, the graph is acyclic. Starting from every vertex provides the same guarantee for disconnected graphs.

### Complexity Analysis

- **Time O(V × (V + E)).** One BFS initializes the vertex arrays and scans the reachable adjacency lists in O(V + E). There are V BFS runs. The solver reconstructs a candidate only when its score is smaller than the current best length, or when the first candidate appears. Each reconstruction strictly improves the best length, which stays in the range 3 through V. There are at most O(V) reconstructions. Following the parent paths takes O(V). Checking the reconstructed edges via adjacency lists costs at most O(V + E) because the cycle visits each vertex once before closing, so the sum of the scanned degrees is at most 2E.
- **Space O(V + E).** The adjacency lists store two entries per edge. The BFS queue, distances, parents, cycle path, and reconstruction/validation buffers take O(V) space. The GUI adds O(V) vertex positions and a fixed set of controls. It reads each event directly from the current BFS state without storing a replay log.

## Source and tests

| Files | Responsibility |
| --- | --- |
| `include/graph.h`, `src/graph.c` | Undirected adjacency lists and graph editing. |
| `include/bfs.h`, `src/bfs.c` | BFS stepper and independent `bfs_shortest_cycle` API. |
| `include/app.h`, `src/app.c`, `src/main.c` | Editing, examples, simulation controls, and window lifecycle. |
| `include/gui.h`, `src/gui.c` | Layout, drawing, graph styling, and educational panels. |
| `tests/test_bfs.c` | Known cases, playback state checks, path validation, and exhaustive comparison. |

The graph and solver headers have no raylib dependency. Call `bfs_shortest_cycle` with a graph and a zero-initialized `CycleResult`. On success, `length` is the number of edges or zero if the graph is acyclic. A nonempty `vertices` array holds `length + 1` zero-based indices in cycle order. Release it with `cycle_result_destroy`. The interface displays one-based vertex labels.

CTest runs the named edge cases, checks graph edits and BFS events, and compares the solver against an independent search that enumerates simple cycles. The exhaustive check covers all **33,868** labeled simple undirected graphs on zero through six vertices. Every returned path is checked against the reference graph for closure, valid edges, and distinct vertices.
