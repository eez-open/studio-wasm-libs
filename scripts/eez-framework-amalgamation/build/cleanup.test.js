"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const cleanup_1 = require("./cleanup");
// Test 1: Simple #if defined(MACRO) - known macro
const test1Input = `#if defined(EEZ_FOR_LVGL)
    // some code
#endif`;
const test1Expected = `    // some code`;
console.log("Test 1: Simple #if defined(MACRO) - known macro");
const test1Result = (0, cleanup_1.cleanupSourceFile)(test1Input, ["EEZ_FOR_LVGL"]);
console.log("Input:\n" + test1Input);
console.log("Expected:\n" + test1Expected);
console.log("Result:\n" + test1Result);
console.log("Pass:", test1Result === test1Expected);
console.log();
// Test 2: #ifdef with known macro
const test2Input = `#ifdef EEZ_FOR_LVGL
    // lvgl code
#endif`;
const test2Expected = `    // lvgl code`;
console.log("Test 2: #ifdef with known macro");
const test2Result = (0, cleanup_1.cleanupSourceFile)(test2Input, ["EEZ_FOR_LVGL"]);
console.log("Pass:", test2Result === test2Expected);
console.log();
// Test 3: #ifndef with known macro (should remove block)
const test3Input = `#ifndef EEZ_FOR_LVGL
    // non-lvgl code
#endif`;
const test3Expected = ``;
console.log("Test 3: #ifndef with known macro (should remove block)");
const test3Result = (0, cleanup_1.cleanupSourceFile)(test3Input, ["EEZ_FOR_LVGL"]);
console.log("Pass:", test3Result === test3Expected);
console.log();
// Test 4: #if with OR expression - one known true
const test4Input = `#if defined(EEZ_FOR_LVGL) || defined(OTHER)
    // some code
#endif`;
const test4Expected = `    // some code`;
console.log("Test 4: #if with OR expression - EEZ_FOR_LVGL is true, so whole expression is true");
const test4Result = (0, cleanup_1.cleanupSourceFile)(test4Input, ["EEZ_FOR_LVGL"]);
console.log("Pass:", test4Result === test4Expected);
console.log();
// Test 5: #if/#else with known macro
const test5Input = `#if defined(EEZ_FOR_LVGL)
    // lvgl code
#else
    // other code
#endif`;
const test5Expected = `    // lvgl code`;
console.log("Test 5: #if/#else with known macro");
const test5Result = (0, cleanup_1.cleanupSourceFile)(test5Input, ["EEZ_FOR_LVGL"]);
console.log("Pass:", test5Result === test5Expected);
console.log();
// Test 6: #if/#elif/#else with unknown first branch and known true second branch
// The else branch is removed since EEZ_FOR_LVGL is true (it will be taken if MACRO_A is false)
const test6Input = `#if defined(MACRO_A)
    // macro a code
#elif defined(EEZ_FOR_LVGL)
    // lvgl code
#else
    // other code
#endif`;
const test6Expected = `#if defined(MACRO_A)
    // macro a code
#else
    // lvgl code
#endif`;
console.log("Test 6: #if/#elif/#else - partial simplification (else removed, elif becomes else)");
const test6Result = (0, cleanup_1.cleanupSourceFile)(test6Input, ["EEZ_FOR_LVGL"]);
console.log("Result:\n" + test6Result);
console.log("Pass:", test6Result === test6Expected);
console.log();
// Test 7: Nested conditionals with inner unknown macro - outer simplified, inner kept
const test7Input = `#if defined(EEZ_FOR_LVGL)
    // outer start
    #ifdef INNER
        // inner code
    #endif
    // outer end
#endif`;
const test7Expected = `    // outer start
    #ifdef INNER
        // inner code
    #endif
    // outer end`;
