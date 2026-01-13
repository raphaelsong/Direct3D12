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

    ThrowIfFailed(m_CommandAlloc->Reset());
    ThrowIfFailed(m_CommandList->Reset(m_CommandAlloc.Get(), nullptr));

    // Rendering Geometry
    BuildGeometry();
    BuildConstantBuffer();
    BuildShader();
    BuildRootSignature();
    BuildInputLayout();
    BuildPipelineState();

    ThrowIfFailed(m_CommandList->Close());

    ID3D12CommandList* CmdsList[] = { m_CommandList.Get() };
    m_CommandQueue->ExecuteCommandLists(_countof(CmdsList), CmdsList);

    FlushCommandQueue();
    return true;
}

void D3DSample::OnResize()
{
    D3DRenderer::OnResize();

    XMMATRIX Proj = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&m_Proj, Proj);
}

void D3DSample::Update(float deltaTime)
{
    UpdateObjectCB(deltaTime);
}

void D3DSample::LateUpdate(float deltaTime)
{
    m_EyePos.x = m_Radius * sinf(m_Phi) * cosf(m_Theta);
    m_EyePos.y = m_Radius * cosf(m_Phi);
    m_EyePos.z = m_Radius * sinf(m_Phi) * sinf(m_Theta);

    // 시야 행렬 
    XMVECTOR EyePos = XMVectorSet(m_EyePos.x, m_EyePos.y, m_EyePos.z, 1.0f);
    XMVECTOR Target = XMVectorZero();
    XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX View = XMMatrixLookAtLH(EyePos, Target, Up);
    XMStoreFloat4x4(&m_View, View);

    UpdatePassCB(deltaTime);
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
    m_CommandList->SetPipelineState(m_PipelineState.Get());

    m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

    // 공용 상수 버퍼(뷰, 투영 행렬)
    D3D12_GPU_VIRTUAL_ADDRESS PassCBAddress = m_PassCB->GetGPUVirtualAddress();
    m_CommandList->SetGraphicsRootConstantBufferView(1, PassCBAddress);

   // 개별 오브젝트 렌더링
    RenderGeometry();
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

void D3DSample::OnMouseDown(WPARAM btnState, int x, int y)
{
    m_LastMousePos.x = x;
    m_LastMousePos.y = y;

    SetCapture(m_hWnd);
}

void D3DSample::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void D3DSample::OnMouseMove(WPARAM btnState, int x, int y)
{
    if ((btnState & MK_LBUTTON) != 0)
    {
        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - m_LastMousePos.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - m_LastMousePos.y));

        m_Theta += dx;
        m_Phi += dy;

        m_Phi = MathHelper::Clamp(m_Phi, 0.1f, MathHelper::Pi - 0.1f);
    }
    else if ((btnState & MK_RBUTTON) != 0)
    {
        float dx = 0.2f * static_cast<float>(x - m_LastMousePos.x);
        float dy = 0.2f * static_cast<float>(y - m_LastMousePos.y);

        m_Radius += dx - dy;

        m_Radius = MathHelper::Clamp(m_Radius, 3.0f, 150.0f);
    }

    m_LastMousePos.x = x;
    m_LastMousePos.y = y;
}

