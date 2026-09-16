/* Exercise the real C helper with real LVGL objects, intercepting only its
 * allocations/event registration so failure paths are deterministic. */
#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int allocations;
static int fail_after = -1;
static bool fail_event;

static void *test_alloc(size_t bytes) {
    if (fail_after == 0) return NULL;
    if (fail_after > 0) fail_after--;
#if LVGL_VERSION_MAJOR >= 9
    void *ptr = lv_malloc(bytes);
#else
    void *ptr = lv_mem_alloc(bytes);
#endif
    if (ptr) allocations++;
    return ptr;
}

static void test_free(void *ptr) {
    if (ptr) allocations--;
#if LVGL_VERSION_MAJOR >= 9
    lv_free(ptr);
#else
    lv_mem_free(ptr);
#endif
}

#if LVGL_VERSION_MAJOR >= 9
static lv_event_dsc_t *test_event(lv_obj_t *obj, lv_event_cb_t cb,
#else
static struct _lv_event_dsc_t *test_event(lv_obj_t *obj, lv_event_cb_t cb,
#endif
    lv_event_code_t filter, void *data) {
    return fail_event ? NULL : lv_obj_add_event_cb(obj, cb, filter, data);
}

#if LVGL_VERSION_MAJOR >= 9
#define lv_malloc test_alloc
#define lv_free test_free
#else
#define lv_mem_alloc test_alloc
#define lv_mem_free test_free
#endif
#define lv_obj_add_event_cb test_event
#ifdef EEZ_TEST_AMALGAMATION
#include "lvgl_button_matrix.h"
#else
#include <eez/flow/lvgl_button_matrix.h>
#endif
#undef lv_obj_add_event_cb
#undef lv_malloc
#undef lv_free
#undef lv_mem_alloc
#undef lv_mem_free

#define CHECK(condition) do { if (!(condition)) { printf("FAIL memory line %d: %s\n", __LINE__, #condition); return __LINE__; } } while (0)

int eez_test_button_matrix_memory(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
#if LVGL_VERSION_MAJOR >= 9
    lv_obj_t *matrix = lv_buttonmatrix_create(screen);
#else
    lv_obj_t *matrix = lv_btnmatrix_create(screen);
#endif
    const char *initial[] = { "initial", "\n", "static", NULL };
    const char *replacement[] = { "owned", "\n", "copy", NULL };
    EEZ_BM_SET_MAP(matrix, initial);
    allocations = 0;
    for (int fail = 0; fail < 2; fail++) {
        fail_after = fail;
        CHECK(!eez_bm_replace(matrix, replacement, 3, NULL, -1, NULL));
        CHECK(allocations == 0);
        CHECK(EEZ_BM_GET_MAP(matrix) == initial);
    }
    fail_after = -1;
    fail_event = true;
    CHECK(!eez_bm_replace(matrix, replacement, 3, NULL, -1, NULL));
    CHECK(allocations == 0);
    CHECK(EEZ_BM_GET_MAP(matrix) == initial);
    fail_event = false;
    CHECK(eez_bm_replace(matrix, replacement, 3, NULL, -1, NULL));
    CHECK(allocations == 2);
    CHECK(EEZ_BM_GET_MAP(matrix) != replacement);
    const char *const *owned_map = EEZ_BM_GET_MAP(matrix);
    fail_after = 0;
    CHECK(!eez_bm_set_text(matrix, 0, "new"));
    CHECK(EEZ_BM_GET_MAP(matrix) == owned_map);
    CHECK(allocations == 2);
    fail_after = -1;
    CHECK(eez_bm_set_text(matrix, 2, owned_map[0]));
    CHECK(!strcmp(EEZ_BM_GET_MAP(matrix)[2], "owned"));
    CHECK(allocations == 2);
    lv_obj_t *child = lv_obj_create(matrix);
    lv_obj_add_flag(child, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_del(child);
    CHECK(allocations == 2);
    CHECK(!strcmp(EEZ_BM_GET_MAP(matrix)[0], "owned"));
    CHECK(!eez_bm_replace(matrix, replacement, UINT16_MAX, NULL, -1, NULL));
    const char *invalid[] = { "" };
    CHECK(!eez_bm_replace(matrix, invalid, 1, NULL, -1, NULL));
    const uint32_t count = 9363; /* 9363 * 7 exceeds a uint16_t row divisor. */
    const char **large_map = malloc(count * sizeof(char *));
    eez_bm_ctrl_t *large_ctrl = malloc(count * sizeof(eez_bm_ctrl_t));
    CHECK(large_map && large_ctrl);
    for (uint32_t i = 0; i < count; i++) { large_map[i] = "x"; large_ctrl[i] = 7; }
    CHECK(!eez_bm_replace(matrix, large_map, count, large_ctrl, -1, NULL));
    free(large_ctrl);
    free(large_map);
    CHECK(allocations == 2);
    EEZ_BM_SET_MAP(matrix, initial); /* A caller may install another static map. */
    CHECK(eez_bm_set_text(matrix, 0, "after external map"));
    CHECK(allocations == 2);
    /* Failures must not detach live expressions, with or without prior state. */
    CHECK(eez_bm_update_bound_text(matrix, 0, LV_SYMBOL_BACKSPACE));
    fail_after = 0;
    CHECK(!eez_bm_set_map(matrix, replacement, 3, NULL));
    fail_after = -1;
    CHECK(eez_bm_update_bound_text(matrix, 0, "still bound"));
    CHECK(!strcmp(EEZ_BM_GET_MAP(matrix)[0], "still bound"));
    CHECK(eez_bm_set_map(matrix, replacement, 3, NULL));
    CHECK(eez_bm_update_bound_text(matrix, 0, LV_SYMBOL_BACKSPACE));
    CHECK(!strcmp(EEZ_BM_GET_MAP(matrix)[0], "owned"));
    CHECK(eez_bm_set_map(matrix, initial, 3, NULL));
    CHECK(eez_bm_update_bound_text(matrix, 2, "stale expression"));
    CHECK(!strcmp(EEZ_BM_GET_MAP(matrix)[2], "static"));
    lv_obj_del(matrix);
    CHECK(allocations == 0);
#if LVGL_VERSION_MAJOR >= 9
    matrix = lv_buttonmatrix_create(screen);
#else
    matrix = lv_btnmatrix_create(screen);
#endif
    EEZ_BM_SET_MAP(matrix, initial);
    fail_event = true;
    CHECK(!eez_bm_set_map(matrix, replacement, 3, NULL));
    fail_event = false;
    CHECK(eez_bm_update_bound_text(matrix, 0, "recreated binding"));
    CHECK(!strcmp(EEZ_BM_GET_MAP(matrix)[0], "recreated binding"));
    lv_obj_del(screen);
    CHECK(allocations == 0);
    printf("PASS: C map helper allocation/event failures, aliasing, limits and bubbled delete\n");
    return 0;
}
