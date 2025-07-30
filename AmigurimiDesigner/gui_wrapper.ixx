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

#include <fstream>
#include <optional>
#include <array>
#include <vector>
#include <iostream>

export module gui_wrapper;

struct vertex_shader_holder
{
    Microsoft::WRL::ComPtr <ID3D11VertexShader> m_pVertexShader;
    Microsoft::WRL::ComPtr <ID3D11InputLayout> m_pInputLayout;
};

struct frame_resources
{
    Microsoft::WRL::ComPtr <ID3D11Buffer> vertex_buffer;
    Microsoft::WRL::ComPtr <ID3D11Buffer> index_buffer;
    UINT m_indexCount;
};

export struct VertexPositionColor
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT3 color;
};

export struct vertex_representation
{
    std::vector<VertexPositionColor> vertices;
    std::vector<unsigned short> indices;
};

void test_hresult(const HRESULT hr, const char* message)
{
    if (hr != S_OK)
    {
        throw std::runtime_error(message);
    }
}

struct constant_buffer_struct {
    DirectX::XMFLOAT4X4 world;
    DirectX::XMFLOAT4X4 view;
    DirectX::XMFLOAT4X4 projection;
};

struct D3DDeviceHolder
{
    Microsoft::WRL::ComPtr<ID3D11Device> g_pd3dDevice;
    Microsoft::WRL::ComPtr <ID3D11DeviceContext> g_pd3dDeviceContext;
    Microsoft::WRL::ComPtr < IDXGISwapChain> g_pSwapChain;

    vertex_shader_holder load_vertex_shader() const
    {
        vertex_shader_holder vs;
        std::ifstream shader_file{ "VertexShader.cso", std::ios::binary };
        const std::vector<char> file_contents((std::istreambuf_iterator<char>(shader_file)), std::istreambuf_iterator<char>());
        HRESULT hr = g_pd3dDevice->CreateVertexShader(
            file_contents.data(), file_contents.size(),
            nullptr, vs.m_pVertexShader.GetAddressOf()
        );
        test_hresult(hr, "could not create vertex shader");

        const std::vector<D3D11_INPUT_ELEMENT_DESC> input_description
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
            0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },

            { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT,
            0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        hr = g_pd3dDevice->CreateInputLayout(
            input_description.data(), (UINT)input_description.size(),
            file_contents.data(), file_contents.size(),
            vs.m_pInputLayout.GetAddressOf()
        );
        test_hresult(hr, "could not create vertex shader");

        return vs;
    }

    Microsoft::WRL::ComPtr <ID3D11PixelShader> load_pixel_shader() const
    {
        Microsoft::WRL::ComPtr <ID3D11PixelShader> m_pPixelShader;
        std::ifstream pShader{ "PixelShader.cso",  std::ios::binary };
        const std::vector<char> fileContents((std::istreambuf_iterator<char>(pShader)),
            std::istreambuf_iterator<char>());

        const HRESULT hr = g_pd3dDevice->CreatePixelShader(
            fileContents.data(),
            fileContents.size(),
            nullptr,
            m_pPixelShader.GetAddressOf()
        );
        test_hresult(hr, "could not create pixel shader");
        return m_pPixelShader;
    }

