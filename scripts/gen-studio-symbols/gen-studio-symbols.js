const fs = require("fs");
const lvgl_runtime_v840 = require('../../release/wasm/lvgl_runtime_v8.4.0.js');
const lvgl_runtime_v922 = require('../../release/wasm/lvgl_runtime_v9.2.2.js');
const lvgl_runtime_v930 = require('../../release/wasm/lvgl_runtime_v9.3.0.js');
const lvgl_runtime_v940 = require('../../release/wasm/lvgl_runtime_v9.4.0.js');
const lvgl_runtime_v950 = require('../../release/wasm/lvgl_runtime_v9.5.0.js');

const versions = [
    {
        name: "V840",
        version: "8.4.0",
        lvgl_runtime: lvgl_runtime_v840
    },
    {
        name: "V922",
        version: "9.2.2",
        lvgl_runtime: lvgl_runtime_v922
    },
    {
        name: "V930",
        version: "9.3.0",
        lvgl_runtime: lvgl_runtime_v930
    },
    {
        name: "V940",
        version: "9.4.0",
        lvgl_runtime: lvgl_runtime_v940
    },
    {
        name: "V950",
        version: "9.5.0",
        lvgl_runtime: lvgl_runtime_v950
    }
];

function UTF8ToString(wasm, ptr) {
    const view = new Uint8Array(wasm.HEAP8);
    let end = ptr;
    while (view[end] !== 0) end++;
    return new TextDecoder('utf-8').decode(view.subarray(ptr, end));
}

function getSymbols(Module) {
    return new Promise(resolve => {
        const wasm = Module(() => {
            resolve(UTF8ToString(wasm, wasm._getStudioSymbols()));
        });
    })
}

async function main() {
    for (const version of versions) {
        console.log(`Get v${version.name} symbols`);
        const symbolsStr = await getSymbols(version.lvgl_runtime);
        try {
            version.data = JSON.parse(symbolsStr);
            await fs.promises.writeFile(`./symbols${version.name}.json`, JSON.stringify(version.data, undefined, 4), "utf-8");
        } catch (err) {
            await fs.promises.writeFile(`./symbols${version.name}.json`, symbolsStr, "utf-8");
            throw err;
        }
    }

    // styles

    for (let i = 1; i < versions.length; i++) {
        if (versions[i].data.styles.length != versions[0].data.styles.length) {
            throw `version ${versions[i].name} has wrong number of styles`;
        }
    }

    let out = `export const LVGL_STYLE_PROP_CODES: {
    [key: string]: LVGLStylePropCode;
} = {
`;

    for (let i = 0; i < versions[0].data.styles.length; i++) {
        out += `    ${versions[0].data.styles[i].name}: { `
        for (let j = 0; j < versions.length; j++) {
            if (j > 0) {
                out += ", ";
            }
            out += `"${versions[j].version}": ${versions[j].data.styles[i].code == null ? "undefined" : versions[j].data.styles[i].code}`;
        }
        out += " },\n";
    }
    out += "};\n";

    // widget_flags
    for (let i = 0; i < versions.length; i++) {
        for (const widget of Object.keys(versions[i].data.widget_flags)) {
            for (let j = i + 1; j < versions.length; j++) {
                if (widget in versions[j].data.widget_flags && versions[i].data.widget_flags[widget] != versions[j].data.widget_flags[widget]) {
                    console.log(`warning: widget "${widget}" different between version ${versions[i].version} and ${versions[j].version}`);
                }
            }
        }
    }

    await fs.promises.writeFile(`lvgl-constants.ts`, out, "utf-8");
}

main();