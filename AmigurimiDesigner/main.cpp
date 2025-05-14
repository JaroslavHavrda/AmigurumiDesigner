#include "gui_wrapper.h"

#include <cmath>
#include <optional>
#include <vector>
#include <string_view>
#include <charconv>
#include <numbers>
#include <ranges>
#include <limits>
#include <iostream>

// Data
UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;

static void draw_bottom_lid(std::vector<VertexPositionColor> & vertices, std::vector<unsigned short> & indices, float diameter, int slice_count)
{
    vertices.emplace_back(DirectX::XMFLOAT3{ 0.f, 0.f, 0.f }, DirectX::XMFLOAT3{ 1.f,   1.f,   1.f });
    unsigned short start_index = (unsigned short)vertices.size();
    for (unsigned short i = 0; i < slice_count; ++i)
    {
        float angle = (float)(i * std::numbers::pi * 2 / slice_count);
        vertices.emplace_back(DirectX::XMFLOAT3{ std::sin(angle) * diameter, 0.f, std::cos(angle) * diameter }, DirectX::XMFLOAT3{ std::sin(angle),   std::cos(angle),   0 });
    }
    for (unsigned short i = 0; i < slice_count; ++i)
    {
        indices.push_back((unsigned short)(start_index + i));
        indices.push_back((unsigned short)(start_index + (i + 1) % slice_count));
        indices.push_back((unsigned short)(start_index - 1));
    }
}

static void draw_top_lid(vertex_representation& vr, float diameter, int slice_count, float level)
{
    vr.CubeVertices.emplace_back(DirectX::XMFLOAT3{ 0.f, level + 1.f, 0.f }, DirectX::XMFLOAT3{ 1.f,   1.f,   1.f });
    unsigned short start_index = (unsigned short)vr.CubeVertices.size();
    for (unsigned short i = 0; i < slice_count; ++i)
    {
        float angle = (float)(i * std::numbers::pi * 2 / slice_count);
        vr.CubeVertices.emplace_back(DirectX::XMFLOAT3{ std::sin(angle) * diameter, level + 1.f, std::cos(angle) * diameter }, DirectX::XMFLOAT3{ std::sin(angle),   std::cos(angle),   1 });
    }
    for (unsigned short i = 0; i < slice_count; ++i)
    {
        vr.CubeIndices.push_back((unsigned short)(start_index + i ));
        vr.CubeIndices.push_back((unsigned short)(start_index - 1));
        vr.CubeIndices.push_back((unsigned short)(start_index + (i + 1) % slice_count));
    }
}

static void draw_side(std::vector<VertexPositionColor>& vertices, std::vector<unsigned short>& indices, float diameter, float previous_diameter, int slice_count, float level)
{
    unsigned short start_index = (unsigned short)vertices.size();
    for (unsigned short i = 0; i < slice_count; ++i)
    {
        float angle = (float)(i * std::numbers::pi * 2 / slice_count);
        vertices.emplace_back(DirectX::XMFLOAT3{ std::sin(angle) * previous_diameter, level + 0.f, std::cos(angle) * previous_diameter }, DirectX::XMFLOAT3{ std::sin(angle),   std::cos(angle),   0 });
        vertices.emplace_back(DirectX::XMFLOAT3{ std::sin(angle) * diameter, level + 1.f, std::cos(angle) * diameter }, DirectX::XMFLOAT3{ std::sin(angle),   std::cos(angle),   1 });
    }
    for (unsigned short i = 0; i < slice_count; ++i)
    {
        for (const auto vertex : { 0,1,2 })
        {
            indices.push_back((unsigned short)(start_index + (2 * i + vertex) % (2 * slice_count)));
        }
        for (const auto vertex : { 1,3,2 })
        {
            indices.push_back((unsigned short)(start_index + (2 * i + vertex) % (2 * slice_count)));
        }

    }
}

static vertex_representation calc_vertices(const std::vector<float> & diameters)
{
    std::vector<VertexPositionColor> vertices{};
    std::vector<unsigned short> indices{};
    vertex_representation vert_res;
    float level = 0.;
    float previous_diameter = 0.;
    bool first_slice = true;
    constexpr int slice_count = 33;
    vert_res.CubeIndices = indices;
    vert_res.CubeVertices = vertices;
    for (const auto &diameter: diameters)
    {
        if (first_slice)
        {
            draw_bottom_lid(vert_res.CubeVertices, vert_res.CubeIndices, diameter, slice_count);
        }
        
        if (vertices.size() > std::numeric_limits<unsigned short>::max())
        {
            throw std::runtime_error("Too many vertices were generated.");
        }

        if (!first_slice)
        {
            draw_side(vert_res.CubeVertices, vert_res.CubeIndices, diameter, previous_diameter, slice_count, level);
            level += 1;
        }
        
        previous_diameter = diameter;
        first_slice = false;
    }
    draw_top_lid(vert_res, previous_diameter, slice_count, level - 1);
    return vert_res;
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

struct prescription_parser
{
    std::vector<float> parsed_numbers{1};
    void update_prescription(const std::string_view& prescription)
    {
        using std::operator""sv;
        std::vector<float> prescriptions_num;
        constexpr auto delim{ ","sv };
        for (const auto& item : std::views::split(prescription, ","sv))
        {
            int parsed_number = 1;
            auto [ptr, err] = std::from_chars(item.data(), item.data() + item.size(), parsed_number);
            if (err != std::errc())
            {
                return;
            }
            prescriptions_num.push_back((float)parsed_number);

        }
        if (prescriptions_num.empty())
        {
            return;
        }
        parsed_numbers = prescriptions_num;
    }
};

int main(int, char**)
{
    application_basics app;
    global_resources application_resources{
        .vertex_shader = app.d3dDevice.load_vertex_shader(),
        .pixel_shader = app.d3dDevice.load_pixel_shader(),
        .constant_buffer = app.d3dDevice.create_constant_buffer()
    };
    gui_wrapper gui;
    prescription_parser prescription;
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
        prescription.update_prescription({ gui.prescription.data(), std::strlen( gui.prescription.data()) });
        vertex_representation vertices = calc_vertices( prescription.parsed_numbers);
        app.draw_vertices(vertices, application_resources);
    }
    return 0;
}