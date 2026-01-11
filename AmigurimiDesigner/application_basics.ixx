module;
#define NOMINMAX

#include <d3d11.h>

#include "imgui.h"
#include <Windows.h>
#include <wrl\client.h>

export module application_basics;
import gui_wrapper;
import direct_x_structures;
import viewport_configuration;
import projections;
import std;

static void setup_imgui()
{
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsLight();
}

export struct application_basics
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
    void update_window_size()
    {
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            target_view = std::optional<render_target_view_holder>{};
            d3dDevice.g_pd3dDeviceContext->ClearState();
            d3dDevice.g_pd3dDeviceContext->Flush();
            HRESULT hr = d3dDevice.g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            test_hresult(hr, "could not resize buffers");
            target_view = init_render_target_view(d3dDevice);
            g_ResizeWidth = g_ResizeHeight = 0;
        }
    }
    void clear_render_target_view()
    {
        const float teal[] = { 0.098f, 0.439f, 0.439f, 1.000f };
        d3dDevice.g_pd3dDeviceContext->ClearRenderTargetView(
            target_view->g_mainRenderTargetView.Get(),
            teal
        );
    }
    void update_constant_struct(const Microsoft::WRL::ComPtr <ID3D11Buffer>& constant_buffer)
    {
        const auto constant_struct = calculate_projections(target_view->m_bbDesc);

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

        const HRESULT res = d3dDevice.g_pd3dDevice->CreateTexture2D(
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
    void draw_vertices(const vertex_representation& vertices, global_resources& application_resources)
    {
        frame_resources frame_res = d3dDevice.prepare_frame_resources(vertices);
        update_constant_struct(application_resources.constant_buffer);
        clear_render_target_view();
        Microsoft::WRL::ComPtr <ID3D11Texture2D> m_pDepthStencil = create_depth_stencil();
        Microsoft::WRL::ComPtr <ID3D11DepthStencilView>  m_pDepthStencilView = d3dDevice.create_depth_stencil_view(m_pDepthStencil);
        set_depth_stencil_to_scene(m_pDepthStencilView);
        d3dDevice.set_buffers(frame_res.vertex_buffer, frame_res.index_buffer);
        d3dDevice.setup_shaders(application_resources.vertex_shader, application_resources.constant_buffer, application_resources.pixel_shader);
        d3dDevice.draw_scene(frame_res.m_indexCount);
    }
};