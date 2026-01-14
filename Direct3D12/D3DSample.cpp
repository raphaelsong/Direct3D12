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
    BuildMaterials();
    BuildRenderItems();
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
    UpdateMaterialCB(deltaTime);
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
    CreateBoxGeometry();
    CreateGridGeometry();
    CreateSphereGeometry();
    CreateCylinderGeometry();
    CreateSkullGeometry();
}

void D3DSample::BuildMaterials()
{
    UINT MatCBIndex = 0;

    auto Brick = std::make_unique<MaterialInfo>();
    Brick->Name = TEXT("Brick");
    Brick->MatCBIndex = MatCBIndex++;
    Brick->Albedo = XMFLOAT4(Colors::Red);
    Brick->Fresnel = XMFLOAT3(0.02f, 0.02f, 0.02f);
    Brick->Roughness = 0.1f;
    m_Materials[Brick->Name] = std::move(Brick);

    auto Stone = std::make_unique<MaterialInfo>();
    Stone->Name = TEXT("Stone");
    Stone->MatCBIndex = MatCBIndex++;
    Stone->Albedo = XMFLOAT4(Colors::LightSteelBlue);
    Stone->Fresnel = XMFLOAT3(0.05f, 0.05f, 0.05f);
    Stone->Roughness = 0.3f;
    m_Materials[Stone->Name] = std::move(Stone);

    auto Tile = std::make_unique<MaterialInfo>();
    Tile->Name = TEXT("Tile");
    Tile->MatCBIndex = MatCBIndex++;
    Tile->Albedo = XMFLOAT4(Colors::LightGray);
    Tile->Fresnel = XMFLOAT3(0.02f, 0.02f, 0.02f);
    Tile->Roughness = 0.2f;
    m_Materials[Tile->Name] = std::move(Tile);

    auto Skull = std::make_unique<MaterialInfo>();
    Skull->Name = TEXT("Skull");
    Skull->MatCBIndex = MatCBIndex++;
    Skull->Albedo = XMFLOAT4(Colors::White);
    Skull->Fresnel = XMFLOAT3(0.05f, 0.05f, 0.05f);
    Skull->Roughness = 0.3f;
    m_Materials[Skull->Name] = std::move(Skull);
}

void D3DSample::BuildRenderItems()
{
    auto GridItem = std::make_unique<RenderItem>();
    GridItem->World = MathHelper::Identity4x4();
    GridItem->Geometry = m_Geometries[TEXT("Grid")].get();
    GridItem->Material = m_Materials[TEXT("Tile")].get();
    m_RenderItems.push_back(std::move(GridItem));

    auto BoxItem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&BoxItem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f));
    BoxItem->Geometry = m_Geometries[TEXT("Box")].get();
    BoxItem->Material = m_Materials[TEXT("Stone")].get();
    m_RenderItems.push_back(std::move(BoxItem));

    auto SkullItem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&SkullItem->World, XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(0.0f, 1.0f, 0.0f));
    SkullItem->Geometry = m_Geometries[TEXT("Skull")].get();
    SkullItem->Material = m_Materials[TEXT("Skull")].get();
    m_RenderItems.push_back(std::move(SkullItem));

    for (int i = 0; i < 5; ++i)
    {
        auto LeftCylinderItem = std::make_unique<RenderItem>();
        auto RightCylinderItem = std::make_unique<RenderItem>();
        auto LeftSphereItem = std::make_unique<RenderItem>();
        auto RightSphereItem = std::make_unique<RenderItem>();

        XMMATRIX LeftCylinderWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
        XMMATRIX RightCylinderWorld = XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f);

        XMMATRIX LeftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
        XMMATRIX RightSphereWorld = XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f);

        XMStoreFloat4x4(&LeftCylinderItem->World, LeftCylinderWorld);
        LeftCylinderItem->Geometry = m_Geometries[TEXT("Cylinder")].get();
        LeftCylinderItem->Material = m_Materials[TEXT("Brick")].get();
        m_RenderItems.push_back(std::move(LeftCylinderItem));

        XMStoreFloat4x4(&RightCylinderItem->World, RightCylinderWorld);
        RightCylinderItem->Geometry = m_Geometries[TEXT("Cylinder")].get();
        RightCylinderItem->Material = m_Materials[TEXT("Brick")].get();
        m_RenderItems.push_back(std::move(RightCylinderItem));

        XMStoreFloat4x4(&LeftSphereItem->World, LeftSphereWorld);
        LeftSphereItem->Geometry = m_Geometries[TEXT("Sphere")].get();
        LeftSphereItem->Material = m_Materials[TEXT("Stone")].get();
        m_RenderItems.push_back(std::move(LeftSphereItem));

        XMStoreFloat4x4(&RightSphereItem->World, RightSphereWorld);
        RightSphereItem->Geometry = m_Geometries[TEXT("Sphere")].get();
        RightSphereItem->Material = m_Materials[TEXT("Stone")].get();
        m_RenderItems.push_back(std::move(RightSphereItem));
    }
}

