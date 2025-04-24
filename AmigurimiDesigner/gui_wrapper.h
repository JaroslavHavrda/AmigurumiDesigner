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

    void present_using_imgui()
    {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Amigurumi designer");
        ImGui::InputTextMultiline("Prescription", prescription.data(), prescription.size());
        ImGui::End();
        ImGui::Begin("Rotation");
        if (ImGui::Button("default view")) {
            viewport_config.reset_defaults();
        }
        if (ImGui::Button("front view")) {
            viewport_config.front_view();
        }
        if (ImGui::Button("top view")) {
            viewport_config.top_view();
        }
        ImGui::End();
        ImGui::Render();
    }
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

    vertex_shader_holder load_vertex_shader() const
    {
        vertex_shader_holder vs;
        std::ifstream vShader{ "VertexShader.cso", std::ios::binary };
        std::vector<char> fileContents((std::istreambuf_iterator<char>(vShader)), std::istreambuf_iterator<char>());
        HRESULT hr = g_pd3dDevice->CreateVertexShader(
            fileContents.data(), fileContents.size(),
            nullptr, vs.m_pVertexShader.GetAddressOf()
        );
        test_hresult(hr, "could not create vertex shader");

        std::vector<D3D11_INPUT_ELEMENT_DESC> iaDesc
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
            0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },

            { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT,
            0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        hr = g_pd3dDevice->CreateInputLayout(
            iaDesc.data(), (UINT)iaDesc.size(),
            fileContents.data(), fileContents.size(),
            vs.m_pInputLayout.GetAddressOf()
        );
        test_hresult(hr, "could not create vertex shader");

        return vs;
    }

    Microsoft::WRL::ComPtr <ID3D11PixelShader> load_pixel_shader() const
    {
        Microsoft::WRL::ComPtr <ID3D11PixelShader> m_pPixelShader;
        std::ifstream pShader{ "PixelShader.cso",  std::ios::binary };
        std::vector<char> fileContents((std::istreambuf_iterator<char>(pShader)),
            std::istreambuf_iterator<char>());

        HRESULT hr = g_pd3dDevice->CreatePixelShader(
            fileContents.data(),
            fileContents.size(),
            nullptr,
            m_pPixelShader.GetAddressOf()
        );
        test_hresult(hr, "could not create pixel shader");
        return m_pPixelShader;
    }

    Microsoft::WRL::ComPtr <ID3D11Buffer> create_constant_buffer()
    {
        Microsoft::WRL::ComPtr <ID3D11Buffer> m_pConstantBuffer;
        CD3D11_BUFFER_DESC cbDesc{
        sizeof(ConstantBufferStruct),
        D3D11_BIND_CONSTANT_BUFFER
        };

        HRESULT hr = g_pd3dDevice->CreateBuffer(
            &cbDesc,
            nullptr,
            m_pConstantBuffer.GetAddressOf()
        );

        if (hr != S_OK || !m_pConstantBuffer)
        {
            throw std::runtime_error("could not create constant buffer");
        }
        return m_pConstantBuffer;
    }

    bool is_ocluded() const
    {
        return g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED;
    }

    Microsoft::WRL::ComPtr <ID3D11Buffer> create_vertex_buffer(const std::vector<VertexPositionColor>& CubeVertices) const
    {
        Microsoft::WRL::ComPtr <ID3D11Buffer> m_pVertexBuffer;
        CD3D11_BUFFER_DESC vDesc{
        (UINT)(CubeVertices.size() * sizeof(VertexPositionColor)),
        D3D11_BIND_VERTEX_BUFFER
        };

        D3D11_SUBRESOURCE_DATA vData{ CubeVertices.data(), 0, 0 };

        HRESULT hr = g_pd3dDevice->CreateBuffer(
            &vDesc,
            &vData,
            m_pVertexBuffer.GetAddressOf()
        );
        if (hr != S_OK)
        {
            throw std::runtime_error("could not creare vertex buffer");
        }
        return m_pVertexBuffer;
    }

    Microsoft::WRL::ComPtr <ID3D11Buffer> create_index_buffer(std::vector<unsigned short> CubeIndices) const
    {
        Microsoft::WRL::ComPtr < ID3D11Buffer> m_pIndexBuffer;
        CD3D11_BUFFER_DESC iDesc{
            (UINT)(CubeIndices.size() * sizeof(unsigned short)),
            D3D11_BIND_INDEX_BUFFER
        };

        D3D11_SUBRESOURCE_DATA iData{ CubeIndices.data(), 0, 0 };

        HRESULT hr = g_pd3dDevice->CreateBuffer(
            &iDesc,
            &iData,
            m_pIndexBuffer.GetAddressOf()
        );
        test_hresult(hr, "could not create index buffer");
        return m_pIndexBuffer;
    }

    frame_resources prepare_frame_resources(const vertex_representation& vertices)
    {
        return {
                .vertex_buffer = create_vertex_buffer(vertices.CubeVertices),
                .index_buffer = create_index_buffer(vertices.CubeIndices),
                .m_indexCount = (UINT)vertices.CubeIndices.size(),
        };
    }

    Microsoft::WRL::ComPtr <ID3D11DepthStencilView> create_depth_stencil_view(Microsoft::WRL::ComPtr<ID3D11Texture2D>& m_pDepthStencil) const
    {
        Microsoft::WRL::ComPtr <ID3D11DepthStencilView>  m_pDepthStencilView;
        CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);

        HRESULT hr = g_pd3dDevice->CreateDepthStencilView(
            m_pDepthStencil.Get(),
            &depthStencilViewDesc,
            m_pDepthStencilView.GetAddressOf()
        );
        test_hresult(hr, "could not create depth stencil view");

        return m_pDepthStencilView;
    }

    void set_buffers(Microsoft::WRL::ComPtr <ID3D11Buffer>& vertex_buffer, Microsoft::WRL::ComPtr < ID3D11Buffer>& index_buffer) const
    {
        // Set up the IA stage by setting the input topology and layout.
        UINT stride = sizeof(VertexPositionColor);
        UINT offset = 0;

        g_pd3dDeviceContext->IASetVertexBuffers(
            0,
            1,
            vertex_buffer.GetAddressOf(),
            &stride,
            &offset
        );

        g_pd3dDeviceContext->IASetIndexBuffer(
            index_buffer.Get(),
            DXGI_FORMAT_R16_UINT,
            0
        );

        g_pd3dDeviceContext->IASetPrimitiveTopology(
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
        );
    }

    void setup_shaders(const vertex_shader_holder& vertex_shader,
        Microsoft::WRL::ComPtr <ID3D11Buffer>& constant_buffer, Microsoft::WRL::ComPtr <ID3D11PixelShader>& pixel_shader) const
    {
        g_pd3dDeviceContext->IASetInputLayout(vertex_shader.m_pInputLayout.Get());

        // Set up the vertex shader stage.
        g_pd3dDeviceContext->VSSetShader(
            vertex_shader.m_pVertexShader.Get(),
            nullptr,
            0
        );

        g_pd3dDeviceContext->VSSetConstantBuffers(
            0,
            (UINT)1,
            constant_buffer.GetAddressOf()
        );

        // Set up the pixel shader stage.
        g_pd3dDeviceContext->PSSetShader(
            pixel_shader.Get(),
            nullptr,
            0
        );
    }

    void draw_scene(int m_indexCount) const
    {
        // Calling Draw tells Direct3D to start sending commands to the graphics device.
        g_pd3dDeviceContext->DrawIndexed(
            m_indexCount,
            0,
            0
        );

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
        test_hresult(hr, "could not draw");
    }
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


struct imgui_holder
{
    imgui_context_holder imgui_context;
    imgui_win32_holder imgui_win32;
    imgui_dx11_holder imgui_dx11;

    imgui_holder(HWND hwnd, Microsoft::WRL::ComPtr<ID3D11Device>& g_pd3dDevice, Microsoft::WRL::ComPtr <ID3D11DeviceContext>& g_pd3dDeviceContext) :
        imgui_context{},
        imgui_win32{ hwnd },
        imgui_dx11{ g_pd3dDevice.Get(), g_pd3dDeviceContext.Get() }
    {

    }
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

    application_basics()
    {
        window.show_window();
        IMGUI_CHECKVERSION();
        setup_imgui();
    }

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