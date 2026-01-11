module;
#include <d3d11.h>
#include <directxmath.h>
export module projections;
import direct_x_structures;
import viewport_configuration;

export constant_buffer_struct calculate_projections(const D3D11_TEXTURE2D_DESC& m_bbDesc)
{
    using namespace DirectX;
    constant_buffer_struct m_constantBufferData{};

    DirectX::XMStoreFloat4x4(
        &m_constantBufferData.view,
        DirectX::XMMatrixTranspose(
            DirectX::XMMatrixLookToRH(viewport_config.calc_eye(), viewport_config.calc_center_direction(), viewport_config.calc_up())));

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