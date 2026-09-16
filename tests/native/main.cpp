#include "eez-flow.h"
extern "C" {
native_var_t native_vars[] = { {} };
void create_screens() {}
void tick_screen(int) {}
}
extern "C" int eez_test_lvgl_actions();

#if LVGL_VERSION_MAJOR >= 9
static void flush(lv_display_t *display, const lv_area_t *, uint8_t *) { lv_display_flush_ready(display); }
#else
static void flush(lv_disp_drv_t *display, const lv_area_t *, lv_color_t *) { lv_disp_flush_ready(display); }
#endif
int main() {
    lv_init();
#if LVGL_VERSION_MAJOR >= 9
    auto display = lv_display_create(320, 240);
    static uint32_t pixels[320 * 20];
    lv_display_set_buffers(display, pixels, nullptr, sizeof(pixels), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);
#else
    static lv_color_t pixels[320 * 20];
    static lv_disp_draw_buf_t buffer;
    lv_disp_draw_buf_init(&buffer, pixels, nullptr, 320 * 20);
    static lv_disp_drv_t driver;
    lv_disp_drv_init(&driver);
    driver.hor_res = 320;
    driver.ver_res = 240;
    driver.draw_buf = &buffer;
    driver.flush_cb = flush;
    lv_disp_drv_register(&driver);
#endif
    int result = eez_test_lvgl_actions();
#if LVGL_VERSION_MAJOR >= 9
    lv_deinit();
#else
    lv_disp_remove(lv_disp_get_default());
#endif
    return result;
}
