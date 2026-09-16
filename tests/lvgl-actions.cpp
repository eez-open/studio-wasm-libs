// End-to-end tests: real Flow bytecode, real action dispatch, real LVGL objects.
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif
#include <stdio.h>
#include <string.h>
#include <initializer_list>
#ifdef EEZ_TEST_AMALGAMATION
#include "eez-flow.h"
#else
#include <eez/flow/components.h>
#include <eez/flow/components/lvgl.h>
#include <eez/flow/hooks.h>
#include <eez/flow/lvgl_api.h>
#include <eez/core/alloc.h>
#endif

namespace eez { namespace flow { void executeLVGLApiComponent(FlowState *, unsigned); } }
extern "C" int eez_test_button_matrix_memory(void);

using namespace eez;
using namespace eez::flow;

namespace {
// ListOfAssetsPtr is a count followed by a relative pointer, as in built assets.
template<class T, size_t N>
void bindList(ListOfAssetsPtr<T> &list, AssetsPtr<T> (&items)[N], uint32_t count = N) {
    static_assert(sizeof(list) == 8, "Asset ABI changed");
    list.count = count;
    auto offset = (int32_t)((uint8_t *)items - ((uint8_t *)&list + 4));
    memcpy((uint8_t *)&list + 4, &offset, sizeof(offset));
}

struct Fixture {
    Assets assets{};
    FlowDefinition definition{};
    Flow flow{};
    LVGLApiComponent component{};
    LVGLApiComponent_ActionType action{};
    FlowState state{};
    AssetsPtr<Component> components[1];
    AssetsPtr<LVGLApiComponent_ActionType> actions[1];
    AssetsPtr<Property> properties[17];
    AssetsPtr<Value> constants[17];
    uint16_t instructions[17][2];
    Value values[17];
    Value result;
    ComponenentExecutionState *executionStates[1]{};

    Fixture() {
        assets.flowDefinition = &definition;
        bindList(definition.constants, constants);
        bindList(flow.components, components);
        components[0] = &component;
        component.type = 1044;
        component.errorCatchOutput = -1;
        bindList(component.actions, actions);
        actions[0] = &action;
        bindList(action.properties, properties, 2);
        for (int i = 0; i < 17; i++) {
            constants[i] = &values[i];
            properties[i] = (Property *)instructions[i];
            instructions[i][0] = EXPR_EVAL_INSTRUCTION_TYPE_PUSH_CONSTANT | i;
            instructions[i][1] = EXPR_EVAL_INSTRUCTION_TYPE_END;
        }
        state.assets = &assets;
        state.flow = &flow;
        state.values = &result;
        state.componenentExecutionStates = executionStates;
    }

    void run(uint32_t id, lv_obj_t *obj, Value argument = Value(), uint32_t count = 2) {
        action.action = id;
        action.properties.count = count;
        values[0] = Value(obj, VALUE_TYPE_WIDGET);
        values[1] = argument;
        instructions[1][0] = (id == 65 || id == 71)
            ? EXPR_EVAL_INSTRUCTION_TYPE_PUSH_LOCAL_VAR
            : EXPR_EVAL_INSTRUCTION_TYPE_PUSH_CONSTANT | 1;
        instructions[2][0] = id == 73
            ? EXPR_EVAL_INSTRUCTION_TYPE_PUSH_LOCAL_VAR
            : EXPR_EVAL_INSTRUCTION_TYPE_PUSH_CONSTANT | 2;
        executeLVGLApiComponent(&state, 0);
    }
};
int errors;
void errorHook() { errors++; }
#define CHECK(condition) do { if (!(condition)) { printf("FAIL line %d: %s\n", __LINE__, #condition); return __LINE__; } } while (0)
Value stringValue(const char *str) { return Value(str, VALUE_TYPE_STRING, 0); }
}

