// Emscripten JS library — implements the extern C functions declared in main.c
//
// These functions are called from C and forwarded to callbacks registered
// by the JavaScript host (page-runtime.ts).

mergeInto(LibraryManager.library, {

    // ── Property getters ────────────────────────────────────────────────
    //
    // The host sets Module._jsGetStrProp, Module._jsGetBoolProp,
    // Module._jsGetIntProp before rendering starts.

    js_get_str_prop: function (prop) {
        if (Module._jsGetStrProp) {
            return Module._jsGetStrProp(prop);
        }
        return 0; // NULL
    },

    js_get_bool_prop: function (prop) {
        if (Module._jsGetBoolProp) {
            return Module._jsGetBoolProp(prop);
        }
        return 0; // false
    },

    js_get_int_prop: function (prop) {
        if (Module._jsGetIntProp) {
            return Module._jsGetIntProp(prop);
        }
        return 0;
    },

    // ── Event callback ──────────────────────────────────────────────────
    //
    // The host sets Module._jsOnEvent to receive widget events.

    js_on_event: function (widgetPtr, eventType) {
        if (Module._jsOnEvent) {
            Module._jsOnEvent(widgetPtr, eventType);
        }
    }
});
