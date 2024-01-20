#pragma once

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

class Imgui_Context_Holder
{
    public:
    Imgui_Context_Holder()
    {
        ImGui::CreateContext();
    }
    ~Imgui_Context_Holder()
    {
        ImGui::DestroyContext();
    }
};

class imgui_impl_opengl3
{
    public:
    imgui_impl_opengl3(SDL_Window * window, SDL_GLContext & gl_context)
    {
        ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    }
    ~imgui_impl_opengl3()
    {
        ImGui_ImplOpenGL3_Shutdown();
    }
};

class imgui_impl_sdl2{
    public:
    imgui_impl_sdl2()
    {
        const char* glsl_version = "#version 130";
        ImGui_ImplOpenGL3_Init(glsl_version);
    }
    ~imgui_impl_sdl2()
    {
        ImGui_ImplSDL2_Shutdown();
    }
};