extern "C" EMSCRIPTEN_KEEPALIVE int eez_test_lvgl_actions() {
    auto previousStopHook = stopScriptHook;
    stopScriptHook = errorHook;
    Fixture fixture;
    auto screen = lv_obj_create(NULL);
    auto textarea = lv_textarea_create(screen);
#if LVGL_VERSION_MAJOR >= 9
    auto matrix = lv_buttonmatrix_create(screen);
#define GET_TEXT lv_buttonmatrix_get_button_text
#define SET_SELECTED lv_buttonmatrix_set_selected_button
#define HAS_CTRL lv_buttonmatrix_has_button_ctrl
#define CTRL_DISABLED LV_BUTTONMATRIX_CTRL_DISABLED
#define CTRL_CHECKED LV_BUTTONMATRIX_CTRL_CHECKED
#else
    auto matrix = lv_btnmatrix_create(screen);
#define GET_TEXT lv_btnmatrix_get_btn_text
#define SET_SELECTED lv_btnmatrix_set_selected_btn
#define HAS_CTRL lv_btnmatrix_has_btn_ctrl
#define CTRL_DISABLED LV_BTNMATRIX_CTRL_DISABLED
#define CTRL_CHECKED LV_BTNMATRIX_CTRL_CHECKED
#endif
    fixture.run(66, textarea, stringValue("hello 世界"));
    CHECK(!strcmp(lv_textarea_get_text(textarea), "hello 世界"));
    fixture.run(69, textarea, stringValue("*"));
    CHECK(!strcmp(lv_textarea_get_password_bullet(textarea), "*"));
    fixture.run(68, textarea, Value(true, VALUE_TYPE_BOOLEAN));
    CHECK(lv_textarea_get_password_mode(textarea));
    lv_obj_set_style_text_font(textarea, &lv_font_montserrat_20, LV_PART_MAIN);
    fixture.run(69, textarea, stringValue(LV_SYMBOL_OK));
    CHECK(!strcmp(lv_textarea_get_password_bullet(textarea), LV_SYMBOL_OK));
    fixture.run(69, textarea, stringValue("*"));
    fixture.run(65, textarea);
    CHECK(fixture.result.isString());
    CHECK(!strcmp(fixture.result.getString(), "hello 世界"));
    fixture.run(66, textarea, stringValue("changed"));
    CHECK(!strcmp(fixture.result.getString(), "hello 世界"));
    fixture.run(67, textarea, Value(true, VALUE_TYPE_BOOLEAN));
    CHECK(lv_textarea_get_one_line(textarea));
    fixture.run(67, textarea, Value(false, VALUE_TYPE_BOOLEAN));
    CHECK(!lv_textarea_get_one_line(textarea));
    fixture.run(70, textarea, stringValue("提示"));
    CHECK(!strcmp(lv_textarea_get_placeholder_text(textarea), "提示"));
    fixture.run(70, textarea, stringValue(""));
    CHECK(!strcmp(lv_textarea_get_placeholder_text(textarea), ""));
    // A conversion creates an owned temporary string. Its lifetime covers the setter.
    fixture.run(70, textarea, Value(12345, VALUE_TYPE_INT32));
    CHECK(!strcmp(lv_textarea_get_placeholder_text(textarea), "12345"));
    fixture.run(68, textarea, Value(false, VALUE_TYPE_BOOLEAN));
    CHECK(!lv_textarea_get_password_mode(textarea));

    // A default symbol binding must not overwrite a later literal Set Map.
    CHECK(eez_flow_set_buttonmatrix_text(matrix, 0, LV_SYMBOL_BACKSPACE));
    CHECK(!strcmp(GET_TEXT(matrix, 0), LV_SYMBOL_BACKSPACE));
    CHECK(!eez_flow_set_buttonmatrix_text(matrix, 999, "x"));
    CHECK(!eez_flow_set_buttonmatrix_text(matrix, 0, "\n"));
    fixture.values[2] = Value(2 | CTRL_DISABLED, VALUE_TYPE_INT32);
    fixture.values[3] = stringValue("\n");
    fixture.values[4] = Value(0, VALUE_TYPE_INT32);
    fixture.values[5] = stringValue(LV_SYMBOL_OK " OK");
    fixture.values[6] = Value(1 | CTRL_CHECKED, VALUE_TYPE_INT32);
    fixture.run(72, matrix, stringValue("first"), 7);
    CHECK(eez_flow_set_buttonmatrix_text(matrix, 0, LV_SYMBOL_BACKSPACE));
    CHECK(!strcmp(GET_TEXT(matrix, 0), "first"));
    CHECK(!strcmp(GET_TEXT(matrix, 1), LV_SYMBOL_OK " OK"));
    CHECK(HAS_CTRL(matrix, 0, CTRL_DISABLED));
    CHECK(HAS_CTRL(matrix, 1, CTRL_CHECKED));
    fixture.result = Value(0, VALUE_TYPE_INT32);
    SET_SELECTED(matrix, 0);
    fixture.run(71, matrix);
    CHECK(fixture.result.getInt() == 0);
    SET_SELECTED(matrix, 1);
    fixture.run(71, matrix);
    CHECK(fixture.result.getInt() == 1);
    SET_SELECTED(matrix, 0xffff);
    fixture.run(71, matrix);
    CHECK(fixture.result.getInt() == 65535);
    fixture.result = Value("", VALUE_TYPE_STRING);
    fixture.run(73, matrix, Value(0, VALUE_TYPE_INT32), 3);
    CHECK(fixture.result.isString());
    CHECK(!strcmp(fixture.result.getString(), "first"));
    fixture.run(73, matrix, Value(1, VALUE_TYPE_INT32), 3);
    CHECK(!strcmp(fixture.result.getString(), LV_SYMBOL_OK " OK"));
    Value buttonSnapshot = fixture.result;
    for (int id : {-1, 2, 65535, 65536, 2147483647}) {
        fixture.run(73, matrix, Value(id, VALUE_TYPE_INT32), 3);
        CHECK(fixture.result.isString());
        CHECK(!strcmp(fixture.result.getString(), ""));
    }
    for (int i = 0; i < 500; i++) {
        fixture.run(72, matrix, stringValue(i % 2 ? "A" : "B"), 7);
        CHECK(eez_flow_set_buttonmatrix_text(matrix, 2, "动态 " LV_SYMBOL_CLOSE));
        CHECK(!strcmp(GET_TEXT(matrix, 1), LV_SYMBOL_OK " OK"));
        CHECK(HAS_CTRL(matrix, 0, CTRL_DISABLED));
        CHECK(eez_flow_set_buttonmatrix_text(matrix, 2, ""));
        CHECK(!strcmp(GET_TEXT(matrix, 1), LV_SYMBOL_OK " OK"));
    }
    CHECK(!strcmp(buttonSnapshot.getString(), LV_SYMBOL_OK " OK"));
    fixture.run(73, matrix, Value(1, VALUE_TYPE_INT32), 3);
    CHECK(!strcmp(fixture.result.getString(), LV_SYMBOL_OK " OK"));
    CHECK(eez_flow_set_buttonmatrix_text(matrix, 2, "动态 " LV_SYMBOL_CLOSE));
    fixture.run(73, matrix, Value(1, VALUE_TYPE_INT32), 3);
    CHECK(!strcmp(fixture.result.getString(), LV_SYMBOL_OK " OK"));
    CHECK(eez_flow_set_buttonmatrix_text(matrix, 99, "x"));
    CHECK(eez_flow_set_buttonmatrix_text(matrix, 1, "x"));
    CHECK(eez_flow_set_buttonmatrix_text(matrix, 2, "\n"));
    errors = 0;
    fixture.run(0xffffffff, textarea);
    CHECK(errors == 1);
    fixture.run(65, textarea, Value(), 1);
    CHECK(errors == 2);
    fixture.run(66, matrix, stringValue("wrong type"));
    CHECK(errors == 3);
    fixture.run(72, matrix, stringValue("invalid"), 2);
    CHECK(errors == 4);
    fixture.run(73, matrix, Value(), 2);
    CHECK(errors == 5);
    fixture.run(73, textarea, Value(0, VALUE_TYPE_INT32), 3);
    CHECK(errors == 6);
    CHECK(!strcmp(GET_TEXT(matrix, 0), "A"));
    fixture.run(72, matrix, Value(), 1);
    fixture.run(73, matrix, Value(0, VALUE_TYPE_INT32), 3);
    CHECK(!strcmp(fixture.result.getString(), ""));
    CHECK(eez_flow_set_buttonmatrix_map(matrix, (const char *const[]){"one"}, 1, NULL));
    fixture.values[2] = Value(1, VALUE_TYPE_INT32);
    fixture.run(72, matrix, Value(123, VALUE_TYPE_INT32), 3);
    CHECK(!strcmp(GET_TEXT(matrix, 0), "123"));
    fixture.result = Value();
    fixture.run(65, textarea);
    lv_obj_del(screen);
    CHECK(!strcmp(buttonSnapshot.getString(), LV_SYMBOL_OK " OK"));
    CHECK(!strcmp(fixture.result.getString(), "changed"));
    fixture.result = Value();
    buttonSnapshot = Value();
    stopScriptHook = previousStopHook;
    CHECK(eez_test_button_matrix_memory() == 0);
    printf("PASS: LVGL %d.%d.%d action dispatch, ownership and 500 map replacements\n", LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH);
    return 0;
}
