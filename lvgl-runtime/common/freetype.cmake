# Keep the upstream FreeType version without a developer-specific library path.
# A separate port/library/header namespace avoids the SDK's older built-in port.
set(EEZ_FREETYPE_PORT "${CMAKE_CURRENT_LIST_DIR}/eez-freetype-2.14.1.py")
add_compile_options("--use-port=${EEZ_FREETYPE_PORT}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} \"--use-port=${EEZ_FREETYPE_PORT}\"")
