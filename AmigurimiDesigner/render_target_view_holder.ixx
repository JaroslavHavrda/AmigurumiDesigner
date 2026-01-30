module;

#include <wrl\client.h>
#include <d3d11.h> 

export module render_target_view_holder;
import direct_x_structures;
import D3DDeviceHolder;

export struct render_target_view_holder
{
    Microsoft::WRL::ComPtr <ID3D11Texture2D> pBackBuffer;
    Microsoft::WRL::ComPtr <ID3D11RenderTargetView> g_mainRenderTargetView;
    D3D11_TEXTURE2D_DESC    m_bbDesc{};
    D3D11_VIEWPORT          m_viewport{};
};

export render_target_view_holder init_render_target_view(const D3DDeviceHolder& d3ddevice)
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