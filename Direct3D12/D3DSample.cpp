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
    BuildTextures();
    BuildMaterials();
    BuildRenderItems();
    BuildConstantBuffer();
    BuildDescriptorHeap();
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
    m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

    // 텍스처 서술자 렌더링 파이프라인에 묶기
    ID3D12DescriptorHeap* DescritprHeaps[] = { m_TextureDescriptorHeap.Get() };
    m_CommandList->SetDescriptorHeaps(_countof(DescritprHeaps), DescritprHeaps);

    // 공용 상수 버퍼(뷰, 투영 행렬)
    D3D12_GPU_VIRTUAL_ADDRESS PassCBAddress = m_PassCB->GetGPUVirtualAddress();
    m_CommandList->SetGraphicsRootConstantBufferView(1, PassCBAddress);

    // 스카이박스 텍스처 렌더링 파이프라인에 묶기
    CD3DX12_GPU_DESCRIPTOR_HANDLE SkyboxHeapAddress(m_TextureDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    SkyboxHeapAddress.Offset(m_SkyboxTexture->TextureHeapIndex, m_CbvSrvUavDescriptorSize);

    m_CommandList->SetGraphicsRootDescriptorTable(4, SkyboxHeapAddress);

    // 스카이박스 오브젝트 렌더링
    m_CommandList->SetPipelineState(m_PipelineStates[RenderLayer::Skybox].Get());
    RenderGeometry(m_RenderItemLayer[(int)RenderLayer::Skybox]);

    // 개별 오브젝트 렌더링
    m_CommandList->SetPipelineState(m_PipelineStates[RenderLayer::Opaque].Get());
    RenderGeometry(m_RenderItemLayer[(int)RenderLayer::Opaque]);

    // 바닥 오브젝트 렌더링
    m_CommandList->SetPipelineState(m_PipelineStates[RenderLayer::QuadPatch].Get());
    RenderGeometry(m_RenderItemLayer[(int)RenderLayer::QuadPatch]);

    // AlphaTested 오브젝트 렌더링
    m_CommandList->SetPipelineState(m_PipelineStates[RenderLayer::AlphaTested].Get());
    RenderGeometry(m_RenderItemLayer[(int)RenderLayer::AlphaTested]);

    // 나무 오브젝트 렌더링
    m_CommandList->SetPipelineState(m_PipelineStates[RenderLayer::Tree].Get());
    RenderGeometry(m_RenderItemLayer[(int)RenderLayer::Tree]);

    // Transparent 오브젝트 렌더링
    m_CommandList->SetPipelineState(m_PipelineStates[RenderLayer::Transparent].Get());
    RenderGeometry(m_RenderItemLayer[(int)RenderLayer::Transparent]);
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
    CreateQuadPatchGeometry();
    CreateTreeGeometry();
}

