#include <emscripten.h>
#include <emscripten/html5.h>

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "eez-gui-lite.h"

#define EM_PORT_API(rettype) rettype EMSCRIPTEN_KEEPALIVE

////////////////////////////////////////////////////////////////////////////////
// JS-imported callbacks (implemented in JavaScript, called from C)
//
// These are declared as extern so that Emscripten links them to the
// corresponding JavaScript functions provided via mergeInto(LibraryManager).

extern const char *js_get_str_prop(uint16_t prop);
extern bool js_get_bool_prop(uint16_t prop);
extern int js_get_int_prop(uint16_t prop);
extern void js_on_event(const eezgui_widget_t *widget, int event_type);

////////////////////////////////////////////////////////////////////////////////
// Property callback wrappers
//
// These are the C functions stored in eezgui_ctx_t callback pointers.
// They forward calls to JavaScript.

static const char *wasm_get_str_prop(uint16_t prop) {
    return js_get_str_prop(prop);
}

static bool wasm_get_bool_prop(uint16_t prop) {
    return js_get_bool_prop(prop);
}

static int wasm_get_int_prop(uint16_t prop) {
    return js_get_int_prop(prop);
}

// Event callback wrapper — forwards widget events to JS
static void wasm_on_event(eezgui_event_info_t *event) {
    js_on_event(event->widget, (int)event->type);
}

////////////////////////////////////////////////////////////////////////////////
// State

#define EEZ_FLOW_CTX_STATE_BUFFER_SIZE 4096

static uint8_t eezgui_state_buffer[EEZ_FLOW_CTX_STATE_BUFFER_SIZE];
static eezgui_ctx_t eezgui_ctx;

// Currently active page data pointer (used to detect page changes)
static const void *current_page_data = NULL;

////////////////////////////////////////////////////////////////////////////////
// Core WASM API — init, mainLoop, getSyncedBuffer

/* Main loop — called from JS requestAnimationFrame */
EM_PORT_API(void) mainLoop(void) {
}

EM_PORT_API(int) initEezGuiLite(int display_width, int display_height) {
    eezgui_display_init(display_width, display_height);

    // Initialize GUI context
    memset(&eezgui_ctx, 0, sizeof(eezgui_ctx));

    // Initialize flow context
    eezgui_set_state_buffer(&eezgui_ctx, eezgui_state_buffer, sizeof(eezgui_state_buffer));

    // Wire up JS callbacks
    eezgui_ctx.get_str_prop = wasm_get_str_prop;
    eezgui_ctx.get_bool_prop = wasm_get_bool_prop;
    eezgui_ctx.get_int_prop = wasm_get_int_prop;

    // Default timing
    eezgui_ctx.long_press_time = 400;
    eezgui_ctx.long_press_repeat_time = 100;

    return 0;
}

EM_PORT_API(uint8_t *) getSyncedBuffer() {
    bool dirty = eezgui_display_convert_to_rgba();
    return dirty ? eezgui_display_get_frame_buffer_rgba() : false;
}

////////////////////////////////////////////////////////////////////////////////
// Colors API
//
// JS allocates a eezgui_color_t array via _malloc, fills it, then calls setColors.

EM_PORT_API(void) setColors(const eezgui_color_t *colors, int num_colors) {
    eezgui_set_colors(&eezgui_ctx, colors, num_colors);
}

////////////////////////////////////////////////////////////////////////////////
// Fonts API
//
// JS allocates font structs (eezgui_font_data_t, eezgui_glyph_data_t, etc.) via _malloc,
// then builds a pointer array and calls setFonts.

EM_PORT_API(void) setFonts(const eezgui_font_data_t **fonts, int num_fonts) {
    eezgui_set_fonts(&eezgui_ctx, fonts, num_fonts);
}

////////////////////////////////////////////////////////////////////////////////
// Styles API
//
// JS allocates an eezgui_style_t array via _malloc, fills it, then calls
// setStyles.

EM_PORT_API(void) setStyles(const eezgui_style_t *styles, int num_styles) {
    eezgui_set_styles(&eezgui_ctx, styles, num_styles);
}

////////////////////////////////////////////////////////////////////////////////
// Widget allocation helpers
//
// These allocate widget structs via malloc so that JS can keep persistent
// pointers (unlike the state buffer which is reused every frame).

EM_PORT_API(eezgui_text_t *) allocTextWidget(void) {
    eezgui_text_t *w = (eezgui_text_t *)malloc(sizeof(eezgui_text_t));
    memset(w, 0, sizeof(eezgui_text_t));
    return w;
}

