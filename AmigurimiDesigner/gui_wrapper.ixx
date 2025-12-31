module;
#define NOMINMAX

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "imgui_hack.h"

#include <windowsx.h>
#include <Windows.h>
#include <wrl\client.h>
#include <d3d11.h>
#include <directxmath.h>

#include <optional>
#include <array>
#include <iostream>

export module gui_wrapper;
import direct_x_structures;
import viewport_configuration;

export struct gui_wrapper
{
    std::array<char, 500> prescription{ '1', ',', '4', ',', '1', ',', '4'};

    void present_using_imgui( const float height, const D3D11_TEXTURE2D_DESC& m_bbDesc, const DirectX::XMFLOAT3 center, const std::string_view error)
    {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Amigurumi designer");
        ImGui::InputTextMultiline("Prescription", prescription.data(), prescription.size());
        ImGui::LabelText("", error.data());
        ImGui::End();
        ImGui::Begin("Rotation");
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
            viewport_config.optimal_size(height, m_bbDesc);
        }
        if (ImGui::Button("center the object"))
        {
            viewport_config.set_at(center);
        }
        ImGui::End();
        ImGui::Render();
    }
};

UINT g_ResizeWidth = 0, g_ResizeHeight = 0;

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

struct rotation_data_gui
{
    float roll{ 0 };
    float pitch{ 0 };
    float yaw{ 0 };
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
    imgui_win32_holder(const HWND hwnd)
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

    imgui_holder(const HWND hwnd, const Microsoft::WRL::ComPtr<ID3D11Device>& d3d_device, const Microsoft::WRL::ComPtr <ID3D11DeviceContext>& g_pd3dDeviceContext) :
        imgui_context{},
        imgui_win32{ hwnd },
        imgui_dx11{ d3d_device.Get(), g_pd3dDeviceContext.Get() }
    {

    }
};

render_target_view_holder init_render_target_view(D3DDeviceHolder& d3ddevice);

constant_buffer_struct calculate_projections(const D3D11_TEXTURE2D_DESC& m_bbDesc)
{
    using namespace DirectX;
    constant_buffer_struct m_constantBufferData{};

    auto eye = viewport_config.calc_eye();
    auto at = viewport_config.calc_at();
    auto center_direction = eye - at;

    DirectX::XMStoreFloat4x4(
        &m_constantBufferData.view,
        DirectX::XMMatrixTranspose(
            DirectX::XMMatrixLookAtRH(
                viewport_config.calc_eye(),
                viewport_config.calc_at(),
                viewport_config.calc_up()
            )
        )
    );

    DirectX::XMStoreFloat4x4(
        &m_constantBufferData.projection,
        DirectX::XMMatrixTranspose(
            DirectX::XMMatrixOrthographicRH(
                (float)m_bbDesc.Width / 1000 * viewport_config.zoom_factor, (float)m_bbDesc.Height / 1000 * viewport_config.zoom_factor, 0, 1000
            )
        )
    );
    DirectX::XMStoreFloat4x4(
        &m_constantBufferData.world,
        DirectX::XMMatrixTranspose(
            DirectX::XMMatrixRotationRollPitchYaw(
                DirectX::XMConvertToRadians(0),
                DirectX::XMConvertToRadians(0),
                DirectX::XMConvertToRadians(0)
            )
        )
    );
    return m_constantBufferData;
}

void setup_imgui()
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

static_assert((sizeof(constant_buffer_struct) % 16) == 0, "Constant Buffer size must be 16-byte aligned");

render_target_view_holder init_render_target_view(D3DDeviceHolder& d3ddevice)
{
    render_target_view_holder target;
    auto res = d3ddevice.g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)(&(target.pBackBuffer)));
    test_hresult(res, "no back buffer");
    target.pBackBuffer->GetDesc(&target.m_bbDesc);

    target.m_viewport = D3D11_VIEWPORT{
        .Width = (float)target.m_bbDesc.Width,
        .Height = (float)target.m_bbDesc.Height,
        .MinDepth = 0,
        .MaxDepth = 1 };

    d3ddevice.g_pd3dDeviceContext->RSSetViewports(
        1,
        &(target.m_viewport)
    );

    res = d3ddevice.g_pd3dDevice->CreateRenderTargetView(target.pBackBuffer.Get(), nullptr, target.g_mainRenderTargetView.GetAddressOf());
    test_hresult(res, "could not create render target view");
    return target;
}

