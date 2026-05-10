#include <eez/core/sound.h>

#include <eez/gui/gui.h>
#include <eez/gui/keypad.h>

////////////////////////////////////////////////////////////////////////////////

namespace eez {
    namespace gui {

        const uint8_t *eezAssets = assets;
        const uint32_t eezAssetsSize = sizeof(assets);

        const EnumItem *g_enumDefinitions[] = { nullptr };

        const int EEZ_DATA_ID_KEYPAD_EDIT_UNIT = DATA_ID_KEYPAD_EDIT_UNIT;
        const int EEZ_DATA_ID_KEYPAD_TEXT = DATA_ID_KEYPAD_TEXT;
        const int EEZ_DATA_ID_ALERT_MESSAGE = DATA_ID_ALERT_MESSAGE;

        const int EEZ_ACTION_ID_DRAG_OVERLAY = ACTION_ID_DRAG_OVERLAY;
        const int EEZ_ACTION_ID_EDIT = ACTION_ID_EDIT;
        const int EEZ_ACTION_ID_SCROLL = ACTION_ID_SCROLL;

        const int EEZ_PAGE_ID_ASYNC_OPERATION_IN_PROGRESS = PAGE_ID_ASYNC_OPERATION_IN_PROGRESS;
        const int EEZ_PAGE_ID_NUMERIC_KEYPAD = PAGE_ID_NUMERIC_KEYPAD;

        const int EEZ_STYLE_ID_DEFAULT = STYLE_ID_DEFAULT;
        const int EEZ_STYLE_ID_INFO_ALERT = STYLE_ID_MESSAGE_BOX_INFO;
        const int EEZ_STYLE_ID_INFO_ALERT_BUTTON = STYLE_ID_MESSAGE_BOX_ERROR_BUTTON;
        const int EEZ_STYLE_ID_ERROR_ALERT = STYLE_ID_MESSAGE_BOX_ERROR;
        const int EEZ_STYLE_ID_ERROR_ALERT_BUTTON = STYLE_ID_MESSAGE_BOX_ERROR_BUTTON;
        const int EEZ_STYLE_ID_SELECT_ENUM_ITEM_POPUP_CONTAINER_S = STYLE_ID_DROP_DOWN_LIST_POPUP_CONTAINER;
        const int EEZ_STYLE_ID_SELECT_ENUM_ITEM_POPUP_CONTAINER = STYLE_ID_DROP_DOWN_LIST_POPUP_CONTAINER;
        const int EEZ_STYLE_ID_SELECT_ENUM_ITEM_POPUP_ITEM_S = STYLE_ID_DROP_DOWN_LIST_POPUP_ITEM;
        const int EEZ_STYLE_ID_SELECT_ENUM_ITEM_POPUP_ITEM = STYLE_ID_DROP_DOWN_LIST_POPUP_ITEM;
        const int EEZ_STYLE_ID_SELECT_ENUM_ITEM_POPUP_DISABLED_ITEM_S = STYLE_ID_DROP_DOWN_LIST_DISABLED_POPUP_ITEM;
        const int EEZ_STYLE_ID_SELECT_ENUM_ITEM_POPUP_DISABLED_ITEM = STYLE_ID_DROP_DOWN_LIST_DISABLED_POPUP_ITEM;
        const int EEZ_STYLE_ID_MENU_WITH_BUTTONS_CONTAINER = STYLE_ID_MESSAGE_BOX_QUESTION_CONTAINER;
        const int EEZ_STYLE_ID_MENU_WITH_BUTTONS_MESSAGE = STYLE_ID_MESSAGE_BOX_QUESTION_MESSAGE;
        const int EEZ_STYLE_ID_MENU_WITH_BUTTONS_BUTTON = STYLE_ID_MESSAGE_BOX_QUESTION_BUTTON;

        const int EEZ_THEME_ID_LEGACY = THEME_ID_DEFAULT;
        const int EEZ_THEME_ID_DEFAULT = THEME_ID_DEFAULT;
        const int EEZ_COLOR_ID_BOOKMARK = COLOR_ID_BOOKMARK;
        const int EEZ_COLOR_ID_BACKDROP = COLOR_ID_BACKDROP;
        const int EEZ_FONT_ID_SHADOW = FONT_ID_SHADOW;

        static class : public AppContext {
        public:
            void stateManagment() override {
                AppContext::stateManagment();
                if (getActivePageId() == PAGE_ID_NONE) {
                    showPage(getMainPageId());
                }
            }

        protected:
            int getMainPageId() override {
                return PAGE_ID_MAIN;
            }
        } g_myAppContext;

        AppContext *getAppContextFromId(int16_t id) { return id == APP_CONTEXT_ID_DEVICE || id == APP_CONTEXT_ID_SIMULATOR_FRONT_PANEL ? &g_myAppContext : nullptr; }
    } // namespace gui
} // namespace eez

////////////////////////////////////////////////////////////////////////////////

namespace eez {
    bool g_shutdown;
    void shutdown() { g_shutdown = true; }

    namespace sound {
        void playBeep(bool force) {}
        void playClick() {}
    } // namespace sound
} // namespace eez

////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#undef INPUT
#undef OUTPUT
#include <Shlobj.h>
#include <Windows.h>
#include <direct.h>
#else
#include <string.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace eez {
    char *getConfFilePath(const char *file_name) {
        static char file_path[1024];

        *file_path = 0;

    #ifdef _WIN32
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, file_path))) {
            stringAppendString(file_path, sizeof(file_path), "\\.min_eez_sample");
            _mkdir(file_path);
            stringAppendString(file_path, sizeof(file_path), "\\");
        }
    #elif defined(__EMSCRIPTEN__)
        stringAppendString(file_path, sizeof(file_path), "/min_eez_sample/");
    #else
        const char *home_dir = 0;
        if ((home_dir = getenv("HOME")) == NULL) {
            home_dir = getpwuid(getuid())->pw_dir;
        }
        if (home_dir) {
            stringAppendString(file_path, sizeof(file_path), home_dir);
            stringAppendString(file_path, sizeof(file_path), "/.min_eez_sample");
            mkdir(file_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            stringAppendString(file_path, sizeof(file_path), "/");
        }
    #endif

        char *q = file_path + strlen(file_path);
        const char *p = file_name;
        while (*p) {
            char ch = *p++;
    #ifdef _WIN32
            if (ch == '/')
                *q++ = '\\';
    #else
            if (ch == '\\')
                *q++ = '/';
    #endif
            else
                *q++ = ch;
        }
        *q = 0;

        return file_path;
    }
} // eez
