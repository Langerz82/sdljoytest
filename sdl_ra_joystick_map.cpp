#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <algorithm>

int DEBUG = 1;

// Function to find udev index from SDL joystick index
int find_index_from_sdl(int sdl_index) {
    if (SDL_Init(SDL_INIT_JOYSTICK) < 0) return -1;

    int count = SDL_NumJoysticks();
    //int count = 4;
    if (sdl_index < 0 || sdl_index >= count) {
        fprintf(stderr, "Invalid SDL Joystick Index\n");
        return -1;
    }
    if (DEBUG) printf("SDL_NumJoysticks %d\n", count);

    // 1. Get Syspath from SDL
    std::vector<std::pair<std::string,int>> sdlpaths = {};

    //sdlpaths.push_back(std::make_pair("/dev/input/event7", 0));
    //sdlpaths.push_back(std::make_pair("/dev/input/event10", 1));
    //sdlpaths.push_back(std::make_pair("/dev/input/event11", 2));
    //sdlpaths.push_back(std::make_pair("/dev/input/event14", 3));

    for(int i=0; i < count; ++i) {
        const char *sdl_syspath = SDL_JoystickPathForIndex(i);
        if (!sdl_syspath) continue;
        std::string syspath_str(sdl_syspath);
        sdlpaths.push_back(std::make_pair(syspath_str, i));
    }

    for(int i=0; i < count; ++i) {
        if (DEBUG) {
            printf("BEFORE SORT");
            printf("%s %d\n", sdlpaths[i].first.c_str(), sdlpaths[i].second);
        }
    }

    std::sort(sdlpaths.begin(), sdlpaths.end());

    for(int i=0; i < count; ++i) {
        if (DEBUG) {
            printf("AFTER SORT");
            printf("%s %d\n", sdlpaths[i].first.c_str(), sdlpaths[i].second);
        }
    }

    SDL_QuitSubSystem(SDL_INIT_JOYSTICK);

    for(int i=0; i < count; ++i) {
        if (sdlpaths[i].second == sdl_index)
            return i;
    }
    return -1;
    //return sdlpaths[sdl_index].second;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <SDL_Joystick_Index>\n", argv[0]);
        return 1;
    }

    int sdl_idx = atoi(argv[1]);
    int udev_idx = find_index_from_sdl(sdl_idx);

    if (udev_idx != -1)
        printf("%d\n", udev_idx);
    else
        printf("\n");

    return 0;
}
