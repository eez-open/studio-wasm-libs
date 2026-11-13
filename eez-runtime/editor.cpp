// Editor mode: the Studio page editor draws EEZ-GUI pages with the framework.
//
// Studio builds uncompressed assets (~EEZ) that hold the edited page as page 1,
// with all fonts, bitmaps, styles and colors of the project and no actions. The
// framework uses them in place, so Studio keeps the buffer until the next
// editorLoad returns. There is no debugger, no file system and no input.

#include <emscripten.h>

#include <vector>

#include <eez/core/assets.h>
#include <eez/core/memory.h>
#include <eez/gui/gui.h>
#include <eez/gui/display.h>
#include <eez/gui/thread.h>
#include <eez/gui/scrolling.h>
#include <eez/gui/widgets/containers/container.h>
#include <eez/flow/flow.h>
#include <eez/flow/hooks.h>
#include <eez/flow/private.h>

#include "./gui/document.h"

extern uint32_t DISPLAY_WIDTH;
extern uint32_t DISPLAY_HEIGHT;

extern void eez_system_tick();

// eez/gui/display.cpp: the last drawn frame (also queues the next vsync)
extern "C" uint8_t *getSyncedBuffer();

namespace eez {
namespace gui {
// gui/app_context.cpp: the pages are drawn without a shadow and a backdrop
extern bool g_editorMode;
}
}

static bool g_editorStarted = false;

// the assets of the last editorLoad (Studio keeps the buffer)
static const uint8_t *g_editorAssets;

// a flow error never stops the editor: the widget is drawn with what its
// properties evaluated to
static void editorStopScript() {
}

// The scroll positions set in the editor (editorSetScrollPositions): pairs of
// the offset of a widget in the assets of the last editorLoad and its position
// in lines (px for a Container).
static std::vector<int32_t> g_scrollPositions;

static int editorScrollPosition(const eez::gui::WidgetCursor &widgetCursor) {
    int32_t offset = (int32_t)((const uint8_t *)widgetCursor.widget - g_editorAssets);
    for (size_t i = 0; i + 1 < g_scrollPositions.size(); i += 2) {
        if (g_scrollPositions[i] == offset) {
            return g_scrollPositions[i + 1];
        }
    }
    return -1;
}

EM_PORT_API(void) editorLoad(uint8_t *assets, uint32_t assetsSize, uint32_t displayWidth, uint32_t displayHeight) {
    using namespace eez;
    using namespace eez::gui;

    g_editorAssets = assets;

    if (!g_editorStarted) {
        g_editorMode = true;
        g_editorScrollPositionHook = editorScrollPosition;
        flow::enableThrowError(false);
        flow::stopScriptHook = editorStopScript;

        initAssetsMemory();
        loadMainAssets(assets, assetsSize);
        DISPLAY_WIDTH = displayWidth;
        DISPLAY_HEIGHT = displayHeight;
        initOtherMemory();
        initAllocHeap(ALLOC_BUFFER, ALLOC_BUFFER_SIZE);

        // starts the flow of the assets and initializes the display
        startThread();
        display::turnOn();

        // what no widget draws shows Studio's background
        display::clearScreenTransparent();

        g_editorStarted = true;
        return;
    }

    // stop the flow of the previous assets now (the next tick does the stop and
    // frees all flow states), and drop the page that was made from them
    flow::stop();
    flow::tick();
    getAppContextFromId(APP_CONTEXT_ID_DEVICE)->removePageFromStack(PAGE_ID_MAIN);

    loadMainAssets(assets, assetsSize);
    if (g_mainAssets->flowDefinition) {
        flow::start(g_mainAssets);
    }

    // the display keeps pointers to the theme colors of the assets
    display::onThemeChanged();

    // the app context shows the main page (page 1) again in the next tick,
    // drawn on a transparent screen
    display::clearScreenTransparent();
    refreshScreen();
}

// The positions of the scrollable widgets (a List, a Grid or a scrolling
// Container that keeps its position): count pairs of the offset of the widget
// in the assets of the last editorLoad and its position in lines (px for a
// Container), clamped as a touch scroll position. They replace the previous
// ones and are drawn by the next editorRender.
EM_PORT_API(void) editorSetScrollPositions(const int32_t *pairs, uint32_t count) {
    g_scrollPositions.assign(pairs, pairs + 2 * count);
}

EM_PORT_API(void) editorSetTheme(uint32_t themeIndex) {
    eez::gui::g_selectedThemeIndex = (uint8_t)themeIndex;
    eez::gui::display::onThemeChanged();
    eez::gui::refreshScreen();
}

// Runs the GUI until the page is drawn and returns the frame
// (DISPLAY_WIDTH x DISPLAY_HEIGHT, 32-bit RGBA).
EM_PORT_API(uint8_t *) editorRender() {
    using namespace eez::gui;

    // the first ticks show the page, the next ones draw it
    for (int i = 0; i < 3; i++) {
        sendMessageToGuiThread(GUI_QUEUE_MESSAGE_TYPE_DISPLAY_VSYNC, 0, 0);
        eez_system_tick();
    }

    return getSyncedBuffer();
}

// The boxes of the widgets of the page drawn last (editorRender), in the order
// the framework enumerates them (hidden widgets are not enumerated): [count,
// then for each widget its offset in the assets of the last editorLoad, x, y,
// w, h on the display, its scroll position and the largest one in lines (px
// for a Container; -1 and 0 if the widget doesn't keep a position)]. Studio
// maps the offsets to its widgets, so the editor shows the widgets of rows and
// columns where the framework lays them out (eez-framework gui/layout.h), and
// the children of a scrolled widget where they are drawn. The values are valid
// until the next call.
static const int WIDGET_BOX_SIZE = 7;
static std::vector<int32_t> g_widgetBoxes;

static void collectWidgetBox() {
    using namespace eez::gui;

    auto &widgetCursor = g_widgetCursor;
    auto widget = widgetCursor.widget;
    g_widgetBoxes.push_back((int32_t)((const uint8_t *)widget - g_editorAssets));
    g_widgetBoxes.push_back(widgetCursor.x);
    g_widgetBoxes.push_back(widgetCursor.y);
    g_widgetBoxes.push_back(widgetCursor.w);
    g_widgetBoxes.push_back(widgetCursor.h);

    // the state of the widget itself (as in isTouchScrollableList)
    int32_t position = -1;
    int32_t maxPosition = 0;
    bool scrollable =
        widget->type == WIDGET_TYPE_LIST || widget->type == WIDGET_TYPE_GRID ||
        (widget->type == WIDGET_TYPE_CONTAINER &&
            ((const ContainerWidget *)widget)->scroll == CONTAINER_WIDGET_SCROLL_VERTICAL);
    if (scrollable && widgetCursor.currentState && widgetCursor.currentState->type == widget->type) {
        auto state = (const ScrollableWidgetState *)widgetCursor.currentState;
        if (state->localPosition && state->itemsPerLine > 0) {
            position = state->startPosition / state->itemsPerLine;
            maxPosition = state->maxPosition;
        }
    }
    g_widgetBoxes.push_back(position);
    g_widgetBoxes.push_back(maxPosition);
}

EM_PORT_API(int32_t *) editorGetWidgetBoxes() {
    g_widgetBoxes.clear();
    g_widgetBoxes.push_back(0);
    eez::gui::forEachWidget(collectWidgetBox);
    g_widgetBoxes[0] = (int32_t)((g_widgetBoxes.size() - 1) / WIDGET_BOX_SIZE);
    return g_widgetBoxes.data();
}