EM_PORT_API(eezgui_button_t *) allocButtonWidget(void) {
    eezgui_button_t *w = (eezgui_button_t *)malloc(sizeof(eezgui_button_t));
    memset(w, 0, sizeof(eezgui_button_t));
    return w;
}

EM_PORT_API(eezgui_rectangle_t *) allocRectangleWidget(void) {
    eezgui_rectangle_t *w = (eezgui_rectangle_t *)malloc(sizeof(eezgui_rectangle_t));
    memset(w, 0, sizeof(eezgui_rectangle_t));
    return w;
}

EM_PORT_API(eezgui_switch_t *) allocSwitchWidget(void) {
    eezgui_switch_t *w = (eezgui_switch_t *)malloc(sizeof(eezgui_switch_t));
    memset(w, 0, sizeof(eezgui_switch_t));
    return w;
}

EM_PORT_API(eezgui_select_t *) allocSelectWidget(void) {
    eezgui_select_t *w = (eezgui_select_t *)malloc(sizeof(eezgui_select_t));
    memset(w, 0, sizeof(eezgui_select_t));
    return w;
}

EM_PORT_API(eezgui_container_t *) allocContainerWidget(void) {
    eezgui_container_t *w = (eezgui_container_t *)malloc(sizeof(eezgui_container_t));
    memset(w, 0, sizeof(eezgui_container_t));
    return w;
}

EM_PORT_API(void) freeWidget(void *widget) {
    free(widget);
}

////////////////////////////////////////////////////////////////////////////////
// Widget base field setters
//
// These let JS set individual fields of the eezgui_widget_t base struct
// given a widget pointer (any widget type — they all start with base).

EM_PORT_API(void) setWidgetFlags(eezgui_widget_t *widget, uint32_t flags) {
    widget->flags = flags;
}

EM_PORT_API(void) setWidgetGeometry(eezgui_widget_t *widget, int16_t x, int16_t y, int16_t w, int16_t h) {
    widget->x = x;
    widget->y = y;
    widget->w = w;
    widget->h = h;
}

EM_PORT_API(void) setWidgetStyle(eezgui_widget_t *widget, uint16_t style) {
    widget->style = style;
}

EM_PORT_API(void) setWidgetVisible(eezgui_widget_t *widget, uint16_t is_visible) {
    widget->is_visible = is_visible;
}

// Text-specific: set text property index
EM_PORT_API(void) setTextWidgetText(eezgui_text_t *widget, uint16_t text_prop) {
    widget->text = text_prop;
}

// Button-specific: set text property index
EM_PORT_API(void) setButtonWidgetText(eezgui_button_t *widget, uint16_t text_prop) {
    widget->text = text_prop;
}

// Switch-specific: set is_checked property index
EM_PORT_API(void) setSwitchWidgetChecked(eezgui_switch_t *widget, uint16_t is_checked_prop) {
    widget->is_checked = is_checked_prop;
}

////////////////////////////////////////////////////////////////////////////////
// Page rendering API
//
// JS calls these in sequence each frame to render the current page:
//   startPage(pageDataPtr, styleIndex)
//   renderTextWidget(widgetPtr)
//   renderButtonWidget(widgetPtr)
//   ...
//   endPage()

EM_PORT_API(void) startPage(const void *page_data, uint16_t style) {
    current_page_data = page_data;
    eezgui_start_page(&eezgui_ctx, page_data, style, wasm_on_event);
}

EM_PORT_API(void) endPage(void) {
    eezgui_end_page(&eezgui_ctx);
}

EM_PORT_API(void) renderTextWidget(const eezgui_text_t *widget) {
    eezgui_text(&eezgui_ctx, widget);
}

EM_PORT_API(void) renderButtonWidget(const eezgui_button_t *widget) {
    eezgui_button(&eezgui_ctx, widget);
}

EM_PORT_API(void) renderRectangleWidget(const eezgui_rectangle_t *widget) {
    eezgui_rectangle(&eezgui_ctx, widget);
}

EM_PORT_API(void) renderSwitchWidget(const eezgui_switch_t *widget) {
    eezgui_switch(&eezgui_ctx, widget);
}

EM_PORT_API(void) renderSelectBegin(const eezgui_select_t *widget) {
    eezgui_select_begin(&eezgui_ctx, widget);
}

EM_PORT_API(void) renderSelectEnd(const eezgui_select_t *widget) {
    eezgui_select_end(&eezgui_ctx, widget);
}

EM_PORT_API(void) renderContainerBegin(const eezgui_container_t *widget) {
    eezgui_container_begin(&eezgui_ctx, widget);
}

EM_PORT_API(void) renderContainerEnd(const eezgui_container_t *widget) {
    eezgui_container_end(&eezgui_ctx, widget);
}

