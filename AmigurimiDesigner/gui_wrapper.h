#pragma once

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <array>
#include <Windows.h>
#include <wrl\client.h>
#include <d3d11.h>
#include <stdexcept>
#include <directxmath.h>
#include <iostream>
#include <windowsx.h>
#include <vector>
#include <fstream>
#include <optional>

// Forward declare message handler from imgui_impl_win32.cpp
IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

struct MousePosition
{
    int x = 0;
    int y = 0;
};

struct ViewportConfigurationManager
{
    DirectX::XMVECTOR eye = DirectX::XMVectorSet(0.0f, 0.7f, 1.5f, 0.f);
    DirectX::XMVECTOR at = DirectX::XMVectorSet(0.0f, -0.1f, 0.0f, 0.f);
    DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.f);
    DirectX::XMVECTOR rotation_center = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.f);

    MousePosition current_pos;
    MousePosition drag_start;
    bool dragging = false;

    DirectX::XMVECTOR calc_eye() const;
    DirectX::XMVECTOR calc_up() const;
    void stop_dragging();
    void reset_defaults();
    void front_view();
    void top_view();
};

extern ViewportConfigurationManager viewport_config;

struct gui_wrapper
{
    std::array<char, 500> prescription{ 0 };

    void present_using_imgui();
};

struct vertex_shader_holder
{
    Microsoft::WRL::ComPtr <ID3D11VertexShader> m_pVertexShader;
    Microsoft::WRL::ComPtr <ID3D11InputLayout> m_pInputLayout;
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

struct ConstantBufferStruct {
    DirectX::XMFLOAT4X4 world;
    DirectX::XMFLOAT4X4 view;
    DirectX::XMFLOAT4X4 projection;
};
static_assert((sizeof(ConstantBufferStruct) % 16) == 0, "Constant Buffer size must be 16-byte aligned");

struct rotation_data_gui
{
    float roll{ 0 };
    float pitch{ 0 };
    float yaw{ 0 };
};

ConstantBufferStruct calculate_projections(const D3D11_TEXTURE2D_DESC& m_bbDesc);

struct frame_resources
{
    Microsoft::WRL::ComPtr <ID3D11Buffer> vertex_buffer;
    Microsoft::WRL::ComPtr <ID3D11Buffer> index_buffer;
    UINT m_indexCount;
};

struct VertexPositionColor
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT3 color;
};

struct vertex_representation
{
    std::vector<VertexPositionColor> CubeVertices;
    std::vector<unsigned short> CubeIndices;
};

void test_hresult(HRESULT hr, const char* message);

struct D3DDeviceHolder
{
    Microsoft::WRL::ComPtr<ID3D11Device> g_pd3dDevice;
    Microsoft::WRL::ComPtr <ID3D11DeviceContext> g_pd3dDeviceContext;
    Microsoft::WRL::ComPtr < IDXGISwapChain> g_pSwapChain;

    vertex_shader_holder load_vertex_shader() const;
    Microsoft::WRL::ComPtr <ID3D11PixelShader> load_pixel_shader() const;
    Microsoft::WRL::ComPtr <ID3D11Buffer> create_constant_buffer();
    bool is_ocluded() const;
    Microsoft::WRL::ComPtr <ID3D11Buffer> create_vertex_buffer(const std::vector<VertexPositionColor>& CubeVertices) const;
    Microsoft::WRL::ComPtr <ID3D11Buffer> create_index_buffer(std::vector<unsigned short> CubeIndices) const;
    frame_resources prepare_frame_resources(const vertex_representation& vertices);
    Microsoft::WRL::ComPtr <ID3D11DepthStencilView> create_depth_stencil_view(Microsoft::WRL::ComPtr<ID3D11Texture2D>& m_pDepthStencil) const;
    void set_buffers(Microsoft::WRL::ComPtr <ID3D11Buffer>& vertex_buffer, Microsoft::WRL::ComPtr < ID3D11Buffer>& index_buffer) const;
    void setup_shaders(const vertex_shader_holder& vertex_shader,
        Microsoft::WRL::ComPtr <ID3D11Buffer>& constant_buffer, Microsoft::WRL::ComPtr <ID3D11PixelShader>& pixel_shader) const;
    void draw_scene(int m_indexCount) const;
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
    imgui_win32_holder(HWND hwnd)
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

D3DDeviceHolder init_d3d_device(HWND hWnd);

render_target_view_holder init_render_target_view(D3DDeviceHolder& d3ddevice);

void setup_imgui();

struct application_basics
{
    WindowClassWrapper window_class;
    HwndWrapper window{ window_class.wc };
    D3DDeviceHolder d3dDevice = init_d3d_device(window.hwnd);
    std::optional<render_target_view_holder> target_view = init_render_target_view(d3dDevice);
    imgui_holder imgui_instance{ window.hwnd, d3dDevice.g_pd3dDevice, d3dDevice.g_pd3dDeviceContext };

    application_basics();

    void update_after_resize()
    {
        target_view = std::optional<render_target_view_holder>{};
        d3dDevice.g_pd3dDeviceContext->ClearState();
        d3dDevice.g_pd3dDeviceContext->Flush();
        HRESULT hr = d3dDevice.g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
        test_hresult(hr, "could not resize buffers");
        target_view = init_render_target_view(d3dDevice);
    }

    void clear_render_target_view()
    {
        const float teal[] = { 0.098f, 0.439f, 0.439f, 1.000f };
        d3dDevice.g_pd3dDeviceContext->ClearRenderTargetView(
            target_view->g_mainRenderTargetView.Get(),
            teal
        );
    }

    void update_constant_struct(Microsoft::WRL::ComPtr <ID3D11Buffer>& constant_buffer)
    {
        auto constant_struct = calculate_projections(target_view->m_bbDesc);

        d3dDevice.g_pd3dDeviceContext->UpdateSubresource(
            constant_buffer.Get(),
            0,
            nullptr,
            &(constant_struct),
            0,
            0
        );
    }

    Microsoft::WRL::ComPtr <ID3D11Texture2D> create_depth_stencil()
    {
        Microsoft::WRL::ComPtr <ID3D11Texture2D> m_pDepthStencil;
        CD3D11_TEXTURE2D_DESC depthStencilDesc(
            DXGI_FORMAT_D24_UNORM_S8_UINT,
            static_cast<UINT> (target_view->m_bbDesc.Width),
            static_cast<UINT> (target_view->m_bbDesc.Height),
            1, // This depth stencil view has only one texture.
            1, // Use a single mipmap level.
            D3D11_BIND_DEPTH_STENCIL
        );

        HRESULT res = d3dDevice.g_pd3dDevice->CreateTexture2D(
            &depthStencilDesc,
            nullptr,
            m_pDepthStencil.GetAddressOf()
        );
        test_hresult(res, "could not create depth stencil");

        return m_pDepthStencil;
    }

    void set_depth_stencil_to_scene(Microsoft::WRL::ComPtr <ID3D11DepthStencilView>& m_pDepthStencilView)
    {
        d3dDevice.g_pd3dDeviceContext->ClearDepthStencilView(
            m_pDepthStencilView.Get(),
            D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
            1.0f,
            0);

        // Set the render target.
        d3dDevice.g_pd3dDeviceContext->OMSetRenderTargets(
            1,
            target_view->g_mainRenderTargetView.GetAddressOf(),
            m_pDepthStencilView.Get()
        );
    }
};