module;

#include <directxmath.h>
#include <d3d11.h>

export module viewport_configuration;

export struct MousePosition
{
    int x = 0;
    int y = 0;
};

export struct ViewportConfigurationManager
{
    DirectX::XMVECTOR eye = DirectX::XMVectorSet(0.0f, 7.f, 15.f, 0.f);
    DirectX::XMVECTOR at = DirectX::XMVectorSet(0.0f, -0.1f, 0.0f, 0.f);
    DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.f);

    float zoom_factor = 10;

    MousePosition current_pos;
    MousePosition drag_start;
    bool dragging = false;
    bool right_dragging = false;

    DirectX::XMVECTOR calc_eye() const
    {
        float y_diff = dragging ? ((float)current_pos.y - drag_start.y) : 0;
        float x_diff = dragging ? ((float)current_pos.x - drag_start.x) : 0;

        return rotaded_eye(x_diff, y_diff);
    }

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
    DirectX::XMVECTOR calc_at() const
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
        return at + y_diff * zoom_factor / 1000 * real_up - x_diff * zoom_factor / 1000 * left;
    }

    DirectX::XMVECTOR shifted_at(int x, int y) const
    {
        using namespace DirectX;
        auto center_direction = eye - at;
        auto normalized_direction = XMVector3Normalize(center_direction);
        auto left = XMVector3Normalize(XMVector3Cross(up, normalized_direction));
        auto real_up = XMVector3Cross(normalized_direction, left);
        return at + y * zoom_factor / 1000 * real_up - x * zoom_factor / 1000 * left;
    }

    DirectX::XMVECTOR shifted_eye(int x, int y) const
    {
        using namespace DirectX;
        auto center_direction = eye - at;
        auto normalized_direction = XMVector3Normalize(center_direction);
        auto left = XMVector3Normalize(XMVector3Cross(up, normalized_direction));
        auto real_up = XMVector3Cross(normalized_direction, left);
        return eye + y * zoom_factor / 1000 * real_up - x * zoom_factor / 1000 * left;
    }

    void stop_dragging()
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

    void reset_defaults(DirectX::XMFLOAT3 pos)
    {
        using namespace DirectX;
        at = DirectX::XMVectorSet(pos.x, pos.y, pos.z, 0.f);
        eye = at + DirectX::XMVectorSet(0.0f, 7.f, 15.f, 0.f);
        up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.f);
        zoom_factor = 10;
    }
    void front_view(DirectX::XMFLOAT3 pos)
    {
        using namespace DirectX;
        auto center_direction = eye - at;
        at = DirectX::XMVectorSet(pos.x, pos.y, pos.z, 0.f);
        up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.f);
        eye = at + XMVectorSet(0.0f, 0.f, 1.f, 0.f);
    }
    void top_view(DirectX::XMFLOAT3 pos)
    {
        using namespace DirectX;
        auto center_direction = eye - at;
        at = DirectX::XMVectorSet(pos.x, pos.y, pos.z, 0.f);
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

export ViewportConfigurationManager viewport_config;