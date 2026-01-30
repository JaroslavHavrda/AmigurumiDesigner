module;

#include "imgui_impl_dx11.h"

#include <wrl\client.h>
#include <d3d11.h>

export module D3DDeviceHolder;
import direct_x_structures;
import std;

export struct D3DDeviceHolder
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

    Microsoft::WRL::ComPtr <ID3D11Buffer> create_constant_buffer() const
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
    Microsoft::WRL::ComPtr <ID3D11Buffer> create_index_buffer(const std::vector<unsigned short>& CubeIndices) const
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

export D3DDeviceHolder init_d3d_device(const HWND hWnd)
{
    D3DDeviceHolder device_holder;
    const DXGI_SWAP_CHAIN_DESC sd{
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
    const UINT createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel{};
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