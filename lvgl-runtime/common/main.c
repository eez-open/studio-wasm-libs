#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <math.h>
#include <emscripten.h>

#include "lvgl/lvgl.h"

#include "src/flow.h"

#define EM_PORT_API(rettype) rettype EMSCRIPTEN_KEEPALIVE

#define EEZ_UNUSED(x) (void)(x)

int hor_res;
int ver_res;

uint32_t *display_fb;
bool display_fb_dirty;

#if LVGL_VERSION_MAJOR >= 9
void my_driver_flush(lv_display_t *disp_drv, const lv_area_t *area, uint8_t *px_map) {
#else
void my_driver_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p) {
#endif
    /*Return if the area is out the screen */
    if (area->x2 < 0 || area->y2 < 0 || area->x1 > hor_res - 1 || area->y1 > ver_res - 1) {
        lv_disp_flush_ready(disp_drv);
        return;
    }

    uint8_t *dst = (uint8_t *)&display_fb[area->y1 * hor_res + area->x1];
    uint32_t s = 4 * (hor_res - lv_area_get_width(area));
    for (int y = area->y1; y <= area->y2 && y < ver_res; y++) {
        for (int x = area->x1; x <= area->x2; x++) {
#if LVGL_VERSION_MAJOR >= 9
            uint8_t *src = px_map;
            px_map += 4;
#else
            uint8_t *src = (uint8_t *)color_p++;
#endif

            // bgr -> rgb
            *dst++ = src[2];
            *dst++ = src[1];
            *dst++ = src[0];
            *dst++ = src[3];
        }

        dst += s;
    }

    lv_disp_flush_ready(disp_drv);

    display_fb_dirty = true;
}

static int mouse_x = 0;
static int mouse_y = 0;
static int mouse_pressed = 0;

#define KEYBOARD_BUFFER_SIZE 1
static uint32_t keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static uint32_t keyboard_buffer_index = 0;
static bool keyboard_pressed = false;

