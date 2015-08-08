#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "platform/input.h"
#include "platform/system.h"
#include "platform/ui.h"
#include "ui/config.h"
#include "ui/manager.h"
#include "ui/menu.h"
#include "gameboy.h"

#include <strings.h>

char gbBiosPath[512] = "";
char gbcBiosPath[512] = "";
char borderPath[512] = "";
char romPath[512] = "";

void controlsParseConfig(char* line);
void controlsPrintConfig(FILE* f);
void controlsCheckConfig();

void generalParseConfig(char* line) {
    char* equalsPos;
    if((equalsPos = strrchr(line, '=')) != 0 && equalsPos != line + strlen(line) - 1) {
        *equalsPos = '\0';
        const char* parameter = line;
        const char* value = equalsPos + 1;

        if(strcasecmp(parameter, "rompath") == 0) {
            strcpy(romPath, value);
        } else if(strcasecmp(parameter, "gbbiosfile") == 0) {
            strcpy(gbBiosPath, value);
        } else if(strcasecmp(parameter, "gbcbiosfile") == 0 || strcasecmp(parameter, "biosfile") == 0) {
            strcpy(gbcBiosPath, value);
        } else if(strcasecmp(parameter, "borderfile") == 0) {
            strcpy(borderPath, value);
        }
    }

    size_t len = strlen(romPath);
    if(len == 0 || *(romPath + len - 1) != '/') {
        char copy[512];
        strncpy(copy, romPath, 512);
        snprintf(romPath, 512, "%s/", copy);
    }

    mgrRefreshBios();
}

void generalPrintConfig(FILE* file) {
    fprintf(file, "rompath=%s\n", romPath);
    fprintf(file, "gbbiosfile=%s\n", gbBiosPath);
    fprintf(file, "gbcbiosfile=%s\n", gbcBiosPath);
    fprintf(file, "borderfile=%s\n", borderPath);
}

bool readConfigFile() {
    FILE* file = fopen(iniPath, "r");
    char line[100];
    void (*configParser)(char*) = generalParseConfig;

    if(file == NULL) {
        goto end;
    }

    struct stat s;
    fstat(fileno(file), &s);
    while(ftell(file) < s.st_size) {
        fgets(line, 100, file);
        char c = 0;
        while(*line != '\0' && ((c = line[strlen(line) - 1]) == ' ' || c == '\n' || c == '\r')) {
            line[strlen(line) - 1] = '\0';
        }

        if(line[0] == '[') {
            char* endBrace;
            if((endBrace = strrchr(line, ']')) != 0) {
                *endBrace = '\0';
                const char* section = line + 1;
                if(strcasecmp(section, "general") == 0) {
                    configParser = generalParseConfig;
                } else if(strcasecmp(section, "console") == 0) {
                    configParser = menuParseConfig;
                } else if(strcasecmp(section, "controls") == 0) {
                    configParser = controlsParseConfig;
                }
            }
        } else {
            configParser(line);
        }
    }

    fclose(file);

    end:
    controlsCheckConfig();
    return file != NULL;
}

void writeConfigFile() {
    FILE* file = fopen(iniPath, "w");
    if(file == NULL) {
        printMenuMessage("Error opening gameyob.ini.");
        return;
    }

    fprintf(file, "[general]\n");
    generalPrintConfig(file);
    fprintf(file, "[console]\n");
    menuPrintConfig(file);
    fprintf(file, "[controls]\n");
    controlsPrintConfig(file);
    fclose(file);

    if(gameboy->isRomLoaded()) {
        char nameBuf[512];
        snprintf(nameBuf, 512, "%s.cht", gameboy->getRomFile()->getFileName().c_str());
        gameboy->getCheatEngine()->saveCheats(nameBuf);
    }
}

const char* gbKeyNames[] = {
        "-",
        "A",
        "B",
        "Left",
        "Right",
        "Up",
        "Down",
        "Start",
        "Select",
        "Menu",
        "Menu/Pause",
        "Save",
        "Autofire A",
        "Autofire B",
        "Fast Forward",
        "FF Toggle",
        "Scale",
        "Reset",
        "Screenshot"
};

std::vector<KeyConfig> keyConfigs;
unsigned int selectedKeyConfig = 0;

