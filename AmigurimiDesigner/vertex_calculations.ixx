module;

#include <directxmath.h>

export module vertex_calculations;
import direct_x_structures;
import std;

export float calc_height_needed(const std::vector<vertex_position_color>& vertices) noexcept
{
    float max_z = 0.f;
    float max_y = 0.f;
    for (const auto& vertex : vertices)
    {
        max_y = std::max(max_z, vertex.pos.y);
        max_z = std::max(max_z, vertex.pos.z);
    }
    return max_z + max_y;
}

export DirectX::XMFLOAT3 calc_center(const std::vector<vertex_position_color>& vertices) noexcept
{
    DirectX::XMFLOAT3 center{ 0,0,0 };
    int n = 0;
    for (const auto& vertex : vertices)
    {
        n = n + 1;
        center.x = center.x + (vertex.pos.x - center.x) / n;
        center.y = center.y + (vertex.pos.y - center.y) / n;
        center.z = center.z + (vertex.pos.z - center.z) / n;
    }

    return center;
}