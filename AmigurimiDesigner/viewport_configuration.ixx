module;

#include <directxmath.h>
#include <d3d11.h>

export module viewport_configuration;
import std;

struct MousePosition
{
    int x = 0;
    int y = 0;
};

struct ViewportConfigurationManager
{
    DirectX::XMVECTOR at = DirectX::XMVectorSet(0.0f, -0.1f, 0.0f, 0.f);
    DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.f);
    DirectX::XMVECTOR ref_up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.f);
    DirectX::XMVECTOR center_direction = DirectX::XMVectorSet(0.0f, 7.f, 15.f, 0.f);

    float zoom_factor = 10;

    MousePosition current_pos;
    MousePosition drag_start;
    bool dragging = false;
    bool right_dragging = false;

    DirectX::XMVECTOR calc_eye() const
    {
        using namespace DirectX;
        return calc_at() - zoom_factor * calc_center_direction();
    }

    DirectX::XMVECTOR calc_center_direction() const
    {
        using namespace DirectX;
        const float y_diff = dragging ? ((float)current_pos.y - drag_start.y) : 0;
        const float x_diff = dragging ? ((float)current_pos.x - drag_start.x) : 0;
        return rotated_center_direction(x_diff, y_diff);
    }

    DirectX::XMVECTOR rotated_center_direction(float x_diff, float y_diff) const
    {
        using namespace DirectX;
        const auto normalized_direction = XMVector3Normalize(center_direction);
        const auto left = XMVector3Normalize(XMVector3Cross(normalized_direction, up));
        const auto real_up = XMVector3Cross(normalized_direction, left);
        const auto x_rotation = XMMatrixRotationAxis(real_up, x_diff / 100);
        const auto y_rotation = XMMatrixRotationAxis(left, -y_diff  / 100);
        const auto x_rotated_vector = XMVector3Transform(normalized_direction, x_rotation);
        const auto rotated_vector = XMVector3Transform(x_rotated_vector, y_rotation);
        const auto normalized_resulting_direction = XMVector3Normalize(rotated_vector);

        return normalized_resulting_direction;
    }
    DirectX::XMVECTOR calc_up()
    {
        using namespace DirectX;
        if (!dragging)
        {
            ref_up = up;
        }
        else {

            ref_up = updated_up();
        }
        return ref_up;
    }
    DirectX::XMVECTOR updated_up() const
    {
        using namespace DirectX;
        const auto left = XMVector3Cross(center_direction, ref_up);
        const auto new_diraction = calc_center_direction();
        const auto new_up = XMVector3Normalize(XMVector3Cross(left, new_diraction));
        if (XMVectorGetX(XMVector3Dot(ref_up, new_up)) < 0)
        {
            return -new_up;
        }
        return new_up;
    }
    DirectX::XMVECTOR calc_at() const
    {
        using namespace DirectX;
        if (!right_dragging)
        {
            return at;
        }
        const float y_diff = (float)current_pos.y - drag_start.y;
        const float x_diff = (float)current_pos.x - drag_start.x;
        return shifted_at(x_diff, y_diff);
    }

    DirectX::XMVECTOR shifted_at(float x, float y) const
    {
        using namespace DirectX;
        const auto normalized_direction = XMVector3Normalize(center_direction);
        const auto left = XMVector3Normalize(XMVector3Cross(normalized_direction, up));
        const auto real_up = XMVector3Cross(normalized_direction, left);
        return at - y * zoom_factor / 1000 * real_up - x * zoom_factor / 1000 * left;
    }

    void stop_dragging_left()
    {
        const auto new_up = calc_up();
        const auto new_direction = calc_center_direction();
        up = new_up;
        center_direction = new_direction;
        dragging = false;
    }

    void stop_dragging_right()
    {
        const auto new_at = calc_at();
        const auto new_direction = calc_center_direction();
        at = new_at;
        right_dragging = false;
    }

    void reset_defaults(DirectX::XMFLOAT3 pos)
    {
        using namespace DirectX;
        at = DirectX::XMVectorSet(pos.x, pos.y, pos.z, 0.f);
        up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.f);
        center_direction = DirectX::XMVectorSet(0.0f, -7.f, -15.f, 0.f);
        zoom_factor = 10;
    }
    void front_view(DirectX::XMFLOAT3 pos)
    {
        using namespace DirectX;
        at = DirectX::XMVectorSet(pos.x, pos.y, pos.z, 0.f);
        up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.f);
        center_direction = XMVectorSet(0.0f, 0.f, 1.f, 0.f);
    }
    void top_view(DirectX::XMFLOAT3 pos)
    {
        using namespace DirectX;
        at = DirectX::XMVectorSet(pos.x, pos.y, pos.z, 0.f);
        up = XMVectorSet(0.0f, 0.0f, -1.0f, 0.f);
        center_direction = XMVectorSet(0.0f, 1.f, 0.f, 0.f);
    }

    void optimal_size(float height, const D3D11_TEXTURE2D_DESC& m_bbDesc)
    {
        using namespace DirectX;
        auto normalized_direction = XMVector3Normalize(center_direction);
        zoom_factor = height * 2000 / m_bbDesc.Height;
    }
    void set_at(DirectX::XMFLOAT3 pos)
    {
        at = DirectX::XMVectorSet(pos.x, pos.y, pos.z, 0.f);
    }
};

export ViewportConfigurationManager viewport_config;