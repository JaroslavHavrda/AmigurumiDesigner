module;
#define NOMINMAX


#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "imgui_hack.h"

#include <windowsx.h>
#include <Windows.h>
#include <wrl\client.h>
#include <d3d11.h>
#include <directxmath.h>

export module gui_wrapper;
import viewport_configuration;
import std;

export struct gui_wrapper
{
    std::array<char, 500> prescription{ '1', ',', '4', ',', '1', ',', '4'};

    void present_using_imgui( const float height, const D3D11_TEXTURE2D_DESC& m_bbDesc, const DirectX::XMFLOAT3 center, const std::string_view error)
    {
        using namespace DirectX;
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Amigurumi designer");
        ImGui::InputTextMultiline("Prescription", prescription.data(), prescription.size());
        ImGui::LabelText("", error.data());
        ImGui::End();
        ImGui::Begin("Rotation");
        if (ImGui::Button("default view")) {
            viewport_config.reset_defaults(center);
        }
        if (ImGui::Button("front view")) {
            viewport_config.front_view(center);
        }
        if (ImGui::Button("top view")) {
            viewport_config.top_view(center);
        }
        if (ImGui::Button("optimal zoom"))
        {
            viewport_config.optimal_size(height, m_bbDesc);
        }
        if (ImGui::Button("center the object"))
        {
            viewport_config.set_at(center);
        }
        if (ImGui::Button("orbit left"))
        {
            viewport_config.center_direction = viewport_config.rotated_center_direction(30, 0);
            viewport_config.up = viewport_config.updated_up();
        }
        if (ImGui::Button("orbit right"))
        {
            viewport_config.center_direction = viewport_config.rotated_center_direction(-30, 0);
            viewport_config.up = viewport_config.updated_up();
        }
        if (ImGui::Button("orbit up"))
        {
            viewport_config.center_direction = viewport_config.rotated_center_direction(0, 30);
            viewport_config.up = viewport_config.updated_up();
        }
        if (ImGui::Button("orbit down"))
        {
            viewport_config.center_direction = viewport_config.rotated_center_direction(0,-30);
            viewport_config.up = viewport_config.updated_up();
        }
        ImGui::End();
        ImGui::Render();
    }
};

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

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

struct rotation_data_gui
{
    float roll{ 0 };
    float pitch{ 0 };
    float yaw{ 0 };
};

export struct HwndWrapper
{
    HWND hwnd;

    HwndWrapper(WNDCLASSEXW& wc)
    {
        hwnd = ::CreateWindowW(wc.lpszClassName, L"Amigurumi designer", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);
        if (hwnd == NULL)
        {
            throw std::runtime_error("could not creatw window");
        }
    }
    ~HwndWrapper()
    {
        ::DestroyWindow(hwnd);
    }
    void show_window() const
    {
        ::ShowWindow(hwnd, SW_SHOWDEFAULT);
        ::UpdateWindow(hwnd);
    }
};

struct imgui_context_holder
{
    imgui_context_holder()
    {
        ImGui::CreateContext();
    }
    ~imgui_context_holder()
    {
        ImGui::DestroyContext();
    }
};

struct imgui_win32_holder
{
    imgui_win32_holder(const HWND hwnd)
    {
        auto res = ImGui_ImplWin32_Init(hwnd);
        if (!res)
        {
            throw std::runtime_error("could not init imgui win32");
        }
    }
    ~imgui_win32_holder()
    {
        ImGui_ImplWin32_Shutdown();
    }
};

struct imgui_dx11_holder
{
    imgui_dx11_holder(ID3D11Device* g_pd3dDevice, ID3D11DeviceContext* g_pd3dDeviceContext)
    {
        auto res = ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
        if (!res)
        {
            throw std::runtime_error("could not init imgui DX11");
        }
    }
    ~imgui_dx11_holder()
    {
        ImGui_ImplDX11_Shutdown();
    }
};

export struct imgui_holder
{
    imgui_context_holder imgui_context;
    imgui_win32_holder imgui_win32;
    imgui_dx11_holder imgui_dx11;

    imgui_holder(const HWND hwnd, const Microsoft::WRL::ComPtr<ID3D11Device>& d3d_device, const Microsoft::WRL::ComPtr <ID3D11DeviceContext>& g_pd3dDeviceContext) :
        imgui_context{},
        imgui_win32{ hwnd },
        imgui_dx11{ d3d_device.Get(), g_pd3dDeviceContext.Get() }
    {

    }
};
