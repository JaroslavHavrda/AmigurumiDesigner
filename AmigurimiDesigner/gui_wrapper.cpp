#include "gui_wrapper.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include <directxmath.h>
#include <windowsx.h>

#include <fstream>
#include <vector>

void setup_imgui()
{
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsLight();
}

struct MousePosition
{
    int x = 0;
    int y = 0;
};

struct ViewportConfigurationManager
{
    DirectX::XMVECTOR eye = DirectX::XMVectorSet(0.0f, 7.f, 15.f, 0.f);
    DirectX::XMVECTOR at = DirectX::XMVectorSet(0.0f, -0.1f, 0.0f, 0.f);
    DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.f);
    DirectX::XMVECTOR rotation_center = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.f);

    MousePosition current_pos;
    MousePosition drag_start;
    bool dragging = false;

    DirectX::XMVECTOR calc_eye() const;
    DirectX::XMVECTOR calc_up() const;
    void stop_dragging();
    void reset_defaults();
    void front_view();
    void top_view();
};


ViewportConfigurationManager viewport_config;

struct ConstantBufferStruct {
    DirectX::XMFLOAT4X4 world;
    DirectX::XMFLOAT4X4 view;
    DirectX::XMFLOAT4X4 projection;
};

static_assert((sizeof(ConstantBufferStruct) % 16) == 0, "Constant Buffer size must be 16-byte aligned");

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

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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

void gui_wrapper::present_using_imgui()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("Amigurumi designer");
    ImGui::InputTextMultiline("Prescription", prescription.data(), prescription.size());
    ImGui::End();
    ImGui::Begin("Rotation");
    if (ImGui::Button("default view")) {
        viewport_config.reset_defaults();
    }
    if (ImGui::Button("front view")) {
        viewport_config.front_view();
    }
    if (ImGui::Button("top view")) {
        viewport_config.top_view();
    }
    ImGui::End();
    ImGui::Render();
}

WindowClassWrapper::WindowClassWrapper()
{
    auto res = ::RegisterClassExW(&wc);
    if (res == 0)
    {
        throw std::runtime_error("could not register window class");
    }
}

WindowClassWrapper::~WindowClassWrapper()
{
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
}

vertex_shader_holder D3DDeviceHolder::load_vertex_shader() const
{
    vertex_shader_holder vs;
    std::ifstream vShader{ "VertexShader.cso", std::ios::binary };
    std::vector<char> fileContents((std::istreambuf_iterator<char>(vShader)), std::istreambuf_iterator<char>());
    HRESULT hr = g_pd3dDevice->CreateVertexShader(
        fileContents.data(), fileContents.size(),
        nullptr, vs.m_pVertexShader.GetAddressOf()
    );
    test_hresult(hr, "could not create vertex shader");

    std::vector<D3D11_INPUT_ELEMENT_DESC> iaDesc
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
        0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },

        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT,
        0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = g_pd3dDevice->CreateInputLayout(
        iaDesc.data(), (UINT)iaDesc.size(),
        fileContents.data(), fileContents.size(),
        vs.m_pInputLayout.GetAddressOf()
    );
    test_hresult(hr, "could not create vertex shader");

    return vs;
}

Microsoft::WRL::ComPtr <ID3D11PixelShader> D3DDeviceHolder::load_pixel_shader() const
{
    Microsoft::WRL::ComPtr <ID3D11PixelShader> m_pPixelShader;
    std::ifstream pShader{ "PixelShader.cso",  std::ios::binary };
    std::vector<char> fileContents((std::istreambuf_iterator<char>(pShader)),
        std::istreambuf_iterator<char>());

    HRESULT hr = g_pd3dDevice->CreatePixelShader(
        fileContents.data(),
        fileContents.size(),
        nullptr,
        m_pPixelShader.GetAddressOf()
    );
    test_hresult(hr, "could not create pixel shader");
    return m_pPixelShader;
}

Microsoft::WRL::ComPtr <ID3D11Buffer> D3DDeviceHolder::create_constant_buffer()
{
    Microsoft::WRL::ComPtr <ID3D11Buffer> m_pConstantBuffer;
    CD3D11_BUFFER_DESC cbDesc{
    sizeof(ConstantBufferStruct),
    D3D11_BIND_CONSTANT_BUFFER
    };

    HRESULT hr = g_pd3dDevice->CreateBuffer(
        &cbDesc,
        nullptr,
        m_pConstantBuffer.GetAddressOf()
    );

    if (hr != S_OK || !m_pConstantBuffer)
    {
        throw std::runtime_error("could not create constant buffer");
    }
    return m_pConstantBuffer;
}

