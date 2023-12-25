#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL.h>
#include <SDL_opengl.h>
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

void set_sdl_attributes()
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

bool should_quit( SDL_Window * window)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
            return true;
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
            return true;
    }
    return false;
}

// Main code
int main(int, char**)
{
    SDL s;
    set_sdl_attributes();
    
    window_holder window;

    SDL_GLContext_Holder gl_context{window.sdl_window()};
    SDL_GL_MakeCurrent(window.sdl_window(), gl_context.gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    IMGUI_CHECKVERSION();
    Imgui_Context_Holder imgui_context;
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();

    imgui_impl_opengl3 imgul_opemngl(window.sdl_window(), gl_context.gl_context);
    imgui_impl_sdl2 imgui_sdl;

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    while (!should_quit(window.sdl_window()))
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        {
            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            ImGui::SameLine();

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window.sdl_window());
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    return 0;
}