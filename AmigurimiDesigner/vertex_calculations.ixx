module;

#include <directxmath.h>

#include "gsl/gsl"

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

export void test_vertex_vector_size(const std::vector<vertex_position_color>& vertices)
{
    if (vertices.size() > std::numeric_limits<unsigned short>::max())
    {
        throw std::runtime_error("Too many vertices were generated.");
    }
}

export void draw_bottom_lid(vertex_representation& vr, const float diameter, const int slice_count)
{
    test_vertex_vector_size(vr.vertices);
    vr.vertices.emplace_back(DirectX::XMFLOAT3{ 0.f, 0.f, 0.f }, DirectX::XMFLOAT3{ 1.f,   1.f,   1.f });
    const unsigned short start_index = gsl::narrow<unsigned short>(vr.vertices.size());
    for (unsigned short i = 0; i < slice_count; ++i)
    {
        const float angle = static_cast<float>(i * std::numbers::pi * 2 / slice_count);
        vr.vertices.emplace_back(DirectX::XMFLOAT3{ std::sin(angle) * diameter, 0.f, std::cos(angle) * diameter }, DirectX::XMFLOAT3{ std::sin(angle),   std::cos(angle),   0 });
    }
    for (unsigned short i = 0; i < slice_count; ++i)
    {
        vr.indices.push_back((unsigned short)(start_index + i));
        vr.indices.push_back((unsigned short)(start_index + (i + 1) % slice_count));
        vr.indices.push_back((unsigned short)(start_index - 1));
    }
    test_vertex_vector_size(vr.vertices);
}