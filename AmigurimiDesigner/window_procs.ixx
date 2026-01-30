module;

#include "imgui_hack.h"

#include <directxmath.h>

#define NOMINMAX
#include <windowsx.h>
#include <Windows.h>

export module window_procs;
import viewport_configuration;
import std;

export UINT g_ResizeWidth = 0, g_ResizeHeight = 0;

static LRESULT handle_size(WPARAM wParam, LPARAM lParam)
{
    if (wParam == SIZE_MINIMIZED)
        return 0;
    g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
    g_ResizeHeight = (UINT)HIWORD(lParam);
    return 0;
}


static LRESULT handle_syscommand(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
        return 0;
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

static LRESULT handle_window_destroy()
{
    ::PostQuitMessage(0);
    return 0;
}

static LRESULT handle_lbutton_down(LPARAM lParam)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
    {
        return 0;
    }
    viewport_config.current_pos.x = GET_X_LPARAM(lParam);
    viewport_config.current_pos.y = GET_Y_LPARAM(lParam);
    viewport_config.drag_start.x = GET_X_LPARAM(lParam);
    viewport_config.drag_start.y = GET_Y_LPARAM(lParam);
    viewport_config.dragging = true;
    return 0;
}

static LRESULT handle_rbutton_down(LPARAM lParam)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
    {
        return 0;
    }
    viewport_config.current_pos.x = GET_X_LPARAM(lParam);
    viewport_config.current_pos.y = GET_Y_LPARAM(lParam);
    viewport_config.drag_start.x = GET_X_LPARAM(lParam);
    viewport_config.drag_start.y = GET_Y_LPARAM(lParam);
    viewport_config.right_dragging = true;
    return 0;
}

static LRESULT handle_mouse_move(LPARAM lParam)
{
    viewport_config.current_pos.x = GET_X_LPARAM(lParam);
    viewport_config.current_pos.y = GET_Y_LPARAM(lParam);
    return 0;
}

static LRESULT handle_l_button_up()
{
    viewport_config.stop_dragging_left();
    return 0;
}

static LRESULT handle_r_button_up()
{
    viewport_config.stop_dragging_right();
    return 0;
}

static LRESULT  handle_key_down(WPARAM wParam)
{
    using namespace DirectX;
    std::cout << "wparam " << wParam << std::endl;
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard)
    {
        return 0;
    }
    switch (wParam)
    {
    case VK_LEFT:
    {
        auto old_eye = viewport_config.calc_eye();
        viewport_config.at = viewport_config.shifted_at(10, 0);
        viewport_config.center_direction = XMVector3Normalize(viewport_config.at - old_eye);
        viewport_config.up = viewport_config.updated_up();
        break;
    }
    case VK_RIGHT:
    {
        auto old_eye = viewport_config.calc_eye();
        viewport_config.at = viewport_config.shifted_at(-10, 0);
        viewport_config.center_direction = XMVector3Normalize(viewport_config.at - old_eye);
        viewport_config.up = viewport_config.updated_up();
        break;
    }
    case VK_UP:
    {
        auto old_eye = viewport_config.calc_eye();
        viewport_config.at = viewport_config.shifted_at(0, 10);
        viewport_config.center_direction = XMVector3Normalize(viewport_config.at - old_eye);
        viewport_config.up = viewport_config.updated_up();
        break;
    }
    case VK_DOWN:
    {
        auto old_eye = viewport_config.calc_eye();
        viewport_config.at = viewport_config.shifted_at(0, -10);
        viewport_config.center_direction = XMVector3Normalize(viewport_config.at - old_eye);
        viewport_config.up = viewport_config.updated_up();
        break;
    }
    case 'A':
        viewport_config.at = viewport_config.shifted_at(10, 0);
        break;
    case 'W':
        viewport_config.at = viewport_config.shifted_at(0, 10);
        break;
    case 'S':
        viewport_config.at = viewport_config.shifted_at(0, -10);
        break;
    case 'D':
        viewport_config.at = viewport_config.shifted_at(-10, 0);
        break;
    case 'Q':
        viewport_config.zoom_factor /= 1.5f;
        break;
    case 'E':
        viewport_config.zoom_factor *= 1.5f;
        break;
    }
    return 0;
}

static LRESULT  handle_mouse_wheel(WPARAM wParam)
{
    using namespace DirectX;
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
    {
        return 0;
    }
    auto delta = (int16_t)HIWORD(wParam);
    auto scaling = exp(delta / 1200.);
    viewport_config.zoom_factor *= (float)scaling;
    return 0;
}

export LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (auto res = ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return res;

    switch (msg)
    {
    case WM_SIZE:
        return handle_size(wParam, lParam);
    case WM_SYSCOMMAND:
        return handle_syscommand(hWnd, msg, wParam, lParam);
    case WM_DESTROY:
        return handle_window_destroy();
    case WM_LBUTTONDOWN:
        return handle_lbutton_down(lParam);
    case WM_RBUTTONDOWN:
        return handle_rbutton_down(lParam);
    case WM_MOUSEMOVE:
        return handle_mouse_move(lParam);
    case WM_RBUTTONUP:
        return handle_r_button_up();
    case WM_LBUTTONUP:
        return handle_l_button_up();
    case WM_MOUSEWHEEL:
        return handle_mouse_wheel(wParam);
    case WM_KEYDOWN:
        return handle_key_down(wParam);
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

export struct WindowClassWrapper
{
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Amigurumi Designer", nullptr };

    WindowClassWrapper()
    {
        auto res = ::RegisterClassExW(&wc);
        if (res == 0)
        {
            throw std::runtime_error("could not register window class");
        }
    }
    ~WindowClassWrapper()
    {
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    }

};