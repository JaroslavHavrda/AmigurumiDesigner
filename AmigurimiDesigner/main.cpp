import gui_wrapper;
import direct_x_structures;
import application_basics;
import vertex_calculations;

#define NOMINMAX
#include <Windows.h>

#include "gsl/gsl"

import std;

// Data

static void draw_top_lid(vertex_representation& vr, const float diameter, const int slice_count, const float level)
{
    test_vertex_vector_size(vr.vertices);
    vr.vertices.emplace_back(DirectX::XMFLOAT3{ 0.f, level, 0.f }, DirectX::XMFLOAT3{ 1.f,   1.f,   1.f });
    const unsigned short start_index = gsl::narrow<unsigned short>( vr.vertices.size() );
    for (unsigned short i = 0; i < slice_count; ++i)
    {
        const float angle = static_cast<float>(i * std::numbers::pi * 2 / slice_count);
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

static void draw_side(vertex_representation & vr , const float diameter, const float previous_diameter, const int slice_count, const float level, const float height)
{
    test_vertex_vector_size(vr.vertices);
    const unsigned short start_index = gsl::narrow<unsigned short>( vr.vertices.size() );
    for (unsigned short i = 0; i < slice_count; ++i)
    {
        const float angle = static_cast<float>(i * std::numbers::pi * 2 / slice_count);
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

static float layer_height(const float previous_radius, const float radius) noexcept
{
    const float distance_difference = (previous_radius - radius);
    constexpr float length = 4.;
    const float height = std::sqrt(length * length - distance_difference * distance_difference);
    return height;
}

static vertex_representation calc_vertices(const std::vector<float> & radiuses)
{
    vertex_representation vert_res;
    float level = 0.;
    float previous_radius = 0.;
    bool first_slice = true;
    constexpr int slice_count = 33;
    for (const auto &radius: radiuses)
    {
        if (first_slice)
        {
            draw_bottom_lid(vert_res, radius, slice_count);
        }
        
        if (!first_slice)
        {
            const float height = layer_height(previous_radius, radius);
            draw_side(vert_res, radius, previous_radius, slice_count, level, height);
            level += height;
        }
                
        previous_radius = radius;
        first_slice = false;
    }
    draw_top_lid(vert_res, previous_radius, slice_count, level);
    return vert_res;
}

static bool process_messages() noexcept
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
    std::string error;
    void update_prescription(const std::string_view& prescription)
    {
        using std::operator""sv;
        using std::operator""s;
        std::vector<float> prescriptions_num;
        constexpr auto delim{ ","sv };
        for (const auto& item : std::views::split(prescription, ","sv))
        {
            int parsed_number = 1;
            std::string sv{ item.begin(), item.end() };
            try {
                parsed_number = std::stoi(sv);
            }
            catch (std::invalid_argument const& )
            {
                error = "syntax_error"s + std::string(item.begin(), item.end());
                return;
            }
            catch (std::out_of_range const& )
            {
                error = "¨too big number:"s + std::string(item.begin(), item.end());
                return;
            }
            prescriptions_num.push_back((float)parsed_number);

        }
        if (prescriptions_num.empty())
        {
            error = "the prescription is empty";
            return;
        }
        parsed_numbers = prescriptions_num;
        error = "";
    }
};

std::vector<float> calc_diameters(const std::vector<float>& prescription)
{
    const auto arc_lengths = prescription | std::views::transform([](float count) {return count * 4; });
    std::vector<float> res;
    std::ranges::transform(arc_lengths, std::back_inserter(res), [](float d) {return d * std::numbers::inv_pi_v<float> /2.f;});
    return res;
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
    prescription_parser prescription;
    while (true)
    {
        const std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        if (process_messages())
            break;
        if (app.d3dDevice.is_ocluded())
        {
            ::Sleep(10);
            continue;
        }
        app.update_window_size();
        const auto diameters = calc_diameters(prescription.parsed_numbers);
        const vertex_representation vertices = calc_vertices(diameters);
        const DirectX::XMFLOAT3 center = calc_center(vertices.vertices);
        const float height = calc_height_needed(vertices.vertices);
        gui.present_using_imgui(height, app.target_view->m_bbDesc, center, prescription.error );
        prescription.update_prescription({ gui.prescription.data(), std::strlen( gui.prescription.data()) });
        app.draw_vertices(vertices, application_resources);
        const std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        //std::cout << "Loop duration = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "[ms]" << std::endl;
    }
    return 0;
}