void controlsParseConfig(char* line2) {
    char line[100];
    strncpy(line, line2, 100);
    while(strlen(line) > 0 && (line[strlen(line) - 1] == '\n' || line[strlen(line) - 1] == ' ')) {
        line[strlen(line) - 1] = '\0';
    }

    if(line[0] == '(') {
        char* bracketEnd;
        if((bracketEnd = strrchr(line, ')')) != 0) {
            *bracketEnd = '\0';
            const char* name = line + 1;

            keyConfigs.push_back(KeyConfig());
            KeyConfig* config = &keyConfigs.back();
            strncpy(config->name, name, 32);
            config->name[31] = '\0';
            for(int i = 0; i < NUM_BUTTONS; i++) {
                if(strlen(inputGetKeyName(i)) > 0) {
                    config->funcKeys[i] = FUNC_KEY_NONE;
                }
            }
        }

        return;
    }

    char* equalsPos;
    if((equalsPos = strrchr(line, '=')) != 0 && equalsPos != line + strlen(line) - 1) {
        *equalsPos = '\0';

        if(strcasecmp(line, "config") == 0) {
            selectedKeyConfig = atoi(equalsPos + 1);
        } else {
            if(strlen(line) > 0) {
                int realKey = -1;
                for(int i = 0; i < NUM_BUTTONS; i++) {
                    if(strcasecmp(line, inputGetKeyName(i)) == 0) {
                        realKey = i;
                        break;
                    }
                }

                int gbKey = -1;
                for(int i = 0; i < NUM_FUNC_KEYS; i++) {
                    if(strcasecmp(equalsPos + 1, gbKeyNames[i]) == 0) {
                        gbKey = i;
                        break;
                    }
                }

                if(gbKey != -1 && realKey != -1) {
                    KeyConfig* config = &keyConfigs.back();
                    config->funcKeys[realKey] = (FuncKey) gbKey;
                }
            }
        }
    }
}

void controlsCheckConfig() {
    if(keyConfigs.empty()) {
        keyConfigs.push_back(inputGetDefaultKeyConfig());
    }

    if(selectedKeyConfig >= keyConfigs.size()) {
        selectedKeyConfig = 0;
    }

    inputLoadKeyConfig(&keyConfigs[selectedKeyConfig]);
}

void controlsPrintConfig(FILE* file) {
    fprintf(file, "config=%d\n", selectedKeyConfig);
    for(unsigned int i = 0; i < keyConfigs.size(); i++) {
        fprintf(file, "(%s)\n", keyConfigs[i].name);
        for(int j = 0; j < NUM_BUTTONS; j++) {
            if(inputIsValidKey(j) && strlen(inputGetKeyName(j)) > 0) {
                fprintf(file, "%s=%s\n", inputGetKeyName(j), gbKeyNames[keyConfigs[i].funcKeys[j]]);
            }
        }
    }
}

int keyConfigChooser_option;
int keyConfigChooser_cursor;
int keyConfigChooser_scrollY;

void redrawKeyConfigChooser() {
    int &option = keyConfigChooser_option;
    int &scrollY = keyConfigChooser_scrollY;
    KeyConfig* config = &keyConfigs[selectedKeyConfig];

    uiClear();

    uiPrint("Config: ");
    if(option == -1) {
        uiSetTextColor(TEXT_COLOR_YELLOW);
        uiPrint("* %s *\n\n", config->name);
        uiSetTextColor(TEXT_COLOR_NONE);
    } else {
        uiPrint("  %s  \n\n", config->name);
    }

    uiPrint("              Button   Function\n\n");

    for(int i = 0, elements = 0; i < NUM_BUTTONS && elements < scrollY + uiGetHeight() - 7; i++) {
        if(!inputIsValidKey(i)) {
            continue;
        }

        if(elements < scrollY) {
            elements++;
            continue;
        }

        int len = 18 - (int) strlen(inputGetKeyName(i));
        while(len > 0) {
            uiPrint(" ");
            len--;
        }

        if(option == i) {
            uiSetLineHighlighted(true);
            uiPrint("* %s | %s *\n", inputGetKeyName(i), gbKeyNames[config->funcKeys[i]]);
            uiSetLineHighlighted(false);
        } else {
            uiPrint("  %s | %s  \n", inputGetKeyName(i), gbKeyNames[config->funcKeys[i]]);
        }

        elements++;
    }

    uiPrint("\nPress X to make a new config.");
    if(selectedKeyConfig != 0) {
        uiPrint("\nPress Y to delete this config.");
    }

    uiFlush();
}

