#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <directxmath.h>
#include <wrl\client.h>
#include <Windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <stdexcept>
#include <array>
#include <cmath>
#include <optional>
#include <vector>
#include <fstream>
#include <iostream>
// Data
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

struct MousePosition
{
    int x = 0;
    int y = 0;
};

struct ViewportConfigurationManager
{
    DirectX::XMVECTOR eye = DirectX::XMVectorSet(0.0f, 0.7f, 1.5f, 0.f);
    DirectX::XMVECTOR at = DirectX::XMVectorSet(0.0f, -0.1f, 0.0f, 0.f);
    DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.f);
    DirectX::XMVECTOR rotation_center = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.f);

    MousePosition current_pos;
    MousePosition drag_start;
    bool dragging = false;

    DirectX::XMVECTOR calc_eye() const
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
        auto left = XMVector3Normalize( XMVector3Cross(up, normalized_direction));
        auto real_up = XMVector3Cross(normalized_direction, left);
        auto final_direction = normalized_direction + y_diff/500 * real_up - x_diff/500 * left;
        auto normalized_resulting_direction = XMVector3Normalize(final_direction);

        return rotation_center + distance * normalized_resulting_direction;
    }

    DirectX::XMVECTOR calc_up() const
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
        //return up;
    }

    void stop_dragging()
    {
        auto new_eye = calc_eye();
        auto new_up = calc_up();
        eye = new_eye;
        up = new_up;
        dragging = false;
    }
};

ViewportConfigurationManager viewport_config;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

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
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

struct WindowClassWrapper
{
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Amigurumi Designer", nullptr };