void D3DSample::BuildTextures()
{
    UINT TextureHeapIndex = 0;

    auto BrickTexture = std::make_unique<TextureInfo>();
    BrickTexture->Name = TEXT("BrickTexture");
    BrickTexture->FileName = TEXT("../Textures/bricks.dds");
    BrickTexture->TextureHeapIndex = TextureHeapIndex++;
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(
        m_D3dDevice.Get(),
        m_CommandList.Get(),
        BrickTexture->FileName.c_str(),
        BrickTexture->Resource,
        BrickTexture->UploadHeap));
    m_Textures[BrickTexture->Name] = std::move(BrickTexture);

    auto BrickNormal = std::make_unique<TextureInfo>();
    BrickNormal->Name = TEXT("BrickNormal");
    BrickNormal->FileName = TEXT("../Textures/bricks_nmap.dds");
    BrickNormal->TextureHeapIndex = TextureHeapIndex++;
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(
        m_D3dDevice.Get(),
        m_CommandList.Get(),
        BrickNormal->FileName.c_str(),
        BrickNormal->Resource,
        BrickNormal->UploadHeap));
    m_Textures[BrickNormal->Name] = std::move(BrickNormal);

    auto StoneTexture = std::make_unique<TextureInfo>();
    StoneTexture->Name = TEXT("StoneTexture");
    StoneTexture->FileName = TEXT("../Textures/stone.dds");
    StoneTexture->TextureHeapIndex = TextureHeapIndex++;
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(
        m_D3dDevice.Get(),
        m_CommandList.Get(),
        StoneTexture->FileName.c_str(),
        StoneTexture->Resource,
        StoneTexture->UploadHeap));
    m_Textures[StoneTexture->Name] = std::move(StoneTexture);

    auto TileTexture = std::make_unique<TextureInfo>();
    TileTexture->Name = TEXT("TileTexture");
    TileTexture->FileName = TEXT("../Textures/tile.dds");
    TileTexture->TextureHeapIndex = TextureHeapIndex++;
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(
        m_D3dDevice.Get(),
        m_CommandList.Get(),
        TileTexture->FileName.c_str(),
        TileTexture->Resource,
        TileTexture->UploadHeap));
    m_Textures[TileTexture->Name] = std::move(TileTexture);

    auto TileNormal = std::make_unique<TextureInfo>();
    TileNormal->Name = TEXT("TileNormal");
    TileNormal->FileName = TEXT("../Textures/tile_nmap.dds");
    TileNormal->TextureHeapIndex = TextureHeapIndex++;
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(
        m_D3dDevice.Get(),
        m_CommandList.Get(),
        TileNormal->FileName.c_str(),
        TileNormal->Resource,
        TileNormal->UploadHeap));
    m_Textures[TileNormal->Name] = std::move(TileNormal);

    auto FenceTexture = std::make_unique<TextureInfo>();
    FenceTexture->Name = TEXT("FenceTexture");
    FenceTexture->FileName = TEXT("../Textures/WireFence.dds");
    FenceTexture->TextureHeapIndex = TextureHeapIndex++;
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(
        m_D3dDevice.Get(),
        m_CommandList.Get(),
        FenceTexture->FileName.c_str(),
        FenceTexture->Resource,
        FenceTexture->UploadHeap));
    m_Textures[FenceTexture->Name] = std::move(FenceTexture);


    auto TreeTexture = std::make_unique<TextureInfo>();
    TreeTexture->Name = TEXT("TreeTexture");
    TreeTexture->FileName = TEXT("../Textures/treearray.dds");
    TreeTexture->TextureHeapIndex = TextureHeapIndex++;
    TreeTexture->TextureType = ETextureType::Texture2DArray;
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(
        m_D3dDevice.Get(),
        m_CommandList.Get(),
        TreeTexture->FileName.c_str(),
        TreeTexture->Resource,
        TreeTexture->UploadHeap));
    m_Textures[TreeTexture->Name] = std::move(TreeTexture);


    // 스카이박스는 모든 텍스처 배열 다음에 추가한 개별 텍스처
    // 스카이박스 텍스처 생성하기
    m_SkyboxTexture = std::make_unique<TextureInfo>();
    m_SkyboxTexture->Name = TEXT("Skybox");
    m_SkyboxTexture->FileName = TEXT("../Textures/grasscube1024.dds");
    m_SkyboxTexture->TextureHeapIndex = TextureHeapIndex++;
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(
        m_D3dDevice.Get(),
        m_CommandList.Get(),
        m_SkyboxTexture->FileName.c_str(),
        m_SkyboxTexture->Resource,
        m_SkyboxTexture->UploadHeap));
}

