#include "gui_wrapper.h"

void setup_imgui()
{
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsLight();
}

ConstantBufferStruct calculate_projections(const D3D11_TEXTURE2D_DESC& m_bbDesc)
{
    ConstantBufferStruct m_constantBufferData{};

    DirectX::XMStoreFloat4x4(
        &m_constantBufferData.view,
        DirectX::XMMatrixTranspose(
            DirectX::XMMatrixLookAtRH(
                viewport_config.calc_eye(),
                viewport_config.at,
                viewport_config.calc_up()
            )
        )
    );

    float aspectRatioX = static_cast<float>(m_bbDesc.Width) / static_cast<float>(m_bbDesc.Height);
    float aspectRatioY = aspectRatioX < (16.0f / 9.0f) ? aspectRatioX / (16.0f / 9.0f) : 1.0f;

    DirectX::XMStoreFloat4x4(
        &m_constantBufferData.projection,
        DirectX::XMMatrixTranspose(
            DirectX::XMMatrixPerspectiveFovRH(
                2.0f * std::atan(std::tan(DirectX::XMConvertToRadians(70) * 0.5f) / aspectRatioY),
                aspectRatioX,
                0.01f,
                100.0f
            )
        )
    );
    rotation_data_gui rot;
    DirectX::XMStoreFloat4x4(
        &m_constantBufferData.world,
        DirectX::XMMatrixTranspose(
            DirectX::XMMatrixRotationRollPitchYaw(
                DirectX::XMConvertToRadians(
                    rot.pitch
                ),
                DirectX::XMConvertToRadians(
                    rot.yaw
                ),
                DirectX::XMConvertToRadians(
                    rot.roll
                )
            )
        )
    );
    return m_constantBufferData;
}

D3DDeviceHolder init_d3d_device(HWND hWnd)
{
    D3DDeviceHolder device_holder;
    DXGI_SWAP_CHAIN_DESC sd{
        .BufferDesc = DXGI_MODE_DESC{.Width = 0, .Height = 0,
            .RefreshRate = DXGI_RATIONAL{.Numerator = 60, .Denominator = 1 },
            .Format = DXGI_FORMAT_B8G8R8A8_UNORM },
        .SampleDesc = DXGI_SAMPLE_DESC{.Count = 1, .Quality = 0 },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = 2,
        .OutputWindow = hWnd,
        .Windowed = TRUE,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
    };
    UINT createDeviceFlags = 0;
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray,
        2, D3D11_SDK_VERSION, &sd, device_holder.g_pSwapChain.GetAddressOf(), device_holder.g_pd3dDevice.GetAddressOf(),
        &featureLevel, device_holder.g_pd3dDeviceContext.GetAddressOf());
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2,
            D3D11_SDK_VERSION, &sd, device_holder.g_pSwapChain.GetAddressOf(), device_holder.g_pd3dDevice.GetAddressOf(),
            &featureLevel, device_holder.g_pd3dDeviceContext.GetAddressOf());
    test_hresult(res, "could not create D3D device");
    return device_holder;
}

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

void test_hresult(HRESULT hr, const char* message)
{
    if (hr != S_OK)
    {
        throw std::runtime_error(message);
    }
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
    case WM_MOUSEMOVE:
        viewport_config.current_pos.x = GET_X_LPARAM(lParam);
        viewport_config.current_pos.y = GET_Y_LPARAM(lParam);
        return 0;
    case WM_LBUTTONUP:
        viewport_config.stop_dragging();
        return 0;
    case WM_MOUSEWHEEL:
    {
        using namespace DirectX;
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse)
        {
            return 0;
        }
        auto delta = (int16_t)HIWORD(wParam);
        auto scaling = exp(delta / 1200.);
        std::cout << "mousewheel " << delta << " " << scaling << "\n";
        viewport_config.eye *= (float)scaling;
        return 0;
    }
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

DirectX::XMVECTOR ViewportConfigurationManager::calc_eye() const
{
    using namespace DirectX;
    if (!dragging)
    {
        return eye;
    }
    auto center_direction = eye - rotation_center;
    auto distance = XMVector3Length(center_direction);
    auto normalized_direction = XMVector3Normalize(center_direction);
    float y_diff = (float)current_pos.y - drag_start.y;
    float x_diff = (float)current_pos.x - drag_start.x;
    auto left = XMVector3Normalize(XMVector3Cross(up, normalized_direction));
    auto real_up = XMVector3Cross(normalized_direction, left);
    auto final_direction = normalized_direction + y_diff / 500 * real_up - x_diff / 500 * left;
    auto normalized_resulting_direction = XMVector3Normalize(final_direction);

    return rotation_center + distance * normalized_resulting_direction;
}

DirectX::XMVECTOR ViewportConfigurationManager::calc_up() const
{
    using namespace DirectX;
    if (!dragging)
    {
        return up;
    }
    auto original_direction = eye - rotation_center;
    auto left = -XMVector3Cross(original_direction, up);
    auto new_eye = calc_eye();
    auto new_diraction = new_eye - rotation_center;
    auto new_up = XMVector3Normalize(XMVector3Cross(new_diraction, left));
    return new_up;
}

void ViewportConfigurationManager::stop_dragging()
{
    auto new_eye = calc_eye();
    auto new_up = calc_up();
    eye = new_eye;
    up = new_up;
    dragging = false;
}

void ViewportConfigurationManager::reset_defaults()
{
    eye = DirectX::XMVectorSet(0.0f, 0.7f, 1.5f, 0.f);
    at = DirectX::XMVectorSet(0.0f, -0.1f, 0.0f, 0.f);
    up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.f);
    rotation_center = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.f);
}

void ViewportConfigurationManager::front_view()
{
    using namespace DirectX;
    auto center_direction = eye - at;
    auto distance = XMVector3Length(center_direction);
    at = XMVectorSet(0.0f, -0.1f, 0.0f, 0.f);
    up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.f);
    rotation_center = XMVectorSet(0.0f, 0.0f, 0.0f, 0.f);
    eye = at + distance * XMVectorSet(0.0f, 0.f, 1.f, 0.f);
}

void ViewportConfigurationManager::top_view()
{
    using namespace DirectX;
    auto center_direction = eye - at;
    auto distance = XMVector3Length(center_direction);
    at = XMVectorSet(0.0f, -0.1f, 0.0f, 0.f);
    up = XMVectorSet(0.0f, 0.0f, -1.0f, 0.f);
    rotation_center = XMVectorSet(0.0f, 0.0f, 0.0f, 0.f);
    eye = at + distance * XMVectorSet(0.0f, 1.f, 0.f, 0.f);
}

ViewportConfigurationManager viewport_config;