// Forward declare message handler from imgui_impl_win32.cpp

LRESULT  handle_mouse_wheel(WPARAM wParam)
{
    using namespace DirectX;
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
    {
        return 0;
    }
    auto delta = (int16_t)HIWORD(wParam);
    auto scaling = exp(delta / 1200.);
    viewport_config.zoom_factor *= (float)scaling;
    return 0;
}

static LRESULT  handle_key_down(WPARAM wParam)
{
    std::cout << "wparam " << wParam << std::endl;
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard)
    {
        return 0;
    }
    switch (wParam)
    {
    case VK_LEFT:
        viewport_config.eye = viewport_config.rotaded_eye(-30, 0);
        viewport_config.up = viewport_config.updated_up();
        break;
    case VK_RIGHT:
        viewport_config.eye = viewport_config.rotaded_eye(30, 0);
        viewport_config.up = viewport_config.updated_up();
        break;
    }
    if (wParam == VK_UP)
    {
        viewport_config.eye = viewport_config.rotaded_eye(0, -30);
        viewport_config.up = viewport_config.updated_up();
    }
    if (wParam == VK_DOWN)
    {
        viewport_config.eye = viewport_config.rotaded_eye(0, 30);
        viewport_config.up = viewport_config.updated_up();
    }
    if (wParam == 'A')
    {
        viewport_config.at = viewport_config.shifted_at(10,0);
        viewport_config.eye = viewport_config.shifted_eye(10, 0);
    }
    if (wParam == 'W')
    {
        viewport_config.at = viewport_config.shifted_at(0, 10);
        viewport_config.eye = viewport_config.shifted_eye(0, 10);
    }
    if (wParam == 'S')
    {
        viewport_config.at = viewport_config.shifted_at(0, -10);
        viewport_config.eye = viewport_config.shifted_eye(0, -10);
    }
    if (wParam == 'D')
    {
        viewport_config.at = viewport_config.shifted_at(-10, 0);
        viewport_config.eye = viewport_config.shifted_eye(-10, 0);
    }
    if (wParam == 'Q')
    {
        viewport_config.zoom_factor /= 1.5f;
    }
    if (wParam == 'E')
    {
        viewport_config.zoom_factor *= 1.5f;
    }
    return 0;
}

static LRESULT handle_button_up()
{
    viewport_config.stop_dragging();
    return 0;
}

static LRESULT handle_mouse_move(LPARAM lParam)
{
    viewport_config.current_pos.x = GET_X_LPARAM(lParam);
    viewport_config.current_pos.y = GET_Y_LPARAM(lParam);
    return 0;
}

static LRESULT handle_rbutton_down(LPARAM lParam)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
    {
        return 0;
    }
    viewport_config.current_pos.x = GET_X_LPARAM(lParam);
    viewport_config.current_pos.y = GET_Y_LPARAM(lParam);
    viewport_config.drag_start.x = GET_X_LPARAM(lParam);
    viewport_config.drag_start.y = GET_Y_LPARAM(lParam);
    viewport_config.right_dragging = true;
    return 0;
}

LRESULT handle_lbutton_down(LPARAM lParam)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
    {
        return 0;
    }
    viewport_config.current_pos.x = GET_X_LPARAM(lParam);
    viewport_config.current_pos.y = GET_Y_LPARAM(lParam);
    viewport_config.drag_start.x = GET_X_LPARAM(lParam);
    viewport_config.drag_start.y = GET_Y_LPARAM(lParam);
    viewport_config.dragging = true;
    return 0;
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (auto res = ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return res;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    case WM_LBUTTONDOWN:
        return handle_lbutton_down(lParam);
    case WM_RBUTTONDOWN:
        return handle_rbutton_down(lParam);
    case WM_MOUSEMOVE:
        return handle_mouse_move(lParam);
    case WM_RBUTTONUP:
    case WM_LBUTTONUP:
        return handle_button_up();
    case WM_MOUSEWHEEL:
        return handle_mouse_wheel(wParam);
    case WM_KEYDOWN:
        return handle_key_down(wParam);
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
