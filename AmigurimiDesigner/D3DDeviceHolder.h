#pragma once

#define NOMINMAX
#include <Windows.h>
#include <wrl\client.h>
#include <d3d11.h>
#include <vector>
#include <directxmath.h>

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

struct VertexPositionColor
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT3 color;
};

struct vertex_representation
{
    std::vector<VertexPositionColor> vertices;
    std::vector<unsigned short> indices;
};

struct D3DDeviceHolder
{
    Microsoft::WRL::ComPtr<ID3D11Device> g_pd3dDevice;
    Microsoft::WRL::ComPtr <ID3D11DeviceContext> g_pd3dDeviceContext;
    Microsoft::WRL::ComPtr < IDXGISwapChain> g_pSwapChain;

    vertex_shader_holder load_vertex_shader() const;
    Microsoft::WRL::ComPtr <ID3D11PixelShader> load_pixel_shader() const;
    Microsoft::WRL::ComPtr <ID3D11Buffer> create_constant_buffer();
    bool is_ocluded() const;
    Microsoft::WRL::ComPtr <ID3D11Buffer> create_vertex_buffer(const std::vector<VertexPositionColor>& CubeVertices) const;
    Microsoft::WRL::ComPtr <ID3D11Buffer> create_index_buffer(std::vector<unsigned short> CubeIndices) const;
    frame_resources prepare_frame_resources(const vertex_representation& vertices);
    Microsoft::WRL::ComPtr <ID3D11DepthStencilView> create_depth_stencil_view(Microsoft::WRL::ComPtr<ID3D11Texture2D>& m_pDepthStencil) const;
    void set_buffers(Microsoft::WRL::ComPtr <ID3D11Buffer>& vertex_buffer, Microsoft::WRL::ComPtr < ID3D11Buffer>& index_buffer) const;
    void setup_shaders(const vertex_shader_holder& vertex_shader,
        Microsoft::WRL::ComPtr <ID3D11Buffer>& constant_buffer, Microsoft::WRL::ComPtr <ID3D11PixelShader>& pixel_shader) const;
    void draw_scene(int m_indexCount) const;
};

D3DDeviceHolder init_d3d_device(HWND hWnd);
