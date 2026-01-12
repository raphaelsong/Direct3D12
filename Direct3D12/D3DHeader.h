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