void D3DSample::BuildMaterials()
{
    UINT MatCBIndex = 0;

    auto Brick = std::make_unique<MaterialInfo>();
    Brick->Name = TEXT("Brick");
    Brick->MatCBIndex = MatCBIndex++;
    Brick->Texture_On = 1;
    Brick->TextureHeapIndex = m_Textures[TEXT("BrickTexture")]->TextureHeapIndex;
    Brick->Normal_On = 1;
    Brick->Albedo = XMFLOAT4(Colors::LightGray);
    Brick->Fresnel = XMFLOAT3(0.02f, 0.02f, 0.02f);
    Brick->Roughness = 0.1f;
    m_Materials[Brick->Name] = std::move(Brick);

    auto Stone = std::make_unique<MaterialInfo>();
    Stone->Name = TEXT("Stone");
    Stone->Texture_On = 1;
    Stone->TextureHeapIndex = m_Textures[TEXT("StoneTexture")]->TextureHeapIndex;
    Stone->MatCBIndex = MatCBIndex++;
    Stone->Albedo = XMFLOAT4(Colors::LightSteelBlue);
    Stone->Fresnel = XMFLOAT3(0.05f, 0.05f, 0.05f);
    Stone->Roughness = 0.3f;
    m_Materials[Stone->Name] = std::move(Stone);

    auto Tile = std::make_unique<MaterialInfo>();
    Tile->Name = TEXT("Tile");
    Tile->Texture_On = 1;
    Tile->TextureHeapIndex = m_Textures[TEXT("TileTexture")]->TextureHeapIndex;
    Tile->Normal_On = 1;
    Tile->MatCBIndex = MatCBIndex++;
    Tile->Albedo = XMFLOAT4(Colors::LightGray);
    Tile->Fresnel = XMFLOAT3(0.02f, 0.02f, 0.02f);
    Tile->Roughness = 0.2f;
    m_Materials[Tile->Name] = std::move(Tile);

    auto Fence = std::make_unique<MaterialInfo>();
    Fence->Name = TEXT("Fence");
    Fence->Texture_On = 1;
    Fence->TextureHeapIndex = m_Textures[TEXT("FenceTexture")]->TextureHeapIndex;
    Fence->MatCBIndex = MatCBIndex++;
    Fence->Albedo = XMFLOAT4(Colors::White);
    Fence->Fresnel = XMFLOAT3(0.1f, 0.1f, 0.1f);
    Fence->Roughness = 0.25f;
    m_Materials[Fence->Name] = std::move(Fence);

    auto Skull = std::make_unique<MaterialInfo>();
    Skull->Name = TEXT("Skull");
    Skull->MatCBIndex = MatCBIndex++;
    Skull->Albedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
    Skull->Fresnel = XMFLOAT3(0.05f, 0.05f, 0.05f);
    Skull->Roughness = 0.3f;
    m_Materials[Skull->Name] = std::move(Skull);

    // 스카이박스 재질 추가
    auto Skybox = std::make_unique<MaterialInfo>();
    Skybox->Name = TEXT("Skybox");
    Skybox->MatCBIndex = MatCBIndex++;
    Skybox->Albedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    Skybox->Fresnel = XMFLOAT3(0.1f, 0.1f, 0.1f);
    Skybox->Roughness = 1.0f;
    m_Materials[Skybox->Name] = std::move(Skybox);

    // 거울 반사 재질
    auto Mirror = std::make_unique<MaterialInfo>();
    Mirror->Name = TEXT("Mirror");
    Mirror->MatCBIndex = MatCBIndex++;
    Mirror->Albedo = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
    Mirror->Fresnel = XMFLOAT3(0.98f, 0.97f, 0.95f);
    Mirror->Roughness = 0.1f;
    m_Materials[Mirror->Name] = std::move(Mirror);

    // 나무 재질
    auto Tree = std::make_unique<MaterialInfo>();
    Tree->Name = TEXT("Tree");
    Tree->MatCBIndex = MatCBIndex++;
    Tree->Texture_On = 1;
    Tree->TextureHeapIndex = m_Textures[TEXT("TreeTexture")]->TextureHeapIndex;
    Tree->Albedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    Tree->Fresnel = XMFLOAT3(0.1f, 0.1f, 0.1f);
    Tree->Roughness = 0.1f;
    m_Materials[Tree->Name] = std::move(Tree);
}

