#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
: "${FREETYPE_SOURCE_DIR:?Set the FreeType source directory}"
: "${FREETYPE_LIBRARY:?Set the Emscripten FreeType static library}"
versions=("${@}")
if [ ${#versions[@]} -eq 0 ]; then versions=(8.4.0 9.2.2 9.3.0 9.4.0 9.5.0); fi
for version in "${versions[@]}"; do
    case "$version" in 8.4.0|9.2.2|9.3.0|9.4.0|9.5.0) ;; *) exit 2 ;; esac
    source_dir="lvgl-runtime/v${version}"
    emcmake cmake -S "$source_dir" -B "$source_dir/build" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release -DEEZ_BUILD_TESTS=ON \
        -DFREETYPE_SOURCE_DIR="$FREETYPE_SOURCE_DIR" -DFREETYPE_LIBRARY="$FREETYPE_LIBRARY"
    cmake --build "$source_dir/build" --target "lvgl_runtime_v${version}" "lvgl_runtime_v${version}_tests" --parallel "${BUILD_JOBS:-2}"
    node tests/run-lvgl-actions.cjs "$source_dir/build/lvgl_runtime_v${version}_tests.js"
done
