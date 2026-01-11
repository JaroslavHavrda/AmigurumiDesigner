module;

#define NOMINMAX

#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <wrl\client.h>

#include <Windows.h>
#include <d3d11.h>

export module imgui_holder;
import std;

export struct imgui_context_holder
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

export struct imgui_win32_holder
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