void D3DSample::BuildGeometry()
{
    std::array<Vertex, 8> Vertices =
    {
        Vertex({XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT4(Colors::Red)}),
        Vertex({XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT4(Colors::Blue)}),
        Vertex({XMFLOAT3(0.5f,  0.5f, -0.5f), XMFLOAT4(Colors::Green)}),
        Vertex({XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT4(Colors::Cyan)}),
        Vertex({XMFLOAT3(0.5f,  0.5f,  0.5f), XMFLOAT4(Colors::Magenta)}),
        Vertex({XMFLOAT3(0.5f, -0.5f,  0.5f), XMFLOAT4(Colors::Yellow)}),
        Vertex({XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT4(Colors::Violet)}),
        Vertex({XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT4(Colors::Pink)}),
    };

    std::array<std::int32_t, 36> Indices =
    {
        0, 1, 2,
        0, 2, 3,

        1, 7, 4,
        1, 4, 2,

        2, 4, 5,
        2, 5, 3,

        3, 5, 6,
        3, 6, 0,

        0, 6, 7,
        0, 7, 1,

        4, 6, 5,
        4, 7, 6,
    };

    m_pGeometry = std::make_unique<GeometryInfo>();
    m_pGeometry->Name = TEXT("Triangle");

    // 정점 버퍼 생성
    m_pGeometry->VertexCount = (UINT)Vertices.size();
    const UINT VBByteSize = m_pGeometry->VertexCount * sizeof(Vertex);

    m_pGeometry->VertexBuffer = d3dUtil::CreateDefaultBuffer(m_D3dDevice.Get(), m_CommandList.Get(), Vertices.data(), VBByteSize, m_pGeometry->VertexUploadBuffer);

    m_pGeometry->VertexBufferView.BufferLocation = m_pGeometry->VertexBuffer->GetGPUVirtualAddress();
    m_pGeometry->VertexBufferView.StrideInBytes = sizeof(Vertex);
    m_pGeometry->VertexBufferView.SizeInBytes = VBByteSize;

    // 인덱스 버퍼 생성
    m_pGeometry->IndexCount = (UINT)Indices.size();
    const UINT IBByteSize = m_pGeometry->IndexCount * sizeof(std::int32_t);

    m_pGeometry->IndexBuffer = d3dUtil::CreateDefaultBuffer(m_D3dDevice.Get(), m_CommandList.Get(), Indices.data(), IBByteSize, m_pGeometry->IndexUploadBuffer);

    m_pGeometry->IndexBufferView.BufferLocation = m_pGeometry->IndexBuffer->GetGPUVirtualAddress();
    m_pGeometry->IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    m_pGeometry->IndexBufferView.SizeInBytes = IBByteSize;
}

void D3DSample::BuildConstantBuffer()
{
    // 오브젝트 상수 버퍼 생성
    UINT ObjectSize = sizeof(ObjectConstant);
    m_ObjectByteSize = ((ObjectSize + 255) & ~255);

    D3D12_RESOURCE_DESC ObjectDesc = CD3DX12_RESOURCE_DESC::Buffer(m_ObjectByteSize);
    D3D12_HEAP_PROPERTIES ObjectHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    m_D3dDevice->CreateCommittedResource(
        &ObjectHeap,
        D3D12_HEAP_FLAG_NONE,
        &ObjectDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_ObjectCB));

    m_ObjectCB->Map(0, nullptr, reinterpret_cast<void**>(&m_ObjectMappedData));

    // 공용 상수 버퍼 생성
    UINT PassSize = sizeof(PassConstant);
    m_PassByteSize = (PassSize + 255) & ~255;

    D3D12_RESOURCE_DESC PassDesc = CD3DX12_RESOURCE_DESC::Buffer(m_PassByteSize);
    D3D12_HEAP_PROPERTIES PassHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    m_D3dDevice->CreateCommittedResource(
        &PassHeap,
        D3D12_HEAP_FLAG_NONE,
        &PassDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_PassCB));

    m_PassCB->Map(0, nullptr, reinterpret_cast<void**>(&m_PassMappedData));

}

void D3DSample::BuildShader()
{
    m_VSByteCode = d3dUtil::CompileShader(TEXT("../Shader/Default.hlsl"), nullptr, "VS", "vs_5_0");
    m_PSByteCode = d3dUtil::CompileShader(TEXT("../Shader/Default.hlsl"), nullptr, "PS", "ps_5_0");
}

