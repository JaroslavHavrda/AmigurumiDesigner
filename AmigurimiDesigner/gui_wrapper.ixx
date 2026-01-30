module;
#define NOMINMAX


#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include <d3d11.h>
#include <directxmath.h>

export module gui_wrapper;
import viewport_configuration;
import std;

export struct gui_wrapper
{
    std::array<char, 500> prescription{ '1', ',', '4', ',', '1', ',', '4'};

    void present_using_imgui( const float height, const D3D11_TEXTURE2D_DESC& desc, const DirectX::XMFLOAT3 center, const std::string_view error)
    {
        using namespace DirectX;
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Amigurumi designer");
        ImGui::InputTextMultiline("Prescription", prescription.data(), prescription.size());
        ImGui::LabelText("", error.data());
        ImGui::End();
        ImGui::Begin("Rotation");
        view_buttons(height, desc, center);
        orbit_buttons();
        ImGui::End();
        ImGui::Render();
    }
private:
    void view_buttons(const float height, const D3D11_TEXTURE2D_DESC& desc, const DirectX::XMFLOAT3 center)
    {
        if (ImGui::Button("default view")) {
            viewport_config.reset_defaults(center);
        }
        if (ImGui::Button("front view")) {
            viewport_config.front_view(center);
        }
        if (ImGui::Button("top view")) {
            viewport_config.top_view(center);
        }
        if (ImGui::Button("optimal zoom"))
        {
            viewport_config.optimal_size(height, desc);
        }
        if (ImGui::Button("center the object"))
        {
            viewport_config.set_at(center);
        }
    }
    void orbit_buttons()
    {
        ImGui::Button("orbit left");
        if (ImGui::IsItemActive())
        {
            viewport_config.rotate_center(3, 0);
        }
        ImGui::Button("orbit right");
        if (ImGui::IsItemActive())
        {
            viewport_config.rotate_center(-3, 0);
        }
        ImGui::Button("orbit up");
        if (ImGui::IsItemActive())
        {
            viewport_config.rotate_center(0, 3);
        }
        ImGui::Button("orbit down");
        if (ImGui::IsItemActive())
        {
            viewport_config.rotate_center(0, -3);
        }
    }
};

