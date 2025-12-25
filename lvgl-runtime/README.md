How to build?

```
source ~/emsdk/emsdk_env.sh
export EMSCRIPTEN=/home/mvladic/emsdk/upstream/emscripten

mkdir -p wasm/lvgl-runtime/v8.3/build
cd wasm/lvgl-runtime/v8.3/build
emcmake cmake ..
emmake make -j4

mkdir -p wasm/lvgl-runtime/v9.0/build
cd wasm/lvgl-runtime/v9.0/build
emcmake cmake ..
emmake make -j4
```

git submodule add -b v9.3.0 https://github.com/lvgl/lvgl wasm/lvgl-runtime/v9.3.0/lvgl