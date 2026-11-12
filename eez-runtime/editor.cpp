// Editor mode: the Studio page editor draws EEZ-GUI pages with the framework.
//
// Studio builds uncompressed assets (~EEZ) that hold the edited page as page 1,
// with all fonts, bitmaps, styles and colors of the project and no actions. The
// framework uses them in place, so Studio keeps the buffer until the next
// editorLoad returns. There is no debugger, no file system and no input.

#include <emscripten.h>

#include <eez/core/assets.h>
#include <eez/core/memory.h>
#include <eez/gui/gui.h>
#include <eez/gui/display.h>
#include <eez/gui/thread.h>
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

// a flow error never stops the editor: the widget is drawn with what its
// properties evaluated to
static void editorStopScript() {
}

EM_PORT_API(void) editorLoad(uint8_t *assets, uint32_t assetsSize, uint32_t displayWidth, uint32_t displayHeight) {
    using namespace eez;
    using namespace eez::gui;

    if (!g_editorStarted) {
        g_editorMode = true;
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
