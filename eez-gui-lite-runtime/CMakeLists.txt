cmake_minimum_required(VERSION 3.10)
project(eez_gui_lite_runtime C)

set(CMAKE_C_STANDARD 99)

# Set C warnings to all
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -g)
endif()

add_definitions(-DEEZGUI_DEBUG=0)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Application sources
set(APP_SOURCES
    main.c
    eez-gui-lite.c
)

# Create executable
add_executable(eez_gui_lite_runtime
    ${APP_SOURCES}
)

# Platform-specific settings
if(EMSCRIPTEN)
    set_target_properties(eez_gui_lite_runtime PROPERTIES
        SUFFIX ".js"
        LINK_FLAGS "-s WASM=1 \
                    -s NODEJS_CATCH_EXIT=0 \
                    -s NODEJS_CATCH_REJECTION=0 \
                    -s ALLOW_MEMORY_GROWTH=1 \
                    -s INITIAL_MEMORY=8388608 \
                    -s LLD_REPORT_UNDEFINED \
                    -s ASSERTIONS=1 \
                    -s EXPORTED_FUNCTIONS=['_malloc','_free'] \
                    -s EXPORTED_RUNTIME_METHODS=['stringToNewUTF8','AsciiToString','UTF8ToString','HEAP8','HEAPU8','HEAP16','HEAPU16','HEAP32','HEAPU32','HEAPF32','HEAPF64'] \
                    --js-library ${PROJECT_SOURCE_DIR}/library.js \
                    --pre-js ${PROJECT_SOURCE_DIR}/pre.js \
                    --post-js ${PROJECT_SOURCE_DIR}/post.js \
                    -O0 \
                    -g \
                    -gsource-map \
                    --source-map-base ./"
    )
else()
    # Native build using SDL2 for display
    find_package(SDL2 REQUIRED)
    target_include_directories(eez_gui_lite_runtime PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(eez_gui_lite_runtime ${SDL2_LIBRARIES})
endif()

add_custom_command(TARGET eez_gui_lite_runtime POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "${PROJECT_SOURCE_DIR}/build/eez_gui_lite_runtime.js"
    "${PROJECT_SOURCE_DIR}/../release/wasm")

add_custom_command(TARGET eez_gui_lite_runtime POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "${PROJECT_SOURCE_DIR}/build/eez_gui_lite_runtime.wasm"
    "${PROJECT_SOURCE_DIR}/../release/wasm")

# Include local copy commands if the file exists (not tracked by git)
if(EXISTS "${PROJECT_SOURCE_DIR}/CMakeLists.local.cmake")
    include("${PROJECT_SOURCE_DIR}/CMakeLists.local.cmake")
endif()