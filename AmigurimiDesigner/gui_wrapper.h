#pragma once

#define NOMINMAX

#include "D3DDeviceHolder.h"

#include <array>
#include <Windows.h>
#include <wrl\client.h>
#include <d3d11.h>
#include <directxmath.h>
#include <vector>
#include <optional>

struct gui_wrapper
{
    std::array<char, 500> prescription{ '1'};

    void present_using_imgui();
};

struct global_resources
{
    vertex_shader_holder vertex_shader;
    Microsoft::WRL::ComPtr <ID3D11PixelShader> pixel_shader;
    Microsoft::WRL::ComPtr <ID3D11Buffer> constant_buffer;
};

extern UINT                     g_ResizeWidth, g_ResizeHeight;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

struct WindowClassWrapper
{
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Amigurumi Designer", nullptr };

    WindowClassWrapper();
    ~WindowClassWrapper();

};

struct rotation_data_gui
{
    float roll{ 0 };
    float pitch{ 0 };
    float yaw{ 0 };
};

struct render_target_view_holder
{
    Microsoft::WRL::ComPtr <ID3D11Texture2D> pBackBuffer;
    Microsoft::WRL::ComPtr <ID3D11RenderTargetView> g_mainRenderTargetView;
    D3D11_TEXTURE2D_DESC    m_bbDesc{};
    D3D11_VIEWPORT          m_viewport{};
};

struct HwndWrapper
{
    HWND hwnd;

    HwndWrapper(WNDCLASSEXW& wc);
    ~HwndWrapper();
    void show_window() const;
};

struct imgui_context_holder
{
    imgui_context_holder();
    ~imgui_context_holder();
};

struct imgui_win32_holder
{
    imgui_win32_holder(HWND hwnd);
    ~imgui_win32_holder();
};

struct imgui_dx11_holder
{
    imgui_dx11_holder(ID3D11Device* g_pd3dDevice, ID3D11DeviceContext* g_pd3dDeviceContext);
    ~imgui_dx11_holder();
};


struct imgui_holder
{
    imgui_context_holder imgui_context;
    imgui_win32_holder imgui_win32;
    imgui_dx11_holder imgui_dx11;

    imgui_holder(HWND hwnd, Microsoft::WRL::ComPtr<ID3D11Device>& g_pd3dDevice, Microsoft::WRL::ComPtr <ID3D11DeviceContext>& g_pd3dDeviceContext);
};

render_target_view_holder init_render_target_view(D3DDeviceHolder& d3ddevice);

struct application_basics
{
    WindowClassWrapper window_class;
    HwndWrapper window{ window_class.wc };
    D3DDeviceHolder d3dDevice = init_d3d_device(window.hwnd);
    std::optional<render_target_view_holder> target_view = init_render_target_view(d3dDevice);
    imgui_holder imgui_instance{ window.hwnd, d3dDevice.g_pd3dDevice, d3dDevice.g_pd3dDeviceContext };

    application_basics();
    void update_after_resize();
    void clear_render_target_view();
    void update_constant_struct(Microsoft::WRL::ComPtr <ID3D11Buffer>& constant_buffer);
    Microsoft::WRL::ComPtr <ID3D11Texture2D> create_depth_stencil();
    void set_depth_stencil_to_scene(Microsoft::WRL::ComPtr <ID3D11DepthStencilView>& m_pDepthStencilView);
    void draw_vertices(const vertex_representation& vertices, global_resources& application_resources);
};