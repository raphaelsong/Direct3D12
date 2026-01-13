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

// 정점 구조체
struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
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

// 오브젝트 개별 상수 버퍼
struct ObjectConstant
{
	XMFLOAT4X4	World = MathHelper::Identity4x4();
};

// 공용 상수 버퍼
struct PassConstant
{
	XMFLOAT4X4	View = MathHelper::Identity4x4();
	XMFLOAT4X4	InvView = MathHelper::Identity4x4();
	XMFLOAT4X4	Proj = MathHelper::Identity4x4();
	XMFLOAT4X4	InvProj = MathHelper::Identity4x4();
	XMFLOAT4X4	ViewProj = MathHelper::Identity4x4();
};