void D3DSample::BuildRenderItems()
{
    UINT ObjectCBIndex = 0;

    //auto GridItem = std::make_unique<RenderItem>();
    //GridItem->ObjectCBIndex = ObjectCBIndex++;
    //GridItem->World = MathHelper::Identity4x4();
    //XMStoreFloat4x4(&GridItem->TextureTransform, XMMatrixScaling(8.0f, 8.0f, 1.0f));
    //GridItem->Geometry = m_Geometries[TEXT("Grid")].get();
    //GridItem->Material = m_Materials[TEXT("Tile")].get();
    //m_RenderItemLayer[(int)RenderLayer::Opaque].push_back(GridItem.get());
    //m_RenderItems.push_back(std::move(GridItem));

    auto BoxItem = std::make_unique<RenderItem>();
    BoxItem->ObjectCBIndex = ObjectCBIndex++;
    XMStoreFloat4x4(&BoxItem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f));
    BoxItem->Geometry = m_Geometries[TEXT("Box")].get();
    BoxItem->Material = m_Materials[TEXT("Fence")].get();
    m_RenderItemLayer[(int)RenderLayer::AlphaTested].push_back(BoxItem.get());
    m_RenderItems.push_back(std::move(BoxItem));

    auto SkullItem = std::make_unique<RenderItem>();
    SkullItem->ObjectCBIndex = ObjectCBIndex++;
    XMStoreFloat4x4(&SkullItem->World, XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(0.0f, 1.0f, 0.0f));
    SkullItem->Geometry = m_Geometries[TEXT("Skull")].get();
    SkullItem->Material = m_Materials[TEXT("Skull")].get();
    m_RenderItemLayer[(int)RenderLayer::Transparent].push_back(SkullItem.get());
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

        LeftCylinderItem->ObjectCBIndex = ObjectCBIndex++;
        XMStoreFloat4x4(&LeftCylinderItem->World, LeftCylinderWorld);
        LeftCylinderItem->Geometry = m_Geometries[TEXT("Cylinder")].get();
        LeftCylinderItem->Material = m_Materials[TEXT("Brick")].get();
        m_RenderItemLayer[(int)RenderLayer::Opaque].push_back(LeftCylinderItem.get());
        m_RenderItems.push_back(std::move(LeftCylinderItem));

        RightCylinderItem->ObjectCBIndex = ObjectCBIndex++;
        XMStoreFloat4x4(&RightCylinderItem->World, RightCylinderWorld);
        RightCylinderItem->Geometry = m_Geometries[TEXT("Cylinder")].get();
        RightCylinderItem->Material = m_Materials[TEXT("Brick")].get();
        m_RenderItemLayer[(int)RenderLayer::Opaque].push_back(RightCylinderItem.get());
        m_RenderItems.push_back(std::move(RightCylinderItem));

        LeftSphereItem->ObjectCBIndex = ObjectCBIndex++;
        XMStoreFloat4x4(&LeftSphereItem->World, LeftSphereWorld);
        LeftSphereItem->Geometry = m_Geometries[TEXT("Sphere")].get();
        LeftSphereItem->Material = m_Materials[TEXT("Mirror")].get();
        m_RenderItemLayer[(int)RenderLayer::Opaque].push_back(LeftSphereItem.get());
        m_RenderItems.push_back(std::move(LeftSphereItem));

        RightSphereItem->ObjectCBIndex = ObjectCBIndex++;
        XMStoreFloat4x4(&RightSphereItem->World, RightSphereWorld);
        RightSphereItem->Geometry = m_Geometries[TEXT("Sphere")].get();
        RightSphereItem->Material = m_Materials[TEXT("Mirror")].get();
        m_RenderItemLayer[(int)RenderLayer::Opaque].push_back(RightSphereItem.get());
        m_RenderItems.push_back(std::move(RightSphereItem));
    }

    // 스카이박스 오브젝트 생성
    auto SkyboxItem = std::make_unique<RenderItem>();
    SkyboxItem->ObjectCBIndex = ObjectCBIndex++;
    XMStoreFloat4x4(&SkyboxItem->World, XMMatrixScaling(5000.0f, 5000.0f, 5000.0f));
    SkyboxItem->Geometry = m_Geometries[TEXT("Box")].get();
    SkyboxItem->Material = m_Materials[TEXT("Skybox")].get();
    m_RenderItemLayer[(int)RenderLayer::Skybox].push_back(SkyboxItem.get());
    m_RenderItems.push_back(std::move(SkyboxItem));

    // 바닥 오브젝트 생성
    auto QuadPathItem = std::make_unique<RenderItem>();
    QuadPathItem->ObjectCBIndex = ObjectCBIndex++;
    QuadPathItem->World = MathHelper::Identity4x4();
    QuadPathItem->Geometry = m_Geometries[TEXT("QuadPatch")].get();
    QuadPathItem->Material = m_Materials[TEXT("Tile")].get();
    QuadPathItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST;
    m_RenderItemLayer[(int)RenderLayer::QuadPatch].push_back(QuadPathItem.get());
    m_RenderItems.push_back(std::move(QuadPathItem));

    // 나무 오브젝트 생성
    auto TreeItem = std::make_unique<RenderItem>();
    TreeItem->ObjectCBIndex = ObjectCBIndex++;
    TreeItem->World = MathHelper::Identity4x4();
    TreeItem->Geometry = m_Geometries[TEXT("Tree")].get();
    TreeItem->Material = m_Materials[TEXT("Tree")].get();
    TreeItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
    m_RenderItemLayer[(int)RenderLayer::Tree].push_back(TreeItem.get());
    m_RenderItems.push_back(std::move(TreeItem));
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

