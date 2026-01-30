module;

#define NOMINMAX

#include <wrl\client.h>
#include <d3d11.h>
#include <directxmath.h>

export module direct_x_structures;
import std;

export struct vertex_shader_holder
{
    Microsoft::WRL::ComPtr <ID3D11VertexShader> m_pVertexShader;
    Microsoft::WRL::ComPtr <ID3D11InputLayout> m_pInputLayout;
};

export struct frame_resources
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

export struct constant_buffer_struct {
    DirectX::XMFLOAT4X4 world;
    DirectX::XMFLOAT4X4 view;
    DirectX::XMFLOAT4X4 projection;
};

static_assert((sizeof(constant_buffer_struct) % 16) == 0, "Constant Buffer size must be 16-byte aligned");

export void test_hresult(const HRESULT hr, const char* message)
{
    if (hr != S_OK)
    {
        throw std::runtime_error(message);
    }
}

export struct global_resources
{
    vertex_shader_holder vertex_shader;
    Microsoft::WRL::ComPtr <ID3D11PixelShader> pixel_shader;
    Microsoft::WRL::ComPtr <ID3D11Buffer> constant_buffer;
};