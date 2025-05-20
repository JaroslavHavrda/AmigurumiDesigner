import gui_wrapper;

#define NOMINMAX
#include <Windows.h>
#include <cmath>
#include <optional>
#include <vector>
#include <string_view>
#include <charconv>
#include <numbers>
#include <ranges>
#include <limits>
#include <iostream>
#include <directxmath.h>

// Data

void test_vertex_vector_size(std::vector<VertexPositionColor> & vertices)
{
    if (vertices.size() > std::numeric_limits<unsigned short>::max())
    {
        throw std::runtime_error("Too many vertices were generated.");
    }
}

static void draw_bottom_lid(vertex_representation & vr, float diameter, int slice_count)
{
    vr.vertices.emplace_back(DirectX::XMFLOAT3{ 0.f, 0.f, 0.f }, DirectX::XMFLOAT3{ 1.f,   1.f,   1.f });
    unsigned short start_index = (unsigned short)vr.vertices.size();
    for (unsigned short i = 0; i < slice_count; ++i)
    {
        float angle = (float)(i * std::numbers::pi * 2 / slice_count);
        vr.vertices.emplace_back(DirectX::XMFLOAT3{ std::sin(angle) * diameter, 0.f, std::cos(angle) * diameter }, DirectX::XMFLOAT3{ std::sin(angle),   std::cos(angle),   0 });
    }
    for (unsigned short i = 0; i < slice_count; ++i)
    {
        vr.indices.push_back((unsigned short)(start_index + i));
        vr.indices.push_back((unsigned short)(start_index + (i + 1) % slice_count));
        vr.indices.push_back((unsigned short)(start_index - 1));
    }
    test_vertex_vector_size(vr.vertices);
}

static void draw_top_lid(vertex_representation& vr, float diameter, int slice_count, float level)
{
    test_vertex_vector_size(vr.vertices);
    vr.vertices.emplace_back(DirectX::XMFLOAT3{ 0.f, level, 0.f }, DirectX::XMFLOAT3{ 1.f,   1.f,   1.f });
    unsigned short start_index = (unsigned short)vr.vertices.size();
    for (unsigned short i = 0; i < slice_count; ++i)
    {
        float angle = (float)(i * std::numbers::pi * 2 / slice_count);
        vr.vertices.emplace_back(DirectX::XMFLOAT3{ std::sin(angle) * diameter, level, std::cos(angle) * diameter }, DirectX::XMFLOAT3{ std::sin(angle),   std::cos(angle),   1 });
    }
    for (unsigned short i = 0; i < slice_count; ++i)
    {
        vr.indices.push_back((unsigned short)(start_index + i ));
        vr.indices.push_back((unsigned short)(start_index - 1));
        vr.indices.push_back((unsigned short)(start_index + (i + 1) % slice_count));
    }
    test_vertex_vector_size(vr.vertices);
}

static void draw_side(vertex_representation & vr , float diameter, float previous_diameter, int slice_count, float level, float height)
{
    test_vertex_vector_size(vr.vertices);
    unsigned short start_index = (unsigned short)vr.vertices.size();
    for (unsigned short i = 0; i < slice_count; ++i)
    {
        float angle = (float)(i * std::numbers::pi * 2 / slice_count);
        vr.vertices.emplace_back(DirectX::XMFLOAT3{ std::sin(angle) * previous_diameter, level + 0.f, std::cos(angle) * previous_diameter }, DirectX::XMFLOAT3{ std::sin(angle),   std::cos(angle),   0 });
        vr.vertices.emplace_back(DirectX::XMFLOAT3{ std::sin(angle) * diameter, level + height, std::cos(angle) * diameter }, DirectX::XMFLOAT3{ std::sin(angle),   std::cos(angle),   1 });
    }
    for (unsigned short i = 0; i < slice_count; ++i)
    {
        for (const auto vertex : { 0,1,2 })
        {
            vr.indices.push_back((unsigned short)(start_index + (2 * i + vertex) % (2 * slice_count)));
        }
        for (const auto vertex : { 1,3,2 })
        {
            vr.indices.push_back((unsigned short)(start_index + (2 * i + vertex) % (2 * slice_count)));
        }
    }
    test_vertex_vector_size(vr.vertices);
}

static vertex_representation calc_vertices(const std::vector<float> & diameters)
{
    std::vector<VertexPositionColor> vertices{};
    std::vector<unsigned short> indices{};
    vertex_representation vert_res{vertices, indices};
    float level = 0.;
    float previous_diameter = 0.;
    bool first_slice = true;
    constexpr int slice_count = 33;
    vert_res.vertices = vertices;
    for (const auto &diameter: diameters)
    {
        if (first_slice)
        {
            draw_bottom_lid(vert_res, diameter, slice_count);
        }
        
        if (!first_slice)
        {
            float distance_difference = previous_diameter - diameter;
            float length = 5.;
            float height = std::sqrt(length * length - distance_difference * distance_difference);

            draw_side(vert_res, diameter, previous_diameter, slice_count, level, height);
            level += height;
        }
                
        previous_diameter = diameter;
        first_slice = false;
    }
    draw_top_lid(vert_res, previous_diameter, slice_count, level);
    std::cout << std::endl;
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