void D3DSample::BuildConstantBuffer()
{
    // 오브젝트 상수 버퍼 생성
    UINT ObjectSize = sizeof(ObjectConstant);
    m_ObjectByteSize = ((ObjectSize + 255) & ~255) * (UINT)m_RenderItems.size();

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

    // 재질 상수 버퍼 생성
    UINT MaterialSize = sizeof(MatConstant);
    m_MaterialByteSize = ((MaterialSize + 255) & ~255) * m_Materials.size();

    D3D12_RESOURCE_DESC MaterialDesc = CD3DX12_RESOURCE_DESC::Buffer(m_MaterialByteSize);
    D3D12_HEAP_PROPERTIES MaterialHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    m_D3dDevice->CreateCommittedResource(
        &MaterialHeap,
        D3D12_HEAP_FLAG_NONE,
        &MaterialDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_MaterialCB));

    m_MaterialCB->Map(0, nullptr, reinterpret_cast<void**>(&m_MaterialMappedData));


}

void D3DSample::BuildShader()
{
    m_VSByteCode = d3dUtil::CompileShader(TEXT("../Shader/Default.hlsl"), nullptr, "VS", "vs_5_0");
    m_PSByteCode = d3dUtil::CompileShader(TEXT("../Shader/Default.hlsl"), nullptr, "PS", "ps_5_0");
}