    Microsoft::WRL::ComPtr <ID3D11Buffer> create_constant_buffer()
    {
        Microsoft::WRL::ComPtr <ID3D11Buffer> m_pConstantBuffer;
        CD3D11_BUFFER_DESC cbDesc{
        sizeof(constant_buffer_struct),
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

    bool is_ocluded() const
    {
        return g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED;
    }
    Microsoft::WRL::ComPtr <ID3D11Buffer> create_vertex_buffer(const std::vector<VertexPositionColor>& CubeVertices) const
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
    Microsoft::WRL::ComPtr <ID3D11Buffer> create_index_buffer(const std::vector<unsigned short> & CubeIndices) const
    {
        Microsoft::WRL::ComPtr < ID3D11Buffer> m_pIndexBuffer;
        CD3D11_BUFFER_DESC iDesc{
            (UINT)(CubeIndices.size() * sizeof(unsigned short)),
            D3D11_BIND_INDEX_BUFFER
        };

        const D3D11_SUBRESOURCE_DATA iData{ CubeIndices.data(), 0, 0 };

        HRESULT hr = g_pd3dDevice->CreateBuffer(
            &iDesc,
            &iData,
            m_pIndexBuffer.GetAddressOf()
        );
        test_hresult(hr, "could not create index buffer");
        return m_pIndexBuffer;
    }

    frame_resources prepare_frame_resources(const vertex_representation& vertices)
    {
        return {
                .vertex_buffer = create_vertex_buffer(vertices.vertices),
                .index_buffer = create_index_buffer(vertices.indices),
                .m_indexCount = (UINT)vertices.indices.size(),
        };
    }
    Microsoft::WRL::ComPtr <ID3D11DepthStencilView> create_depth_stencil_view(Microsoft::WRL::ComPtr<ID3D11Texture2D>& m_pDepthStencil) const
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

    void set_buffers(const Microsoft::WRL::ComPtr <ID3D11Buffer>& vertex_buffer, const Microsoft::WRL::ComPtr < ID3D11Buffer>& index_buffer) const
    {
        // Set up the IA stage by setting the input topology and layout.
        const UINT stride = sizeof(VertexPositionColor);
        const UINT offset = 0;

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

    void setup_shaders(const vertex_shader_holder& vertex_shader,
        const Microsoft::WRL::ComPtr <ID3D11Buffer>& constant_buffer, const Microsoft::WRL::ComPtr <ID3D11PixelShader>& pixel_shader) const
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
    void draw_scene(const int m_indexCount) const
    {
        // Calling Draw tells Direct3D to start sending commands to the graphics device.
        g_pd3dDeviceContext->DrawIndexed(
            m_indexCount,
            0,
            0
        );

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        const HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
        test_hresult(hr, "could not draw");
    }
};

D3DDeviceHolder init_d3d_device(HWND hWnd);

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

    float zoom_factor = 10;

    MousePosition current_pos;
    MousePosition drag_start;
    bool dragging = false;
    bool right_dragging = false;

    DirectX::XMVECTOR calc_eye() const;
    DirectX::XMVECTOR rotaded_eye(float x_diff, float y_diff) const
    {
        using namespace DirectX;
        auto center_direction = eye - at;
        auto normalized_direction = XMVector3Normalize(center_direction);
        auto left = XMVector3Normalize(XMVector3Cross(up, normalized_direction));
        auto real_up = XMVector3Cross(normalized_direction, left);
        auto final_direction = normalized_direction + y_diff / 500 * real_up - x_diff / 500 * left;
        auto normalized_resulting_direction = XMVector3Normalize(final_direction);

        return calc_at() + zoom_factor * normalized_resulting_direction;
    }
    DirectX::XMVECTOR calc_up() const
    {
        using namespace DirectX;
        if (!dragging)
        {
            return up;
        }

        return updated_up();
    }
    DirectX::XMVECTOR updated_up() const
    {
        using namespace DirectX;
        auto original_direction = eye - at;
        auto left = -XMVector3Cross(original_direction, up);
        auto new_eye = calc_eye();
        auto new_diraction = new_eye - at;
        auto new_up = XMVector3Normalize(XMVector3Cross(new_diraction, left));
        return new_up;
    }
    DirectX::XMVECTOR calc_at() const;
    void stop_dragging();
    void reset_defaults()
    {
        eye = DirectX::XMVectorSet(0.0f, 7.f, 15.f, 0.f);
        at = DirectX::XMVectorSet(0.0f, -0.1f, 0.0f, 0.f);
        up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.f);
        zoom_factor = 10;
    }
    void front_view()
    {
        using namespace DirectX;
        auto center_direction = eye - at;
        at = XMVectorSet(0.0f, -0.1f, 0.0f, 0.f);
        up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.f);
        eye = at + XMVectorSet(0.0f, 0.f, 1.f, 0.f);
    }
    void top_view()
    {
        using namespace DirectX;
        auto center_direction = eye - at;
        at = XMVectorSet(0.0f, -0.1f, 0.0f, 0.f);
        up = XMVectorSet(0.0f, 0.0f, -1.0f, 0.f);
        eye = at + XMVectorSet(0.0f, 1.f, 0.f, 0.f);
    }