console.log("Test 7: Nested conditionals - outer simplified, inner unknown kept");
const test7Result = (0, cleanup_1.cleanupSourceFile)(test7Input, ["EEZ_FOR_LVGL"]);
console.log("Result:\n" + test7Result);
console.log("Expected:\n" + test7Expected);
console.log("Pass:", test7Result === test7Expected);
console.log();
// Test 8: Unknown macros should be kept as-is
const test8Input = `#if defined(UNKNOWN_MACRO)
    // unknown code
#endif`;
console.log("Test 8: Unknown macros should be kept as-is");
const test8Result = (0, cleanup_1.cleanupSourceFile)(test8Input, ["EEZ_FOR_LVGL"]);
console.log("Pass:", test8Result === test8Input);
console.log();
// Test 9: Complex AND expression with unknown - should keep as-is
const test9Input = `#if defined(EEZ_FOR_LVGL) && defined(OTHER)
    // both defined
#endif`;
console.log("Test 9: Complex AND expression (OTHER unknown - keep as-is)");
const test9Result = (0, cleanup_1.cleanupSourceFile)(test9Input, ["EEZ_FOR_LVGL"]);
console.log("Result:\n" + test9Result);
console.log("Pass:", test9Result === test9Input);
console.log();
// Test 10: Negation of known macro
const test10Input = `#if !defined(EEZ_FOR_LVGL)
    // not lvgl
#endif`;
console.log("Test 10: Negation !defined of known macro - block removed");
const test10Result = (0, cleanup_1.cleanupSourceFile)(test10Input, ["EEZ_FOR_LVGL"]);
console.log("Pass:", test10Result === "");
console.log();
// Test 11: Multiple known defines - both present
const test11Input = `#if defined(EEZ_FOR_LVGL) && defined(EEZ_FOR_LVGL_LZ4_OPTION)
    // both defined
#endif`;
const test11Expected = `    // both defined`;
console.log("Test 11: Multiple known defines both present");
const test11Result = (0, cleanup_1.cleanupSourceFile)(test11Input, ["EEZ_FOR_LVGL", "EEZ_FOR_LVGL_LZ4_OPTION"]);
console.log("Pass:", test11Result === test11Expected);
console.log();
// Test 12: Preserve surrounding code
const test12Input = `// before
#if defined(EEZ_FOR_LVGL)
    // lvgl code
#endif
// after`;
const test12Expected = `// before
    // lvgl code
// after`;
console.log("Test 12: Preserve surrounding code");
const test12Result = (0, cleanup_1.cleanupSourceFile)(test12Input, ["EEZ_FOR_LVGL"]);
console.log("Pass:", test12Result === test12Expected);
console.log();
// Test 13: #ifdef with unknown macro - keep as-is
const test13Input = `#ifdef UNKNOWN
    // code
#endif`;
console.log("Test 13: #ifdef with unknown macro - keep as-is");
const test13Result = (0, cleanup_1.cleanupSourceFile)(test13Input, ["EEZ_FOR_LVGL"]);
console.log("Pass:", test13Result === test13Input);
console.log();
// Test 14: #ifndef with unknown macro - keep as-is
const test14Input = `#ifndef UNKNOWN
    // code
#endif`;
console.log("Test 14: #ifndef with unknown macro - keep as-is");
const test14Result = (0, cleanup_1.cleanupSourceFile)(test14Input, ["EEZ_FOR_LVGL"]);
console.log("Pass:", test14Result === test14Input);
console.log();
// Test 15: OR with known true - simplifies even with unknown
const test15Input = `#if defined(EEZ_FOR_LVGL) || defined(UNKNOWN)
    // code
#endif`;
const test15Expected = `    // code`;
console.log("Test 15: OR with known true - simplifies (true || unknown = true)");
const test15Result = (0, cleanup_1.cleanupSourceFile)(test15Input, ["EEZ_FOR_LVGL"]);
console.log("Pass:", test15Result === test15Expected);
console.log();
// Test 16: AND with known false - simplifies even with unknown
const test16Input = `#if defined(NOT_DEFINED_KNOWN) && defined(UNKNOWN)
    // code
#endif`;
console.log("Test 16: AND where first is not in defines, second unknown - keep as-is");
const test16Result = (0, cleanup_1.cleanupSourceFile)(test16Input, ["EEZ_FOR_LVGL"]);
// Since NOT_DEFINED_KNOWN is not in defines, it's unknown, so the whole thing is unknown
console.log("Result:\n" + test16Result);
console.log("Pass:", test16Result === test16Input);
console.log();
// Test 17: Numeric comparison with unknown macros - should keep as-is
const test17Input = `#if LVGL_VERSION_MAJOR > 9 || (LVGL_VERSION_MAJOR == 9 && LVGL_VERSION_MINOR > 1)
#include <lvgl/src/lvgl_private.h>
#endif`;
console.log("Test 17: Numeric comparison with unknown macros - keep as-is");
const test17Result = (0, cleanup_1.cleanupSourceFile)(test17Input, ["EEZ_FOR_LVGL"]);
console.log("Result:\n" + test17Result);
console.log("Pass:", test17Result === test17Input);
console.log();
// Test 18: MACRO=VALUE syntax - value is 0 (falsy)
const test18Input = `#if EEZ_OPTION_GUI
    // gui code
#endif`;
console.log("Test 18: MACRO=VALUE with value 0 - block removed");
const test18Result = (0, cleanup_1.cleanupSourceFile)(test18Input, ["EEZ_OPTION_GUI=0"]);
console.log("Result:\n" + test18Result);
console.log("Pass:", test18Result === "");
console.log();
// Test 19: MACRO=VALUE syntax - value is 1 (truthy)
const test19Input = `#if EEZ_OPTION_GUI
    // gui code
#endif`;
const test19Expected = `    // gui code`;
console.log("Test 19: MACRO=VALUE with value 1 - block kept");
const test19Result = (0, cleanup_1.cleanupSourceFile)(test19Input, ["EEZ_OPTION_GUI=1"]);
console.log("Result:\n" + test19Result);
console.log("Pass:", test19Result === test19Expected);
console.log();
// Test 20: MACRO=VALUE with numeric comparison
const test20Input = `#if EEZ_OPTION_GUI == 0
    // no gui
#else
    // with gui
#endif`;
const test20Expected = `    // no gui`;
console.log("Test 20: MACRO=VALUE with numeric comparison == 0");
const test20Result = (0, cleanup_1.cleanupSourceFile)(test20Input, ["EEZ_OPTION_GUI=0"]);
console.log("Result:\n" + test20Result);
console.log("Pass:", test20Result === test20Expected);
console.log();
// Test 21: MACRO=VALUE - defined() still works
const test21Input = `#if defined(EEZ_OPTION_GUI)
    // defined
#endif`;
const test21Expected = `    // defined`;
console.log("Test 21: MACRO=VALUE - defined() still returns true");
const test21Result = (0, cleanup_1.cleanupSourceFile)(test21Input, ["EEZ_OPTION_GUI=0"]);
console.log("Result:\n" + test21Result);
console.log("Pass:", test21Result === test21Expected);
console.log();
// Test 22: Mixed MACRO and MACRO=VALUE
const test22Input = `#if defined(EEZ_FOR_LVGL) && EEZ_OPTION_GUI
    // lvgl with gui
#endif`;
console.log("Test 22: Mixed MACRO and MACRO=VALUE (GUI=0) - block removed");
const test22Result = (0, cleanup_1.cleanupSourceFile)(test22Input, ["EEZ_FOR_LVGL", "EEZ_OPTION_GUI=0"]);
console.log("Result:\n" + test22Result);
console.log("Pass:", test22Result === "");
console.log();
// Test 23: Mixed MACRO and MACRO=VALUE - both truthy
const test23Input = `#if defined(EEZ_FOR_LVGL) && EEZ_OPTION_GUI
    // lvgl with gui
#endif`;
const test23Expected = `    // lvgl with gui`;
console.log("Test 23: Mixed MACRO and MACRO=VALUE (GUI=1) - block kept");
const test23Result = (0, cleanup_1.cleanupSourceFile)(test23Input, ["EEZ_FOR_LVGL", "EEZ_OPTION_GUI=1"]);
console.log("Result:\n" + test23Result);
console.log("Pass:", test23Result === test23Expected);
console.log();
// Test 24: notDefined - #ifdef with notDefined macro - block removed
const test24Input = `#ifdef EEZ_PLATFORM_STM32
    // stm32 code
#endif`;
console.log("Test 24: #ifdef with notDefined macro - block removed");
const test24Result = (0, cleanup_1.cleanupSourceFile)(test24Input, ["EEZ_FOR_LVGL"], ["EEZ_PLATFORM_STM32"]);
console.log("Result:\n" + test24Result);
console.log("Pass:", test24Result === "");
console.log();
// Test 25: notDefined - #ifndef with notDefined macro - block kept
const test25Input = `#ifndef EEZ_PLATFORM_STM32
    // non-stm32 code
#endif`;
const test25Expected = `    // non-stm32 code`;
console.log("Test 25: #ifndef with notDefined macro - block kept");
const test25Result = (0, cleanup_1.cleanupSourceFile)(test25Input, ["EEZ_FOR_LVGL"], ["EEZ_PLATFORM_STM32"]);
console.log("Result:\n" + test25Result);
console.log("Pass:", test25Result === test25Expected);
console.log();
// Test 26: notDefined - #if defined() with notDefined macro
const test26Input = `#if defined(EEZ_PLATFORM_STM32)
    // stm32 code
#else
    // other code
#endif`;
const test26Expected = `    // other code`;
console.log("Test 26: #if defined() with notDefined macro - else branch kept");
const test26Result = (0, cleanup_1.cleanupSourceFile)(test26Input, ["EEZ_FOR_LVGL"], ["EEZ_PLATFORM_STM32"]);
console.log("Result:\n" + test26Result);
console.log("Pass:", test26Result === test26Expected);
console.log();
// Test 27: notDefined - complex expression with notDefined
const test27Input = `#if defined(EEZ_FOR_LVGL) && !defined(EEZ_PLATFORM_STM32)
    // lvgl but not stm32
#endif`;
const test27Expected = `    // lvgl but not stm32`;
console.log("Test 27: Complex expression with defined and notDefined");
const test27Result = (0, cleanup_1.cleanupSourceFile)(test27Input, ["EEZ_FOR_LVGL"], ["EEZ_PLATFORM_STM32"]);
console.log("Result:\n" + test27Result);
console.log("Pass:", test27Result === test27Expected);
console.log();
// Test 28: notDefined - macro used directly (not with defined())
const test28Input = `#if EEZ_DASHBOARD_API
    // dashboard code
#endif`;
console.log("Test 28: notDefined macro used directly - block removed");
const test28Result = (0, cleanup_1.cleanupSourceFile)(test28Input, ["EEZ_FOR_LVGL"], ["EEZ_DASHBOARD_API"]);
console.log("Result:\n" + test28Result);
console.log("Pass:", test28Result === "");
console.log();
// Test 29: removeIncludePrefixes - remove #include <eez/...>
const test29Input = `#include <stdio.h>
#include <eez/core/util.h>
#include <eez/flow/flow.h>
#include <string.h>`;
const test29Expected = `#include <stdio.h>
#include <string.h>`;
console.log("Test 29: removeIncludePrefixes - remove #include <eez/...>");
const test29Result = (0, cleanup_1.cleanupSourceFile)(test29Input, [], [], ["eez/"]);
console.log("Result:\n" + test29Result);
console.log("Pass:", test29Result === test29Expected);
console.log();
// Test 30: removeIncludePrefixes - remove #include "eez/..."
const test30Input = `#include "myheader.h"
#include "eez/core/util.h"
#include "eez/flow/flow.h"
#include "other.h"`;
const test30Expected = `#include "myheader.h"
#include "other.h"`;
console.log("Test 30: removeIncludePrefixes - remove #include \"eez/...\"");
const test30Result = (0, cleanup_1.cleanupSourceFile)(test30Input, [], [], ["eez/"]);
console.log("Result:\n" + test30Result);
console.log("Pass:", test30Result === test30Expected);
console.log();
// Test 31: removeIncludePrefixes - multiple prefixes
const test31Input = `#include <stdio.h>
#include <eez/core/util.h>
#include <lvgl/lvgl.h>
#include "other/file.h"`;
const test31Expected = `#include <stdio.h>
#include "other/file.h"`;
console.log("Test 31: removeIncludePrefixes - multiple prefixes");
const test31Result = (0, cleanup_1.cleanupSourceFile)(test31Input, [], [], ["eez/", "lvgl/"]);
console.log("Result:\n" + test31Result);
console.log("Pass:", test31Result === test31Expected);
console.log();
// Test 32: removeIncludePrefixes combined with defines
const test32Input = `#include <eez/core/util.h>
#ifdef EEZ_FOR_LVGL
    // lvgl code
#endif`;
const test32Expected = `    // lvgl code`;
console.log("Test 32: removeIncludePrefixes combined with defines");
const test32Result = (0, cleanup_1.cleanupSourceFile)(test32Input, ["EEZ_FOR_LVGL"], [], ["eez/"]);
console.log("Result:\n" + test32Result);
console.log("Pass:", test32Result === test32Expected);
console.log();
// Test 33: Consecutive empty lines are collapsed to single empty line
const test33Input = `line1


line2



line3`;
const test33Expected = `line1

line2

line3`;
console.log("Test 33: Consecutive empty lines collapsed to single");
const test33Result = (0, cleanup_1.cleanupSourceFile)(test33Input, []);
console.log("Result:\n" + test33Result);
console.log("Pass:", test33Result === test33Expected);
console.log();
// Test 34: Leading empty lines removed
const test34Input = `

line1
line2`;
const test34Expected = `line1
line2`;
console.log("Test 34: Leading empty lines removed");
const test34Result = (0, cleanup_1.cleanupSourceFile)(test34Input, []);
console.log("Result:\n" + test34Result);
console.log("Pass:", test34Result === test34Expected);
console.log();
// Test 35: Trailing empty lines removed
const test35Input = `line1
line2

`;
const test35Expected = `line1
line2`;
console.log("Test 35: Trailing empty lines removed");
const test35Result = (0, cleanup_1.cleanupSourceFile)(test35Input, []);
console.log("Result:\n" + test35Result);
console.log("Pass:", test35Result === test35Expected);
console.log();
// Test 36: Empty lines after block removal are normalized
const test36Input = `before

#ifdef NOT_DEFINED
    // removed
#endif

after`;
const test36Expected = `before

after`;
console.log("Test 36: Empty lines after block removal normalized");
const test36Result = (0, cleanup_1.cleanupSourceFile)(test36Input, [], ["NOT_DEFINED"]);
console.log("Result:\n" + test36Result);
console.log("Pass:", test36Result === test36Expected);
console.log();
// Test 37: 
const test37Input = `#if defined(EEZ_PLATFORM_STM32)
    return HAL_GetTick();
#elif defined(__EMSCRIPTEN__)
    return (uint32_t)emscripten_get_now();
#elif defined(EEZ_PLATFORM_SIMULATOR)
    return osKernelGetTickCount();
#elif defined(EEZ_PLATFORM_ESP32)
    return (unsigned long) (esp_timer_get_time() / 1000ULL);
#elif defined(EEZ_PLATFORM_PICO)
    auto abs_time = get_absolute_time();
    return to_ms_since_boot(abs_time);
#elif defined(EEZ_PLATFORM_RASPBERRY)
    unsigned nStartTicks = CTimer::Get()->GetClockTicks();
    return nStartTicks / 1000;
#elif defined(EEZ_FOR_LVGL)
    return lv_tick_get();
#else
    #error "Missing millis implementation";
#endif`;
const test37Expected = `#if defined(__EMSCRIPTEN__)
    return (uint32_t)emscripten_get_now();
#else
    return lv_tick_get();
#endif`;
console.log("Test 37: complex");
const test37Result = (0, cleanup_1.cleanupSourceFile)(test37Input, 
// defines
[
    "EEZ_FOR_LVGL",
    "EEZ_OPTION_GUI=0",
    "OPTION_SCPI=0"
], 
// not defined
[
    "EEZ_PLATFORM_STM32",
    "EEZ_PLATFORM_ESP32",
    "EEZ_PLATFORM_PICO",
    "EEZ_PLATFORM_RASPBERRY",
    "EEZ_DASHBOARD_API",
    "EEZ_PLATFORM_SIMULATOR",
    "EEZ_PLATFORM_SIMULATOR_WIN32"
]);
console.log("Result:\n" + test37Result);
console.log("Pass:", test37Result === test37Expected);
console.log();
console.log("All tests completed!");