bool D3DDeviceHolder::is_ocluded() const
{
    return g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED;
}

Microsoft::WRL::ComPtr <ID3D11Buffer> D3DDeviceHolder::create_vertex_buffer(const std::vector<VertexPositionColor>& CubeVertices) const
{
    Microsoft::WRL::ComPtr <ID3D11Buffer> m_pVertexBuffer;
    CD3D11_BUFFER_DESC vDesc{
    (UINT)(CubeVertices.size() * sizeof(VertexPositionColor)),
    D3D11_BIND_VERTEX_BUFFER
    };

    D3D11_SUBRESOURCE_DATA vData{ CubeVertices.data(), 0, 0 };

    HRESULT hr = g_pd3dDevice->CreateBuffer(
        &vDesc,
        &vData,
        m_pVertexBuffer.GetAddressOf()
    );
    if (hr != S_OK)
    {
        throw std::runtime_error("could not creare vertex buffer");
    }
    return m_pVertexBuffer;
}

Microsoft::WRL::ComPtr <ID3D11Buffer> D3DDeviceHolder::create_index_buffer(std::vector<unsigned short> CubeIndices) const
{
    Microsoft::WRL::ComPtr < ID3D11Buffer> m_pIndexBuffer;
    CD3D11_BUFFER_DESC iDesc{
        (UINT)(CubeIndices.size() * sizeof(unsigned short)),
        D3D11_BIND_INDEX_BUFFER
    };

    D3D11_SUBRESOURCE_DATA iData{ CubeIndices.data(), 0, 0 };

    HRESULT hr = g_pd3dDevice->CreateBuffer(
        &iDesc,
        &iData,
        m_pIndexBuffer.GetAddressOf()
    );
    test_hresult(hr, "could not create index buffer");
    return m_pIndexBuffer;
}

frame_resources D3DDeviceHolder::prepare_frame_resources(const vertex_representation& vertices)
{
    return {
            .vertex_buffer = create_vertex_buffer(vertices.CubeVertices),
            .index_buffer = create_index_buffer(vertices.CubeIndices),
            .m_indexCount = (UINT)vertices.CubeIndices.size(),
    };
}

Microsoft::WRL::ComPtr <ID3D11DepthStencilView> D3DDeviceHolder::create_depth_stencil_view(Microsoft::WRL::ComPtr<ID3D11Texture2D>& m_pDepthStencil) const
{
    Microsoft::WRL::ComPtr <ID3D11DepthStencilView>  m_pDepthStencilView;
    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);

    HRESULT hr = g_pd3dDevice->CreateDepthStencilView(
        m_pDepthStencil.Get(),
        &depthStencilViewDesc,
        m_pDepthStencilView.GetAddressOf()
    );
    test_hresult(hr, "could not create depth stencil view");

    return m_pDepthStencilView;
}

void D3DDeviceHolder::set_buffers(Microsoft::WRL::ComPtr <ID3D11Buffer>& vertex_buffer, Microsoft::WRL::ComPtr < ID3D11Buffer>& index_buffer) const
{
    // Set up the IA stage by setting the input topology and layout.
    UINT stride = sizeof(VertexPositionColor);
    UINT offset = 0;

    g_pd3dDeviceContext->IASetVertexBuffers(
        0,
        1,
        vertex_buffer.GetAddressOf(),
        &stride,
        &offset
    );

    g_pd3dDeviceContext->IASetIndexBuffer(
        index_buffer.Get(),
        DXGI_FORMAT_R16_UINT,
        0
    );

    g_pd3dDeviceContext->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    );
}

void D3DDeviceHolder::setup_shaders(const vertex_shader_holder& vertex_shader,
    Microsoft::WRL::ComPtr <ID3D11Buffer>& constant_buffer, Microsoft::WRL::ComPtr <ID3D11PixelShader>& pixel_shader) const
{
    g_pd3dDeviceContext->IASetInputLayout(vertex_shader.m_pInputLayout.Get());

    // Set up the vertex shader stage.
    g_pd3dDeviceContext->VSSetShader(
        vertex_shader.m_pVertexShader.Get(),
        nullptr,
        0
    );

    g_pd3dDeviceContext->VSSetConstantBuffers(
        0,
        (UINT)1,
        constant_buffer.GetAddressOf()
    );

    // Set up the pixel shader stage.
    g_pd3dDeviceContext->PSSetShader(
        pixel_shader.Get(),
        nullptr,
        0
    );
}