////////////////////////////////////////////////////////////////////////////////
// Pointer / touch input

EM_PORT_API(void) pointerInput(int x, int y, bool pressed) {
    eezgui_pointer_input(&eezgui_ctx, x, y, pressed);
}

////////////////////////////////////////////////////////////////////////////////
// Refresh request (force full redraw)

EM_PORT_API(void) requestRefresh(void) {
    eezgui_refresh(&eezgui_ctx);
}

////////////////////////////////////////////////////////////////////////////////
// Struct size queries
//
// JS needs to know exact struct sizes to allocate memory and fill fields
// at correct offsets. These helpers return sizeof() for each struct.

EM_PORT_API(int) sizeofStyle(void) {
    return sizeof(eezgui_style_t);
}

EM_PORT_API(int) sizeofColor(void) {
    return sizeof(eezgui_color_t);
}

EM_PORT_API(int) sizeofFontData(void) {
    return sizeof(eezgui_font_data_t);
}

EM_PORT_API(int) sizeofGlyphData(void) {
    return sizeof(eezgui_glyph_data_t);
}

EM_PORT_API(int) sizeofGlyphsGroup(void) {
    return sizeof(eezgui_glyphs_group_t);
}

EM_PORT_API(int) sizeofTextWidget(void) {
    return sizeof(eezgui_text_t);
}

EM_PORT_API(int) sizeofButtonWidget(void) {
    return sizeof(eezgui_button_t);
}

EM_PORT_API(int) sizeofRectangleWidget(void) {
    return sizeof(eezgui_rectangle_t);
}

EM_PORT_API(int) sizeofSwitchWidget(void) {
    return sizeof(eezgui_switch_t);
}

EM_PORT_API(int) sizeofSelectWidget(void) {
    return sizeof(eezgui_select_t);
}

EM_PORT_API(int) sizeofContainerWidget(void) {
    return sizeof(eezgui_container_t);
}

////////////////////////////////////////////////////////////////////////////////
// Struct field offset queries
//
// Return byte offsets of important fields so JS can write directly into
// WASM memory at the correct positions.

// eezgui_style_t offsets
EM_PORT_API(int) offsetofStyleFlags(void) { return offsetof(eezgui_style_t, flags); }
EM_PORT_API(int) offsetofStyleBgColor(void) { return offsetof(eezgui_style_t, background_color); }
EM_PORT_API(int) offsetofStyleColor(void) { return offsetof(eezgui_style_t, color); }
EM_PORT_API(int) offsetofStyleActiveBgColor(void) { return offsetof(eezgui_style_t, active_background_color); }
EM_PORT_API(int) offsetofStyleActiveColor(void) { return offsetof(eezgui_style_t, active_color); }
EM_PORT_API(int) offsetofStyleBorderSizeTop(void) { return offsetof(eezgui_style_t, border_size_top); }
EM_PORT_API(int) offsetofStyleBorderSizeRight(void) { return offsetof(eezgui_style_t, border_size_right); }
EM_PORT_API(int) offsetofStyleBorderSizeBottom(void) { return offsetof(eezgui_style_t, border_size_bottom); }
EM_PORT_API(int) offsetofStyleBorderSizeLeft(void) { return offsetof(eezgui_style_t, border_size_left); }
EM_PORT_API(int) offsetofStyleBorderColor(void) { return offsetof(eezgui_style_t, border_color); }
EM_PORT_API(int) offsetofStyleFont(void) { return offsetof(eezgui_style_t, font); }
EM_PORT_API(int) offsetofStylePaddingTop(void) { return offsetof(eezgui_style_t, padding_top); }
EM_PORT_API(int) offsetofStylePaddingRight(void) { return offsetof(eezgui_style_t, padding_right); }
EM_PORT_API(int) offsetofStylePaddingBottom(void) { return offsetof(eezgui_style_t, padding_bottom); }
EM_PORT_API(int) offsetofStylePaddingLeft(void) { return offsetof(eezgui_style_t, padding_left); }

// eezgui_glyph_data_t offsets
EM_PORT_API(int) offsetofGlyphDx(void) { return offsetof(eezgui_glyph_data_t, dx); }
EM_PORT_API(int) offsetofGlyphW(void) { return offsetof(eezgui_glyph_data_t, w); }
EM_PORT_API(int) offsetofGlyphH(void) { return offsetof(eezgui_glyph_data_t, h); }
EM_PORT_API(int) offsetofGlyphX(void) { return offsetof(eezgui_glyph_data_t, x); }
EM_PORT_API(int) offsetofGlyphY(void) { return offsetof(eezgui_glyph_data_t, y); }
EM_PORT_API(int) offsetofGlyphPixelsIndex(void) { return offsetof(eezgui_glyph_data_t, pixels_index); }

