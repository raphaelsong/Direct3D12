#include "D3DSample.h"

D3DSample::D3DSample(HINSTANCE hInstance)
    : D3DRenderer(hInstance)
{
}

D3DSample::~D3DSample()
{
}

bool D3DSample::Initialize()
{
    if (!D3DRenderer::Initialize())
        return false;

    return true;
}

void D3DSample::OnResize()
{
    D3DRenderer::OnResize();
}

void D3DSample::Update(float deltaTime)
{
}

void D3DSample::LateUpdate(float deltaTime)
{
}

void D3DSample::BeginRender()
{
    ThrowIfFailed(m_CommandAlloc->Reset());

    ThrowIfFailed(m_CommandList->Reset(m_CommandAlloc.Get(), nullptr));

    m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    m_CommandList->RSSetViewports(1, &m_ScreenViewport);
    m_CommandList->RSSetScissorRects(1, &m_ScissorRect);

    m_CommandList->ClearRenderTargetView(GetRenderTargetView(), Colors::LightSkyBlue, 0, nullptr);
    m_CommandList->ClearDepthStencilView(GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    m_CommandList->OMSetRenderTargets(1, &GetRenderTargetView(), true, &GetDepthStencilView());
}

void D3DSample::Render()
{
}

void D3DSample::EndRender()
{
    m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    ThrowIfFailed(m_CommandList->Close());

    ID3D12CommandList* CmdsLists[] = { m_CommandList.Get() };
    m_CommandQueue->ExecuteCommandLists(_countof(CmdsLists), CmdsLists);

    ThrowIfFailed(m_SwapChain->Present(0, 0));
    m_CurrentBackBufferIndex = (m_CurrentBackBufferIndex + 1) % SWAP_CHAIN_BUFFER_COUNT;

    FlushCommandQueue();
}
