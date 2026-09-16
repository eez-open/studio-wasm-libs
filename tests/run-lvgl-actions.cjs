const assert = require("node:assert/strict");
const path = require("node:path");
const modulePath = path.resolve(process.argv[2]);
let wasm;
let timeout = setTimeout(() => { console.error("WASM initialization timed out"); process.exit(1); }, 30000);
wasm = require(modulePath)(message => {
    if (message.init) setImmediate(() => {
        clearTimeout(timeout);
        try {
            wasm._init(1, 0, 0, 0, 320, 240, false, 0, false);
            assert.equal(wasm._eez_test_lvgl_actions(), 0);
            process.exit(0);
        } catch (error) {
            console.error(error);
            process.exit(1);
        }
    });
});