// eezgui_glyphs_group_t offsets
EM_PORT_API(int) offsetofGroupEncoding(void) { return offsetof(eezgui_glyphs_group_t, encoding); }
EM_PORT_API(int) offsetofGroupGlyphIndex(void) { return offsetof(eezgui_glyphs_group_t, glyphIndex); }
EM_PORT_API(int) offsetofGroupLength(void) { return offsetof(eezgui_glyphs_group_t, length); }

// eezgui_font_data_t offsets
EM_PORT_API(int) offsetofFontAscent(void) { return offsetof(eezgui_font_data_t, ascent); }
EM_PORT_API(int) offsetofFontDescent(void) { return offsetof(eezgui_font_data_t, descent); }
EM_PORT_API(int) offsetofFontEncodingStart(void) { return offsetof(eezgui_font_data_t, encodingStart); }
EM_PORT_API(int) offsetofFontEncodingEnd(void) { return offsetof(eezgui_font_data_t, encodingEnd); }
EM_PORT_API(int) offsetofFontGroups(void) { return offsetof(eezgui_font_data_t, groups); }
EM_PORT_API(int) offsetofFontGlyphs(void) { return offsetof(eezgui_font_data_t, glyphs); }
EM_PORT_API(int) offsetofFontPixels(void) { return offsetof(eezgui_font_data_t, pixels); }

// eezgui_widget_t (base) offsets
EM_PORT_API(int) offsetofWidgetFlags(void) { return offsetof(eezgui_widget_t, flags); }
EM_PORT_API(int) offsetofWidgetX(void) { return offsetof(eezgui_widget_t, x); }
EM_PORT_API(int) offsetofWidgetY(void) { return offsetof(eezgui_widget_t, y); }
EM_PORT_API(int) offsetofWidgetW(void) { return offsetof(eezgui_widget_t, w); }
EM_PORT_API(int) offsetofWidgetH(void) { return offsetof(eezgui_widget_t, h); }
EM_PORT_API(int) offsetofWidgetStyle(void) { return offsetof(eezgui_widget_t, style); }
EM_PORT_API(int) offsetofWidgetVisible(void) { return offsetof(eezgui_widget_t, is_visible); }

// eezgui_text_t offsets (text field is after base)
EM_PORT_API(int) offsetofTextWidgetText(void) { return offsetof(eezgui_text_t, text); }

// eezgui_button_t offsets
EM_PORT_API(int) offsetofButtonWidgetText(void) { return offsetof(eezgui_button_t, text); }

// eezgui_switch_t offsets
EM_PORT_API(int) offsetofSwitchWidgetChecked(void) { return offsetof(eezgui_switch_t, is_checked); }

////////////////////////////////////////////////////////////////////////////////
// Constants
//
// Expose C-side constants so JS doesn't need to duplicate them.

EM_PORT_API(uint32_t) getWidgetFlagClickable(void) {
    return EEZGUI_WIDGET_FLAG_CLICKABLE;
}

EM_PORT_API(uint16_t) getStyleFlagHorzAlignLeft(void) { return EEZGUI_STYLE_FLAG_HORZ_ALIGN_LEFT; }
EM_PORT_API(uint16_t) getStyleFlagHorzAlignRight(void) { return EEZGUI_STYLE_FLAG_HORZ_ALIGN_RIGHT; }
EM_PORT_API(uint16_t) getStyleFlagHorzAlignCenter(void) { return EEZGUI_STYLE_FLAG_HORZ_ALIGN_CENTER; }
EM_PORT_API(uint16_t) getStyleFlagVertAlignTop(void) { return EEZGUI_STYLE_FLAG_VERT_ALIGN_TOP; }
EM_PORT_API(uint16_t) getStyleFlagVertAlignBottom(void) { return EEZGUI_STYLE_FLAG_VERT_ALIGN_BOTTOM; }
EM_PORT_API(uint16_t) getStyleFlagVertAlignCenter(void) { return EEZGUI_STYLE_FLAG_VERT_ALIGN_CENTER; }
EM_PORT_API(uint16_t) getStyleFlagBlink(void) { return EEZGUI_STYLE_FLAG_BLINK; }

////////////////////////////////////////////////////////////////////////////////
// Color conversion helper
//
// Converts 8-bit RGB to the internal RGB565 eezgui_color_t format.

EM_PORT_API(eezgui_color_t) makeColor(uint8_t r, uint8_t g, uint8_t b) {
    return EEZGUI_MAKE_COLOR(r, g, b);
}