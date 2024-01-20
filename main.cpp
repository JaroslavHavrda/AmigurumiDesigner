#include "include/SDL_wrappers.h"
#include "include/ImGUI_Wrapper.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <sstream>
#include <exception>


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

void setup_sdl(SDL_Window * window, SDL_GLContext & gl_context)
{
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);
}

ImGuiIO& setup_imgui()
{
    IMGUI_CHECKVERSION();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    ImGui::StyleColorsDark();
    return io;
}

class imput_controls{
    char amigurimi_prescription[200];
    public:
    void draw_controls()
    {
        ImGui::NewFrame();
        ImGui::Begin("Amigurumi prescription");
        ImGui::InputTextMultiline("prescription", amigurimi_prescription, sizeof(amigurimi_prescription));
        ImGui::End();
        ImGui::Render();
    }
};

void draw_result( ImGuiIO&  io)
{
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
}

int main(int, char**)
{
    SDL s;
    set_sdl_attributes();
    
    window_holder window;
    SDL_GLContext_Holder gl_context{window.sdl_window()};

    setup_sdl(window.sdl_window(), gl_context.gl_context);
    Imgui_Context_Holder imgui_context;
    auto io = setup_imgui();

    imgui_impl_opengl3 imgul_opemngl(window.sdl_window(), gl_context.gl_context);
    imgui_impl_sdl2 imgui_sdl;

    imput_controls input;

    while (!should_quit(window.sdl_window()))
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();

        input.draw_controls();
        draw_result(io);
        
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window.sdl_window());
    }
    return 0;
}