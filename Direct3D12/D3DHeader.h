#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <WindowsX.h>
#include <DirectXColors.h>

#include "../Common/d3dUtil.h"
#include "../Common/Camera.h"
#include "../Common/GameTimer.h"
#include "../Common/MathHelper.h"
#include "../Common/UploadBuffer.h"
#include "../Common/DDSTextureLoader.h"
#include "../Common/GeometryGenerator.h"

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;
using namespace std;
using namespace DirectX;

const UINT SWAP_CHAIN_BUFFER_COUNT = 2;

enum class RenderLayer : int
{
	Opaque = 0,
	Transparent,
	AlphaTested,
	Skybox,
	QuadPatch,
	Tree,
	ShadowMap,
	ShadowMapDebug,
	Count
};

enum class ETextureType : int
{
	Texture2D = 0,
	Texture2DArray,
	Count
};

// 정점 구조체
struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT3 Normal;
	XMFLOAT2 Uv;
	XMFLOAT3 TangentU;
};

// 바닥 정점 구조체
struct QuadPatchVertex
{
	XMFLOAT3 Pos;
};

// 나무 정점 구조체
struct TreeVertex
{
	XMFLOAT3 Pos;
	XMFLOAT2 Size;
};

// 오브젝트 구조체
struct GeometryInfo
{
	std::wstring Name;

	// 정점 버퍼
	ComPtr<ID3D12Resource>		VertexBuffer = nullptr;
	ComPtr<ID3D12Resource>		VertexUploadBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW	VertexBufferView = {};

	// 인덱스 버퍼
	ComPtr<ID3D12Resource>		IndexBuffer = nullptr;
	ComPtr<ID3D12Resource>		IndexUploadBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW		IndexBufferView = {};

	// 정점 갯수
	UINT VertexCount = 0;

	// 인덱스 갯수
	UINT IndexCount = 0;
};

struct TextureInfo
{
	std::wstring Name;
	std::wstring FileName;

	UINT TextureHeapIndex = 0;

	ETextureType TextureType = ETextureType::Texture2D;

	ComPtr<ID3D12Resource> Resource = nullptr;
	ComPtr<ID3D12Resource> UploadHeap = nullptr;
};

struct MaterialInfo
{
	std::wstring Name;

	UINT MatCBIndex = 0;
	UINT Texture_On = 0;
	UINT TextureHeapIndex = 0;

	UINT Normal_On = 0;

	XMFLOAT4 Albedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	XMFLOAT3 Fresnel = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.25f;
};

// 렌더링할 아이템 구조체
struct RenderItem
{
	RenderItem() = default;

	UINT ObjectCBIndex = 0;

	XMFLOAT4X4 World = MathHelper::Identity4x4();
	XMFLOAT4X4 TextureTransform = MathHelper::Identity4x4();

	// Primitive Topology
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	GeometryInfo* Geometry = nullptr;
	MaterialInfo* Material = nullptr;
};

// 조명 정보
#define MAX_LIGHT 16

struct LightInfo
{
	UINT		LightType = 0;
	XMFLOAT3	Strength = { 0.5f, 0.5f, 0.5f };
	float		FalloffStart = 1.0f;
	XMFLOAT3	Position = { 0.0f, 0.0f, 0.0f };
	float		FalloffEnd = 10.0f;
	XMFLOAT3	Direction = { 0.0f, -1.0f, 0.0f };
	float		SpotPower = 64.0f;
	XMFLOAT3	Padding = { 0.0f, 0.0f, 0.0f };
};

// 오브젝트 개별 상수 버퍼
struct ObjectConstant
{
	XMFLOAT4X4	World = MathHelper::Identity4x4();
	XMFLOAT4X4	TextureTransform = MathHelper::Identity4x4();
};

// 공용 상수 버퍼
struct PassConstant
{
	XMFLOAT4X4	View = MathHelper::Identity4x4();
	XMFLOAT4X4	InvView = MathHelper::Identity4x4();
	XMFLOAT4X4	Proj = MathHelper::Identity4x4();
	XMFLOAT4X4	InvProj = MathHelper::Identity4x4();
	XMFLOAT4X4	ViewProj = MathHelper::Identity4x4();
	XMFLOAT4X4	ShadowTransform = MathHelper::Identity4x4();

	XMFLOAT4	AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };
	XMFLOAT3	EyePosW = { 0.0f, 0.0f, 0.0f };
	UINT		LightCount = 0;
	LightInfo	Lights[MAX_LIGHT];

	XMFLOAT4	FogColor = { 0.7f, 0.7f, 0.7f, 1.0f };
	float		FogStart = 5.0f;
	float		FogRange = 150.0f;
	XMFLOAT2	FogPadding = { 0.0f, 0.0f };
};

// 재질 상수 버퍼
struct MatConstant
{
	XMFLOAT4 Albedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	XMFLOAT3 Fresnel = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.25f;
	UINT Texture_On = 0;
	UINT Normal_On = 0;
	XMFLOAT2 Padding = { 0.0f, 0.0f };
};