void D3DSample::BuildDescriptorHeap()
{
    // SRV Heap
    // 텍스처 맵 갯수 + 스카이박스 텍스처 갯수
    D3D12_DESCRIPTOR_HEAP_DESC TextureHeapDesc = {};
    TextureHeapDesc.NumDescriptors = m_Textures.size() + 1;
    TextureHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    TextureHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(m_D3dDevice->CreateDescriptorHeap(&TextureHeapDesc, IID_PPV_ARGS(&m_TextureDescriptorHeap)));

    for (auto& Texture : m_Textures)
    {
        TextureInfo* TexInfo = Texture.second.get();

        CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(m_TextureDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
        hDescriptor.Offset(TexInfo->TextureHeapIndex, m_CbvSrvUavDescriptorSize);

        auto TexResource = TexInfo->Resource;
        D3D12_SHADER_RESOURCE_VIEW_DESC TexDesc = {};
        TexDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        TexDesc.Format = TexResource->GetDesc().Format;

        switch (TexInfo->TextureType)
        {
        case ETextureType::Texture2D:
            TexDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            TexDesc.Texture2D.MostDetailedMip = 0;
            TexDesc.Texture2D.MipLevels = TexResource->GetDesc().MipLevels;
            TexDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            break;
        case ETextureType::Texture2DArray:
            TexDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            TexDesc.Texture2DArray.MostDetailedMip = 0;
            TexDesc.Texture2DArray.MipLevels = -1;
            TexDesc.Texture2DArray.FirstArraySlice = 0;
            TexDesc.Texture2DArray.ArraySize = TexResource->GetDesc().DepthOrArraySize;
            break;
        }        

        m_D3dDevice->CreateShaderResourceView(TexResource.Get(), &TexDesc, hDescriptor);
    }

    // 스카이박스 텍스처 힙 생성하기
    CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(m_TextureDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    hDescriptor.Offset(m_SkyboxTexture->TextureHeapIndex, m_CbvSrvUavDescriptorSize);

    auto SkyboxResource = m_SkyboxTexture->Resource;
    D3D12_SHADER_RESOURCE_VIEW_DESC SkyboxDesc = {};
    SkyboxDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    SkyboxDesc.Format = SkyboxResource->GetDesc().Format;
    SkyboxDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    SkyboxDesc.TextureCube.MostDetailedMip = 0;
    SkyboxDesc.TextureCube.MipLevels = SkyboxResource->GetDesc().MipLevels;
    SkyboxDesc.TextureCube.ResourceMinLODClamp = 0.0f;

    m_D3dDevice->CreateShaderResourceView(SkyboxResource.Get(), &SkyboxDesc, hDescriptor);
}

void D3DSample::BuildShader()
{
    const D3D_SHADER_MACRO FogDefines[] =
    {
        "FOG", "1",
        NULL, NULL
    };
    const D3D_SHADER_MACRO AlphaTestedDefines[] =
    {
        "FOG", "1",
        "ALPHA_TESTED", "1",
        NULL, NULL
    };

    m_Shaders[TEXT("VS")] = d3dUtil::CompileShader(TEXT("../Shader/Default.hlsl"), nullptr, "VS", "vs_5_0");
    m_Shaders[TEXT("PS")] = d3dUtil::CompileShader(TEXT("../Shader/Default.hlsl"), FogDefines, "PS", "ps_5_0");
    m_Shaders[TEXT("AlphaTestedPS")] = d3dUtil::CompileShader(TEXT("../Shader/Default.hlsl"), AlphaTestedDefines, "PS", "ps_5_0");

    m_Shaders[TEXT("SkyboxVS")] = d3dUtil::CompileShader(TEXT("../Shader/Skybox.hlsl"), nullptr, "VS", "vs_5_0");
    m_Shaders[TEXT("SkyboxPS")] = d3dUtil::CompileShader(TEXT("../Shader/Skybox.hlsl"), nullptr, "PS", "ps_5_0");

    m_Shaders[TEXT("TessVS")] = d3dUtil::CompileShader(TEXT("../Shader/QuadPatch.hlsl"), nullptr, "VS", "vs_5_0");
    m_Shaders[TEXT("TessHS")] = d3dUtil::CompileShader(TEXT("../Shader/QuadPatch.hlsl"), nullptr, "HS", "hs_5_0");
    m_Shaders[TEXT("TessDS")] = d3dUtil::CompileShader(TEXT("../Shader/QuadPatch.hlsl"), nullptr, "DS", "ds_5_0");
    m_Shaders[TEXT("TessPS")] = d3dUtil::CompileShader(TEXT("../Shader/QuadPatch.hlsl"), nullptr, "PS", "ps_5_0");

    m_Shaders[TEXT("TreeVS")] = d3dUtil::CompileShader(TEXT("../Shader/Tree.hlsl"), nullptr, "VS", "vs_5_0");
    m_Shaders[TEXT("TreeGS")] = d3dUtil::CompileShader(TEXT("../Shader/Tree.hlsl"), nullptr, "GS", "gs_5_0");
    m_Shaders[TEXT("TreePS")] = d3dUtil::CompileShader(TEXT("../Shader/Tree.hlsl"), AlphaTestedDefines, "PS", "ps_5_0");
}

void D3DSample::BuildRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE TextureTable[2];
    TextureTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0 : Diffuse Texture
    TextureTable[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); // t1 : Normal Texture

    CD3DX12_DESCRIPTOR_RANGE SkyboxTable[1];
    SkyboxTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2); // t2 : Skybox Texture

    CD3DX12_ROOT_PARAMETER Params[5];
    Params[0].InitAsConstantBufferView(0); // 0번 -> b0 : CBV m_ObjectCB
    Params[1].InitAsConstantBufferView(1); // 1번 -> b1 : CBV m_PassCB
    Params[2].InitAsConstantBufferView(2); // 2번 -> b2 : CBV m_MaterialCB
    Params[3].InitAsDescriptorTable(_countof(TextureTable), TextureTable); // 3번 -> TexTable
    Params[4].InitAsDescriptorTable(_countof(SkyboxTable), SkyboxTable); // 4번 -> Skybox Table

    D3D12_STATIC_SAMPLER_DESC SamplerDesc = CD3DX12_STATIC_SAMPLER_DESC(0); // s0 : Sampler

    D3D12_ROOT_SIGNATURE_DESC RSDesc = CD3DX12_ROOT_SIGNATURE_DESC(_countof(Params), Params, 1, &SamplerDesc);
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
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    m_QuadPatchInputLayout =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    m_TreeInputLayout =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
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
        reinterpret_cast<BYTE*>(m_Shaders[TEXT("VS")]->GetBufferPointer()),
        m_Shaders[TEXT("VS")]->GetBufferSize()
    };
    ObjectDesc.PS =
    {
        reinterpret_cast<BYTE*>(m_Shaders[TEXT("PS")]->GetBufferPointer()),
        m_Shaders[TEXT("PS")]->GetBufferSize()
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

    ThrowIfFailed(m_D3dDevice->CreateGraphicsPipelineState(&ObjectDesc, IID_PPV_ARGS(&m_PipelineStates[RenderLayer::Opaque])));

    // PSO : AlphaTested Objects
    D3D12_GRAPHICS_PIPELINE_STATE_DESC AlphaTestedDesc = ObjectDesc;
    AlphaTestedDesc.PS = 
    {
        reinterpret_cast<BYTE*>(m_Shaders[TEXT("AlphaTestedPS")]->GetBufferPointer()),
        m_Shaders[TEXT("AlphaTestedPS")]->GetBufferSize()
    };
    AlphaTestedDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    ThrowIfFailed(m_D3dDevice->CreateGraphicsPipelineState(&AlphaTestedDesc, IID_PPV_ARGS(&m_PipelineStates[RenderLayer::AlphaTested])));

    // PSO : Transparent Objects
    D3D12_GRAPHICS_PIPELINE_STATE_DESC TransparentDesc = ObjectDesc;
    D3D12_RENDER_TARGET_BLEND_DESC TransparentBlendDesc;
    TransparentBlendDesc.BlendEnable = true;
    TransparentBlendDesc.LogicOpEnable = false;
    TransparentBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    TransparentBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    TransparentBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
    TransparentBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
    TransparentBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
    TransparentBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    TransparentBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
    TransparentBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    TransparentDesc.BlendState.RenderTarget[0] = TransparentBlendDesc;

    ThrowIfFailed(m_D3dDevice->CreateGraphicsPipelineState(&TransparentDesc, IID_PPV_ARGS(&m_PipelineStates[RenderLayer::Transparent])));


    // PSO : Skybox Objects
    D3D12_GRAPHICS_PIPELINE_STATE_DESC SkyboxDesc = ObjectDesc;
    // 컬링 제거
    SkyboxDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    // 깊이스텐실 버퍼 컬링 제거
    SkyboxDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    SkyboxDesc.pRootSignature = m_RootSignature.Get();

    SkyboxDesc.VS =
    {
        reinterpret_cast<BYTE*>(m_Shaders[TEXT("SkyboxVS")]->GetBufferPointer()),
        m_Shaders[TEXT("SkyboxVS")]->GetBufferSize()
    };
    SkyboxDesc.PS =
    {
        reinterpret_cast<BYTE*>(m_Shaders[TEXT("SkyboxPS")]->GetBufferPointer()),
        m_Shaders[TEXT("SkyboxPS")]->GetBufferSize()
    };

    ThrowIfFailed(m_D3dDevice->CreateGraphicsPipelineState(&SkyboxDesc, IID_PPV_ARGS(&m_PipelineStates[RenderLayer::Skybox])));

    // PSO : QuadPatch Objects
    D3D12_GRAPHICS_PIPELINE_STATE_DESC QuadPatchDesc = ObjectDesc;
    QuadPatchDesc.VS =
    {
        reinterpret_cast<BYTE*>(m_Shaders[TEXT("TessVS")]->GetBufferPointer()),
        m_Shaders[TEXT("TessVS")]->GetBufferSize()
    };
    QuadPatchDesc.HS =
    {
        reinterpret_cast<BYTE*>(m_Shaders[TEXT("TessHS")]->GetBufferPointer()),
        m_Shaders[TEXT("TessHS")]->GetBufferSize()
    };
    QuadPatchDesc.DS =
    {
        reinterpret_cast<BYTE*>(m_Shaders[TEXT("TessDS")]->GetBufferPointer()),
        m_Shaders[TEXT("TessDS")]->GetBufferSize()
    };
    QuadPatchDesc.PS =
    {
        reinterpret_cast<BYTE*>(m_Shaders[TEXT("TessPS")]->GetBufferPointer()),
        m_Shaders[TEXT("TessPS")]->GetBufferSize()
    };

    QuadPatchDesc.InputLayout = { m_QuadPatchInputLayout.data(), (UINT)m_QuadPatchInputLayout.size() };
    QuadPatchDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    QuadPatchDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;

    ThrowIfFailed(m_D3dDevice->CreateGraphicsPipelineState(&QuadPatchDesc, IID_PPV_ARGS(&m_PipelineStates[RenderLayer::QuadPatch])));

    // PSO : Tree Objects
    D3D12_GRAPHICS_PIPELINE_STATE_DESC TreeDesc = ObjectDesc;
    TreeDesc.VS =
    {
        reinterpret_cast<BYTE*>(m_Shaders[TEXT("TreeVS")]->GetBufferPointer()),
        m_Shaders[TEXT("TreeVS")]->GetBufferSize()
    };
    TreeDesc.GS =
    {
        reinterpret_cast<BYTE*>(m_Shaders[TEXT("TreeGS")]->GetBufferPointer()),
        m_Shaders[TEXT("TreeGS")]->GetBufferSize()
    };
    TreeDesc.PS =
    {
        reinterpret_cast<BYTE*>(m_Shaders[TEXT("TreePS")]->GetBufferPointer()),
        m_Shaders[TEXT("TreePS")]->GetBufferSize()
    };

    TreeDesc.InputLayout = { m_TreeInputLayout.data(), (UINT)m_TreeInputLayout.size() };
    TreeDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    TreeDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    ThrowIfFailed(m_D3dDevice->CreateGraphicsPipelineState(&TreeDesc, IID_PPV_ARGS(&m_PipelineStates[RenderLayer::Tree])));
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
        Vertices[i].Uv = Mesh.Vertices[i].TexC;
        Vertices[i].TangentU = Mesh.Vertices[i].TangentU;
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
        Vertices[i].Uv = Mesh.Vertices[i].TexC;
        Vertices[i].TangentU = Mesh.Vertices[i].TangentU;
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
        Vertices[i].Uv = Mesh.Vertices[i].TexC;
        Vertices[i].TangentU = Mesh.Vertices[i].TangentU;
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
        Vertices[i].Uv = Mesh.Vertices[i].TexC;
        Vertices[i].TangentU = Mesh.Vertices[i].TangentU;
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

void D3DSample::CreateQuadPatchGeometry()
{
    std::array<QuadPatchVertex, 4> Vertices =
    {
        XMFLOAT3(-50.0f, 0.0f, +50.0f),
        XMFLOAT3(+50.0f, 0.0f, +50.0f),
        XMFLOAT3(-50.0f, 0.0f, -50.0f),
        XMFLOAT3(+50.0f, 0.0f, -50.0f),
    };

    std::array<std::int32_t, 4> Indices = { 0, 1, 2, 3 };

    auto Geometry = std::make_unique<GeometryInfo>();
    Geometry->Name = TEXT("QuadPatch");

    // 정점 버퍼
    Geometry->VertexCount = (UINT)Vertices.size();
    const UINT VBByteSize = Geometry->VertexCount * sizeof(QuadPatchVertex);

    Geometry->VertexBuffer = d3dUtil::CreateDefaultBuffer(m_D3dDevice.Get(), m_CommandList.Get(), Vertices.data(), VBByteSize, Geometry->VertexUploadBuffer);

    Geometry->VertexBufferView.BufferLocation = Geometry->VertexBuffer->GetGPUVirtualAddress();
    Geometry->VertexBufferView.StrideInBytes = sizeof(QuadPatchVertex);
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

void D3DSample::CreateTreeGeometry()
{
    static const int TreeCount = 32;
    std::array<TreeVertex, TreeCount> Vertices;
    for (UINT i = 0; i < TreeCount; ++i)
    {
        float x = MathHelper::RandF(-45.0f, 45.0f);
        float z = MathHelper::RandF(-45.0f, 50.0f);
        float y = 4.0f;

        Vertices[i].Pos = XMFLOAT3(x, y, z);
        Vertices[i].Size = XMFLOAT2(10.0f, 10.0f);
    }

    std::array<std::int32_t, TreeCount> Indices;
    for (UINT i = 0; i < TreeCount; ++i)
    {
        Indices[i] = i;
    }

    auto Geometry = std::make_unique<GeometryInfo>();
    Geometry->Name = TEXT("Tree");

    // 정점 버퍼
    Geometry->VertexCount = (UINT)Vertices.size();
    const UINT VBByteSize = Geometry->VertexCount * sizeof(TreeVertex);

    Geometry->VertexBuffer = d3dUtil::CreateDefaultBuffer(m_D3dDevice.Get(), m_CommandList.Get(), Vertices.data(), VBByteSize, Geometry->VertexUploadBuffer);

    Geometry->VertexBufferView.BufferLocation = Geometry->VertexBuffer->GetGPUVirtualAddress();
    Geometry->VertexBufferView.StrideInBytes = sizeof(TreeVertex);
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
        XMMATRIX TextureTransform = XMLoadFloat4x4(&RenderItem->TextureTransform);

        ObjectConstant ObjectCB;
        XMStoreFloat4x4(&ObjectCB.World, XMMatrixTranspose(World));
        XMStoreFloat4x4(&ObjectCB.TextureTransform, XMMatrixTranspose(TextureTransform));

        UINT ElementIndex = RenderItem->ObjectCBIndex;
        UINT ElementByteSize = (sizeof(ObjectConstant) + 255) & ~255;
        memcpy(&m_ObjectMappedData[ElementIndex * ElementByteSize], &ObjectCB, sizeof(ObjectCB));
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
        MaterialCB.Texture_On = MatInfo->Texture_On;
        MaterialCB.Normal_On = MatInfo->Normal_On;

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
        ObjectCBAddress += RenderItem->ObjectCBIndex * ObjectCBByteSize;

        m_CommandList->SetGraphicsRootConstantBufferView(0, ObjectCBAddress);

        // 개별 오브젝트 재질 버퍼 정보 업로드
        D3D12_GPU_VIRTUAL_ADDRESS MaterialCBAddress = m_MaterialCB->GetGPUVirtualAddress();
        MaterialCBAddress += RenderItem->Material->MatCBIndex * MaterialCBByteSize;

        m_CommandList->SetGraphicsRootConstantBufferView(2, MaterialCBAddress);

        // 텍스처 버퍼 서술자 업로드
        CD3DX12_GPU_DESCRIPTOR_HANDLE TextureHeapAddress(m_TextureDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
        TextureHeapAddress.Offset(RenderItem->Material->TextureHeapIndex, m_CbvSrvUavDescriptorSize);

        if (RenderItem->Material->Texture_On)
        {
            m_CommandList->SetGraphicsRootDescriptorTable(3, TextureHeapAddress);
        }

        // 정점 인덱스 토폴로지 연결
        m_CommandList->IASetVertexBuffers(0, 1, &RenderItem->Geometry->VertexBufferView);
        m_CommandList->IASetIndexBuffer(&RenderItem->Geometry->IndexBufferView);
        m_CommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // 렌더링
        m_CommandList->DrawIndexedInstanced(RenderItem->Geometry->IndexCount, 1, 0, 0, 0);
    }
}

void D3DSample::RenderGeometry(const std::vector<RenderItem*>& RenderItems)
{
    UINT ObjectCBByteSize = (sizeof(ObjectConstant) + 255) & ~255;
    UINT MaterialCBByteSize = (sizeof(MatConstant) + 255) & ~255;

    for (size_t i = 0; i < RenderItems.size(); ++i)
    {
        auto RenderItem = RenderItems[i];

        // 개별 오브젝트 상수(월드행렬) 버퍼 정보 업로드
        D3D12_GPU_VIRTUAL_ADDRESS ObjectCBAddress = m_ObjectCB->GetGPUVirtualAddress();
        ObjectCBAddress += RenderItem->ObjectCBIndex * ObjectCBByteSize;

        m_CommandList->SetGraphicsRootConstantBufferView(0, ObjectCBAddress);

        // 개별 오브젝트 재질 버퍼 정보 업로드
        D3D12_GPU_VIRTUAL_ADDRESS MaterialCBAddress = m_MaterialCB->GetGPUVirtualAddress();
        MaterialCBAddress += RenderItem->Material->MatCBIndex * MaterialCBByteSize;

        m_CommandList->SetGraphicsRootConstantBufferView(2, MaterialCBAddress);

        // 텍스처 버퍼 서술자 업로드
        CD3DX12_GPU_DESCRIPTOR_HANDLE TextureHeapAddress(m_TextureDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
        TextureHeapAddress.Offset(RenderItem->Material->TextureHeapIndex, m_CbvSrvUavDescriptorSize);

        if (RenderItem->Material->Texture_On)
        {
            m_CommandList->SetGraphicsRootDescriptorTable(3, TextureHeapAddress);
        }

        // 정점 인덱스 토폴로지 연결
        m_CommandList->IASetVertexBuffers(0, 1, &RenderItem->Geometry->VertexBufferView);
        m_CommandList->IASetIndexBuffer(&RenderItem->Geometry->IndexBufferView);
        m_CommandList->IASetPrimitiveTopology(RenderItem->PrimitiveType);

        // 렌더링
        m_CommandList->DrawIndexedInstanced(RenderItem->Geometry->IndexCount, 1, 0, 0, 0);
    }
}
