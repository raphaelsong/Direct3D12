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
	void BuildGeometry();
	void BuildTextures();
	void BuildMaterials();
	void BuildRenderItems();
	void BuildConstantBuffer();
	void BuildDescriptorHeap();
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

public:
	void UpdateObjectCB(float deltaTime);
	void UpdatePassCB(float deltaTime);
	void UpdateMaterialCB(float deltaTime);

	void RenderGeometry();
	void RenderGeometry(const std::vector<RenderItem*>& RenderItems);

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

	// 셰이더 맵
	std::unordered_map<std::wstring, ComPtr<ID3DBlob>> m_Shaders;

	// 렌더링 파이프라인 맵
	std::unordered_map<RenderLayer, ComPtr<ID3D12PipelineState>> m_PipelineStates;

private:
	// 시야 행렬, 투영 행렬
	XMFLOAT4X4 m_View = MathHelper::Identity4x4();
	XMFLOAT4X4 m_Proj = MathHelper::Identity4x4();

	// 카메라 위치 
	XMFLOAT3 m_EyePos = { 0.0f, 0.0f, 0.0f };

	// 구면 좌표 제어 값
	float m_Theta = 1.5f * XM_PI;
	float m_Phi = XM_PIDIV4;
	float m_Radius = 5.0f;

	// 마우스 좌표
	POINT m_LastMousePos = { 0, 0 };
#pragma endregion

};