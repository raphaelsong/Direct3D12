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
	void BuildConstantBuffer();
	void BuildShader();
	void BuildRootSignature();
	void BuildInputLayout();
	void BuildPipelineState();

public:
	void UpdateObjectCB(float deltaTime);
	void UpdatePassCB(float deltaTime);

	void RenderGeometry();

private:
	std::unique_ptr<GeometryInfo> m_pGeometry;

	// 오브젝트 상수 버퍼
	ComPtr<ID3D12Resource>	m_ObjectCB = nullptr;
	BYTE* m_ObjectMappedData = nullptr;
	UINT m_ObjectByteSize = 0;

	// 공용 상수 버퍼
	ComPtr<ID3D12Resource>	m_PassCB = nullptr;
	BYTE* m_PassMappedData = nullptr;
	UINT m_PassByteSize = 0;

	// 정점 셰이더, 픽셀 셰이더 변수
	ComPtr<ID3DBlob> m_VSByteCode = nullptr;
	ComPtr<ID3DBlob> m_PSByteCode = nullptr;

	// 루트 시그니처
	ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;

	// 입력 배치
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputLayout;

	// 렌더링 파이프라인 상태 
	ComPtr<ID3D12PipelineState> m_PipelineState = nullptr;

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