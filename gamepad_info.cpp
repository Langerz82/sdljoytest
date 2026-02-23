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

SDL_Joystick *joy = NULL;
SDL_GameController *gamepad = NULL;

int SDL_joystick_has_hat = 0;
SDL_JoystickID instanceID = -1;
int device_index_in_use = -1;
int SDL_joystick_is_gamepad = 0;

// Comparison function for ascending order
int compare(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

/**
 * Enumerates joysticks and returns an array of JoystickInfo structs.
 * @param count Pointer to an integer to store the number of joysticks found.
 * @return A dynamically allocated array of JoystickInfo structs.
 */
int* get_joystick_list(int *count) {
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
        return NULL;
    }

    int *list = (int*)malloc(sizeof(int) * n);
    int i = 0;

    udev_list_entry_foreach(entry, devices) {
        const char *path = udev_list_entry_get_name(entry);
        struct udev_device *dev = udev_device_new_from_syspath(udev, path);
        const char *sysname = udev_device_get_sysname(dev); // e.g., "js0" or "eventX"

        // Only process the legacy 'js' nodes to easily extract the index
        if (strncmp(sysname, "js", 2) == 0) {
            list[i++] = atoi(sysname + 2);
        }
        udev_device_unref(dev);
    }

    *count = i;
    udev_enumerate_unref(enumerate);
    udev_unref(udev);

    qsort(list, i, sizeof(int), compare);

    return list;
}

// Read UDEV name from /sys/class/input/jsX/device/name
static void get_udev_name(int index, char *buf, size_t bufsize) {
    char path[128];
    snprintf(path, sizeof(path), "/sys/class/input/js%d/device/name", index);
    FILE *f = fopen(path, "r");
    if (!f) {
        snprintf(buf, bufsize, "(unavailable)");
        return;
    }
    if (!fgets(buf, bufsize, f)) {
        snprintf(buf, bufsize, "(unknown)");
        fclose(f);
        return;
    }
    buf[strcspn(buf, "\n")] = '\0';
    fclose(f);
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
    int *joystick_indexes = get_joystick_list(&joystick_count);

    for (int i = 0; i < numJoysticks; i++) {
        char udev_name[128];
        if (i < joystick_count)
          get_udev_name(joystick_indexes[i], udev_name, sizeof(udev_name));

        // Try to open as gamepad first
        gamepad = SDL_GameControllerOpen(i);
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
                printf("jsindex:         %d\n", joystick_indexes[i]);
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
                printf("jsindex:         %d\n", joystick_indexes[i]);
            } else {
                printf("%s\n", mapping ? mapping : "(no mapping)");
            }

            SDL_GameControllerClose(gamepad);
            gamepad = NULL;
        }

    }

    free(joystick_indexes);

    SDL_QuitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC);
    SDL_Quit();
    return 0;
}
