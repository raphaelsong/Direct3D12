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
}

void D3DSample::Render()
{
}

void D3DSample::EndRender()
{
}
