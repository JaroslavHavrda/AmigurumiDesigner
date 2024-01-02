#pragma once
#include <SDL.h>
#include <sstream>
#include <exception>

class SDL
{
    public:
    SDL()
    {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
        {
            std::stringstream ss;
            ss << "Error: " << SDL_GetError();
            throw std::runtime_error(ss.str());
        }
    }
    ~SDL()
    {
        SDL_Quit();
    }
};