#if LVGL_VERSION_MAJOR >= 9
void my_mouse_read(lv_indev_t * indev_drv, lv_indev_data_t * data) {
#else
void my_mouse_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data) {
#endif
    EEZ_UNUSED(indev_drv);

    /*Store the collected data*/
    data->point.x = (lv_coord_t)mouse_x;
    data->point.y = (lv_coord_t)mouse_y;
    data->state = mouse_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

#if LVGL_VERSION_MAJOR >= 9
void my_keyboard_read(lv_indev_t * indev_drv, lv_indev_data_t * data) {
#else
void my_keyboard_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data) {
#endif
    EEZ_UNUSED(indev_drv);

    if (keyboard_pressed) {
        /*Send a release manually*/
        keyboard_pressed = false;
        data->state = LV_INDEV_STATE_RELEASED;
    } else if (keyboard_buffer_index > 0) {
        /*Send the pressed character*/
        keyboard_pressed = true;
        data->state = LV_INDEV_STATE_PRESSED;
        data->key = keyboard_buffer[--keyboard_buffer_index];
    }


}

static int mouse_wheel_delta = 0;
static int mouse_wheel_pressed = 0;

#if LVGL_VERSION_MAJOR >= 9
void my_mousewheel_read(lv_indev_t * indev_drv, lv_indev_data_t * data) {
#else
void my_mousewheel_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data) {
#endif
    (void) indev_drv;      /*Unused*/

    data->state = mouse_wheel_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    data->enc_diff = (int16_t)mouse_wheel_delta;

    mouse_wheel_delta = 0;
}

////////////////////////////////////////////////////////////////////////////////
// memory based file system

uint16_t my_cache_size = 0;

#if LV_USE_USER_DATA
void *my_user_data = 0;
#endif

typedef struct {
    uint8_t *ptr;
    uint32_t pos;
} my_file_t;

#if LVGL_VERSION_MAJOR >= 9
#if LVGL_VERSION_MINOR >= 3
bool my_ready_cb(lv_fs_drv_t * drv) {
#else
bool my_ready_cb(struct lv_fs_drv_t * drv) {
#endif
#else
bool my_ready_cb(struct _lv_fs_drv_t * drv) {
#endif
    EEZ_UNUSED(drv);
    return true;
}

#if LVGL_VERSION_MAJOR >= 9
#if LVGL_VERSION_MINOR >= 3
void *my_open_cb(lv_fs_drv_t * drv, const char * path, lv_fs_mode_t mode) {
    my_file_t *file = (my_file_t *)lv_malloc(sizeof(my_file_t));
#else
void *my_open_cb(struct lv_fs_drv_t * drv, const char * path, lv_fs_mode_t mode) {
    my_file_t *file = (my_file_t *)lv_malloc(sizeof(my_file_t));
#endif
#else
void *my_open_cb(struct _lv_fs_drv_t * drv, const char * path, lv_fs_mode_t mode) {
    my_file_t *file = (my_file_t *)lv_mem_alloc(sizeof(my_file_t));
#endif
    EEZ_UNUSED(drv);
    EEZ_UNUSED(mode);
    file->ptr = (void *)atoi(path);
    file->pos = 0;
    return file;
}

#if LVGL_VERSION_MAJOR >= 9
#if LVGL_VERSION_MINOR >= 3
lv_fs_res_t my_close_cb(lv_fs_drv_t * drv, void * file_p) {
    lv_free(file_p);
#else
lv_fs_res_t my_close_cb(struct lv_fs_drv_t * drv, void * file_p) {
    lv_free(file_p);
#endif
#else
lv_fs_res_t my_close_cb(struct _lv_fs_drv_t * drv, void * file_p) {
    lv_mem_free(file_p);
#endif
    EEZ_UNUSED(drv);
    return LV_FS_RES_OK;
}

#if LVGL_VERSION_MAJOR >= 9
#if LVGL_VERSION_MINOR >= 3
lv_fs_res_t my_read_cb(lv_fs_drv_t * drv, void * file_p, void * buf, uint32_t btr, uint32_t * br) {
#else
lv_fs_res_t my_read_cb(struct lv_fs_drv_t * drv, void * file_p, void * buf, uint32_t btr, uint32_t * br) {
#endif
#else
lv_fs_res_t my_read_cb(struct _lv_fs_drv_t * drv, void * file_p, void * buf, uint32_t btr, uint32_t * br) {
#endif
    EEZ_UNUSED(drv);
    my_file_t *file = (my_file_t *)file_p;
    memcpy(buf, file->ptr + file->pos, btr);
    file->pos += btr;
    if (br != 0)
        *br = btr;
    return LV_FS_RES_OK;
}

#if LVGL_VERSION_MAJOR >= 9
#if LVGL_VERSION_MINOR >= 3
lv_fs_res_t my_seek_cb(lv_fs_drv_t * drv, void * file_p, uint32_t pos, lv_fs_whence_t whence) {
#else
lv_fs_res_t my_seek_cb(struct lv_fs_drv_t * drv, void * file_p, uint32_t pos, lv_fs_whence_t whence) {
#endif
#else
lv_fs_res_t my_seek_cb(struct _lv_fs_drv_t * drv, void * file_p, uint32_t pos, lv_fs_whence_t whence) {
#endif
    EEZ_UNUSED(drv);
    my_file_t *file = (my_file_t *)file_p;
    if (whence == LV_FS_SEEK_SET) {
        file->pos = pos;
        return LV_FS_RES_OK;
    }
    if (whence == LV_FS_SEEK_CUR) {
        file->pos += pos;
        return LV_FS_RES_OK;
    }
    return LV_FS_RES_NOT_IMP;
}

#if LVGL_VERSION_MAJOR >= 9
#if LVGL_VERSION_MINOR >= 3
lv_fs_res_t my_tell_cb(lv_fs_drv_t * drv, void * file_p, uint32_t * pos_p) {
#else
lv_fs_res_t my_tell_cb(struct lv_fs_drv_t * drv, void * file_p, uint32_t * pos_p) {
#endif
#else
lv_fs_res_t my_tell_cb(struct _lv_fs_drv_t * drv, void * file_p, uint32_t * pos_p) {
#endif
    EEZ_UNUSED(drv);
    my_file_t *file = (my_file_t *)file_p;
    *pos_p = file->pos;
    return LV_FS_RES_OK;
}

static void init_fs_driver() {
    static lv_fs_drv_t drv;                   /*Needs to be static or global*/
    lv_fs_drv_init(&drv);                     /*Basic initialization*/

    drv.letter = 'M';                         /*An uppercase letter to identify the drive */
    drv.cache_size = my_cache_size;           /*Cache size for reading in bytes. 0 to not cache.*/

    drv.ready_cb = my_ready_cb;               /*Callback to tell if the drive is ready to use */
    drv.open_cb = my_open_cb;                 /*Callback to open a file */
    drv.close_cb = my_close_cb;               /*Callback to close a file */
    drv.read_cb = my_read_cb;                 /*Callback to read a file */
    drv.write_cb = 0;               /*Callback to write a file */
    drv.seek_cb = my_seek_cb;                 /*Callback to seek in a file (Move cursor) */
    drv.tell_cb = my_tell_cb;                 /*Callback to tell the cursor position  */

    drv.dir_open_cb = 0;         /*Callback to open directory to read its content */
    drv.dir_read_cb = 0;         /*Callback to read a directory's content */
    drv.dir_close_cb = 0;       /*Callback to close a directory */

#if LV_USE_USER_DATA
    drv.user_data = my_user_data;             /*Any custom data if required*/
#endif

    lv_fs_drv_register(&drv);                 /*Finally register the drive*/
}

////////////////////////////////////////////////////////////////////////////////

lv_indev_t *encoder_indev;
lv_indev_t *keyboard_indev;

EM_PORT_API(void) lvglSetEncoderGroup(lv_group_t *group) {
    lv_indev_set_group(encoder_indev, group);
}

EM_PORT_API(void) lvglSetKeyboardGroup(lv_group_t *group) {
    lv_indev_set_group(keyboard_indev, group);
}

EM_PORT_API(void) hal_init(bool is_editor) {
    // alloc memory for the display front buffer
    display_fb = (uint32_t *)malloc(sizeof(uint32_t) * hor_res * ver_res);
    memset(display_fb, 0x44, hor_res * ver_res * sizeof(uint32_t));

#if LVGL_VERSION_MAJOR >= 9
    lv_display_t * disp = lv_display_create(hor_res, ver_res);
    lv_display_set_flush_cb(disp, my_driver_flush);

    uint8_t *buf1 = malloc(sizeof(uint32_t) * hor_res * ver_res);
    uint8_t *buf2 = NULL;
    lv_display_set_buffers(disp, buf1, buf2, sizeof(uint32_t) * hor_res * ver_res, LV_DISPLAY_RENDER_MODE_PARTIAL);
#else
    /*Create a display buffer*/
    static lv_disp_draw_buf_t disp_buf1;
    lv_color_t * buf1_1 = malloc(sizeof(lv_color_t) * hor_res * ver_res);
    lv_disp_draw_buf_init(&disp_buf1, buf1_1, NULL, hor_res * ver_res);

    /*Create a display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);            /*Basic initialization*/
    disp_drv.draw_buf = &disp_buf1;
    disp_drv.flush_cb = my_driver_flush;    /*Used when `LV_VDB_SIZE != 0` in lv_conf.h (buffered drawing)*/
    disp_drv.hor_res = hor_res;
    disp_drv.ver_res = ver_res;
    lv_disp_drv_register(&disp_drv);
#endif

    if (!is_editor) {
        // mouse init
#if LVGL_VERSION_MAJOR >= 9
        lv_indev_t * indev1 = lv_indev_create();
        lv_indev_set_type(indev1, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(indev1, my_mouse_read);
        //lv_indev_set_mode(indev1, LV_INDEV_MODE_EVENT);
#else
        static lv_indev_drv_t indev_drv_1;
        lv_indev_drv_init(&indev_drv_1); /*Basic initialization*/
        indev_drv_1.type = LV_INDEV_TYPE_POINTER;
        indev_drv_1.read_cb = my_mouse_read;
        lv_indev_drv_register(&indev_drv_1);
#endif

        // keyboard init
#if LVGL_VERSION_MAJOR >= 9
        keyboard_indev = lv_indev_create();
        lv_indev_set_type(keyboard_indev, LV_INDEV_TYPE_KEYPAD);
        lv_indev_set_read_cb(keyboard_indev, my_keyboard_read);
        //lv_indev_set_mode(keyboard_indev, LV_INDEV_MODE_EVENT);
#else
        static lv_indev_drv_t indev_drv_2;
        lv_indev_drv_init(&indev_drv_2);
        indev_drv_2.type = LV_INDEV_TYPE_KEYPAD;
        indev_drv_2.read_cb = my_keyboard_read;
        keyboard_indev = lv_indev_drv_register(&indev_drv_2);
#endif

        // mousewheel init
#if LVGL_VERSION_MAJOR >= 9
        encoder_indev = lv_indev_create();
        lv_indev_set_type(encoder_indev, LV_INDEV_TYPE_ENCODER);
        lv_indev_set_read_cb(encoder_indev, my_mousewheel_read);
        //lv_indev_set_mode(encoder_indev, LV_INDEV_MODE_EVENT);
#else
        static lv_indev_drv_t indev_drv_3;
        lv_indev_drv_init(&indev_drv_3);
        indev_drv_3.type = LV_INDEV_TYPE_ENCODER;
        indev_drv_3.read_cb = my_mousewheel_read;
        encoder_indev = lv_indev_drv_register(&indev_drv_3);
#endif
    }

    init_fs_driver();
}

bool initialized = false;

#if LVGL_VERSION_MAJOR >= 9
static uint32_t g_prevTick;
#endif

EM_PORT_API(void) init(uint32_t wasmModuleId, uint32_t debuggerMessageSubsciptionFilter, uint8_t *assets, uint32_t assetsSize, uint32_t displayWidth, uint32_t displayHeight, bool darkTheme, uint32_t timeZone, bool screensLifetimeSupport) {
    bool is_editor = assetsSize == 0;

    hor_res = displayWidth;
    ver_res = displayHeight;

    /*Initialize LittlevGL*/
    lv_init();

    /*Initialize the HAL (display, input devices, tick) for LittlevGL*/
    hal_init(is_editor);

    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), darkTheme, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);

    if (!is_editor && assets) {
        flowInit(wasmModuleId, debuggerMessageSubsciptionFilter, assets, assetsSize, darkTheme, timeZone, screensLifetimeSupport);
    }

#if LVGL_VERSION_MAJOR >= 9
    g_prevTick = (uint32_t)emscripten_get_now();
#endif

    initialized = true;
}

EM_PORT_API(bool) mainLoop() {
    if (!initialized) {
        return true;
    }

#if LVGL_VERSION_MAJOR >= 9
    uint32_t currentTick = (uint32_t)emscripten_get_now();
    lv_tick_inc(currentTick - g_prevTick);
    g_prevTick = currentTick;
#endif

    /* Periodically call the lv_task handler */
    lv_task_handler();

    return flowTick();
}

EM_PORT_API(uint8_t*) getSyncedBuffer() {
    if (display_fb_dirty) {
        display_fb_dirty = false;
        return (uint8_t*)display_fb;
    }
	return NULL;
}

EM_PORT_API(bool) isRTL() {
    return false;
}

EM_PORT_API(void) onPointerEvent(int x, int y, int pressed) {
    if (x < 0) x = 0;
    else if (x >= hor_res) x = hor_res - 1;
    mouse_x = x;

    if (y < 0) y = 0;
    else if (y >= ver_res) y = ver_res - 1;
    mouse_y = y;

    mouse_pressed = pressed;
}

EM_PORT_API(void) onMouseWheelEvent(double yMouseWheel, int pressed) {
    if (yMouseWheel >= 100 || yMouseWheel <= -100) {
        yMouseWheel /= 100;
    }
    mouse_wheel_delta = round(yMouseWheel);
    mouse_wheel_pressed = pressed;
}

EM_PORT_API(void) onKeyPressed(uint32_t key) {
    if (keyboard_buffer_index < KEYBOARD_BUFFER_SIZE) {
        keyboard_buffer[keyboard_buffer_index++] = key;
    }
}

////////////////////////////////////////////////////////////////////////////////

#define SYMBOLS_STRING_INIT_ALLOCATED 1024 * 1024

typedef struct SymbolsString {
    char *ptr;
    unsigned size;
    unsigned allocated;
} SymbolsString;

static SymbolsString g_symbolsString;

void symbols_append(const char *format, ...) {
    va_list args;
    va_start(args, format);

    va_list args_copy;
    va_copy(args_copy, args);
    int needed = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);

    if (needed < 0) {
        va_end(args);
        return; // encoding error
    }

    unsigned total_needed = g_symbolsString.size + needed + 1;

    // Ensure we have enough space
    assert(total_needed <= g_symbolsString.allocated);

    // Now actually write the formatted string at the end of the buffer
    int written = vsnprintf(g_symbolsString.ptr + g_symbolsString.size, 
                            g_symbolsString.allocated - g_symbolsString.size, 
                            format, args);

    va_end(args);

    if (written < 0 || written > needed) {
        // Something went wrong (shouldn't happen with proper vsnprintf)
        return;
    }

    g_symbolsString.size += written;
    // symstr->ptr is always null-terminated by vsnprintf
}

lv_obj_t *lv_spinner_create_adapt(lv_obj_t *parentObj) {
#if LVGL_VERSION_MAJOR >= 9
    return lv_spinner_create(parentObj);
#else
    return lv_spinner_create(parentObj, 1000, 60);
#endif
}

#if LVGL_VERSION_MAJOR >= 9
#else
lv_obj_t *lv_colorwheel_create_adapt(lv_obj_t *parentObj) {
    return lv_colorwheel_create(parentObj, false);
}
#endif

lv_obj_t *lv_qrcode_create_adapt(lv_obj_t *parentObj) {
#if LVGL_VERSION_MAJOR >= 9
    return lv_qrcode_create(parentObj);
#else
    lv_color_t dark_color;
    dark_color.ch.blue = 255;
    dark_color.ch.green = 0;
    dark_color.ch.red = 0;
    lv_color_t light_color;
    light_color.ch.blue = 0;
    light_color.ch.green = 0;
    light_color.ch.red = 255;
    return lv_qrcode_create(parentObj, 120, dark_color, light_color);
#endif
}
    
typedef struct {
    const char *name;
    lv_obj_t *(*create)(lv_obj_t *);
} WidgetInfo;

WidgetInfo widgets[] = {
    { "Screen", lv_obj_create },
    { "Label", lv_label_create },
    { "Button", lv_btn_create },
    { "Panel", lv_obj_create },
    { "Image", lv_img_create },
    { "Slider", lv_slider_create },
    { "Roller", lv_roller_create },
    { "Switch", lv_switch_create },
    { "Bar", lv_bar_create },
    { "Dropdown", lv_dropdown_create },
    { "Arc", lv_arc_create },
    { "Spinner", lv_spinner_create_adapt },
    { "Checkbox", lv_checkbox_create },
    { "Textarea", lv_textarea_create },
    { "Keyboard", lv_keyboard_create },
    { "Chart", lv_chart_create },
    { "Calendar", lv_calendar_create },
#if LVGL_VERSION_MAJOR >= 9
    { "Scale", lv_scale_create },
#else
    { "Colorwheel", lv_colorwheel_create_adapt },
    { "ImageButton", lv_imgbtn_create },
    { "Meter", lv_meter_create },
#endif
    { "QRCode", lv_qrcode_create_adapt },
    { "Spinbox", lv_spinbox_create },
};

typedef struct {
    const char *name;
    int code;
} FlagInfo;

FlagInfo flags[] = {
    { "HIDDEN", LV_OBJ_FLAG_HIDDEN },
    { "CLICKABLE", LV_OBJ_FLAG_CLICKABLE },
    { "CLICK_FOCUSABLE", LV_OBJ_FLAG_CLICK_FOCUSABLE },
    { "CHECKABLE", LV_OBJ_FLAG_CHECKABLE },
    { "SCROLLABLE", LV_OBJ_FLAG_SCROLLABLE },
    { "SCROLL_ELASTIC", LV_OBJ_FLAG_SCROLL_ELASTIC },
    { "SCROLL_MOMENTUM", LV_OBJ_FLAG_SCROLL_MOMENTUM },
    { "SCROLL_ONE", LV_OBJ_FLAG_SCROLL_ONE },
    { "SCROLL_CHAIN_HOR", LV_OBJ_FLAG_SCROLL_CHAIN_HOR },
    { "SCROLL_CHAIN_VER", LV_OBJ_FLAG_SCROLL_CHAIN_VER },
    { "SCROLL_CHAIN", LV_OBJ_FLAG_SCROLL_CHAIN },
    { "SCROLL_ON_FOCUS", LV_OBJ_FLAG_SCROLL_ON_FOCUS },
    { "SCROLL_WITH_ARROW", LV_OBJ_FLAG_SCROLL_WITH_ARROW },
    { "SNAPPABLE", LV_OBJ_FLAG_SNAPPABLE },
    { "PRESS_LOCK", LV_OBJ_FLAG_PRESS_LOCK },
    { "EVENT_BUBBLE", LV_OBJ_FLAG_EVENT_BUBBLE },
    { "GESTURE_BUBBLE", LV_OBJ_FLAG_GESTURE_BUBBLE },
    { "ADV_HITTEST", LV_OBJ_FLAG_ADV_HITTEST },
    { "IGNORE_LAYOUT", LV_OBJ_FLAG_IGNORE_LAYOUT },
    { "FLOATING", LV_OBJ_FLAG_FLOATING },
    { "OVERFLOW_VISIBLE", LV_OBJ_FLAG_OVERFLOW_VISIBLE }
};

void dump_widget_flags_info(WidgetInfo *info, lv_obj_t *obj, bool last) {
    symbols_append("\"");
    symbols_append(info->name);
    symbols_append("\": \"");

    bool first = true;

    for (size_t i = 0; i < sizeof(flags) / sizeof(FlagInfo); i++) {
        if (lv_obj_has_flag(obj, flags[i].code)) {
            if (first) {
                first = false;
            } else {
                symbols_append(" | ");
            }
            symbols_append(flags[i].name);
        }
    }

    symbols_append("\"");
    if (!last) {
        symbols_append(",");
    }
}

void dump_widgets_flags_info() {
    lv_obj_t *parent_obj = 0;

    for (size_t i = 0; i < sizeof(widgets) / sizeof(WidgetInfo); i++) {
        lv_obj_t *obj = widgets[i].create(parent_obj);
        if (!parent_obj) {
            parent_obj = obj;
        }
        dump_widget_flags_info(widgets + i, obj, i == sizeof(widgets) / sizeof(WidgetInfo) - 1);
    }

    lv_obj_del(parent_obj);
}

void style_append(const char *name, int code) {
    symbols_append("{\"name\": \"%s\", \"code\": %d},", name, code);
}

void style_append_with_new_name(const char *name, int code, const char *new_name) {
    symbols_append("{\"name\": \"%s\", \"code\": %d, \"new_name\": \"%s\"},", name, code, new_name);
}

void style_append_last(const char *name, int code) {
    symbols_append("{\"name\": \"%s\", \"code\": %d}", name, code);
}

void style_append_undefined(const char *name) {
    symbols_append("{\"name\": \"%s\", \"code\": null},", name);
}

void dump_custom_styles() {
    /*Group 0*/
    style_append("LV_STYLE_WIDTH", LV_STYLE_WIDTH); // { "8.3": 1, "9.0": 1 },
    style_append("LV_STYLE_HEIGHT", LV_STYLE_HEIGHT); // "8.3": 4, "9.0": 2 },

#if LVGL_VERSION_MAJOR >= 9
    style_append("LV_STYLE_LENGTH", LV_STYLE_LENGTH); // "8.3": undefined, "9.0": 3 }, // ONLY 9.0
#else
    style_append_undefined("LV_STYLE_LENGTH");
#endif

    style_append("LV_STYLE_MIN_WIDTH", LV_STYLE_MIN_WIDTH); // "8.3": 2, "9.0": 4 },
    style_append("LV_STYLE_MAX_WIDTH", LV_STYLE_MAX_WIDTH); // "8.3": 3, "9.0": 5 },
    style_append("LV_STYLE_MIN_HEIGHT", LV_STYLE_MIN_HEIGHT); // "8.3": 5, "9.0": 6 },
    style_append("LV_STYLE_MAX_HEIGHT", LV_STYLE_MAX_HEIGHT); // "8.3": 6, "9.0": 7 },

    style_append("LV_STYLE_X", LV_STYLE_X); // "8.3": 7, "9.0": 8 },
    style_append("LV_STYLE_Y", LV_STYLE_Y); // "8.3": 8, "9.0": 9 },
    style_append("LV_STYLE_ALIGN", LV_STYLE_ALIGN); // "8.3": 9, "9.0": 10 },

    style_append("LV_STYLE_RADIUS", LV_STYLE_RADIUS); // "8.3": 11, "9.0": 12 },

    /*Group 1*/
    style_append("LV_STYLE_PAD_TOP", LV_STYLE_PAD_TOP); // "8.3": 16, "9.0": 16 },
    style_append("LV_STYLE_PAD_BOTTOM", LV_STYLE_PAD_BOTTOM); // "8.3": 17, "9.0": 17 },
    style_append("LV_STYLE_PAD_LEFT", LV_STYLE_PAD_LEFT); // "8.3": 18, "9.0": 18 },
    style_append("LV_STYLE_PAD_RIGHT", LV_STYLE_PAD_RIGHT); // "8.3": 19, "9.0": 19 },

    style_append("LV_STYLE_PAD_ROW", LV_STYLE_PAD_ROW); // "8.3": 20, "9.0": 20 },
    style_append("LV_STYLE_PAD_COLUMN", LV_STYLE_PAD_COLUMN); // "8.3": 21, "9.0": 21 },
    style_append("LV_STYLE_LAYOUT", LV_STYLE_LAYOUT); // "8.3": 10, "9.0": 22 },

#if LVGL_VERSION_MAJOR >= 9
    style_append("LV_STYLE_MARGIN_TOP", LV_STYLE_MARGIN_TOP); // "8.3": undefined, "9.0": 24 }, // ONLY 9.0
    style_append("LV_STYLE_MARGIN_BOTTOM", LV_STYLE_MARGIN_BOTTOM); // "8.3": undefined, "9.0": 25 }, // ONLY 9.0
    style_append("LV_STYLE_MARGIN_LEFT", LV_STYLE_MARGIN_LEFT); // "8.3": undefined, "9.0": 26 }, // ONLY 9.0
    style_append("LV_STYLE_MARGIN_RIGHT", LV_STYLE_MARGIN_RIGHT); // "8.3": undefined, "9.0": 27 }, // ONLY 9.0
#else
    style_append_undefined("LV_STYLE_MARGIN_TOP");
    style_append_undefined("LV_STYLE_MARGIN_BOTTOM");
    style_append_undefined("LV_STYLE_MARGIN_LEFT");
    style_append_undefined("LV_STYLE_MARGIN_RIGHT");
#endif

    /*Group 2*/
    style_append("LV_STYLE_BG_COLOR", LV_STYLE_BG_COLOR); // "8.3": 32, "9.0": 28 },
    style_append("LV_STYLE_BG_OPA", LV_STYLE_BG_OPA); // "8.3": 33, "9.0": 29 },

    style_append("LV_STYLE_BG_GRAD_DIR", LV_STYLE_BG_GRAD_DIR); // "8.3": 35, "9.0": 32 },
    style_append("LV_STYLE_BG_MAIN_STOP", LV_STYLE_BG_MAIN_STOP); // "8.3": 36, "9.0": 33 },
    style_append("LV_STYLE_BG_GRAD_STOP", LV_STYLE_BG_GRAD_STOP); // "8.3": 37, "9.0": 34 },
    style_append("LV_STYLE_BG_GRAD_COLOR", LV_STYLE_BG_GRAD_COLOR); // "8.3": 34, "9.0": 35 },

#if LVGL_VERSION_MAJOR >= 9
    style_append("LV_STYLE_BG_MAIN_OPA", LV_STYLE_BG_MAIN_OPA); // "8.3": undefined, "9.0": 36 }, // ONLY 9.0
    style_append("LV_STYLE_BG_GRAD_OPA", LV_STYLE_BG_GRAD_OPA); // "8.3": undefined, "9.0": 37 }, // ONLY 9.0
#else
    style_append_undefined("LV_STYLE_BG_MAIN_OPA");
    style_append_undefined("LV_STYLE_BG_GRAD_OPA");
#endif

    style_append("LV_STYLE_BG_GRAD", LV_STYLE_BG_GRAD); // "8.3": 38, "9.0": 38 },
    style_append("LV_STYLE_BASE_DIR", LV_STYLE_BASE_DIR); // "8.3": 22, "9.0": 39 },

#if LVGL_VERSION_MAJOR >= 9
    style_append_undefined("LV_STYLE_BG_DITHER_MODE");
#else
    style_append("LV_STYLE_BG_DITHER_MODE", LV_STYLE_BG_DITHER_MODE); // "8.3": 39, "9.0": undefined }, // ONLY 8.3
#endif

#if LVGL_VERSION_MAJOR >= 9
    style_append_with_new_name("LV_STYLE_BG_IMG_SRC", LV_STYLE_BG_IMAGE_SRC, "LV_STYLE_BG_IMAGE_SRC"); // "8.3": 40, "9.0": 40 },
    style_append_with_new_name("LV_STYLE_BG_IMG_OPA", LV_STYLE_BG_IMAGE_OPA, "LV_STYLE_BG_IMAGE_OPA"); // "8.3": 41, "9.0": 41 },
    style_append_with_new_name("LV_STYLE_BG_IMG_RECOLOR", LV_STYLE_BG_IMAGE_RECOLOR, "LV_STYLE_BG_IMAGE_RECOLOR"); // "8.3": 42, "9.0": 42 },
    style_append_with_new_name("LV_STYLE_BG_IMG_RECOLOR_OPA", LV_STYLE_BG_IMAGE_RECOLOR_OPA, "LV_STYLE_BG_IMAGE_RECOLOR_OPA"); // "8.3": 43, "9.0": 43 },
#else
    style_append("LV_STYLE_BG_IMG_SRC", LV_STYLE_BG_IMG_SRC); // "8.3": 40, "9.0": 40 },
    style_append("LV_STYLE_BG_IMG_OPA", LV_STYLE_BG_IMG_OPA); // "8.3": 41, "9.0": 41 },
    style_append("LV_STYLE_BG_IMG_RECOLOR", LV_STYLE_BG_IMG_RECOLOR); // "8.3": 42, "9.0": 42 },
    style_append("LV_STYLE_BG_IMG_RECOLOR_OPA", LV_STYLE_BG_IMG_RECOLOR_OPA); // "8.3": 43, "9.0": 43 },
#endif

#if LVGL_VERSION_MAJOR >= 9
    style_append_with_new_name("LV_STYLE_BG_IMG_TILED", LV_STYLE_BG_IMAGE_TILED, "LV_STYLE_BG_IMAGE_TILED"); // "8.3": 44, "9.0": 44 },
#else
    style_append("LV_STYLE_BG_IMG_TILED", LV_STYLE_BG_IMG_TILED); // "8.3": 44, "9.0": 44 },
#endif

    style_append("LV_STYLE_CLIP_CORNER", LV_STYLE_CLIP_CORNER); // "8.3": 23, "9.0": 45 },

    /*Group 3*/
    style_append("LV_STYLE_BORDER_WIDTH", LV_STYLE_BORDER_WIDTH); // "8.3": 50, "9.0": 48 },
    style_append("LV_STYLE_BORDER_COLOR", LV_STYLE_BORDER_COLOR); // "8.3": 48, "9.0": 49 },
    style_append("LV_STYLE_BORDER_OPA", LV_STYLE_BORDER_OPA); // "8.3": 49, "9.0": 50 },

    style_append("LV_STYLE_BORDER_SIDE", LV_STYLE_BORDER_SIDE); // "8.3": 51, "9.0": 52 },
    style_append("LV_STYLE_BORDER_POST", LV_STYLE_BORDER_POST); // "8.3": 52, "9.0": 53 },

    style_append("LV_STYLE_OUTLINE_WIDTH", LV_STYLE_OUTLINE_WIDTH); // "8.3": 53, "9.0": 56 },
    style_append("LV_STYLE_OUTLINE_COLOR", LV_STYLE_OUTLINE_COLOR); // "8.3": 54, "9.0": 57 },
    style_append("LV_STYLE_OUTLINE_OPA", LV_STYLE_OUTLINE_OPA); // "8.3": 55, "9.0": 58 },
    style_append("LV_STYLE_OUTLINE_PAD", LV_STYLE_OUTLINE_PAD); // "8.3": 56, "9.0": 59 },

    /*Group 4*/
    style_append("LV_STYLE_SHADOW_WIDTH", LV_STYLE_SHADOW_WIDTH); // "8.3": 64, "9.0": 60 },
    style_append("LV_STYLE_SHADOW_COLOR", LV_STYLE_SHADOW_COLOR); // "8.3": 68, "9.0": 61 },
    style_append("LV_STYLE_SHADOW_OPA", LV_STYLE_SHADOW_OPA); // "8.3": 69, "9.0": 62 },

    style_append("LV_STYLE_SHADOW_OFS_X", LV_STYLE_SHADOW_OFS_X); // "8.3": 65, "9.0": 64 },
    style_append("LV_STYLE_SHADOW_OFS_Y", LV_STYLE_SHADOW_OFS_Y); // "8.3": 66, "9.0": 65 },
    style_append("LV_STYLE_SHADOW_SPREAD", LV_STYLE_SHADOW_SPREAD); // "8.3": 67, "9.0": 66 },

#if LVGL_VERSION_MAJOR >= 9
    style_append_with_new_name("LV_STYLE_IMG_OPA", LV_STYLE_IMAGE_OPA, "LV_STYLE_IMAGE_OPA"); // "8.3": 70, "9.0": 68 },
    style_append_with_new_name("LV_STYLE_IMG_RECOLOR", LV_STYLE_IMAGE_RECOLOR, "LV_STYLE_IMAGE_RECOLOR"); // "8.3": 71, "9.0": 69 },
    style_append_with_new_name("LV_STYLE_IMG_RECOLOR_OPA", LV_STYLE_IMAGE_RECOLOR_OPA, "LV_STYLE_IMAGE_RECOLOR_OPA"); // "8.3": 72, "9.0": 70 },
#else
    style_append("LV_STYLE_IMG_OPA", LV_STYLE_IMG_OPA); // "8.3": 70, "9.0": 68 },
    style_append("LV_STYLE_IMG_RECOLOR", LV_STYLE_IMG_RECOLOR); // "8.3": 71, "9.0": 69 },
    style_append("LV_STYLE_IMG_RECOLOR_OPA", LV_STYLE_IMG_RECOLOR_OPA); // "8.3": 72, "9.0": 70 },
#endif

    style_append("LV_STYLE_LINE_WIDTH", LV_STYLE_LINE_WIDTH); // "8.3": 73, "9.0": 72 },
    style_append("LV_STYLE_LINE_DASH_WIDTH", LV_STYLE_LINE_DASH_WIDTH); // "8.3": 74, "9.0": 73 },
    style_append("LV_STYLE_LINE_DASH_GAP", LV_STYLE_LINE_DASH_GAP); // "8.3": 75, "9.0": 74 },
    style_append("LV_STYLE_LINE_ROUNDED", LV_STYLE_LINE_ROUNDED); // "8.3": 76, "9.0": 75 },
    style_append("LV_STYLE_LINE_COLOR", LV_STYLE_LINE_COLOR); // "8.3": 77, "9.0": 76 },
    style_append("LV_STYLE_LINE_OPA", LV_STYLE_LINE_OPA); // "8.3": 78, "9.0": 77 },

    /*Group 5*/
    style_append("LV_STYLE_ARC_WIDTH", LV_STYLE_ARC_WIDTH); // "8.3": 80, "9.0": 80 },
    style_append("LV_STYLE_ARC_ROUNDED", LV_STYLE_ARC_ROUNDED); // "8.3": 81, "9.0": 81 },
    style_append("LV_STYLE_ARC_COLOR", LV_STYLE_ARC_COLOR); // "8.3": 82, "9.0": 82 },
    style_append("LV_STYLE_ARC_OPA", LV_STYLE_ARC_OPA); // "8.3": 83, "9.0": 83 },

#if LVGL_VERSION_MAJOR >= 9
    style_append_with_new_name("LV_STYLE_ARC_IMG_SRC", LV_STYLE_ARC_IMAGE_SRC, "LV_STYLE_ARC_IMAGE_SRC"); // "8.3": 84, "9.0": 84 },
#else
    style_append("LV_STYLE_ARC_IMG_SRC", LV_STYLE_ARC_IMG_SRC); // "8.3": 84, "9.0": 84 },
#endif

    style_append("LV_STYLE_TEXT_COLOR", LV_STYLE_TEXT_COLOR); // "8.3": 85, "9.0": 88 },
    style_append("LV_STYLE_TEXT_OPA", LV_STYLE_TEXT_OPA); // "8.3": 86, "9.0": 89 },
    style_append("LV_STYLE_TEXT_FONT", LV_STYLE_TEXT_FONT); // "8.3": 87, "9.0": 90 },

    style_append("LV_STYLE_TEXT_LETTER_SPACE", LV_STYLE_TEXT_LETTER_SPACE); // "8.3": 88, "9.0": 91 },
    style_append("LV_STYLE_TEXT_LINE_SPACE", LV_STYLE_TEXT_LINE_SPACE); // "8.3": 89, "9.0": 92 },
    style_append("LV_STYLE_TEXT_DECOR", LV_STYLE_TEXT_DECOR); // "8.3": 90, "9.0": 93 },
    style_append("LV_STYLE_TEXT_ALIGN", LV_STYLE_TEXT_ALIGN); // "8.3": 91, "9.0": 94 },

    style_append("LV_STYLE_OPA", LV_STYLE_OPA); // "8.3": 96, "9.0": 95 },
    style_append("LV_STYLE_OPA_LAYERED", LV_STYLE_OPA_LAYERED); // "8.3": 97, "9.0": 96 },
    style_append("LV_STYLE_COLOR_FILTER_DSC", LV_STYLE_COLOR_FILTER_DSC); // "8.3": 98, "9.0": 97 },
    style_append("LV_STYLE_COLOR_FILTER_OPA", LV_STYLE_COLOR_FILTER_OPA); // "8.3": 99, "9.0": 98 },

    style_append("LV_STYLE_ANIM", LV_STYLE_ANIM); // "8.3": 100, "9.0": 99 },

#if LVGL_VERSION_MAJOR >= 9
    style_append_undefined("LV_STYLE_ANIM_TIME");
#else
    style_append("LV_STYLE_ANIM_TIME", LV_STYLE_ANIM_TIME); // "8.3": 101, "9.0": undefined }, // ONLY 8.3
#endif

#if LVGL_VERSION_MAJOR >= 9
    style_append("LV_STYLE_ANIM_DURATION", LV_STYLE_ANIM_DURATION); // "8.3": undefined, "9.0": 100 }, // ONLY 9.0
#else
    style_append_undefined("LV_STYLE_ANIM_DURATION");
#endif

#if LVGL_VERSION_MAJOR >= 9
    style_append_undefined("LV_STYLE_ANIM_SPEED");
#else
    style_append("LV_STYLE_ANIM_SPEED", LV_STYLE_ANIM_SPEED); // "8.3": 102, "9.0": undefined }, // ONLY 8.3
#endif

    style_append("LV_STYLE_TRANSITION", LV_STYLE_TRANSITION); // "8.3": 103, "9.0": 102 },

    style_append("LV_STYLE_BLEND_MODE", LV_STYLE_BLEND_MODE); // "8.3": 104, "9.0": 103 },
    style_append("LV_STYLE_TRANSFORM_WIDTH", LV_STYLE_TRANSFORM_WIDTH); // "8.3": 105, "9.0": 104 },
    style_append("LV_STYLE_TRANSFORM_HEIGHT", LV_STYLE_TRANSFORM_HEIGHT); // "8.3": 106, "9.0": 105 },
    style_append("LV_STYLE_TRANSLATE_X", LV_STYLE_TRANSLATE_X); // "8.3": 107, "9.0": 106 },
    style_append("LV_STYLE_TRANSLATE_Y", LV_STYLE_TRANSLATE_Y); // "8.3": 108, "9.0": 107 },

#if LVGL_VERSION_MAJOR >= 9
    style_append_undefined("LV_STYLE_TRANSFORM_ZOOM");
#else
    style_append("LV_STYLE_TRANSFORM_ZOOM", LV_STYLE_TRANSFORM_ZOOM); // "8.3": 109, "9.0": undefined }, // ONLY 8.3
#endif

#if LVGL_VERSION_MAJOR >= 9
    style_append("LV_STYLE_TRANSFORM_SCALE_X", LV_STYLE_TRANSFORM_SCALE_X); // "8.3": undefined, "9.0": 108 }, // ONLY 9.0
    style_append("LV_STYLE_TRANSFORM_SCALE_Y", LV_STYLE_TRANSFORM_SCALE_Y); // "8.3": undefined, "9.0": 109 }, // ONLY 9.0
#else
    style_append_undefined("LV_STYLE_TRANSFORM_SCALE_X");
    style_append_undefined("LV_STYLE_TRANSFORM_SCALE_Y");
#endif

#if LVGL_VERSION_MAJOR >= 9
    style_append_undefined("LV_STYLE_TRANSFORM_ANGLE");
#else
    style_append("LV_STYLE_TRANSFORM_ANGLE", LV_STYLE_TRANSFORM_ANGLE); // "8.3": 110, "9.0": undefined }, // ONLY 8.3
#endif

#if LVGL_VERSION_MAJOR >= 9
    style_append("LV_STYLE_TRANSFORM_ROTATION", LV_STYLE_TRANSFORM_ROTATION); // "8.3": undefined, "9.0": 110 }, // ONLY 9.0
#else
    style_append_undefined("LV_STYLE_TRANSFORM_ROTATION");
#endif

    style_append("LV_STYLE_TRANSFORM_PIVOT_X", LV_STYLE_TRANSFORM_PIVOT_X); // "8.3": 111, "9.0": 111 },
    style_append("LV_STYLE_TRANSFORM_PIVOT_Y", LV_STYLE_TRANSFORM_PIVOT_Y); // "8.3": 112, "9.0": 112 },

#if LVGL_VERSION_MAJOR >= 9
    style_append("LV_STYLE_TRANSFORM_SKEW_X", LV_STYLE_TRANSFORM_SKEW_X); // "8.3": undefined, "9.0": 113 }, // ONLY 9.0
    style_append("LV_STYLE_TRANSFORM_SKEW_Y", LV_STYLE_TRANSFORM_SKEW_Y); // "8.3": undefined, "9.0": 114 }, // ONLY 9.0
#else
    style_append_undefined("LV_STYLE_TRANSFORM_SKEW_X");
    style_append_undefined("LV_STYLE_TRANSFORM_SKEW_Y");
#endif

    style_append("LV_STYLE_FLEX_FLOW", LV_STYLE_FLEX_FLOW);
    style_append("LV_STYLE_FLEX_MAIN_PLACE", LV_STYLE_FLEX_MAIN_PLACE);
    style_append("LV_STYLE_FLEX_CROSS_PLACE", LV_STYLE_FLEX_CROSS_PLACE);
    style_append("LV_STYLE_FLEX_TRACK_PLACE", LV_STYLE_FLEX_TRACK_PLACE);
    style_append("LV_STYLE_FLEX_GROW", LV_STYLE_FLEX_GROW);

    style_append("LV_STYLE_GRID_COLUMN_ALIGN", LV_STYLE_GRID_COLUMN_ALIGN);
    style_append("LV_STYLE_GRID_ROW_ALIGN", LV_STYLE_GRID_ROW_ALIGN);
    style_append("LV_STYLE_GRID_ROW_DSC_ARRAY", LV_STYLE_GRID_ROW_DSC_ARRAY);
    style_append("LV_STYLE_GRID_COLUMN_DSC_ARRAY", LV_STYLE_GRID_COLUMN_DSC_ARRAY);
    style_append("LV_STYLE_GRID_CELL_COLUMN_POS", LV_STYLE_GRID_CELL_COLUMN_POS);
    style_append("LV_STYLE_GRID_CELL_COLUMN_SPAN", LV_STYLE_GRID_CELL_COLUMN_SPAN);
    style_append("LV_STYLE_GRID_CELL_X_ALIGN", LV_STYLE_GRID_CELL_X_ALIGN);
    style_append("LV_STYLE_GRID_CELL_ROW_POS", LV_STYLE_GRID_CELL_ROW_POS);
    style_append("LV_STYLE_GRID_CELL_ROW_SPAN", LV_STYLE_GRID_CELL_ROW_SPAN);
    style_append_last("LV_STYLE_GRID_CELL_Y_ALIGN", LV_STYLE_GRID_CELL_Y_ALIGN);
}

void dump_constant(const char *name, int value) {
    symbols_append("\"%s\": %d,", name, value);
}

void dump_constant_last(const char *name, int value) {
    symbols_append("\"%s\": %d", name, value);
}

int getNumShifts(int code) {
    if (code == 1) return 0;
    return 1 + getNumShifts(code >> 1);
}

void dump_flag(const char *name, int code, const char *description) {
    symbols_append("\"%s\": {\"code\": \"1 << %d\", \"description\": \"%s\"},", name, getNumShifts(code), description);
}

void dump_constant_hex(const char *name, int value, int hex_digits) {
    symbols_append("\"%s\": \"0x%0*x\",", name, hex_digits, value);
}

void dump_constant_undefined(const char *name) {
    symbols_append("\"%s\": null,", name);
}

void dump_constants() {
    dump_constant("LV_FLEX_FLOW_ROW", LV_FLEX_FLOW_ROW);
    dump_constant("LV_FLEX_FLOW_COLUMN", LV_FLEX_FLOW_COLUMN);
    dump_constant("LV_FLEX_FLOW_ROW_WRAP", LV_FLEX_FLOW_ROW_WRAP);
    dump_constant("LV_FLEX_FLOW_ROW_REVERSE", LV_FLEX_FLOW_ROW_REVERSE);
    dump_constant("LV_FLEX_FLOW_ROW_WRAP_REVERSE", LV_FLEX_FLOW_ROW_WRAP_REVERSE);
    dump_constant("LV_FLEX_FLOW_COLUMN_WRAP", LV_FLEX_FLOW_COLUMN_WRAP);
    dump_constant("LV_FLEX_FLOW_COLUMN_REVERSE", LV_FLEX_FLOW_COLUMN_REVERSE);
    dump_constant("LV_FLEX_FLOW_COLUMN_WRAP_REVERSE", LV_FLEX_FLOW_COLUMN_WRAP_REVERSE);

    dump_constant("LV_FLEX_ALIGN_START", LV_FLEX_ALIGN_START);
    dump_constant("LV_FLEX_ALIGN_END", LV_FLEX_ALIGN_END);
    dump_constant("LV_FLEX_ALIGN_CENTER", LV_FLEX_ALIGN_CENTER);
    dump_constant("LV_FLEX_ALIGN_SPACE_EVENLY", LV_FLEX_ALIGN_SPACE_EVENLY);
    dump_constant("LV_FLEX_ALIGN_SPACE_AROUND", LV_FLEX_ALIGN_SPACE_AROUND);
    dump_constant("LV_FLEX_ALIGN_SPACE_BETWEEN", LV_FLEX_ALIGN_SPACE_BETWEEN);

    dump_constant("LV_GRID_ALIGN_START", LV_GRID_ALIGN_START);
    dump_constant("LV_GRID_ALIGN_CENTER", LV_GRID_ALIGN_CENTER);
    dump_constant("LV_GRID_ALIGN_END", LV_GRID_ALIGN_END);
    dump_constant("LV_GRID_ALIGN_STRETCH", LV_GRID_ALIGN_STRETCH);
    dump_constant("LV_GRID_ALIGN_SPACE_EVENLY", LV_GRID_ALIGN_SPACE_EVENLY);
    dump_constant("LV_GRID_ALIGN_SPACE_AROUND", LV_GRID_ALIGN_SPACE_AROUND);
    dump_constant("LV_GRID_ALIGN_SPACE_BETWEEN", LV_GRID_ALIGN_SPACE_BETWEEN);

    dump_constant("LV_SCROLLBAR_MODE_OFF", LV_SCROLLBAR_MODE_OFF);
    dump_constant("LV_SCROLLBAR_MODE_ON", LV_SCROLLBAR_MODE_ON);
    dump_constant("LV_SCROLLBAR_MODE_ACTIVE", LV_SCROLLBAR_MODE_ACTIVE);
    dump_constant("LV_SCROLLBAR_MODE_AUTO", LV_SCROLLBAR_MODE_AUTO);

    dump_constant("LV_DIR_NONE", LV_DIR_NONE);
    dump_constant("LV_DIR_LEFT", LV_DIR_LEFT);
    dump_constant("LV_DIR_RIGHT", LV_DIR_RIGHT);
    dump_constant("LV_DIR_TOP", LV_DIR_TOP);
    dump_constant("LV_DIR_BOTTOM", LV_DIR_BOTTOM);
    dump_constant("LV_DIR_HOR", LV_DIR_HOR);
    dump_constant("LV_DIR_VER", LV_DIR_VER);
    dump_constant("LV_DIR_ALL", LV_DIR_ALL);

    dump_constant("LV_KEY_UP", LV_KEY_UP);
    dump_constant("LV_KEY_DOWN", LV_KEY_DOWN);
    dump_constant("LV_KEY_RIGHT", LV_KEY_RIGHT);
    dump_constant("LV_KEY_LEFT", LV_KEY_LEFT);
    dump_constant("LV_KEY_ESC", LV_KEY_ESC);
    dump_constant("LV_KEY_DEL", LV_KEY_DEL);
    dump_constant("LV_KEY_BACKSPACE", LV_KEY_BACKSPACE);
    dump_constant("LV_KEY_ENTER", LV_KEY_ENTER);
    dump_constant("LV_KEY_NEXT", LV_KEY_NEXT);
    dump_constant("LV_KEY_PREV", LV_KEY_PREV);
    dump_constant("LV_KEY_HOME", LV_KEY_HOME);
    dump_constant("LV_KEY_END", LV_KEY_END);
    
    dump_constant("LV_ANIM_OFF", LV_ANIM_OFF);
    dump_constant("LV_ANIM_ON", LV_ANIM_ON);
    dump_constant("LV_ANIM_REPEAT_INFINITE", LV_ANIM_REPEAT_INFINITE);

    dump_constant("LV_SCROLL_SNAP_NONE", LV_SCROLL_SNAP_NONE);
    dump_constant("LV_SCROLL_SNAP_START", LV_SCROLL_SNAP_START);
    dump_constant("LV_SCROLL_SNAP_END", LV_SCROLL_SNAP_END);
    dump_constant("LV_SCROLL_SNAP_CENTER", LV_SCROLL_SNAP_CENTER);
    
    dump_flag("HIDDEN", LV_OBJ_FLAG_HIDDEN, "Make the object hidden. (Like it wasn't there at all)");
    dump_flag("CLICKABLE", LV_OBJ_FLAG_CLICKABLE, "Make the object clickable by the input devices");
    dump_flag("CLICK_FOCUSABLE", LV_OBJ_FLAG_CLICK_FOCUSABLE, "Add focused state to the object when clicked");
    dump_flag("CHECKABLE", LV_OBJ_FLAG_CHECKABLE, "Toggle checked state when the object is clicked");
    dump_flag("SCROLLABLE", LV_OBJ_FLAG_SCROLLABLE, "Make the object scrollable");
    dump_flag("SCROLL_ELASTIC", LV_OBJ_FLAG_SCROLL_ELASTIC, "Allow scrolling inside but with slower speed");
    dump_flag("SCROLL_MOMENTUM", LV_OBJ_FLAG_SCROLL_MOMENTUM, "Make the object scroll further when \\\"thrown\\\"");
    dump_flag("SCROLL_ONE", LV_OBJ_FLAG_SCROLL_ONE, "Allow scrolling only one snappable children");
    dump_flag("SCROLL_CHAIN_HOR", LV_OBJ_FLAG_SCROLL_CHAIN_HOR, "Allow propagating the horizontal scroll to a parent");
    dump_flag("SCROLL_CHAIN_VER", LV_OBJ_FLAG_SCROLL_CHAIN_VER, "Allow propagating the vertical scroll to a parent");
    dump_flag("SCROLL_ON_FOCUS", LV_OBJ_FLAG_SCROLL_ON_FOCUS, "Automatically scroll object to make it visible when focused");
    dump_flag("SCROLL_WITH_ARROW", LV_OBJ_FLAG_SCROLL_WITH_ARROW, "Allow scrolling the focused object with arrow keys");
    dump_flag("SNAPPABLE", LV_OBJ_FLAG_SNAPPABLE, "If scroll snap is enabled on the parent it can snap to this object");
    dump_flag("PRESS_LOCK", LV_OBJ_FLAG_PRESS_LOCK, "Keep the object pressed even if the press slid from the object");
    dump_flag("EVENT_BUBBLE", LV_OBJ_FLAG_EVENT_BUBBLE, "Propagate the events to the parent too");
    dump_flag("GESTURE_BUBBLE", LV_OBJ_FLAG_GESTURE_BUBBLE, "Propagate the gestures to the parent");
    dump_flag("ADV_HITTEST", LV_OBJ_FLAG_ADV_HITTEST, "Allow performing more accurate hit (click) test. E.g. consider rounded corners.");
    dump_flag("IGNORE_LAYOUT", LV_OBJ_FLAG_IGNORE_LAYOUT, "Make the object position-able by the layouts");
    dump_flag("FLOATING", LV_OBJ_FLAG_FLOATING, "Do not scroll the object when the parent scrolls and ignore layout");
    dump_flag("OVERFLOW_VISIBLE", LV_OBJ_FLAG_OVERFLOW_VISIBLE, "not clip the children's content to the parent's boundary*/");

    dump_constant_hex("LV_STATE_DEFAULT", LV_STATE_DEFAULT, 4);
    dump_constant_hex("LV_STATE_CHECKED", LV_STATE_CHECKED, 4);
    dump_constant_hex("LV_STATE_FOCUSED", LV_STATE_FOCUSED, 4);
    dump_constant_hex("LV_STATE_FOCUS_KEY", LV_STATE_FOCUS_KEY, 4);
    dump_constant_hex("LV_STATE_EDITED", LV_STATE_EDITED, 4);
    dump_constant_hex("LV_STATE_HOVERED", LV_STATE_HOVERED, 4);
    dump_constant_hex("LV_STATE_PRESSED", LV_STATE_PRESSED, 4);
    dump_constant_hex("LV_STATE_SCROLLED", LV_STATE_SCROLLED, 4);
    dump_constant_hex("LV_STATE_DISABLED", LV_STATE_DISABLED, 4);
    dump_constant_hex("LV_STATE_USER_1", LV_STATE_USER_1, 4);
    dump_constant_hex("LV_STATE_USER_2", LV_STATE_USER_2, 4);
    dump_constant_hex("LV_STATE_USER_3", LV_STATE_USER_3, 4);
    dump_constant_hex("LV_STATE_USER_4", LV_STATE_USER_4, 4);
    dump_constant_hex("LV_STATE_ANY", LV_STATE_ANY, 4);

    dump_constant_hex("LV_PART_MAIN", LV_PART_MAIN, 6);
    dump_constant_hex("LV_PART_SCROLLBAR", LV_PART_SCROLLBAR, 6);
    dump_constant_hex("LV_PART_INDICATOR", LV_PART_INDICATOR, 6);
    dump_constant_hex("LV_PART_KNOB", LV_PART_KNOB, 6);
    dump_constant_hex("LV_PART_SELECTED", LV_PART_SELECTED, 6);
    dump_constant_hex("LV_PART_ITEMS", LV_PART_ITEMS, 6);
#if LVGL_VERSION_MAJOR >= 9
    dump_constant_undefined("LV_PART_TICKS");
#else
    dump_constant_hex("LV_PART_TICKS", LV_PART_TICKS, 6);
#endif
    dump_constant_hex("LV_PART_CURSOR", LV_PART_CURSOR, 6);
    dump_constant_hex("LV_PART_CUSTOM_FIRST", LV_PART_CUSTOM_FIRST, 6);
    dump_constant_hex("LV_PART_TEXTAREA_PLACEHOLDER", LV_PART_TEXTAREA_PLACEHOLDER, 6);
    dump_constant_hex("LV_PART_ANY", LV_PART_ANY, 6);

    dump_constant("LV_EVENT_PRESSED", LV_EVENT_PRESSED);
    dump_constant("LV_EVENT_PRESSING", LV_EVENT_PRESSING);
    dump_constant("LV_EVENT_PRESS_LOST", LV_EVENT_PRESS_LOST);
    dump_constant("LV_EVENT_SHORT_CLICKED", LV_EVENT_SHORT_CLICKED);
#if LVGL_VERSION_MAJOR >= 9 && LVGL_VERSION_MINOR >= 3
    dump_constant("LV_EVENT_SINGLE_CLICKED", LV_EVENT_SINGLE_CLICKED);
    dump_constant("LV_EVENT_DOUBLE_CLICKED", LV_EVENT_DOUBLE_CLICKED);
    dump_constant("LV_EVENT_TRIPLE_CLICKED", LV_EVENT_TRIPLE_CLICKED);
#else
    dump_constant_undefined("LV_EVENT_SINGLE_CLICKED");
    dump_constant_undefined("LV_EVENT_DOUBLE_CLICKED");
    dump_constant_undefined("LV_EVENT_TRIPLE_CLICKED");
#endif
    dump_constant("LV_EVENT_LONG_PRESSED", LV_EVENT_LONG_PRESSED);
    dump_constant("LV_EVENT_LONG_PRESSED_REPEAT", LV_EVENT_LONG_PRESSED_REPEAT);
    dump_constant("LV_EVENT_CLICKED", LV_EVENT_CLICKED);
    dump_constant("LV_EVENT_RELEASED", LV_EVENT_RELEASED);
    dump_constant("LV_EVENT_SCROLL_BEGIN", LV_EVENT_SCROLL_BEGIN);
#if LVGL_VERSION_MAJOR >= 9
    dump_constant("LV_EVENT_SCROLL_THROW_BEGIN", LV_EVENT_SCROLL_THROW_BEGIN);
#else
    dump_constant_undefined("LV_EVENT_SCROLL_THROW_BEGIN");
#endif
    dump_constant("LV_EVENT_SCROLL_END", LV_EVENT_SCROLL_END);
    dump_constant("LV_EVENT_SCROLL", LV_EVENT_SCROLL);
    dump_constant("LV_EVENT_GESTURE", LV_EVENT_GESTURE);
    dump_constant("LV_EVENT_KEY", LV_EVENT_KEY);
#if LVGL_VERSION_MAJOR >= 9
    dump_constant("LV_EVENT_ROTARY", LV_EVENT_ROTARY);
#else
    dump_constant_undefined("LV_EVENT_ROTARY");
#endif
    dump_constant("LV_EVENT_FOCUSED", LV_EVENT_FOCUSED);
    dump_constant("LV_EVENT_DEFOCUSED", LV_EVENT_DEFOCUSED);
    dump_constant("LV_EVENT_LEAVE", LV_EVENT_LEAVE);
    dump_constant("LV_EVENT_HIT_TEST", LV_EVENT_HIT_TEST);
#if LVGL_VERSION_MAJOR >= 9
    dump_constant("LV_EVENT_INDEV_RESET", LV_EVENT_INDEV_RESET);
    dump_constant("LV_EVENT_HOVER_OVER", LV_EVENT_HOVER_OVER);
    dump_constant("LV_EVENT_HOVER_LEAVE", LV_EVENT_HOVER_LEAVE);
#else
    dump_constant_undefined("LV_EVENT_INDEV_RESET");
    dump_constant_undefined("LV_EVENT_HOVER_OVER");
    dump_constant_undefined("LV_EVENT_HOVER_LEAVE");
#endif
    dump_constant("LV_EVENT_COVER_CHECK", LV_EVENT_COVER_CHECK);
    dump_constant("LV_EVENT_REFR_EXT_DRAW_SIZE", LV_EVENT_REFR_EXT_DRAW_SIZE);
    dump_constant("LV_EVENT_DRAW_MAIN_BEGIN", LV_EVENT_DRAW_MAIN_BEGIN);
    dump_constant("LV_EVENT_DRAW_MAIN", LV_EVENT_DRAW_MAIN);
    dump_constant("LV_EVENT_DRAW_MAIN_END", LV_EVENT_DRAW_MAIN_END);
    dump_constant("LV_EVENT_DRAW_POST_BEGIN", LV_EVENT_DRAW_POST_BEGIN);
    dump_constant("LV_EVENT_DRAW_POST", LV_EVENT_DRAW_POST);
    dump_constant("LV_EVENT_DRAW_POST_END", LV_EVENT_DRAW_POST_END);
#if LVGL_VERSION_MAJOR >= 9
    dump_constant("LV_EVENT_DRAW_TASK_ADDED", LV_EVENT_DRAW_TASK_ADDED);
    dump_constant_undefined("LV_EVENT_DRAW_PART_BEGIN");
    dump_constant_undefined("LV_EVENT_DRAW_PART_END");
#else
    dump_constant_undefined("LV_EVENT_DRAW_TASK_ADDED");
    dump_constant("LV_EVENT_DRAW_PART_BEGIN", LV_EVENT_DRAW_PART_BEGIN);
    dump_constant("LV_EVENT_DRAW_PART_END", LV_EVENT_DRAW_PART_END);
#endif
    dump_constant("LV_EVENT_VALUE_CHANGED", LV_EVENT_VALUE_CHANGED);
    dump_constant("LV_EVENT_INSERT", LV_EVENT_INSERT);
    dump_constant("LV_EVENT_REFRESH", LV_EVENT_REFRESH);
    dump_constant("LV_EVENT_READY", LV_EVENT_READY);
    dump_constant("LV_EVENT_CANCEL", LV_EVENT_CANCEL);
#if LVGL_VERSION_MAJOR >= 9
    dump_constant("LV_EVENT_CREATE", LV_EVENT_CREATE);
#else
    dump_constant_undefined("LV_EVENT_CREATE");
#endif
    dump_constant("LV_EVENT_DELETE", LV_EVENT_DELETE);
    dump_constant("LV_EVENT_CHILD_CHANGED", LV_EVENT_CHILD_CHANGED);
    dump_constant("LV_EVENT_CHILD_CREATED", LV_EVENT_CHILD_CREATED);
    dump_constant("LV_EVENT_CHILD_DELETED", LV_EVENT_CHILD_DELETED);
    dump_constant("LV_EVENT_SCREEN_UNLOAD_START", LV_EVENT_SCREEN_UNLOAD_START);
    dump_constant("LV_EVENT_SCREEN_LOAD_START", LV_EVENT_SCREEN_LOAD_START);
    dump_constant("LV_EVENT_SCREEN_LOADED", LV_EVENT_SCREEN_LOADED);
    dump_constant("LV_EVENT_SCREEN_UNLOADED", LV_EVENT_SCREEN_UNLOADED);
    dump_constant("LV_EVENT_SIZE_CHANGED", LV_EVENT_SIZE_CHANGED);
    dump_constant("LV_EVENT_STYLE_CHANGED", LV_EVENT_STYLE_CHANGED);
    dump_constant("LV_EVENT_LAYOUT_CHANGED", LV_EVENT_LAYOUT_CHANGED);
    dump_constant("LV_EVENT_GET_SELF_SIZE", LV_EVENT_GET_SELF_SIZE);
#if LVGL_VERSION_MAJOR >= 9
    dump_constant("LV_EVENT_INVALIDATE_AREA", LV_EVENT_INVALIDATE_AREA);
    dump_constant("LV_EVENT_RESOLUTION_CHANGED", LV_EVENT_RESOLUTION_CHANGED);
    dump_constant("LV_EVENT_COLOR_FORMAT_CHANGED", LV_EVENT_COLOR_FORMAT_CHANGED);
    dump_constant("LV_EVENT_REFR_REQUEST", LV_EVENT_REFR_REQUEST);
    dump_constant("LV_EVENT_REFR_START", LV_EVENT_REFR_START);
    dump_constant("LV_EVENT_REFR_READY", LV_EVENT_REFR_READY);
    dump_constant("LV_EVENT_RENDER_START", LV_EVENT_RENDER_START);
    dump_constant("LV_EVENT_RENDER_READY", LV_EVENT_RENDER_READY);
    dump_constant("LV_EVENT_FLUSH_START", LV_EVENT_FLUSH_START);
    dump_constant("LV_EVENT_FLUSH_FINISH", LV_EVENT_FLUSH_FINISH);
    dump_constant("LV_EVENT_FLUSH_WAIT_START", LV_EVENT_FLUSH_WAIT_START);
    dump_constant("LV_EVENT_FLUSH_WAIT_FINISH", LV_EVENT_FLUSH_WAIT_FINISH);
    dump_constant("LV_EVENT_VSYNC", LV_EVENT_VSYNC);
#else
    dump_constant_undefined("LV_EVENT_INVALIDATE_AREA");
    dump_constant_undefined("LV_EVENT_RESOLUTION_CHANGED");
    dump_constant_undefined("LV_EVENT_COLOR_FORMAT_CHANGED");
    dump_constant_undefined("LV_EVENT_REFR_REQUEST");
    dump_constant_undefined("LV_EVENT_REFR_START");
    dump_constant_undefined("LV_EVENT_REFR_READY");
    dump_constant_undefined("LV_EVENT_RENDER_START");
    dump_constant_undefined("LV_EVENT_RENDER_READY");
    dump_constant_undefined("LV_EVENT_FLUSH_START");
    dump_constant_undefined("LV_EVENT_FLUSH_FINISH");
    dump_constant_undefined("LV_EVENT_FLUSH_WAIT_START");
    dump_constant_undefined("LV_EVENT_FLUSH_WAIT_FINISH");
    dump_constant_undefined("LV_EVENT_VSYNC");
#endif

#if LVGL_VERSION_MAJOR >= 9
    dump_constant("LV_IMG_CF_RAW", LV_COLOR_FORMAT_RAW);
    dump_constant("LV_IMG_CF_RAW_ALPHA", LV_COLOR_FORMAT_RAW_ALPHA);
    dump_constant_undefined("LV_IMG_CF_RAW_CHROMA");
    dump_constant("LV_IMG_CF_INDEXED_1_BIT", LV_COLOR_FORMAT_I1);
    dump_constant("LV_IMG_CF_INDEXED_2_BIT", LV_COLOR_FORMAT_I2);
    dump_constant("LV_IMG_CF_INDEXED_4_BIT", LV_COLOR_FORMAT_I4);
    dump_constant("LV_IMG_CF_INDEXED_8_BIT", LV_COLOR_FORMAT_I8);
    dump_constant("LV_IMG_CF_ALPHA_1_BIT", LV_COLOR_FORMAT_A1);
    dump_constant("LV_IMG_CF_ALPHA_2_BIT", LV_COLOR_FORMAT_A2);
    dump_constant("LV_IMG_CF_ALPHA_4_BIT", LV_COLOR_FORMAT_A4);
    dump_constant("LV_IMG_CF_ALPHA_8_BIT", LV_COLOR_FORMAT_A8);
    dump_constant("LV_IMG_CF_L8", LV_COLOR_FORMAT_L8);
    dump_constant("LV_IMG_CF_RGB565", LV_COLOR_FORMAT_RGB565);
    dump_constant("LV_IMG_CF_RGB565A8", LV_COLOR_FORMAT_RGB565A8);
    dump_constant("LV_IMG_CF_TRUE_COLOR", LV_COLOR_FORMAT_RGB888);
    dump_constant("LV_IMG_CF_TRUE_COLOR_ALPHA", LV_COLOR_FORMAT_ARGB8888);
    dump_constant("LV_IMG_CF_TRUE_COLOR_CHROMA", LV_COLOR_FORMAT_XRGB8888);
#else
    dump_constant("LV_IMG_CF_RAW", LV_IMG_CF_RAW);
    dump_constant("LV_IMG_CF_RAW_ALPHA", LV_IMG_CF_RAW_ALPHA);
    dump_constant("LV_IMG_CF_RAW_CHROMA", LV_IMG_CF_RAW_CHROMA_KEYED);
    dump_constant("LV_IMG_CF_INDEXED_1_BIT", LV_IMG_CF_INDEXED_1BIT);
    dump_constant("LV_IMG_CF_INDEXED_2_BIT", LV_IMG_CF_INDEXED_2BIT);
    dump_constant("LV_IMG_CF_INDEXED_4_BIT", LV_IMG_CF_INDEXED_4BIT);
    dump_constant("LV_IMG_CF_INDEXED_8_BIT", LV_IMG_CF_INDEXED_8BIT);
    dump_constant("LV_IMG_CF_ALPHA_1_BIT", LV_IMG_CF_ALPHA_1BIT);
    dump_constant("LV_IMG_CF_ALPHA_2_BIT", LV_IMG_CF_ALPHA_2BIT);
    dump_constant("LV_IMG_CF_ALPHA_4_BIT", LV_IMG_CF_ALPHA_4BIT);
    dump_constant("LV_IMG_CF_ALPHA_8_BIT", LV_IMG_CF_ALPHA_8BIT);
    dump_constant_undefined("LV_IMG_CF_L8");
    dump_constant("LV_IMG_CF_RGB565", LV_IMG_CF_RGB565);
    dump_constant("LV_IMG_CF_RGB565A8", LV_IMG_CF_RGB565A8);
    dump_constant("LV_IMG_CF_TRUE_COLOR", LV_IMG_CF_TRUE_COLOR);
    dump_constant("LV_IMG_CF_TRUE_COLOR_ALPHA", LV_IMG_CF_TRUE_COLOR_ALPHA);
    dump_constant("LV_IMG_CF_TRUE_COLOR_CHROMA", LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED);
#endif

    dump_constant("LV_LABEL_LONG_WRAP", LV_LABEL_LONG_WRAP);
    dump_constant("LV_LABEL_LONG_DOT", LV_LABEL_LONG_DOT);
    dump_constant("LV_LABEL_LONG_SCROLL", LV_LABEL_LONG_SCROLL);
    dump_constant("LV_LABEL_LONG_SCROLL_CIRCULAR", LV_LABEL_LONG_SCROLL_CIRCULAR);
    dump_constant("LV_LABEL_LONG_CLIP", LV_LABEL_LONG_CLIP);

    dump_constant("LV_SLIDER_MODE_NORMAL", LV_SLIDER_MODE_NORMAL);
    dump_constant("LV_SLIDER_MODE_SYMMETRICAL", LV_SLIDER_MODE_SYMMETRICAL);
    dump_constant("LV_SLIDER_MODE_RANGE", LV_SLIDER_MODE_RANGE);

    dump_constant("LV_ROLLER_MODE_NORMAL", LV_ROLLER_MODE_NORMAL);
    dump_constant("LV_ROLLER_MODE_INFINITE", LV_ROLLER_MODE_INFINITE);
    
    dump_constant("LV_BAR_MODE_NORMAL", LV_BAR_MODE_NORMAL);
    dump_constant("LV_BAR_MODE_SYMMETRICAL", LV_BAR_MODE_SYMMETRICAL);
    dump_constant("LV_BAR_MODE_RANGE", LV_BAR_MODE_RANGE);
    
    dump_constant("LV_ARC_MODE_NORMAL", LV_ARC_MODE_NORMAL);
    dump_constant("LV_ARC_MODE_SYMMETRICAL", LV_ARC_MODE_SYMMETRICAL);
    dump_constant("LV_ARC_MODE_REVERSE", LV_ARC_MODE_REVERSE);

#if LVGL_VERSION_MAJOR >= 9
    dump_constant_undefined("LV_COLORWHEEL_MODE_HUE");
    dump_constant_undefined("LV_COLORWHEEL_MODE_SATURATION");
    dump_constant_undefined("LV_COLORWHEEL_MODE_VALUE");
#else
    dump_constant("LV_COLORWHEEL_MODE_HUE", LV_COLORWHEEL_MODE_HUE);
    dump_constant("LV_COLORWHEEL_MODE_SATURATION", LV_COLORWHEEL_MODE_SATURATION);
    dump_constant("LV_COLORWHEEL_MODE_VALUE", LV_COLORWHEEL_MODE_VALUE);
#endif
    
#if LVGL_VERSION_MAJOR >= 9
    dump_constant("LV_IMAGEBUTTON_STATE_RELEASED", LV_IMAGEBUTTON_STATE_RELEASED);
    dump_constant("LV_IMAGEBUTTON_STATE_PRESSED", LV_IMAGEBUTTON_STATE_PRESSED);
    dump_constant("LV_IMAGEBUTTON_STATE_DISABLED", LV_IMAGEBUTTON_STATE_DISABLED);
    dump_constant("LV_IMAGEBUTTON_STATE_CHECKED_RELEASED", LV_IMAGEBUTTON_STATE_CHECKED_RELEASED);
    dump_constant("LV_IMAGEBUTTON_STATE_CHECKED_PRESSED", LV_IMAGEBUTTON_STATE_CHECKED_PRESSED);
    dump_constant("LV_IMAGEBUTTON_STATE_CHECKED_DISABLED", LV_IMAGEBUTTON_STATE_CHECKED_DISABLED);
#else
    dump_constant("LV_IMGBTN_STATE_RELEASED", LV_IMGBTN_STATE_RELEASED);
    dump_constant("LV_IMGBTN_STATE_PRESSED", LV_IMGBTN_STATE_PRESSED);
    dump_constant("LV_IMGBTN_STATE_DISABLED", LV_IMGBTN_STATE_DISABLED);
    dump_constant("LV_IMGBTN_STATE_CHECKED_RELEASED", LV_IMGBTN_STATE_CHECKED_RELEASED);
    dump_constant("LV_IMGBTN_STATE_CHECKED_PRESSED", LV_IMGBTN_STATE_CHECKED_PRESSED);
    dump_constant("LV_IMGBTN_STATE_CHECKED_DISABLED", LV_IMGBTN_STATE_CHECKED_DISABLED);
#endif

    dump_constant("LV_KEYBOARD_MODE_TEXT_LOWER", LV_KEYBOARD_MODE_TEXT_LOWER);
    dump_constant("LV_KEYBOARD_MODE_TEXT_UPPER", LV_KEYBOARD_MODE_TEXT_UPPER);
    dump_constant("LV_KEYBOARD_MODE_SPECIAL", LV_KEYBOARD_MODE_SPECIAL);
    dump_constant("LV_KEYBOARD_MODE_NUMBER", LV_KEYBOARD_MODE_NUMBER);
    dump_constant("LV_KEYBOARD_MODE_USER_1", LV_KEYBOARD_MODE_USER_1);
    dump_constant("LV_KEYBOARD_MODE_USER_2", LV_KEYBOARD_MODE_USER_2);
    dump_constant("LV_KEYBOARD_MODE_USER_3", LV_KEYBOARD_MODE_USER_3);
    dump_constant("LV_KEYBOARD_MODE_USER_4", LV_KEYBOARD_MODE_USER_4);
    
#if LVGL_VERSION_MAJOR >= 9    
    dump_constant("LV_SCALE_MODE_HORIZONTAL_TOP", LV_SCALE_MODE_HORIZONTAL_TOP);
    dump_constant("LV_SCALE_MODE_HORIZONTAL_BOTTOM", LV_SCALE_MODE_HORIZONTAL_BOTTOM);
    dump_constant("LV_SCALE_MODE_VERTICAL_LEFT", LV_SCALE_MODE_VERTICAL_LEFT);
    dump_constant("LV_SCALE_MODE_VERTICAL_RIGHT", LV_SCALE_MODE_VERTICAL_RIGHT);
    dump_constant("LV_SCALE_MODE_ROUND_INNER", LV_SCALE_MODE_ROUND_INNER);
    dump_constant("LV_SCALE_MODE_ROUND_OUTER", LV_SCALE_MODE_ROUND_OUTER);
#else
    dump_constant_undefined("LV_SCALE_MODE_HORIZONTAL_TOP");
    dump_constant_undefined("LV_SCALE_MODE_HORIZONTAL_BOTTOM");
    dump_constant_undefined("LV_SCALE_MODE_VERTICAL_LEFT");
    dump_constant_undefined("LV_SCALE_MODE_VERTICAL_RIGHT");
    dump_constant_undefined("LV_SCALE_MODE_ROUND_INNER");
    dump_constant_undefined("LV_SCALE_MODE_ROUND_OUTER");
#endif

#if LVGL_VERSION_MAJOR >= 9
    dump_constant("LV_IMAGE_ALIGN_DEFAULT", LV_IMAGE_ALIGN_DEFAULT);
    dump_constant("LV_IMAGE_ALIGN_TOP_LEFT", LV_IMAGE_ALIGN_TOP_LEFT);
    dump_constant("LV_IMAGE_ALIGN_TOP_MID", LV_IMAGE_ALIGN_TOP_MID);
    dump_constant("LV_IMAGE_ALIGN_TOP_RIGHT", LV_IMAGE_ALIGN_TOP_RIGHT);
    dump_constant("LV_IMAGE_ALIGN_BOTTOM_LEFT", LV_IMAGE_ALIGN_BOTTOM_LEFT);
    dump_constant("LV_IMAGE_ALIGN_BOTTOM_MID", LV_IMAGE_ALIGN_BOTTOM_MID);
    dump_constant("LV_IMAGE_ALIGN_BOTTOM_RIGHT", LV_IMAGE_ALIGN_BOTTOM_RIGHT);
    dump_constant("LV_IMAGE_ALIGN_LEFT_MID", LV_IMAGE_ALIGN_LEFT_MID);
    dump_constant("LV_IMAGE_ALIGN_RIGHT_MID", LV_IMAGE_ALIGN_RIGHT_MID);
    dump_constant("LV_IMAGE_ALIGN_CENTER", LV_IMAGE_ALIGN_CENTER);
#if LVGL_VERSION_MINOR >= 4
    dump_constant("_LV_IMAGE_ALIGN_AUTO_TRANSFORM", _LV_IMAGE_ALIGN_AUTO_TRANSFORM);
#else
    dump_constant("LV_IMAGE_ALIGN_AUTO_TRANSFORM", LV_IMAGE_ALIGN_AUTO_TRANSFORM);
#endif
    dump_constant("LV_IMAGE_ALIGN_STRETCH", LV_IMAGE_ALIGN_STRETCH);
    dump_constant("LV_IMAGE_ALIGN_TILE", LV_IMAGE_ALIGN_TILE);
#else
    dump_constant_undefined("LV_IMAGE_ALIGN_DEFAULT");
    dump_constant_undefined("LV_IMAGE_ALIGN_TOP_LEFT");
    dump_constant_undefined("LV_IMAGE_ALIGN_TOP_MID");
    dump_constant_undefined("LV_IMAGE_ALIGN_TOP_RIGHT");
    dump_constant_undefined("LV_IMAGE_ALIGN_BOTTOM_LEFT");
    dump_constant_undefined("LV_IMAGE_ALIGN_BOTTOM_MID");
    dump_constant_undefined("LV_IMAGE_ALIGN_BOTTOM_RIGHT");
    dump_constant_undefined("LV_IMAGE_ALIGN_LEFT_MID");
    dump_constant_undefined("LV_IMAGE_ALIGN_RIGHT_MID");
    dump_constant_undefined("LV_IMAGE_ALIGN_CENTER");
    dump_constant_undefined("LV_IMAGE_ALIGN_AUTO_TRANSFORM");
    dump_constant_undefined("LV_IMAGE_ALIGN_STRETCH");
    dump_constant_undefined("LV_IMAGE_ALIGN_TILE");
#endif

    dump_constant("LV_SCR_LOAD_ANIM_NONE", LV_SCR_LOAD_ANIM_NONE);
    dump_constant("LV_SCR_LOAD_ANIM_OVER_LEFT", LV_SCR_LOAD_ANIM_OVER_LEFT);
    dump_constant("LV_SCR_LOAD_ANIM_OVER_RIGHT", LV_SCR_LOAD_ANIM_OVER_RIGHT);
    dump_constant("LV_SCR_LOAD_ANIM_OVER_TOP", LV_SCR_LOAD_ANIM_OVER_TOP);
    dump_constant("LV_SCR_LOAD_ANIM_OVER_BOTTOM", LV_SCR_LOAD_ANIM_OVER_BOTTOM);
    dump_constant("LV_SCR_LOAD_ANIM_MOVE_LEFT", LV_SCR_LOAD_ANIM_MOVE_LEFT);
    dump_constant("LV_SCR_LOAD_ANIM_MOVE_RIGHT", LV_SCR_LOAD_ANIM_MOVE_RIGHT);
    dump_constant("LV_SCR_LOAD_ANIM_MOVE_TOP", LV_SCR_LOAD_ANIM_MOVE_TOP);
    dump_constant("LV_SCR_LOAD_ANIM_MOVE_BOTTOM", LV_SCR_LOAD_ANIM_MOVE_BOTTOM);
    dump_constant("LV_SCR_LOAD_ANIM_FADE_IN", LV_SCR_LOAD_ANIM_FADE_IN);
    dump_constant("LV_SCR_LOAD_ANIM_FADE_OUT", LV_SCR_LOAD_ANIM_FADE_OUT);
    dump_constant("LV_SCR_LOAD_ANIM_OUT_LEFT", LV_SCR_LOAD_ANIM_OUT_LEFT);
    dump_constant("LV_SCR_LOAD_ANIM_OUT_RIGHT", LV_SCR_LOAD_ANIM_OUT_RIGHT);
    dump_constant("LV_SCR_LOAD_ANIM_OUT_TOP", LV_SCR_LOAD_ANIM_OUT_TOP);
    dump_constant("LV_SCR_LOAD_ANIM_OUT_BOTTOM", LV_SCR_LOAD_ANIM_OUT_BOTTOM);

#if LVGL_VERSION_MAJOR >= 9
    dump_constant_hex("LV_BUTTONMATRIX_CTRL_HIDDEN", LV_BUTTONMATRIX_CTRL_HIDDEN, 4);
    dump_constant_hex("LV_BUTTONMATRIX_CTRL_NO_REPEAT", LV_BUTTONMATRIX_CTRL_NO_REPEAT, 4);
    dump_constant_hex("LV_BUTTONMATRIX_CTRL_DISABLED", LV_BUTTONMATRIX_CTRL_DISABLED, 4);
    dump_constant_hex("LV_BUTTONMATRIX_CTRL_CHECKABLE", LV_BUTTONMATRIX_CTRL_CHECKABLE, 4);
    dump_constant_hex("LV_BUTTONMATRIX_CTRL_CHECKED", LV_BUTTONMATRIX_CTRL_CHECKED, 4);
    dump_constant_hex("LV_BUTTONMATRIX_CTRL_CLICK_TRIG", LV_BUTTONMATRIX_CTRL_CLICK_TRIG, 4);
    dump_constant_hex("LV_BUTTONMATRIX_CTRL_POPOVER", LV_BUTTONMATRIX_CTRL_POPOVER, 4);
#if LVGL_VERSION_MINOR >= 3
    dump_constant_hex("LV_BUTTONMATRIX_CTRL_RECOLOR", LV_BUTTONMATRIX_CTRL_RECOLOR, 4);
#else
    dump_constant_undefined("LV_BUTTONMATRIX_CTRL_RECOLOR");
#endif
    dump_constant_hex("LV_BUTTONMATRIX_CTRL_CUSTOM_1", LV_BUTTONMATRIX_CTRL_CUSTOM_1, 4);
    dump_constant_hex("LV_BUTTONMATRIX_CTRL_CUSTOM_2", LV_BUTTONMATRIX_CTRL_CUSTOM_2, 4);
#else
    dump_constant_hex("LV_BTNMATRIX_CTRL_HIDDEN", LV_BTNMATRIX_CTRL_HIDDEN, 4);
    dump_constant_hex("LV_BTNMATRIX_CTRL_NO_REPEAT", LV_BTNMATRIX_CTRL_NO_REPEAT, 4);
    dump_constant_hex("LV_BTNMATRIX_CTRL_DISABLED", LV_BTNMATRIX_CTRL_DISABLED, 4);
    dump_constant_hex("LV_BTNMATRIX_CTRL_CHECKABLE", LV_BTNMATRIX_CTRL_CHECKABLE, 4);
    dump_constant_hex("LV_BTNMATRIX_CTRL_CHECKED", LV_BTNMATRIX_CTRL_CHECKED, 4);
    dump_constant_hex("LV_BTNMATRIX_CTRL_CLICK_TRIG", LV_BTNMATRIX_CTRL_CLICK_TRIG, 4);
    dump_constant_hex("LV_BTNMATRIX_CTRL_POPOVER", LV_BTNMATRIX_CTRL_POPOVER, 4);
    dump_constant_hex("LV_BTNMATRIX_CTRL_RECOLOR", LV_BTNMATRIX_CTRL_RECOLOR, 4);
    dump_constant_hex("LV_BTNMATRIX_CTRL_CUSTOM_1", LV_BTNMATRIX_CTRL_CUSTOM_1, 4);
    dump_constant_hex("LV_BTNMATRIX_CTRL_CUSTOM_2", LV_BTNMATRIX_CTRL_CUSTOM_2, 4);
#endif

#if LVGL_VERSION_MAJOR >= 9
    dump_constant("LV_COORD_TYPE_SHIFT", LV_COORD_TYPE_SHIFT);
    dump_constant("LV_COORD_TYPE_SPEC", LV_COORD_TYPE_SPEC);
#else
    dump_constant("LV_COORD_TYPE_SHIFT", _LV_COORD_TYPE_SHIFT);
    dump_constant("LV_COORD_TYPE_SPEC", _LV_COORD_TYPE_SPEC);
#endif
    dump_constant("LV_COORD_MAX", LV_COORD_MAX);
    dump_constant("LV_SIZE_CONTENT", LV_SIZE_CONTENT);
#if LVGL_VERSION_MAJOR >= 9
    dump_constant("LV_PCT_STORED_MAX", LV_PCT_STORED_MAX);
    dump_constant("LV_PCT_POS_MAX", LV_PCT_POS_MAX);
#else
    dump_constant_undefined("LV_PCT_STORED_MAX");
    dump_constant_undefined("LV_PCT_POS_MAX");
#endif

    dump_constant("LV_ALIGN_CENTER", LV_ALIGN_CENTER);

    dump_constant_last("dummy", 0);
}

EM_PORT_API(const char *) getStudioSymbols() {
    hor_res = 400;
    ver_res = 400;
    lv_init();
    hal_init(true);

    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);

    g_symbolsString.ptr = (char *)malloc(SYMBOLS_STRING_INIT_ALLOCATED);
    if (!g_symbolsString.ptr) {
        return "memory allocation error";
    }
 
    g_symbolsString.allocated = SYMBOLS_STRING_INIT_ALLOCATED;
    g_symbolsString.size = 0;
 
    g_symbolsString.ptr[0] = 0;

    symbols_append("{\"styles\":[");
    dump_custom_styles();
    symbols_append("],\"widget_flags\":{");
    dump_widgets_flags_info();
    symbols_append("},\"constants\":{");
    dump_constants();
    symbols_append("}}");

    return g_symbolsString.ptr;
}

/*
 * Compile-only function that references LVGL APIs to ensure
 * their symbols and signatures are validated across supported LVGL versions.
 * This function is intentionally unused and must NOT be invoked at runtime.
 */
static void __attribute__((used)) api_usage(void) {
    /* dummy arguments used only to ensure code compiles */
    lv_obj_t *obj = (lv_obj_t *)0;
    const char *str = "";
    lv_color_t color = lv_color_hex(0);
    int32_t iv = 0;
    uint32_t uiv = 0;
    bool b = false;
    lv_point_t pts[2] = {{0, 0}, {0, 0}};
#if LVGL_VERSION_MAJOR >= 9
    lv_point_precise_t pts_precise[2] = {{0, 0}, {0, 0}};
#endif
    lv_style_t style;
    lv_style_transition_dsc_t trans;
    lv_calendar_date_t date = {0};
    lv_style_value_t style_val;
    /* silence unused-variable warnings (compile-only references) */
    (void)iv; (void)uiv; (void)b; (void)pts; (void)style; (void)trans; (void)date; (void)style_val; (void)str;
#if LVGL_VERSION_MAJOR >= 9
    (void)pts_precise;
#endif
    lv_style_init(&style);

    /* Common calls across LVGL versions */
    lv_animimg_set_duration(obj, 0);
    lv_animimg_set_repeat_count(obj, 0);
    lv_animimg_set_src(obj, (const void *[]){NULL}, 0);
    lv_animimg_start(obj);
    lv_arc_set_bg_end_angle(obj, 0);
    lv_arc_set_bg_start_angle(obj, 0);
    lv_arc_set_mode(obj, (lv_arc_mode_t)0);
    lv_arc_set_range(obj, 0, 100);
    lv_arc_set_rotation(obj, 0);
    lv_arc_set_value(obj, 0);
    lv_bar_set_mode(obj, (lv_bar_mode_t)0);
    lv_bar_set_range(obj, 0, 100);
    lv_bar_set_start_value(obj, 0, (lv_anim_enable_t)0);
    lv_bar_set_value(obj, 0, (lv_anim_enable_t)0);

    const char *btn_map[] = {""};
    /* btnmatrix API name changed between versions */
#if LVGL_VERSION_MAJOR >= 9
    lv_buttonmatrix_set_map(obj, btn_map);
#else
    lv_btnmatrix_set_map(obj, btn_map);
#endif
    lv_btnmatrix_set_ctrl_map(obj, (const lv_btnmatrix_ctrl_t[]){0});
    lv_btnmatrix_set_one_checked(obj, true);
#if LVGL_VERSION_MAJOR >= 9
    lv_buttonmatrix_set_ctrl_map(obj, (const lv_buttonmatrix_ctrl_t[]){0});
    lv_buttonmatrix_set_one_checked(obj, true);
#endif

    lv_dropdown_set_dir(obj, (lv_dir_t)0);
    lv_dropdown_set_options(obj, "");
    lv_dropdown_set_selected(obj, 0);
    lv_dropdown_set_symbol(obj, "");
    (void)lv_event_get_code((lv_event_t *)0);
    (void)lv_event_get_user_data((lv_event_t *)0);
    lv_label_set_text(obj, "");
    lv_label_set_long_mode(obj, (lv_label_long_mode_t)0);
#if LVGL_VERSION_MAJOR >= 9
    (void)lv_color_to_32(color, LV_OPA_COVER);
#else
    (void)lv_color_to32(color);
#endif
    lv_led_set_brightness(obj, 0);
    (void)lv_led_get_brightness(obj);
    lv_led_set_color(obj, color);
    lv_obj_get_state(obj);
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 0, 0);
    lv_obj_update_layout(obj);

    /* qrcode: size setter exists only in LVGL 9+ */
#if LVGL_VERSION_MAJOR >= 9
    lv_qrcode_set_size(obj, 0);
#endif

    lv_spinbox_set_range(obj, 0, 100);
    lv_spinbox_set_step(obj, 1);
    lv_spinbox_set_digit_format(obj, 0, 0);
    lv_spinbox_set_rollover(obj, true);
    lv_spinbox_set_value(obj, 0);

    /* tabview_tab_bar_size only in LVGL 9+ */
#if LVGL_VERSION_MAJOR >= 9
    lv_tabview_set_tab_bar_size(obj, 0);
#endif

    lv_textarea_set_one_line(obj, true);
    lv_textarea_set_password_mode(obj, true);
    lv_textarea_set_placeholder_text(obj, "");
    lv_textarea_set_accepted_chars(obj, "");
    lv_textarea_set_max_length(obj, 0);
    lv_textarea_set_text(obj, "");

    /* roller requires a mode and anim params across versions */
    lv_roller_set_options(obj, "", (lv_roller_mode_t)0);
    lv_roller_set_selected(obj, 0, (lv_anim_enable_t)0);
    (void)lv_roller_get_option_cnt(obj);

    lv_slider_set_mode(obj, (lv_slider_mode_t)0);
    lv_slider_set_range(obj, 0, 100);
    lv_slider_set_left_value(obj, 0, (lv_anim_enable_t)0);
    lv_slider_set_value(obj, 0, (lv_anim_enable_t)0);
    /* forward declarations for functions provided in studio_api.cpp */
    extern uint32_t to_lvgl_color(uint32_t color);

    /* getters and value-returning functions */
    (void)lv_arc_get_max_value(obj);
    (void)lv_arc_get_min_value(obj);
    (void)lv_arc_get_value(obj);
    (void)lv_bar_get_start_value(obj);
    (void)lv_bar_get_value(obj);
    (void)lv_dropdown_get_options(obj);
    (void)lv_dropdown_get_selected(obj);
#if LVGL_VERSION_MAJOR >= 9
    (void)lv_event_get_draw_task((lv_event_t *)0);
#endif
    (void)lv_label_get_text(obj);
#if LV_USE_METER
    (void)lv_meter_add_needle_img(obj, (lv_meter_scale_t *)0, (const lv_img_dsc_t *)0, 0, 0);
    (void)lv_meter_add_needle_line(obj, (lv_meter_scale_t *)0, 0, lv_palette_main(LV_PALETTE_BLUE), 0);
    (void)lv_meter_add_scale_lines(obj, (lv_meter_scale_t *)0, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, 0);
    (void)lv_meter_add_arc(obj, (lv_meter_scale_t *)0, 0, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_meter_set_indicator_end_value(obj, (lv_meter_indicator_t *)0, 0);
    lv_meter_set_indicator_start_value(obj, (lv_meter_indicator_t *)0, 0);
    lv_meter_set_indicator_value(obj, (lv_meter_indicator_t *)0, 0);
#endif
    (void)lv_roller_get_options(obj);
    (void)lv_roller_get_selected(obj);
    (void)lv_slider_get_max_value(obj);
    (void)lv_slider_get_min_value(obj);
    (void)lv_slider_get_left_value(obj);
    (void)lv_spinbox_get_step(obj);
    (void)lv_spinbox_get_value(obj);
    (void)lv_textarea_get_max_length(obj);
    (void)lv_textarea_get_text(obj);

    /* inline / free functions */
    (void)lv_obj_get_parent(obj);
    (void)to_lvgl_color(0);
    lv_obj_add_event_cb(obj, NULL, LV_EVENT_ALL, NULL);
    lv_obj_add_flag(obj, (lv_obj_flag_t)0);
    lv_obj_add_state(obj, (lv_state_t)0);
    lv_obj_clear_flag(obj, (lv_obj_flag_t)0);
    lv_obj_clear_state(obj, (lv_state_t)0);
    lv_obj_has_flag(obj, (lv_obj_flag_t)0);
    lv_obj_has_state(obj, (lv_state_t)0);
    lv_obj_remove_style(obj, NULL, (lv_style_selector_t)0);
    lv_obj_set_scroll_dir(obj, (lv_dir_t)0);
    lv_obj_set_scroll_snap_x(obj, (lv_scroll_snap_t)0);
    lv_obj_set_scroll_snap_y(obj, (lv_scroll_snap_t)0);
    lv_obj_set_scrollbar_mode(obj, (lv_scrollbar_mode_t)0);

    /* other free functions with assignment */
    (void)lv_event_get_target((lv_event_t *)0);
    (void)lv_roller_get_selected(obj);
    (void)lv_textarea_get_text(obj);

    /* creation/getters */
#if LVGL_VERSION_MAJOR >= 9
    lv_buttonmatrix_create(obj);
#else
    lv_btnmatrix_create(obj);
#endif
    lv_btn_create(obj);
#if LVGL_VERSION_MAJOR >= 9
    lv_button_create(obj);
#endif
    (void)lv_animimg_create(obj);
    (void)lv_arc_create(obj);
    (void)lv_bar_create(obj);
    (void)lv_calendar_create(obj);
    (void)lv_calendar_header_arrow_create(obj);
    lv_calendar_set_showed_date(obj, 0, 0);
    lv_calendar_set_today_date(obj, 0, 0, 0);
    (void)lv_canvas_create(obj);
    (void)lv_chart_create(obj);
    (void)lv_checkbox_create(obj);
    lv_checkbox_set_text(obj, "");
    /* colorwheel exists only when enabled */
#if LV_USE_COLORWHEEL
    (void)lv_colorwheel_create(obj
#if LVGL_VERSION_MAJOR < 9
                                , false
#endif
    );
    lv_colorwheel_set_mode(obj, (lv_colorwheel_mode_t)0);
    lv_colorwheel_set_mode_fixed(obj, true);
#endif
    (void)lv_label_create(obj);
    (void)lv_keyboard_create(obj);
    (void)lv_led_create(obj);
    (void)lv_line_create(obj);
    lv_line_set_points(obj,
#if LVGL_VERSION_MAJOR >= 9
                       pts_precise,
#else
                       pts,
#endif
                       2);
    lv_line_set_y_invert(obj, true);
    (void)lv_list_create(obj);
#if LV_USE_LOTTIE
    (void)lv_lottie_create(obj);
#endif
    (void)lv_menu_create(obj);
#if LVGL_VERSION_MAJOR >= 9
    (void)lv_msgbox_create(obj);
#else
    (void)lv_msgbox_create(obj, "", "", (const char *[]){"",""}, false);
#endif
#if LV_USE_METER
    (void)lv_meter_create(obj);
    (void)lv_meter_add_scale(obj);
    lv_meter_set_scale_major_ticks(obj, (lv_meter_scale_t *)0, 0, 0, 0, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_meter_set_scale_range(obj, (lv_meter_scale_t *)0, 0, 0, 0, 0);
    lv_meter_set_scale_ticks(obj, (lv_meter_scale_t *)0, 0, 0, 0, lv_palette_main(LV_PALETTE_BLUE));
#endif
    (void)lv_obj_create(obj);
    lv_obj_add_style(obj, &style, (lv_style_selector_t)0);
    style_val = lv_obj_get_style_prop(obj, LV_PART_MAIN, (lv_style_prop_t)0);
    (void)style_val;
    lv_obj_set_local_style_prop(obj, (lv_style_prop_t)0, (lv_style_value_t){0}, (lv_style_selector_t)0);
    lv_obj_set_style_bg_color(obj, color, (lv_style_selector_t)0);
    lv_obj_set_style_border_width(obj, 0, (lv_style_selector_t)0);
    (void)lv_spangroup_create(obj);
    (void)lv_table_create(obj);
#if LVGL_VERSION_MAJOR >= 9
    (void)lv_tabview_create(obj);
#else
    (void)lv_tabview_create(obj, (lv_dir_t)0, 0);
#endif
    lv_tabview_set_act(obj, 0, (lv_anim_enable_t)0);
#if LVGL_VERSION_MAJOR >= 9
    lv_tabview_set_active(obj, 0, (lv_anim_enable_t)0);
    lv_tabview_set_tab_bar_position(obj, (lv_dir_t)0);
#endif
    (void)lv_tileview_create(obj);
#if LVGL_VERSION_MAJOR >= 9
    (void)lv_win_create(obj);
#else
    (void)lv_win_create(obj, 0);
#endif
    (void)lv_dropdown_create(obj);
    /* image* APIs are available in LVGL 9+ */
#if LVGL_VERSION_MAJOR >= 9
    (void)lv_image_create(obj);
    lv_image_set_inner_align(obj, (lv_image_align_t)0);
    lv_image_set_pivot(obj, 0, 0);
    lv_image_set_rotation(obj, 0);
    lv_image_set_scale(obj, 0);
    lv_image_set_src(obj, NULL);
    (void)lv_imagebutton_create(obj);
    lv_imagebutton_set_src(obj, 0, NULL, NULL, NULL);
#endif
    (void)lv_img_create(obj);
    lv_img_set_angle(obj, 0);
    lv_img_set_pivot(obj, 0, 0);
    lv_img_set_src(obj, NULL);
    lv_img_set_zoom(obj, 0);
#if LVGL_VERSION_MAJOR < 9
    (void)lv_imgbtn_create(obj);
    lv_imgbtn_set_src(obj, (lv_imgbtn_state_t)0, NULL, NULL, NULL);
#endif
    (void)lv_keyboard_create(obj);
    lv_keyboard_set_mode(obj, (lv_keyboard_mode_t)0);
    lv_keyboard_set_textarea(obj, (lv_obj_t *)0);
    lv_obj_add_flag(obj, (lv_obj_flag_t)0);
#if LVGL_VERSION_MAJOR >= 9
    (void)lv_qrcode_create(obj);
    lv_qrcode_set_dark_color(obj, color);
    lv_qrcode_set_light_color(obj, color);
    lv_qrcode_set_size(obj, 0);
    lv_qrcode_update(obj, "", 0);
#else
    (void)lv_qrcode_create(obj, 0, color, color);
    lv_qrcode_update(obj, "", 0);
#endif
    (void)lv_roller_create(obj);
#if LVGL_VERSION_MAJOR >= 9 && LV_USE_SCALE
    lv_scale_create(obj);
    lv_scale_set_label_show(obj, true);
    lv_scale_set_major_tick_every(obj, 0);
    lv_scale_set_mode(obj, (lv_scale_mode_t)0);
    lv_scale_set_range(obj, 0, 0);
    lv_scale_set_total_tick_count(obj, 0);
#endif
    (void)lv_slider_create(obj);
    lv_slider_set_left_value(obj, 0, (lv_anim_enable_t)0);
    lv_slider_set_value(obj, 0, (lv_anim_enable_t)0);
    (void)lv_spangroup_create(obj);
    (void)lv_spinbox_create(obj);
    lv_spinbox_set_value(obj, 0);
    (void)lv_spinner_create(obj
#if LVGL_VERSION_MAJOR < 9
                              , 0, 0
#endif
    );
#if LVGL_VERSION_MAJOR >= 9
    lv_spinner_set_anim_params(obj, 0, 0);
#endif
    (void)lv_table_create(obj);
    (void)lv_dropdown_get_list(obj);
    (void)lv_tabview_add_tab(obj, "");
}
