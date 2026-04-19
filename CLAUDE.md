# CLAUDE.md — Road Editor 2

## Build

```bash
# Configure (once, or after adding new .cpp files)
cmake -S . -B build -G "MinGW Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_MAKE_PROGRAM="C:/qt/Tools/mingw1310_64/bin/mingw32-make.exe" \
  -DCMAKE_PREFIX_PATH="C:/qt/6.11.0/mingw_64"

# Build (prepend MinGW bin to PATH in PowerShell)
$env:PATH='C:\qt\Tools\mingw1310_64\bin;' + $env:PATH
cmake --build build

# Run
build/bin/RoadEditor2.exe
# or double-click start.bat
```

CMakeLists.txt uses `GLOB_RECURSE` — re-run configure whenever new `.cpp` files are added.
Without the PATH line above, `g++` may fail to launch `cc1plus.exe` correctly in this environment.

## Coordinate system

- **World space**: Y-up, Z+ forward (toward viewer), X+ to the **left** of travel direction, **left-handed**
- **GL space**: X is flipped (`toGL(p) = {-p.x, p.y, p.z}`) so GLM/OpenGL right-handed math works correctly
- All model data (`RoadNetwork`, JSON) is stored in world space. Only the rendering layer applies the X-flip.
- Transform gizmo works in GL space internally; drag deltas are converted back via `{-gl.x, gl.y, gl.z}`

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
  app/          MainWindow — dock layout, File > Open/Save, toolbar
                OutlinerPanel — road list tree
                PropertiesPanel — per-road lane/speed/mesh settings
  viewport/     Viewport3D (QOpenGLWidget), Camera (orbit), Grid, AxisGizmo
  renderer/     Shader, LineBatch, Mesh — thin GL wrappers
  model/        RoadNetwork, Road, Intersection, Serializer (JSON)
  generator/    ClothoidGen   — clothoid (Euler spiral) centerline builder
                RoadMeshGen   — centerline → quad-strip surface mesh
                IntersectionMeshGen — intersection fill mesh from connected roads
  scene/        RoadRenderer    — orchestrates all draw calls
                TransformGizmo  — XYZ + screen-plane drag handles
  editor/       EditorState — Selection struct + undo/redo deque
shaders/
  line.vert/frag   — colored lines (centerlines, grid, gizmos)
  road.vert/frag   — road surface mesh with Lambert shading
  point.vert/frag  — screen-space circular control point dots (GL_PROGRAM_POINT_SIZE)
```

## Viewport controls

| Input | Action |
|---|---|
| Alt + LMB drag | Orbit |
| Alt + MMB drag | Pan |
| Alt + Scroll wheel | Zoom |
| LMB click (Select mode) | Select road by edge proximity |
| LMB click (Edit mode) | Select control point or road |
| LMB drag on gizmo X/Y/Z arrow | Move control point along that axis |
| LMB drag on gizmo center square | Move control point on camera-parallel plane |
| LMB drag outside gizmo (Edit mode) | Move control point on horizontal Y-plane |
| Ctrl+Z | Undo |
| Ctrl+Y | Redo |

## Curve generation (`ClothoidGen`)

Roads use clothoid (Euler spiral) curves, not linear segments.

- `buildCenterline(pts, sampleInterval, equalMidpoint)` — main entry point
- `buildCenterlineDetailed(pts, sampleInterval, equalMidpoint)` — returns `CurvePt` with `SegKind` tag per point
- `SegKind`: `Straight` (white), `Clothoid` (orange), `Arc` (green) — used for centerline coloring
- Bend pivot (edge midpoint): proportional by default (`equalMidpoint=false`), or true midpoint (`true`)
- `kClothoidAngleRatio = 0.2f` — 20% of turning angle per transition section
- `resamplePolyline` / `buildAndResample` — uniform arc-length resampling

## Road data model (`Road` struct)

```cpp
// Lane flags (±1, ±2 numbering; negative=left, positive=right)
bool  useLaneLeft2, useLaneLeft1, useLaneCenter, useLaneRight1, useLaneRight2
float defaultWidthLaneLeft2/Left1/Center/Right1/Right2

// Mesh
float segmentLength   // tessellation interval along road (m), default 1.0
bool  equalMidpoint   // clothoid bend pivot: true=midpoint, false=proportional

// Intersection connections
string startIntersectionId, endIntersectionId
```

Lane mesh width = sum of active lane widths. One-sided road: enable only Left1 or Right1.

## Intersection geometry

- `Intersection.entryDist` — distance from intersection center at which roads are trimmed
- `RoadMeshGen::generate(..., net)` — trims road centerline start/end by `entryDist` when `net != nullptr`
- `IntersectionMeshGen::generate(ix, net, verts, indices)` — fan-triangulates fill mesh from connected road entry cross-sections
- `RoadRenderer` passes `&net` to both generators and draws intersection mesh with road shader (slightly different grey)

## Transform gizmo (`TransformGizmo`)

- Drawn at selected control point in Edit mode, depth-test disabled (always visible)
- Arrow length scales with camera distance (`kGizmoScale = 0.12`)
- `Axis::X` (red), `Axis::Y` (green), `Axis::Z` (blue), `Axis::Screen` (white square, camera-facing)
- Hit test: screen-space proximity to projected shaft lines; center square uses bounding radius
- Axis drag: ray–axis closest-point formula (`axisTParam`)
- Screen drag: ray–plane intersection where plane normal = `normalize(camPos - vertexPos)`
- X axis direction in GL space is `(1,0,0)` (not `-1`) — movement is in GL +X = world -X

## Properties panel

- **Lanes** group: per-lane checkbox + width spinbox for Left2/Left1/Center/Right1/Right2
- **Mesh** group: Segment Length (tessellation interval), Equal midpoint checkbox
- Apply button emits `roadModified(int, RoadProperties)` signal → `Viewport3D::applyRoadProperties`

## Data format

JSON version 3. Top-level keys: `version`, `groups`, `intersections`, `roads`.
New lane fields (`useLaneLeft1` etc.) have defaults so older JSON files load correctly.
See `docs/road_data_format.json` for a full example.