void D3DSample::BuildRootSignature()
{
    CD3DX12_ROOT_PARAMETER Params[2];
    Params[0].InitAsConstantBufferView(0); // 0번 -> b0 : CBV m_ObjectCB
    Params[1].InitAsConstantBufferView(1); // 1번 -> b1 : CBV m_PassCB

    D3D12_ROOT_SIGNATURE_DESC RSDesc = CD3DX12_ROOT_SIGNATURE_DESC(_countof(Params), Params);
    RSDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> BlobSignature;
    ComPtr<ID3DBlob> BlobError;
    ::D3D12SerializeRootSignature(&RSDesc, D3D_ROOT_SIGNATURE_VERSION_1, &BlobSignature, &BlobError);

    m_D3dDevice->CreateRootSignature(0, BlobSignature->GetBufferPointer(), BlobSignature->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature));
}

void D3DSample::BuildInputLayout()
{
    m_InputLayout =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
}

void D3DSample::BuildPipelineState()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC ObjectDesc;
    ZeroMemory(&ObjectDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

    ObjectDesc.InputLayout = { m_InputLayout.data(), (UINT)m_InputLayout.size() };
    ObjectDesc.pRootSignature = m_RootSignature.Get();
    ObjectDesc.VS =
    {
        reinterpret_cast<BYTE*>(m_VSByteCode->GetBufferPointer()),
        m_VSByteCode->GetBufferSize()
    };
    ObjectDesc.PS =
    {
        reinterpret_cast<BYTE*>(m_PSByteCode->GetBufferPointer()),
        m_PSByteCode->GetBufferSize()
    };
    ObjectDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    ObjectDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    ObjectDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    ObjectDesc.SampleMask = UINT_MAX;
    ObjectDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    ObjectDesc.NumRenderTargets = 1;
    ObjectDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    ObjectDesc.SampleDesc.Count = 1;
    ObjectDesc.SampleDesc.Quality = 0;
    ObjectDesc.DSVFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

    ThrowIfFailed(m_D3dDevice->CreateGraphicsPipelineState(&ObjectDesc, IID_PPV_ARGS(&m_PipelineState)));
}

void D3DSample::UpdateObjectCB(float deltaTime)
{
    XMMATRIX World = XMLoadFloat4x4(&MathHelper::Identity4x4());

    ObjectConstant ObjectCB;
    XMStoreFloat4x4(&ObjectCB.World, XMMatrixTranspose(World));

    UINT ElementByteSize = (sizeof(ObjectConstant) + 255) & ~255;
    memcpy(&m_ObjectMappedData[0], &ObjectCB, sizeof(ObjectCB));
}

void D3DSample::UpdatePassCB(float deltaTime)
{
    XMMATRIX View = XMLoadFloat4x4(&m_View);
    XMMATRIX Proj = XMLoadFloat4x4(&m_Proj);
    XMMATRIX InvView = XMMatrixInverse(&XMMatrixDeterminant(View), View);
    XMMATRIX InvProj = XMMatrixInverse(&XMMatrixDeterminant(Proj), Proj);
    XMMATRIX ViewProj = XMMatrixMultiply(View, Proj);

    PassConstant PassCB;
    XMStoreFloat4x4(&PassCB.View, XMMatrixTranspose(View));
    XMStoreFloat4x4(&PassCB.Proj, XMMatrixTranspose(Proj));
    XMStoreFloat4x4(&PassCB.InvView, XMMatrixTranspose(InvView));
    XMStoreFloat4x4(&PassCB.InvProj, XMMatrixTranspose(InvProj));
    XMStoreFloat4x4(&PassCB.ViewProj, XMMatrixTranspose(ViewProj));

    memcpy(&m_PassMappedData[0], &PassCB, sizeof(PassCB));
}

void D3DSample::RenderGeometry()
{
    // 개별 오브젝트 상수 버프(월드 행렬)
    D3D12_GPU_VIRTUAL_ADDRESS ObjectCBAddress = m_ObjectCB->GetGPUVirtualAddress();
    m_CommandList->SetGraphicsRootConstantBufferView(0, ObjectCBAddress);

    m_CommandList->IASetVertexBuffers(0, 1, &m_pGeometry->VertexBufferView);
    m_CommandList->IASetIndexBuffer(&m_pGeometry->IndexBufferView);
    m_CommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_CommandList->DrawIndexedInstanced(m_pGeometry->IndexCount, 1, 0, 0, 0);
}