void D3DDeviceHolder::draw_scene(int m_indexCount) const
{
    // Calling Draw tells Direct3D to start sending commands to the graphics device.
    g_pd3dDeviceContext->DrawIndexed(
        m_indexCount,
        0,
        0
    );

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // Present
    HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
    test_hresult(hr, "could not draw");
}

HwndWrapper::HwndWrapper(WNDCLASSEXW& wc)
{
    hwnd = ::CreateWindowW(wc.lpszClassName, L"Amigurumi designer", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);
    if (hwnd == NULL)
    {
        throw std::runtime_error("could not creatw window");
    }
}

HwndWrapper::~HwndWrapper()
{
    ::DestroyWindow(hwnd);
}

void HwndWrapper::show_window() const
{
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);
}

imgui_context_holder::imgui_context_holder()
{
    ImGui::CreateContext();
}

imgui_context_holder::~imgui_context_holder()
{
    ImGui::DestroyContext();
}

imgui_dx11_holder::imgui_dx11_holder(ID3D11Device* g_pd3dDevice, ID3D11DeviceContext* g_pd3dDeviceContext)
{
    auto res = ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    if (!res)
    {
        throw std::runtime_error("could not init imgui DX11");
    }
}

imgui_dx11_holder::~imgui_dx11_holder()
{
    ImGui_ImplDX11_Shutdown();
}

imgui_holder::imgui_holder(HWND hwnd, Microsoft::WRL::ComPtr<ID3D11Device>& g_pd3dDevice, Microsoft::WRL::ComPtr <ID3D11DeviceContext>& g_pd3dDeviceContext) :
    imgui_context{},
    imgui_win32{ hwnd },
    imgui_dx11{ g_pd3dDevice.Get(), g_pd3dDeviceContext.Get() }
{

}

application_basics::application_basics()
{
    window.show_window();
    IMGUI_CHECKVERSION();
    setup_imgui();
}

void application_basics::update_after_resize()
{
    target_view = std::optional<render_target_view_holder>{};
    d3dDevice.g_pd3dDeviceContext->ClearState();
    d3dDevice.g_pd3dDeviceContext->Flush();
    HRESULT hr = d3dDevice.g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
    test_hresult(hr, "could not resize buffers");
    target_view = init_render_target_view(d3dDevice);
}

void application_basics::clear_render_target_view()
{
    const float teal[] = { 0.098f, 0.439f, 0.439f, 1.000f };
    d3dDevice.g_pd3dDeviceContext->ClearRenderTargetView(
        target_view->g_mainRenderTargetView.Get(),
        teal
    );
}

void application_basics::update_constant_struct(Microsoft::WRL::ComPtr <ID3D11Buffer>& constant_buffer)
{
    auto constant_struct = calculate_projections(target_view->m_bbDesc);

    d3dDevice.g_pd3dDeviceContext->UpdateSubresource(
        constant_buffer.Get(),
        0,
        nullptr,
        &(constant_struct),
        0,
        0
    );
}

Microsoft::WRL::ComPtr <ID3D11Texture2D> application_basics::create_depth_stencil()
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

    HRESULT res = d3dDevice.g_pd3dDevice->CreateTexture2D(
        &depthStencilDesc,
        nullptr,
        m_pDepthStencil.GetAddressOf()
    );
    test_hresult(res, "could not create depth stencil");

    return m_pDepthStencil;
}

void application_basics::set_depth_stencil_to_scene(Microsoft::WRL::ComPtr <ID3D11DepthStencilView>& m_pDepthStencilView)
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

imgui_win32_holder::imgui_win32_holder(HWND hwnd)
{
    auto res = ImGui_ImplWin32_Init(hwnd);
    if (!res)
    {
        throw std::runtime_error("could not init imgui win32");
    }
}

imgui_win32_holder::~imgui_win32_holder()
{
    ImGui_ImplWin32_Shutdown();
}