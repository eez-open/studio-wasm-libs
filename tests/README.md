# LVGL regression tests

`scripts/build-lvgl-tests.sh` rebuilds the five supported runtimes and a separate
regression executable. The test executable invokes the **real Flow expression
evaluator and LVGL action dispatcher** with asset-layout fixtures; it is not
included in release binaries. CMake requires Emscripten 4.0.20 and a FreeType
2.14.1 library built for that toolchain (`FREETYPE_SOURCE_DIR`, `FREETYPE_LIBRARY`).
The GitHub workflow pins the toolchain, FreeType and all LVGL/framework commits.

Covered: Textarea setters/getter ownership, password mode, Unicode, empty
placeholder, temporary string conversion, ButtonMatrix selection/NONE, complete
map/control replacement, detached original bindings after Set Map, 500 replacements, deletion, invalid
action IDs, invalid parameter counts and wrong widget types.

The same test target also compiles the map helper as C and deterministically
injects allocation and event-registration failures into that helper. It verifies
unchanged maps on failure, aliasing, count/row-width bounds, a child's bubbled
DELETE event, an externally replaced static map, and balanced owned allocations.
LVGL's own allocation/assert behavior is unchanged.

Until the corresponding framework PR is merged, initialize its pinned commit
from the contributor's fork, as the CI workflow does:

```sh
git submodule set-url eez-framework https://github.com/AwangYes/eez-framework.git
git submodule update --init eez-framework
git submodule update --init lvgl-runtime/v8.4.0/lvgl # Repeat for each version
```

Release builds normalize repository paths in diagnostics. CI rebuilds every
engine using the pinned toolchain, tests action dispatch, and uploads the rebuilt
JS/WASM artifacts. Amalgamation generation uses the framework commit's timestamp via
`SOURCE_DATE_EPOCH` and UTC; its own CI job verifies regeneration is unchanged.

Native checks compile **the generated amalgamation**, not the framework source:

```sh
cmake -S tests/native -B /tmp/eez-native -G Ninja -DLVGL_VERSION=8.4.0
cmake --build /tmp/eez-native --target lvgl-native-tests --parallel 2
/tmp/eez-native/lvgl-native-tests
```

The native target enables AddressSanitizer and UndefinedBehaviorSanitizer. Repeat
with LVGL_VERSION=9.5.0. Release runtimes contain neither test exports nor sanitizer
instrumentation. Negative tests deliberately produce four Flow error messages.
