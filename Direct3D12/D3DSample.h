#pragma once

#include "D3DRenderer.h"

class D3DSample : public D3DRenderer
{
public:
	D3DSample(HINSTANCE hInstance);
	~D3DSample();

	virtual bool Initialize()override;
	virtual void OnResize()override;

private:
	virtual void Update(float deltaTime)override;
	virtual void LateUpdate(float deltaTime)override;
	virtual void BeginRender()override;
	virtual void Render()override;
	virtual void EndRender()override;

private:
	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

#pragma region Rendering Geometry

public:
	void BuildShadowMap();
	void BuildGeometry();
	void BuildTextures();
	void BuildMaterials();
	void BuildRenderItems();
	void BuildConstantBuffer();
	void BuildDescriptorHeap();
	void BuildDsvDescriptorHeap();
	void BuildShader();
	void BuildRootSignature();
	void BuildInputLayout();
	void BuildPipelineState();

private:
	void CreateBoxGeometry();
	void CreateGridGeometry();
	void CreateSphereGeometry();
	void CreateCylinderGeometry();
	void CreateSkullGeometry();
	void CreateQuadPatchGeometry();
	void CreateTreeGeometry();
	void CreateQuadGeometry();
	void CreateSkinnedModel();

public:
	void UpdateObjectCB(float deltaTime);
	void UpdatePassCB(float deltaTime);
	void UpdateMaterialCB(float deltaTime);
	void UpdateShadowMapPassCB(float deltaTime);
	void UpdateSkinnedCB(float deltaTime);

	void UpdateCamera(float deltaTime);
	void UpdateLight(float deltaTime);

	void RenderGeometry();
	void RenderGeometry(const std::vector<RenderItem*>& RenderItems);

	void RenderSceneToShadowMap();

private:
	// 기하도형 맵
	std::unordered_map<std::wstring, std::unique_ptr<GeometryInfo>> m_Geometries;

	// 텍스처 맵
	std::unordered_map<std::wstring, std::unique_ptr<TextureInfo>> m_Textures;

	// 재질 맵
	std::unordered_map<std::wstring, std::unique_ptr<MaterialInfo>> m_Materials;

	// 렌더링 할 오브젝트 리스트
	std::vector<std::unique_ptr<RenderItem>> m_RenderItems;

	// 렌더링 아이템을 레이어로 분리
	std::vector<RenderItem*> m_RenderItemLayer[(int)RenderLayer::Count];

	// 스카이박스 텍스처 정보
	std::unique_ptr<TextureInfo> m_SkyboxTexture;

	// 오브젝트 상수 버퍼
	ComPtr<ID3D12Resource>	m_ObjectCB = nullptr;
	BYTE* m_ObjectMappedData = nullptr;
	UINT m_ObjectByteSize = 0;

	// 공용 상수 버퍼
	ComPtr<ID3D12Resource>	m_PassCB = nullptr;
	BYTE* m_PassMappedData = nullptr;
	UINT m_PassByteSize = 0;

	// 재질 상수 버퍼
	ComPtr<ID3D12Resource>	m_MaterialCB = nullptr;
	BYTE* m_MaterialMappedData = nullptr;
	UINT m_MaterialByteSize = 0;

	// 스킨드 오브젝트 상수 버퍼
	ComPtr<ID3D12Resource>	m_SkinnedCB = nullptr;
	BYTE* m_SkinnedMappedData = nullptr;
	UINT m_SkinnedByteSize = 0;

	// 텍스처 서술자 힙
	ComPtr<ID3D12DescriptorHeap> m_TextureDescriptorHeap = nullptr;

	// 루트 시그니처
	ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;

	// 입력 배치
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputLayout;

	// 바닥 입력 배치
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_QuadPatchInputLayout;

	// 나무 입력 배치
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_TreeInputLayout;

	// 스킨드 오브젝트 입력 배치
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_SkinnedInputLayout;

	// 셰이더 맵
	std::unordered_map<std::wstring, ComPtr<ID3DBlob>> m_Shaders;

	// 렌더링 파이프라인 맵
	std::unordered_map<RenderLayer, ComPtr<ID3D12PipelineState>> m_PipelineStates;

// 쉐도우 맵 정보
private:
	// ShadowMap 리소스 & 정보
	ComPtr<ID3D12Resource>			m_ShadowMapResource = nullptr;
	CD3DX12_CPU_DESCRIPTOR_HANDLE	m_hShadowMapDsv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE	m_hShadowMapSrv;

	// ShadowMap 텍스처 인덱스
	UINT							m_ShadowMapHeapIndex = 0;

	D3D12_VIEWPORT	m_ShadowMapViewport;
	D3D12_RECT		m_ShadowMapScissorRect;

	UINT			m_ShadowMapWidth = 2048;
	UINT			m_ShadowMapHeight = 2048;

// Skinned Model 정보 
private:
	// Skinned Model Load 정보
	std::string m_SkinnedModelFileName = "../3DModels/soldier.m3d";
	std::vector<M3DLoader::Subset> m_SkinnedSubsets;
	std::vector<M3DLoader::M3dMaterial> m_SkinnedMaterials;
	SkinnedData m_SkinnedInfo;

	std::unique_ptr<SkinnedModelAnimation> m_SkinnedModelAnimation;

private:
	// 카메라 클래스
	Camera m_Camera;

	// 라이트 정보
	XMFLOAT3 m_BaseLightDirection = XMFLOAT3(0.57735f, -0.57735f, 0.57735f);
	XMFLOAT3 m_RotatedLightDirection;
	float m_LightRotationAngle = 0.0f;
	float m_LightRotationSpeed = 0.1f;

	// 라이트 행렬
	float m_LightNearZ = 0.0f;
	float m_LightFarZ = 0.0f;
	XMFLOAT3 m_LightPosW;
	XMFLOAT4X4 m_LightView = MathHelper::Identity4x4();
	XMFLOAT4X4 m_LightProj = MathHelper::Identity4x4();
	XMFLOAT4X4 m_ShadowTransform = MathHelper::Identity4x4();

	// 경계 구
	BoundingSphere m_SceneBounds;

	// 구면 좌표 제어 값
	float m_Theta = 1.5f * XM_PI;
	float m_Phi = XM_PIDIV4;
	float m_Radius = 5.0f;

	// 마우스 좌표
	POINT m_LastMousePos = { 0, 0 };
#pragma endregion

};