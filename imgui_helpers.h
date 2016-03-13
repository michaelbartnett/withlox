#ifndef IMGUI_HELPERS_H

#include "dearimgui/imgui.h"
#include <SDL_video.h>

inline ImVec2 ImGui_SDLWindowSize(SDL_Window *window)
{
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    return ImVec2(w, h);
}

#define IMGUI_HELPERS_H
#endif
