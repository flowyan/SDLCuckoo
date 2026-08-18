#pragma once
#include "../App.h"

inline SDL_AppResult Check(const bool didnt_pass, const char *name) {
    if (didnt_pass) {
        SDL_LogError(App::APP_LOG_CATEGORY_GENERIC, "Error check for '%s' failed: %s", name, SDL_GetError());
        throw SDL_APP_FAILURE;
    }
    return SDL_APP_CONTINUE;
}