    WindowClassWrapper()
    {
        auto res =::RegisterClassExW(&wc);
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

struct HwndWrapper
{
    HWND hwnd;

    HwndWrapper(WNDCLASSEXW & wc)
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

struct VertexPositionColor
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT3 color;
};

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

struct vertex_representation
{
    std::vector<VertexPositionColor> CubeVertices;
    std::vector<unsigned short> CubeIndices;

};

struct ConstantBufferStruct {
    DirectX::XMFLOAT4X4 world;
    DirectX::XMFLOAT4X4 view;
    DirectX::XMFLOAT4X4 projection;
};
static_assert((sizeof(ConstantBufferStruct) % 16) == 0, "Constant Buffer size must be 16-byte aligned");

void test_hresult(HRESULT hr, const char * message)
{
    if (hr != S_OK)
    {
        throw std::runtime_error(message);
    }
}

struct D3DDeviceHolder
{
    Microsoft::WRL::ComPtr<ID3D11Device> g_pd3dDevice;
    Microsoft::WRL::ComPtr <ID3D11DeviceContext> g_pd3dDeviceContext;
    Microsoft::WRL::ComPtr < IDXGISwapChain> g_pSwapChain;

    vertex_shader_holder load_vertex_shader() const
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

    Microsoft::WRL::ComPtr <ID3D11PixelShader> load_pixel_shader() const
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

    Microsoft::WRL::ComPtr <ID3D11Buffer> create_constant_buffer()
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

    Microsoft::WRL::ComPtr <ID3D11Buffer> create_index_buffer(std::vector<unsigned short> CubeIndices) const
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

    frame_resources prepare_frame_resources(const vertex_representation& vertices)
    {
        return {
                .vertex_buffer = create_vertex_buffer(vertices.CubeVertices),
                .index_buffer = create_index_buffer(vertices.CubeIndices),
                .m_indexCount = (UINT)vertices.CubeIndices.size(),
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

    void set_buffers(Microsoft::WRL::ComPtr <ID3D11Buffer>& vertex_buffer, Microsoft::WRL::ComPtr < ID3D11Buffer>& index_buffer) const
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

    void setup_shaders(const vertex_shader_holder& vertex_shader,
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

    void draw_scene(int m_indexCount) const
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
};

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

struct render_target_view_holder
{
    Microsoft::WRL::ComPtr <ID3D11Texture2D> pBackBuffer;
    Microsoft::WRL::ComPtr <ID3D11RenderTargetView> g_mainRenderTargetView;
    D3D11_TEXTURE2D_DESC    m_bbDesc{};
    D3D11_VIEWPORT          m_viewport{};
};

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

void setup_imgui()
{
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsLight();
}

struct imgui_win32_holder
{
    imgui_win32_holder(HWND hwnd)
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

vertex_representation calc_vertices()
{
    return {
        std::vector<VertexPositionColor>
        {
            {DirectX::XMFLOAT3{ -0.5f,-0.5f,-0.5f}, DirectX::XMFLOAT3{0,   0,   0},},
            {DirectX::XMFLOAT3{-0.5f,-0.5f, 0.5f}, DirectX::XMFLOAT3{0,   0,   1}, },
            {DirectX::XMFLOAT3{ -0.5f, 0.5f,-0.5f}, DirectX::XMFLOAT3{0,   1,   0},},
            {DirectX::XMFLOAT3{ -0.5f, 0.5f, 0.5f}, DirectX::XMFLOAT3{0,   1,   1},},

            {DirectX::XMFLOAT3{0.5f,-0.5f,-0.5f}, DirectX::XMFLOAT3{1,   0,   0}, },
            {DirectX::XMFLOAT3{0.5f,-0.5f, 0.5f}, DirectX::XMFLOAT3{1,   0,   1}, },
            {DirectX::XMFLOAT3{0.5f, 0.5f,-0.5f}, DirectX::XMFLOAT3{1,   1,   0}, },
            {DirectX::XMFLOAT3{0.5f, 0.5f, 0.5f}, DirectX::XMFLOAT3{0,   0,   0},},
        },
        std::vector<unsigned short>
        {
            0,2,1, // -x
            1,2,3,

            4,5,6, // +x
            5,7,6,

            0,1,5, // -y
            0,5,4,

            2,6,7, // +y
            2,7,3,

            0,4,6, // -z
            0,6,2,

            1,3,7, // +z
            1,7,5,
        }
    };
}

struct rotation_data_gui
{
    float roll{ 0 };
    float pitch{ 0 };
    float yaw{ 0 };
};

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

bool process_messages()
{
    bool done = false;
    // Poll and handle messages (inputs, window resize, etc.)
    // See the WndProc() function below for our to dispatch events to the Win32 backend.
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

struct imgui_holder
{
    imgui_context_holder imgui_context;
    imgui_win32_holder imgui_win32;
    imgui_dx11_holder imgui_dx11;

    imgui_holder(HWND hwnd, Microsoft::WRL::ComPtr<ID3D11Device>& g_pd3dDevice, Microsoft::WRL::ComPtr <ID3D11DeviceContext>& g_pd3dDeviceContext) :
        imgui_context{},
        imgui_win32{ hwnd },
        imgui_dx11{ g_pd3dDevice.Get(), g_pd3dDeviceContext.Get() }
    {

    }
};

struct application_basics
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

    void update_after_resize()
    {
        target_view = std::optional<render_target_view_holder>{};
        d3dDevice.g_pd3dDeviceContext->ClearState();
        d3dDevice.g_pd3dDeviceContext->Flush();
        HRESULT hr = d3dDevice.g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
        test_hresult(hr, "could not resize buffers");
        target_view = init_render_target_view(d3dDevice);
    }

    void clear_render_target_view()
    {
        const float teal[] = { 0.098f, 0.439f, 0.439f, 1.000f };
        d3dDevice.g_pd3dDeviceContext->ClearRenderTargetView(
            target_view->g_mainRenderTargetView.Get(),
            teal
        );
    }

    void update_constant_struct(Microsoft::WRL::ComPtr <ID3D11Buffer>& constant_buffer)
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

        HRESULT res = d3dDevice.g_pd3dDevice->CreateTexture2D(
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
};

struct gui_wrapper
{
    std::array<char, 500> prescription{ 0 };

    void present_using_imgui()
    {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Amigurumi designer");
        ImGui::InputTextMultiline("Prescription", prescription.data(), prescription.size());
        ImGui::End();
        ImGui::Begin("Rotation");
        ImGui::End();
        ImGui::Render();  
    }
};

struct global_resources
{
    vertex_shader_holder vertex_shader;
    Microsoft::WRL::ComPtr <ID3D11PixelShader> pixel_shader;
    Microsoft::WRL::ComPtr <ID3D11Buffer> constant_buffer;
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
        vertex_representation vertices = calc_vertices();
        frame_resources frame_res = app.d3dDevice.prepare_frame_resources(vertices);
        app.update_constant_struct(application_resources.constant_buffer);
        app.clear_render_target_view();
        Microsoft::WRL::ComPtr <ID3D11Texture2D> m_pDepthStencil = app.create_depth_stencil();
        Microsoft::WRL::ComPtr <ID3D11DepthStencilView>  m_pDepthStencilView = app.d3dDevice.create_depth_stencil_view(m_pDepthStencil);
        app.set_depth_stencil_to_scene(m_pDepthStencilView);
        app.d3dDevice.set_buffers(frame_res.vertex_buffer, frame_res.index_buffer);
        app.d3dDevice.setup_shaders(application_resources.vertex_shader, application_resources.constant_buffer, application_resources.pixel_shader);
        app.d3dDevice.draw_scene(frame_res.m_indexCount);
    }
        
    return 0;
}