    void optimal_size(float height, const D3D11_TEXTURE2D_DESC& m_bbDesc)
    {
        using namespace DirectX;
        auto center_direction = eye - at;
        auto normalized_direction = XMVector3Normalize(center_direction);
        zoom_factor = height * 2000 / m_bbDesc.Height;
        eye = at + zoom_factor
            * normalized_direction;
    }
    void set_at(DirectX::XMFLOAT3 pos)
    {
        at = DirectX::XMVectorSet(pos.x, pos.y, pos.z, 0.f);
    }
};

ViewportConfigurationManager viewport_config;

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
            viewport_config.reset_defaults();
        }
        if (ImGui::Button("front view")) {
            viewport_config.front_view();
        }
        if (ImGui::Button("top view")) {
            viewport_config.top_view();
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

export struct global_resources
{
    vertex_shader_holder vertex_shader;
    Microsoft::WRL::ComPtr <ID3D11PixelShader> pixel_shader;
    Microsoft::WRL::ComPtr <ID3D11Buffer> constant_buffer;
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

LRESULT  handle_key_down(WPARAM wParam)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard)
    {
        return 0;
    }
    if (wParam == VK_LEFT)
    {
        viewport_config.eye = viewport_config.rotaded_eye(-30, 0);
        viewport_config.up = viewport_config.updated_up();
    }
    if (wParam == VK_RIGHT)
    {
        viewport_config.eye = viewport_config.rotaded_eye(30, 0);
        viewport_config.up = viewport_config.updated_up();
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
    case WM_RBUTTONDOWN:
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
    case WM_MOUSEMOVE:
        viewport_config.current_pos.x = GET_X_LPARAM(lParam);
        viewport_config.current_pos.y = GET_Y_LPARAM(lParam);
        return 0;
    case WM_RBUTTONUP:
    case WM_LBUTTONUP:
        viewport_config.stop_dragging();
        return 0;
    case WM_MOUSEWHEEL:
        return handle_mouse_wheel(wParam);
    case WM_KEYDOWN:
        return handle_key_down(wParam);
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

DirectX::XMVECTOR ViewportConfigurationManager::calc_eye() const
{
    float y_diff = dragging ? ((float)current_pos.y - drag_start.y) : 0;
    float x_diff = dragging ? ((float)current_pos.x - drag_start.x) : 0;

    return rotaded_eye(x_diff, y_diff);
}

DirectX::XMVECTOR ViewportConfigurationManager::calc_at() const
{
    using namespace DirectX;
    if (!right_dragging)
    {
        return at;
    }
    auto center_direction = eye - at;
    auto normalized_direction = XMVector3Normalize(center_direction);
    auto left = XMVector3Normalize(XMVector3Cross(up, normalized_direction));
    auto real_up = XMVector3Cross(normalized_direction, left);
    float y_diff = (float)current_pos.y - drag_start.y;
    float x_diff = (float)current_pos.x - drag_start.x;
    return at + y_diff /60 * real_up - x_diff /60 * left;
}

void ViewportConfigurationManager::stop_dragging()
{
    auto new_eye = calc_eye();
    auto new_up = calc_up();
    auto new_at = calc_at();
    eye = new_eye;
    up = new_up;
    at = new_at;
    dragging = false;
    right_dragging = false;
}
