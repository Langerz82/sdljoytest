/* gamepad_info, display gamepad SDLGamecontrollerDB mappings and information
 * used in EmuELEC, original taken from:
 *
 * Test gamepad axis/buttons with SDL2
 * Also tests new SDL2 features: game controller, joystick/gamepad, hotplug and haptics/rumble.
 *
 * (c) Wintermute0110 <wintermute0110@gmail.com> December 2014
 */
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <libudev.h>
#include <vector>
#include <algorithm>

SDL_Joystick *joy = NULL;
SDL_GameController *gamepad = NULL;

int SDL_joystick_has_hat = 0;
SDL_JoystickID instanceID = -1;
int device_index_in_use = -1;
int SDL_joystick_is_gamepad = 0;

std::vector<std::pair<int,const char*>> get_joystick_list(int *count) {
    std::vector<std::pair<int,const char*>> list;
    struct udev *udev = udev_new();
    struct udev_enumerate *enumerate = udev_enumerate_new(udev);

    // Match only the "joystick" device nodes (jsX)
    udev_enumerate_add_match_subsystem(enumerate, "input");
    udev_enumerate_add_match_property(enumerate, "ID_INPUT_JOYSTICK", "1");
    udev_enumerate_scan_devices(enumerate);

    struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry *entry;

    int n = 0;
    udev_list_entry_foreach(entry, devices) { n++; }

    if (n == 0) {
        *count = 0;
        udev_enumerate_unref(enumerate);
        udev_unref(udev);
        return list;
    }

    int *list = (int*)malloc(sizeof(int) * n);
    int i = 0;

    udev_list_entry_foreach(entry, devices) {
        const char *path = udev_list_entry_get_name(entry);
        struct udev_device *dev = udev_device_new_from_syspath(udev, path);
        const char *sysname = udev_device_get_sysname(dev); // e.g., "js0" or "eventX"

        // Only process the legacy 'js' nodes to easily extract the index
        if (strncmp(sysname, "js", 2) == 0) {
            int index = atoi(sysname + 2);
            const char *name = udev_device_get_sysattr_value(dev, "name");
            list.push_back(std::make_pair(index, name));
        }
        udev_device_unref(dev);
    }

    *count = i;
    udev_enumerate_unref(enumerate);
    udev_unref(udev);

    std::sort(list.begin(), list.end());

    return list;
}

int main(int argn, char **argv)
{
    bool moreinfo = false;

    if (argn > 1 && strncmp(argv[1], "-more", 10) == 0) {
        moreinfo = true;
    }

    // Init SDL subsystems
    if (SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC)) {
        printf("SDL_Init() failed: %s\n", SDL_GetError());
        return 1;
    }

    // Load custom gamecontrollerdb file if environment variable is set
    const char *db_file = SDL_getenv("SDL_GAMECONTROLLERCONFIG_FILE");
    if (db_file) {
        if (moreinfo)
            printf("Loading mappings from %s\n", db_file);
        SDL_GameControllerAddMappingsFromFile(db_file);
    }

    int numJoysticks = SDL_NumJoysticks();
    if (numJoysticks <= 0) {
        printf("No joystick/gamepad detected.\n");
        SDL_Quit();
        return 0;
    }

    int joystick_count = 0;
    std::vector<std::pair<int,const char*>> udev_list = get_joystick_list(&joystick_count);

    for (int i = 0; i < numJoysticks; i++) {
        // Try to open as gamepad first
        gamepad = SDL_GameControllerOpen(i);

        int udev_index = (i < udev_list.size()) ? udev_list[i].first : -1;
        const char* udev_name = (i < udev_list.size()) ? udev_list[i].second : "";

        if (gamepad == NULL) {
            SDL_joystick_is_gamepad = 0;
            joy = SDL_JoystickOpen(i);
            if (joy == NULL) {
                printf("Could not open joystick %d: %s\n", i, SDL_GetError());
                continue;
            }

            instanceID = SDL_JoystickInstanceID(joy);
            device_index_in_use = i;

            char guid[64];
            SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy), guid, sizeof(guid));
            int num_hats = SDL_JoystickNumHats(joy);
            SDL_joystick_has_hat = num_hats > 0;

            if (moreinfo) {
                printf("\nJoystick %d \n", i);
                printf("UDEV name:       %s\n", udev_name);
                printf("SDL name:        %s\n", SDL_JoystickName(joy));
                printf("SDL GUID:        %s\n", guid);
                printf("Axes:            %d\n", SDL_JoystickNumAxes(joy));
/*
                printf("Buttons:         %d\n", SDL_JoystickNumButtons(joy));
                printf("Hats:            %d\n", num_hats);
                printf("Balls:           %d\n", SDL_JoystickNumBalls(joy));
*/
                printf("Instance ID:     %d\n", instanceID);
                printf("jsindex:         %d\n", udev_index);
            } else {
                printf("%s%%%s\n", SDL_JoystickName(joy), guid);
            }

            SDL_JoystickClose(joy);
            joy = NULL;

        } else {
            SDL_joystick_is_gamepad = 1;
            joy = SDL_GameControllerGetJoystick(gamepad);
            instanceID = SDL_JoystickInstanceID(joy);
            device_index_in_use = i;

            char guid[64];
            SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy), guid, sizeof(guid));
            const char *mapping = SDL_GameControllerMapping(gamepad);

            if (moreinfo) {
                printf("\nGamepad %d\n", i);
                printf("UDEV name:       %s\n", udev_name);
                printf("SDL name:        %s\n", SDL_GameControllerName(gamepad));
                printf("SDL GUID:        %s\n", guid);
                printf("Mapping:         %s\n", mapping ? mapping : "(no mapping)");
/*              printf("Axes:            %d\n", SDL_JoystickNumAxes(joy));
                printf("Buttons:         %d\n", SDL_JoystickNumButtons(joy));
                printf("Hats:            %d\n", SDL_JoystickNumHats(joy));
*/
                printf("Instance ID:     %d\n", instanceID);
                printf("jsindex:         %d\n", udev_index);
            } else {
                printf("%s\n", mapping ? mapping : "(no mapping)");
            }

            SDL_GameControllerClose(gamepad);
            gamepad = NULL;
        }

    }

    SDL_QuitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC);
    SDL_Quit();
    return 0;
}
