# CLAUDE.md — Road Editor 2

## Build

```bash
# Configure (once, or after adding new .cpp files)
cmake -S . -B build -G "MinGW Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_MAKE_PROGRAM="C:/qt/Tools/mingw1310_64/bin/mingw32-make.exe" \
  -DCMAKE_PREFIX_PATH="C:/qt/6.11.0/mingw_64"

# Build
cmake --build build --config Release -- -j4

# Run
build/bin/RoadEditor2.exe
# or double-click start.bat
```

CMakeLists.txt uses `GLOB_RECURSE` — re-run configure whenever new `.cpp` files are added.

## Coordinate system

- **World space**: Y-up, Z+ forward (toward viewer), X+ to the **left** of travel direction, **left-handed**
- **GL space**: X is flipped (`toGL(p) = {-p.x, p.y, p.z}`) so GLM/OpenGL right-handed math works correctly
- All model data (`RoadNetwork`, JSON) is stored in world space. Only the rendering layer applies the X-flip.

## Tech stack

| Layer | Choice |
|---|---|
| UI | Qt 6 QMainWindow + Dock widgets |
| 3D viewport | QOpenGLWidget, OpenGL 4.1 Core Profile |
| Language | C++17 |
| Build | CMake 3.20+, MinGW 13 |
| Math | glm 1.0.1 (FetchContent) |
| JSON | nlohmann/json v3.11.3 (FetchContent) |
| Qt path | `C:/qt/6.11.0/mingw_64` |

## Project structure

```
src/
  app/          MainWindow — dock layout, File > Open
  viewport/     Viewport3D (QOpenGLWidget), Camera (orbit), Grid
  renderer/     Shader, LineBatch, Mesh — thin GL wrappers
  model/        RoadNetwork, Road, Intersection, Serializer (JSON)
  generator/    RoadMeshGen — polyline → quad-strip surface mesh
  scene/        RoadRenderer — orchestrates LineBatch + Mesh draw calls
  editor/       EditorState — Selection struct + undo/redo deque
shaders/
  line.vert/frag   — colored lines (centerlines, grid, gizmos)
  road.vert/frag   — road surface mesh with Lambert shading
```

## Viewport controls

| Input | Action |
|---|---|
| Alt + LMB drag | Orbit |
| Alt + MMB drag | Pan |
| Alt + Scroll wheel | Zoom |
| LMB click | Select control point (orange cross) |
| LMB drag (on selected point) | Move control point on horizontal plane |
| Ctrl+Z | Undo |
| Ctrl+Y | Redo |

## Data format

JSON version 3. Top-level keys: `version`, `groups`, `intersections`, `roads`.
See `docs/road_data_format.json` for a full example.

## Pending phases

- **Phase 5**: UI panels — tool mode buttons, properties panel (speed/width/lanes), outliner tree