void updateKeyConfigChooser() {
    bool redraw = false;

    int &option = keyConfigChooser_option;
    int &cursor = keyConfigChooser_cursor;
    int &scrollY = keyConfigChooser_scrollY;
    KeyConfig* config = &keyConfigs[selectedKeyConfig];

    UIKey key;
    while((key = uiReadKey()) != UI_KEY_NONE) {
        if(key == UI_KEY_B) {
            inputLoadKeyConfig(config);
            closeSubMenu();
        } else if(key == UI_KEY_X) {
            keyConfigs.push_back(KeyConfig(*config));
            selectedKeyConfig = (u32) keyConfigs.size() - 1;
            char name[32];
            sprintf(name, "Custom %d", (int) keyConfigs.size() - 1);
            strcpy(keyConfigs.back().name, name);
            option = -1;
            cursor = -1;
            scrollY = 0;
            redraw = true;
        } else if(key == UI_KEY_Y) {
            if(selectedKeyConfig != 0) /* can't erase the default */ {
                keyConfigs.erase(keyConfigs.begin() + selectedKeyConfig);
                if(selectedKeyConfig >= keyConfigs.size()) {
                    selectedKeyConfig = (u32) keyConfigs.size() - 1;
                }

                redraw = true;
            }
        } else if(key == UI_KEY_DOWN) {
            if(option == (int) NUM_BUTTONS - 1) {
                option = -1;
                cursor = -1;
            } else {
                cursor++;
                option++;
                while(!inputIsValidKey(option)) {
                    option++;
                    if(option >= (int) NUM_BUTTONS) {
                        option = -1;
                        cursor = -1;
                        break;
                    }
                }
            }

            if(cursor < 0) {
                scrollY = 0;
            } else {
                while(cursor < scrollY) {
                    scrollY--;
                }

                while(cursor >= scrollY + uiGetHeight() - 7) {
                    scrollY++;
                }
            }

            redraw = true;
        } else if(key == UI_KEY_UP) {
            if(option == -1) {
                option = (int) NUM_BUTTONS - 1;
                while(!inputIsValidKey(option)) {
                    option--;
                    if(option < 0) {
                        option = -1;
                        cursor = -1;
                        break;
                    }
                }

                if(option != -1) {
                    cursor = 0;
                    for(int i = 0; i < NUM_BUTTONS; i++) {
                        if(inputIsValidKey(i)) {
                            cursor++;
                        }
                    }
                }
            } else {
                cursor--;
                option--;
                while(!inputIsValidKey(option)) {
                    option--;
                    if(option < 0) {
                        option = -1;
                        cursor = -1;
                        break;
                    }
                }
            }

            if(cursor < 0) {
                scrollY = 0;
            } else {
                while(cursor < scrollY) {
                    scrollY--;
                }

                while(cursor >= scrollY + uiGetHeight() - 7) {
                    scrollY++;
                }
            }

            redraw = true;
        } else if(key == UI_KEY_LEFT) {
            if(option == -1) {
                if(selectedKeyConfig == 0) {
                    selectedKeyConfig = keyConfigs.size() - 1;
                } else {
                    selectedKeyConfig--;
                }
            } else {
                if(config->funcKeys[option] <= 0) {
                    config->funcKeys[option] = (FuncKey) (NUM_FUNC_KEYS - 1);
                } else {
                    config->funcKeys[option] = (FuncKey) ((u32) config->funcKeys[option] - 1);
                }
            }

            redraw = true;
        } else if(key == UI_KEY_RIGHT) {
            if(option == -1) {
                selectedKeyConfig++;
                if(selectedKeyConfig >= keyConfigs.size()) {
                    selectedKeyConfig = 0;
                }
            } else {
                if(config->funcKeys[option] >= NUM_FUNC_KEYS - 1) {
                    config->funcKeys[option] = FUNC_KEY_NONE;
                } else {
                    config->funcKeys[option] = (FuncKey) ((u32) config->funcKeys[option] + 1);
                }
            }

            redraw = true;
        }
    }

    if(redraw) {
        redrawKeyConfigChooser();
    }
}

void startKeyConfigChooser() {
    keyConfigChooser_option = -1;
    keyConfigChooser_cursor = -1;
    keyConfigChooser_scrollY = 0;
    displaySubMenu(updateKeyConfigChooser);
    redrawKeyConfigChooser();
}