void D3DSample::BuildRootSignature()
{
    CD3DX12_ROOT_PARAMETER Params[3];
    Params[0].InitAsConstantBufferView(0); // 0번 -> b0 : CBV m_ObjectCB
    Params[1].InitAsConstantBufferView(1); // 1번 -> b1 : CBV m_PassCB
    Params[2].InitAsConstantBufferView(2); // 2번 -> b2 : CBV m_MaterialCB

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
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
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

void D3DSample::CreateBoxGeometry()
{
    GeometryGenerator GeoGenerator;
    GeometryGenerator::MeshData Mesh = GeoGenerator.CreateBox(1.5f, 0.5f, 1.5f, 3);

    std::vector<Vertex> Vertices(Mesh.Vertices.size());
    for (size_t i = 0; i < Mesh.Vertices.size(); ++i)
    {
        Vertices[i].Pos = Mesh.Vertices[i].Position;
        Vertices[i].Normal = Mesh.Vertices[i].Normal;
    }

    std::vector<std::int32_t> Indices;
    Indices.insert(Indices.end(), std::begin(Mesh.Indices32), std::end(Mesh.Indices32));

    auto Geometry = std::make_unique<GeometryInfo>();
    Geometry->Name = TEXT("Box");

    // 정점 버퍼
    Geometry->VertexCount = (UINT)Vertices.size();
    const UINT VBByteSize = Geometry->VertexCount * sizeof(Vertex);

    Geometry->VertexBuffer = d3dUtil::CreateDefaultBuffer(m_D3dDevice.Get(), m_CommandList.Get(), Vertices.data(), VBByteSize, Geometry->VertexUploadBuffer);

    Geometry->VertexBufferView.BufferLocation = Geometry->VertexBuffer->GetGPUVirtualAddress();
    Geometry->VertexBufferView.StrideInBytes = sizeof(Vertex);
    Geometry->VertexBufferView.SizeInBytes = VBByteSize;

    // 인덱스 버퍼
    Geometry->IndexCount = (UINT)Indices.size();
    const UINT IBByteSize = Geometry->IndexCount * sizeof(std::int32_t);

    Geometry->IndexBuffer = d3dUtil::CreateDefaultBuffer(m_D3dDevice.Get(), m_CommandList.Get(), Indices.data(), IBByteSize, Geometry->IndexUploadBuffer);

    Geometry->IndexBufferView.BufferLocation = Geometry->IndexBuffer->GetGPUVirtualAddress();
    Geometry->IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    Geometry->IndexBufferView.SizeInBytes = IBByteSize;

    m_Geometries[Geometry->Name] = std::move(Geometry);
}

void D3DSample::CreateGridGeometry()
{
    GeometryGenerator GeoGenerator;
    GeometryGenerator::MeshData Mesh = GeoGenerator.CreateGrid(20.0f, 30.0f, 60, 40);

    std::vector<Vertex> Vertices(Mesh.Vertices.size());
    for (size_t i = 0; i < Mesh.Vertices.size(); ++i)
    {
        Vertices[i].Pos = Mesh.Vertices[i].Position;
        Vertices[i].Normal = Mesh.Vertices[i].Normal;
    }

    std::vector<std::int32_t> Indices;
    Indices.insert(Indices.end(), std::begin(Mesh.Indices32), std::end(Mesh.Indices32));

    auto Geometry = std::make_unique<GeometryInfo>();
    Geometry->Name = TEXT("Grid");

    // 정점 버퍼
    Geometry->VertexCount = (UINT)Vertices.size();
    const UINT VBByteSize = Geometry->VertexCount * sizeof(Vertex);

    Geometry->VertexBuffer = d3dUtil::CreateDefaultBuffer(m_D3dDevice.Get(), m_CommandList.Get(), Vertices.data(), VBByteSize, Geometry->VertexUploadBuffer);

    Geometry->VertexBufferView.BufferLocation = Geometry->VertexBuffer->GetGPUVirtualAddress();
    Geometry->VertexBufferView.StrideInBytes = sizeof(Vertex);
    Geometry->VertexBufferView.SizeInBytes = VBByteSize;

    // 인덱스 버퍼
    Geometry->IndexCount = (UINT)Indices.size();
    const UINT IBByteSize = Geometry->IndexCount * sizeof(std::int32_t);

    Geometry->IndexBuffer = d3dUtil::CreateDefaultBuffer(m_D3dDevice.Get(), m_CommandList.Get(), Indices.data(), IBByteSize, Geometry->IndexUploadBuffer);

    Geometry->IndexBufferView.BufferLocation = Geometry->IndexBuffer->GetGPUVirtualAddress();
    Geometry->IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    Geometry->IndexBufferView.SizeInBytes = IBByteSize;

    m_Geometries[Geometry->Name] = std::move(Geometry);
}

void D3DSample::CreateSphereGeometry()
{
    GeometryGenerator GeoGenerator;
    GeometryGenerator::MeshData Mesh = GeoGenerator.CreateSphere(0.5f, 20, 20);

    std::vector<Vertex> Vertices(Mesh.Vertices.size());
    for (size_t i = 0; i < Mesh.Vertices.size(); ++i)
    {
        Vertices[i].Pos = Mesh.Vertices[i].Position;
        Vertices[i].Normal = Mesh.Vertices[i].Normal;
    }

    std::vector<std::int32_t> Indices;
    Indices.insert(Indices.end(), std::begin(Mesh.Indices32), std::end(Mesh.Indices32));

    auto Geometry = std::make_unique<GeometryInfo>();
    Geometry->Name = TEXT("Sphere");

    // 정점 버퍼
    Geometry->VertexCount = (UINT)Vertices.size();
    const UINT VBByteSize = Geometry->VertexCount * sizeof(Vertex);

    Geometry->VertexBuffer = d3dUtil::CreateDefaultBuffer(m_D3dDevice.Get(), m_CommandList.Get(), Vertices.data(), VBByteSize, Geometry->VertexUploadBuffer);

    Geometry->VertexBufferView.BufferLocation = Geometry->VertexBuffer->GetGPUVirtualAddress();
    Geometry->VertexBufferView.StrideInBytes = sizeof(Vertex);
    Geometry->VertexBufferView.SizeInBytes = VBByteSize;

    // 인덱스 버퍼
    Geometry->IndexCount = (UINT)Indices.size();
    const UINT IBByteSize = Geometry->IndexCount * sizeof(std::int32_t);

    Geometry->IndexBuffer = d3dUtil::CreateDefaultBuffer(m_D3dDevice.Get(), m_CommandList.Get(), Indices.data(), IBByteSize, Geometry->IndexUploadBuffer);

    Geometry->IndexBufferView.BufferLocation = Geometry->IndexBuffer->GetGPUVirtualAddress();
    Geometry->IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    Geometry->IndexBufferView.SizeInBytes = IBByteSize;

    m_Geometries[Geometry->Name] = std::move(Geometry);
}

void D3DSample::CreateCylinderGeometry()
{
    GeometryGenerator GeoGenerator;
    GeometryGenerator::MeshData Mesh = GeoGenerator.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

    std::vector<Vertex> Vertices(Mesh.Vertices.size());
    for (size_t i = 0; i < Mesh.Vertices.size(); ++i)
    {
        Vertices[i].Pos = Mesh.Vertices[i].Position;
        Vertices[i].Normal = Mesh.Vertices[i].Normal;
    }

    std::vector<std::int32_t> Indices;
    Indices.insert(Indices.end(), std::begin(Mesh.Indices32), std::end(Mesh.Indices32));

    auto Geometry = std::make_unique<GeometryInfo>();
    Geometry->Name = TEXT("Cylinder");

    // 정점 버퍼
    Geometry->VertexCount = (UINT)Vertices.size();
    const UINT VBByteSize = Geometry->VertexCount * sizeof(Vertex);

    Geometry->VertexBuffer = d3dUtil::CreateDefaultBuffer(m_D3dDevice.Get(), m_CommandList.Get(), Vertices.data(), VBByteSize, Geometry->VertexUploadBuffer);

    Geometry->VertexBufferView.BufferLocation = Geometry->VertexBuffer->GetGPUVirtualAddress();
    Geometry->VertexBufferView.StrideInBytes = sizeof(Vertex);
    Geometry->VertexBufferView.SizeInBytes = VBByteSize;

    // 인덱스 버퍼
    Geometry->IndexCount = (UINT)Indices.size();
    const UINT IBByteSize = Geometry->IndexCount * sizeof(std::int32_t);

    Geometry->IndexBuffer = d3dUtil::CreateDefaultBuffer(m_D3dDevice.Get(), m_CommandList.Get(), Indices.data(), IBByteSize, Geometry->IndexUploadBuffer);

    Geometry->IndexBufferView.BufferLocation = Geometry->IndexBuffer->GetGPUVirtualAddress();
    Geometry->IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    Geometry->IndexBufferView.SizeInBytes = IBByteSize;

    m_Geometries[Geometry->Name] = std::move(Geometry);
}

void D3DSample::CreateSkullGeometry()
{
    // Skull 파일 로드
    std::ifstream InputFile("../Models/skull.txt");

    if (!InputFile)
    {
        MessageBox(0, TEXT("../Models/skull.txt not found"), 0, 0);
        return;
    }

    UINT VertexCount = 0;
    UINT TriangleCount = 0;
    
    std::string ignore;

    InputFile >> ignore >> VertexCount;
    InputFile >> ignore >> TriangleCount;
    InputFile >> ignore >> ignore >> ignore >> ignore;

    std::vector<Vertex> Vertices(VertexCount);
    for (UINT i = 0; i < VertexCount; ++i)
    {
        InputFile >> Vertices[i].Pos.x >> Vertices[i].Pos.y >> Vertices[i].Pos.z;
        InputFile >> Vertices[i].Normal.x >> Vertices[i].Normal.y >> Vertices[i].Normal.z;
    }

    InputFile >> ignore >> ignore >> ignore;

    std::vector<std::int32_t> Indices(TriangleCount * 3);
    for (UINT i = 0; i < TriangleCount; ++i)
    {
        InputFile >> Indices[i * 3 + 0] >> Indices[i * 3 + 1] >> Indices[i * 3 + 2];
    }

    InputFile.close();

    auto Geometry = std::make_unique<GeometryInfo>();
    Geometry->Name = TEXT("Skull");

    // 정점 버퍼
    Geometry->VertexCount = (UINT)Vertices.size();
    const UINT VBByteSize = Geometry->VertexCount * sizeof(Vertex);

    Geometry->VertexBuffer = d3dUtil::CreateDefaultBuffer(m_D3dDevice.Get(), m_CommandList.Get(), Vertices.data(), VBByteSize, Geometry->VertexUploadBuffer);

    Geometry->VertexBufferView.BufferLocation = Geometry->VertexBuffer->GetGPUVirtualAddress();
    Geometry->VertexBufferView.StrideInBytes = sizeof(Vertex);
    Geometry->VertexBufferView.SizeInBytes = VBByteSize;

    // 인덱스 버퍼
    Geometry->IndexCount = (UINT)Indices.size();
    const UINT IBByteSize = Geometry->IndexCount * sizeof(std::int32_t);

    Geometry->IndexBuffer = d3dUtil::CreateDefaultBuffer(m_D3dDevice.Get(), m_CommandList.Get(), Indices.data(), IBByteSize, Geometry->IndexUploadBuffer);

    Geometry->IndexBufferView.BufferLocation = Geometry->IndexBuffer->GetGPUVirtualAddress();
    Geometry->IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    Geometry->IndexBufferView.SizeInBytes = IBByteSize;

    m_Geometries[Geometry->Name] = std::move(Geometry);
}

void D3DSample::UpdateObjectCB(float deltaTime)
{
    for (size_t i = 0; i < m_RenderItems.size(); ++i)
    {
        auto RenderItem = m_RenderItems[i].get();
        XMMATRIX World = XMLoadFloat4x4(&RenderItem->World);

        ObjectConstant ObjectCB;
        XMStoreFloat4x4(&ObjectCB.World, XMMatrixTranspose(World));

        UINT ElementByteSize = (sizeof(ObjectConstant) + 255) & ~255;
        memcpy(&m_ObjectMappedData[i * ElementByteSize], &ObjectCB, sizeof(ObjectCB));
    }
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

    PassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
    PassCB.EyePosW = m_EyePos;
    PassCB.LightCount = 3;

    PassCB.Lights[0].LightType = 0;
    PassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
    PassCB.Lights[0].Strength = { 0.6f, 0.6f, 0.6f };

    PassCB.Lights[1].LightType = 0;
    PassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
    PassCB.Lights[1].Strength = { 0.3f, 0.3f, 0.3f };

    PassCB.Lights[2].LightType = 0;
    PassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
    PassCB.Lights[2].Strength = { 0.15f, 0.15f, 0.15f };

    memcpy(&m_PassMappedData[0], &PassCB, sizeof(PassCB));
}

void D3DSample::UpdateMaterialCB(float deltaTime)
{
    for (auto& Material : m_Materials)
    {
        MaterialInfo* MatInfo = Material.second.get();

        MatConstant MaterialCB;
        MaterialCB.Albedo = MatInfo->Albedo;
        MaterialCB.Fresnel = MatInfo->Fresnel;
        MaterialCB.Roughness = MatInfo->Roughness;

        UINT MaterialIndex = MatInfo->MatCBIndex;
        UINT MaterialByteSize = (sizeof(MatConstant) + 255) & ~255;
        memcpy(&m_MaterialMappedData[MaterialIndex * MaterialByteSize], &MaterialCB, sizeof(MaterialCB));
    }
}

void D3DSample::RenderGeometry()
{
    UINT ObjectCBByteSize = (sizeof(ObjectConstant) + 255) & ~255;
    UINT MaterialCBByteSize = (sizeof(MatConstant) + 255) & ~255;

    for (size_t i = 0; i < m_RenderItems.size(); ++i)
    {
        auto RenderItem = m_RenderItems[i].get();

        // 개별 오브젝트 상수(월드행렬) 버퍼 정보 업로드
        D3D12_GPU_VIRTUAL_ADDRESS ObjectCBAddress = m_ObjectCB->GetGPUVirtualAddress();
        ObjectCBAddress += i * ObjectCBByteSize;

        m_CommandList->SetGraphicsRootConstantBufferView(0, ObjectCBAddress);

        // 개별 오브젝트 재질 버퍼 정보 업로드
        D3D12_GPU_VIRTUAL_ADDRESS MaterialCBAddress = m_MaterialCB->GetGPUVirtualAddress();
        MaterialCBAddress += RenderItem->Material->MatCBIndex * MaterialCBByteSize;

        m_CommandList->SetGraphicsRootConstantBufferView(2, MaterialCBAddress);

        // 정점 인덱스 토폴로지 연결
        m_CommandList->IASetVertexBuffers(0, 1, &RenderItem->Geometry->VertexBufferView);
        m_CommandList->IASetIndexBuffer(&RenderItem->Geometry->IndexBufferView);
        m_CommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // 렌더링
        m_CommandList->DrawIndexedInstanced(RenderItem->Geometry->IndexCount, 1, 0, 0, 0);
    }
}
