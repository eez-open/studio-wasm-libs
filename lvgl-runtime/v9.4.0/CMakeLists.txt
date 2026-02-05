cmake_minimum_required(VERSION 3.12)
project(lvgl_runtime_v9.4.0)

get_filename_component(EXPORTED_FUNCTIONS_FILE_PATH exported-functions.txt ABSOLUTE)

# release
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2 --no-entry")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L/home/mvladic/freetype-2.14.1/build -lfreetype -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=0 -s NODEJS_CATCH_EXIT=0 -s NODEJS_CATCH_REJECTION=0 -s INITIAL_MEMORY=83886080 -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_FUNCTIONS=@${EXPORTED_FUNCTIONS_FILE_PATH} -s EXPORTED_RUNTIME_METHODS=stringToNewUTF8,AsciiToString,UTF8ToString,HEAP8,HEAPU8,HEAP16,HEAPU16,HEAP32,HEAPU32,HEAPF32,HEAPF64,FS --pre-js ${PROJECT_SOURCE_DIR}/../common/pre.js --post-js ${PROJECT_SOURCE_DIR}/../common/post.js")

# debug:
# set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -Wunused-const-variable -Wno-nested-anon-types -Wno-dollar-in-identifier-extension -O2 --no-entry -g")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wunused-const-variable -Wno-nested-anon-types -Wno-dollar-in-identifier-extension -O2 --no-entry -g")
# # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=0 -s NODEJS_CATCH_EXIT=0 -s NODEJS_CATCH_REJECTION=0 -s INITIAL_MEMORY=83886080 -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_FUNCTIONS=${EXPORTED_FUNCTIONS} -s EXPORTED_RUNTIME_METHODS=stringToNewUTF8,AsciiToString,UTF8ToString,HEAP8,HEAPU8,HEAP16,HEAPU16,HEAP32,HEAPU32,HEAPF32,HEAPF64 --pre-js ${PROJECT_SOURCE_DIR}/../common/pre.js --post-js ${PROJECT_SOURCE_DIR}/../common/post.js")
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=0 -s NODEJS_CATCH_EXIT=0 -s NODEJS_CATCH_REJECTION=0 -s INITIAL_MEMORY=83886080 -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_FUNCTIONS=@${EXPORTED_FUNCTIONS_FILE_PATH} -s EXPORTED_RUNTIME_METHODS=stringToNewUTF8,AsciiToString,UTF8ToString,HEAP8,HEAPU8,HEAP16,HEAPU16,HEAP32,HEAPU32,HEAPF32,HEAPF64 --pre-js ${PROJECT_SOURCE_DIR}/../common/pre.js --post-js ${PROJECT_SOURCE_DIR}/../common/post.js")
# add_definitions(-DEMCC_DEBUG)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/../common)
include_directories(/home/mvladic/freetype-2.14.1/include)

set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)

# lvgl
add_subdirectory(lvgl)

# EEZ Framework
add_definitions(-DEEZ_FOR_LVGL)
add_subdirectory(../../eez-framework [EXCLUDE_FROM_ALL])

# lvgl_runtime
file(GLOB_RECURSE SOURCES
    ../common/*.c
    ../common/src/*.cpp
    ../common/src/*.c
)

add_executable(lvgl_runtime_v9.4.0 ${SOURCES})

set(CMAKE_EXECUTABLE_SUFFIX ".html")

target_link_libraries(lvgl_runtime_v9.4.0
    lvgl
    eez-framework
)

#
add_custom_command(TARGET lvgl_runtime_v9.4.0 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "${PROJECT_SOURCE_DIR}/build/lvgl_runtime_v9.4.0.js"
    "${PROJECT_SOURCE_DIR}/../../release/wasm")

add_custom_command(TARGET lvgl_runtime_v9.4.0 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "${PROJECT_SOURCE_DIR}/build/lvgl_runtime_v9.4.0.wasm"
    "${PROJECT_SOURCE_DIR}/../../release/wasm")
