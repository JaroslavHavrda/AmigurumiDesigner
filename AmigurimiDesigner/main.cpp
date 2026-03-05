import gui_wrapper;
import direct_x_structures;
import application_basics;
import vertex_calculations;
import prescription_parser;

#define NOMINMAX
#include <Windows.h>

import std;

// Data

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
    }
    return 0;
}