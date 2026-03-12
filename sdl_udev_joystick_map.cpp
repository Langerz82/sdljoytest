#include <SDL2/SDL.h>
#include <libudev.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <filesystem>
#include <regex>

namespace fs = std::filesystem;

int DEBUG = 1;

struct udev_enumerate *enumerate = NULL;
struct udev *udev_joypad_fd = NULL;
struct udev_monitor *udev_joypad_mon    = NULL;

// Function to find udev index from SDL joystick index
int find_udev_index_from_sdl(int sdl_index) {
    if (SDL_Init(SDL_INIT_JOYSTICK) < 0) return -1;

    if (sdl_index < 0 || sdl_index >= SDL_NumJoysticks()) {
        fprintf(stderr, "Invalid SDL Joystick Index\n");
        return -1;
    }
    if (DEBUG) printf("SDL_NumJoysticks %d\n", SDL_NumJoysticks());

    // 1. Get Syspath from SDL
    const char *sdl_syspath = SDL_JoystickPathForIndex(sdl_index);
    if (!sdl_syspath) return -1;

    if (DEBUG) printf("sdl_syspath %s\n", sdl_syspath);

    if (!(udev_joypad_fd = udev_new()))
       return -1;

    if ((udev_joypad_mon = udev_monitor_new_from_netlink(udev_joypad_fd, "udev")))
    {
       udev_monitor_filter_add_match_subsystem_devtype(
             udev_joypad_mon, "input", NULL);
       udev_monitor_enable_receiving(udev_joypad_mon);
    }

    if (!(enumerate = udev_enumerate_new(udev_joypad_fd)))
       goto error;

    // 2. Iterate udev devices
//    struct udev *udev = udev_new();
    //struct udev_enumerate *enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_property(enumerate, "ID_INPUT_JOYSTICK", "1");
    udev_enumerate_add_match_subsystem(enumerate, "input");
    udev_enumerate_scan_devices(enumerate);
    struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry *entry;

    int udev_index = 0;
    int found_index = -1;

    std::regex event_pattern(R"(^/dev/input/event[0-9]+$)");
    udev_list_entry_foreach(entry, devices) {
        const char *path = udev_list_entry_get_name(entry);
        struct udev_device *dev = udev_device_new_from_syspath(udev_joypad_fd, path);

        // Match /sys path
        const char *devnode = udev_device_get_devnode(dev);
        if (devnode) {
            if (DEBUG) printf("devnode %s\n", devnode);

            if (std::regex_match(devnode, event_pattern))
            {
                if (strcmp(devnode, sdl_syspath) == 0) {
                    found_index = udev_index;
                    udev_device_unref(dev);
                    break;
                }
                udev_index++;
            }
        }
        udev_device_unref(dev);
    }

    udev_enumerate_unref(enumerate);
    udev_unref(udev_joypad_fd);
    SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
    return found_index;

error:
    udev_joypad_destroy();
    return -1;
}

void udev_joypad_destroy(void)
{
   if (udev_joypad_mon)
      udev_monitor_unref(udev_joypad_mon);

   if (udev_joypad_fd)
      udev_unref(udev_joypad_fd);

   udev_joypad_mon = NULL;
   udev_joypad_fd  = NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <SDL_Joystick_Index>\n", argv[0]);
        return 1;
    }

    int sdl_idx = atoi(argv[1]);
    int udev_idx = find_udev_index_from_sdl(sdl_idx);

    if (udev_idx != -1)
        printf("%d\n", udev_idx);
    else
        printf("\n");

    /*if (udev_idx != -1) {
        printf("SDL Index %d matches udev index: %d\n", sdl_idx, udev_idx);
    } else {
        printf("Could not find matching udev device for SDL index %d\n", sdl_idx);
    }*/

    return 0;
}
