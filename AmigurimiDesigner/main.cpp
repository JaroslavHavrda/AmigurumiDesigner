#include "gui_wrapper.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <directxmath.h>
#include <wrl\client.h>
#include <Windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <stdexcept>
#include <array>
#include <cmath>
#include <optional>
#include <vector>
#include <fstream>
#include <iostream>

// Data
UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;

vertex_representation calc_vertices()
{
    return {
        std::vector<VertexPositionColor>
        {
            {DirectX::XMFLOAT3{ -0.5f,-0.5f,-0.5f}, DirectX::XMFLOAT3{0,   0,   0},},
            {DirectX::XMFLOAT3{-0.5f,-0.5f, 0.5f}, DirectX::XMFLOAT3{0,   0,   1}, },
            {DirectX::XMFLOAT3{ -0.5f, 0.5f,-0.5f}, DirectX::XMFLOAT3{0,   1,   0},},
            {DirectX::XMFLOAT3{ -0.5f, 0.5f, 0.5f}, DirectX::XMFLOAT3{0,   1,   1},},

            {DirectX::XMFLOAT3{0.5f,-0.5f,-0.5f}, DirectX::XMFLOAT3{1,   0,   0}, },
            {DirectX::XMFLOAT3{0.5f,-0.5f, 0.5f}, DirectX::XMFLOAT3{1,   0,   1}, },
            {DirectX::XMFLOAT3{0.5f, 0.5f,-0.5f}, DirectX::XMFLOAT3{1,   1,   0}, },
            {DirectX::XMFLOAT3{0.5f, 0.5f, 0.5f}, DirectX::XMFLOAT3{0,   0,   0},},
        },
        std::vector<unsigned short>
        {
            0,2,1, // -x
            1,2,3,

            4,5,6, // +x
            5,7,6,

            0,1,5, // -y
            0,5,4,

            2,6,7, // +y
            2,7,3,

            0,4,6, // -z
            0,6,2,

            1,3,7, // +z
            1,7,5,
        }
    };
}

bool process_messages()
{
    bool done = false;
    // Poll and handle messages (inputs, window resize, etc.)
    // See the WndProc() function below for our to dispatch events to the Win32 backend.
    MSG msg;
    while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
        if (msg.message == WM_QUIT)
            done = true;
    }
    return done;
}

void draw_vertices(application_basics& app, const vertex_representation& vertices, global_resources& application_resources)
{
    frame_resources frame_res = app.d3dDevice.prepare_frame_resources(vertices);
    app.update_constant_struct(application_resources.constant_buffer);
    app.clear_render_target_view();
    Microsoft::WRL::ComPtr <ID3D11Texture2D> m_pDepthStencil = app.create_depth_stencil();
    Microsoft::WRL::ComPtr <ID3D11DepthStencilView>  m_pDepthStencilView = app.d3dDevice.create_depth_stencil_view(m_pDepthStencil);
    app.set_depth_stencil_to_scene(m_pDepthStencilView);
    app.d3dDevice.set_buffers(frame_res.vertex_buffer, frame_res.index_buffer);
    app.d3dDevice.setup_shaders(application_resources.vertex_shader, application_resources.constant_buffer, application_resources.pixel_shader);
    app.d3dDevice.draw_scene(frame_res.m_indexCount);
}

int main(int, char**)
{
    application_basics app;
    global_resources application_resources{
        .vertex_shader = app.d3dDevice.load_vertex_shader(),
        .pixel_shader = app.d3dDevice.load_pixel_shader(),
        .constant_buffer = app.d3dDevice.create_constant_buffer()
    };
    gui_wrapper gui;
    while (true)
    {
        if (process_messages())
            break;
        if (app.d3dDevice.is_ocluded())
        {
            ::Sleep(10);
            continue;
        }
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            app.update_after_resize();
            g_ResizeWidth = g_ResizeHeight = 0;
        }                             
        gui.present_using_imgui();
        vertex_representation vertices = calc_vertices();
        draw_vertices(app, vertices, application_resources);
    }
        
    return 0;
}