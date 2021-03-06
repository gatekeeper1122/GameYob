#include <sstream>

#include "platform/common/menu/cheatmenu.h"
#include "platform/common/menu/menu.h"
#include "platform/common/manager.h"
#include "platform/ui.h"
#include "cheatengine.h"
#include "gameboy.h"

bool CheatsMenu::processInput(UIKey key, u32 width, u32 height) {
    CheatEngine* cheatEngine = &mgrGetGameboy()->cheatEngine;
    u32 numCheats = cheatEngine->getNumCheats();

    if(numCheats > 0) {
        if(key == UI_KEY_UP) {
            if(selection > 0) {
                selection--;

                return true;
            }
        } else if(key == UI_KEY_DOWN) {
            if(selection < numCheats - 1) {
                selection++;

                return true;
            }
        } else if(key == UI_KEY_RIGHT || key == UI_KEY_LEFT) {
            cheatEngine->toggleCheat(selection, !cheatEngine->isCheatEnabled(selection));

            return true;
        } else if(key == UI_KEY_R) {
            selection += cheatsPerPage;
            if(selection >= numCheats) {
                selection = 0;
            }

            return true;
        } else if(key == UI_KEY_L) {
            selection -= cheatsPerPage;
            if(selection < 0) {
                selection = numCheats - 1;
            }

            return true;
        }
    }

    if(key == UI_KEY_B) {
        menuPop();
    } else if(key == UI_KEY_X) {
        if(uiIsStringInputSupported()) {
            std::string name;
            std::string value;

            if(uiInputString(name, 128, "Cheat Name") && uiInputString(value, 128, "Cheat Value")) {
                cheatEngine->addCheat(name, value);
            }

            return true;
        }
    }

    return false;
}

void CheatsMenu::draw(u32 width, u32 height) {
    cheatsPerPage = height - 2;
    if(uiIsStringInputSupported()) {
        cheatsPerPage--;
    }

    CheatEngine* cheatEngine = &mgrGetGameboy()->cheatEngine;

    u32 numCheats = cheatEngine->getNumCheats();
    u32 numPages = numCheats > 0 ? (numCheats - 1) / cheatsPerPage + 1 : 0;
    u32 page = selection / cheatsPerPage;

    std::stringstream titleStream;
    titleStream << "Cheats - Page " << (numPages > 0 ? page + 1 : 0) << "/" << numPages;
    const std::string title = titleStream.str();

    uiAdvanceCursor((width - 1 - title.length()) / 2);
    uiPrint("%s\n\n", title.c_str());
    for(u32 i = page * cheatsPerPage; i < numCheats && i < (page + 1) * cheatsPerPage; i++) {
        if(selection == i) {
            uiSetLineHighlighted(true);
        }

        std::string name = cheatEngine->getCheatName(i);

        u32 maxNameLength = width - 8;
        if(name.length() > maxNameLength) {
            name = name.substr(0, maxNameLength);
        }

        uiPrint("%s", name.c_str());

        uiAdvanceCursor(maxNameLength - name.length());

        if(cheatEngine->isCheatEnabled(i)) {
            if(selection == i) {
                uiSetTextColor(TEXT_COLOR_YELLOW);
                uiPrint("* ");
                uiSetTextColor(TEXT_COLOR_GREEN);
                uiPrint("On");
                uiSetTextColor(TEXT_COLOR_YELLOW);
                uiPrint("  *\n");
                uiSetTextColor(TEXT_COLOR_NONE);
            } else {
                uiSetTextColor(TEXT_COLOR_GREEN);
                uiPrint("  On\n");
                uiSetTextColor(TEXT_COLOR_NONE);
            }
        } else {
            if(selection == i) {
                uiSetTextColor(TEXT_COLOR_YELLOW);
                uiPrint("* ");
                uiSetTextColor(TEXT_COLOR_RED);
                uiPrint("Off");
                uiSetTextColor(TEXT_COLOR_YELLOW);
                uiPrint(" *\n");
                uiSetTextColor(TEXT_COLOR_NONE);
            } else {
                uiSetTextColor(TEXT_COLOR_RED);
                uiPrint("  Off\n");
                uiSetTextColor(TEXT_COLOR_NONE);
            }
        }

        if(selection == i) {
            uiSetLineHighlighted(false);
        }
    }

    if(uiIsStringInputSupported()) {
        uiSetLine(height - 1);
        uiPrint("Press X to add a cheat.");
    }
}