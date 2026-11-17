This folder contains C++ projects that are build using Emscripten to create WASM libraries for:

- flow runtime engine for the Dashboard and EEZ-GUI projects (eez-runtime)
- flow runtime engine for the LVGL (lvgl-runtime, for varius versions)
- LZ4 compression used by the Studio

How to add submodules?

- EEZ-Framework
    - We always use master commit.
    - git submodule add https://github.com/eez-open/eez-framework eez-framework

- LVGL
    - git submodule add https://github.com/lvgl/lvgl lvgl-runtime/v8.4.0/lvgl
    - Then checkout desired commit

## FreeType for the LVGL runtimes

The five LVGL builds use `lvgl-runtime/common/freetype.cmake` and its local
Emscripten port recipe. This preserves the existing FreeType **2.14.1** version
without a developer-specific absolute library/include path. The official
`freetype-2.14.1.tar.gz` source archive is checked against the SHA-512 in the
recipe. The archive's published SHA-256 is
`174d9e53402e1bf9ec7277e22ec199ba3e55a6be2c0740cb18c0ee9850fc8c34`.

The recipe uses a separate versioned library/header cache namespace and the
full FreeType module set and zlib configuration of Emscripten's port. Do not
replace it with `-sUSE_FREETYPE=1`: Emscripten 4.0.21's built-in port provides
2.13.3. The EEZ-GUI runtime keeps its own existing vendored dependency.

To check both the selected headers and the actual linked library, using the
same Emscripten/configuration as the runtime builds:

```powershell
emcc --use-port=./lvgl-runtime/common/eez-freetype-2.14.1.py tests/freetype-version.c -o freetype-version.js
node freetype-version.js
```

The probe must print `FreeType 2.14.1` and exit successfully. Run/Debug tests of
generated preview assets remain separate from this dependency-version check.
