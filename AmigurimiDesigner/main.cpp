#include "gui_wrapper.h"

#include <cmath>
#include <optional>
#include <vector>
#include <string_view>
#include <charconv>
#include <numbers>

// Data
UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;

static vertex_representation calc_vertices(const std::string_view & prescription)
{
    int diameter = 1;
    /*auto [ptr, err] =*/ std::from_chars(prescription.data(), prescription.data() + prescription.size(), diameter);
    std::vector<VertexPositionColor> vertices{};
    std::vector<unsigned short> indices{};
    constexpr int slice_count = 33;
    vertices.emplace_back(DirectX::XMFLOAT3{ 0.f, 1.f, 0.f }, DirectX::XMFLOAT3{ 1.f,   1.f,   1.f });
    vertices.emplace_back(DirectX::XMFLOAT3{ 0.f, 0.f, 0.f }, DirectX::XMFLOAT3{ 1.f,   1.f,   1.f });
    unsigned short start_index = vertices.size();
    for (unsigned short i = 0; i < slice_count; ++i)
    {
        float angle = (float)( i * std::numbers::pi * 2 / slice_count);
        vertices.emplace_back( DirectX::XMFLOAT3{ std::sin(angle) * diameter, 0.f, std::cos(angle) * diameter }, DirectX::XMFLOAT3{ std::sin(angle),   std::cos(angle),   0 });
        vertices.emplace_back(DirectX::XMFLOAT3{ std::sin(angle) * diameter, 1.f, std::cos(angle) * diameter }, DirectX::XMFLOAT3{ std::sin(angle),   std::cos(angle),   1 });
        for (const auto vertex : { 0,1,2 })
        {
            indices.push_back((unsigned short)(start_index + (2 * i + vertex)%(2*slice_count)  ));
        }
        for (const auto vertex : { 1,3,2 })
        {
            indices.push_back((unsigned short)(start_index + (2 * i + vertex) % (2*slice_count)));
        }
        indices.push_back((unsigned short)(start_index + 2 * i));
        indices.push_back((unsigned short)(start_index + (2 * i + 2) % (2 * slice_count)));
        indices.push_back((unsigned short)(start_index - 1));
        indices.push_back((unsigned short)(start_index + (2 * i + 1)%(2*slice_count)));
        indices.push_back((unsigned short)(start_index - 2));
        indices.push_back((unsigned short)(start_index + (2 * i + 3) % (2 * slice_count)));
    }
    return { vertices, indices};
}

static bool process_messages()
{
    bool done = false;
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
        vertex_representation vertices = calc_vertices({ gui.prescription.data(), gui.prescription.size()});
        app.draw_vertices(vertices, application_resources);
    }
        
    return 0;
}