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

class SDL_GLContext_Holder
{
    public:
    SDL_GLContext gl_context;
    SDL_GLContext_Holder( SDL_Window* window)
    {
        gl_context = SDL_GL_CreateContext(window);
    }
    ~SDL_GLContext_Holder()
    {
        SDL_GL_DeleteContext(gl_context);
    }
};

inline void set_sdl_attributes()
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
}

class window_holder
{
    SDL_Window *window;

public:
    window_holder()
    {
        SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
        if (!window)
        {
            std::stringstream ss;
            ss << "Error: SDL_CreateWindow() " << SDL_GetError();
            throw std::runtime_error(ss.str());
        }
    }
    ~window_holder()
    {
        SDL_DestroyWindow(window);
    }
    auto sdl_window()
    {
        return window;
    }
};