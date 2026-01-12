#pragma once

#include "D3DHeader.h"

class D3DRenderer
{
protected:
    D3DRenderer(HINSTANCE hInstance);
    D3DRenderer(const D3DRenderer& rhs) = delete;
    D3DRenderer& operator=(const D3DRenderer& rhs) = delete;
    virtual ~D3DRenderer();

public:
    static D3DRenderer* GetRenderer();

    HINSTANCE MainInst()const;
    HWND MainWnd()const;
    float AspectRatio()const;

    virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    virtual bool Initialize();
    virtual void OnResize();
    virtual int Run();

protected:
    virtual void Update(float deltaTime) = 0;
    virtual void LateUpdate(float deltaTime) = 0;
    virtual void BeginRender() = 0;
    virtual void Render() = 0;
    virtual void EndRender() = 0;

    // Convenience overrides for handling mouse input.
    virtual void OnMouseDown(WPARAM btnState, int x, int y) {}
    virtual void OnMouseUp(WPARAM btnState, int x, int y) {}
    virtual void OnMouseMove(WPARAM btnState, int x, int y) {}

protected:
    bool CreateMainWindow();
    void CalculateFrameStats();

protected:
    BOOL InitDirect3D12();
    void CreateDevice();
    void CreateCommandList();
    void CreateSwapChain();
    void CreateDescriptorSize();
    void CreateRtvDescriptorHeap();
    void CreateDsvDescriptorHeap();
    void CreateFence();
    void CreateViewport();

    void FlushCommandQueue();

public:
    inline ID3D12Resource* CurrentBackBuffer() const
    {
        return m_SwapChainBuffer[m_CurrentBackBufferIndex].Get();
    }

    inline D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() const
    {
        return m_SwapChainBufferView[m_CurrentBackBufferIndex];
    }

    inline D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const
    {
        return m_DepthStencilBufferView;
    }

protected:
    ComPtr<IDXGIFactory4>   m_DxgiFactory;
    ComPtr<ID3D12Device>    m_D3dDevice;

    ComPtr<ID3D12CommandQueue>          m_CommandQueue;
    ComPtr<ID3D12CommandAllocator>      m_CommandAlloc;
    ComPtr<ID3D12GraphicsCommandList>   m_CommandList;

    ComPtr<IDXGISwapChain>  m_SwapChain;
    ComPtr<ID3D12Resource>  m_SwapChainBuffer[SWAP_CHAIN_BUFFER_COUNT];

    UINT m_CurrentBackBufferIndex = 0;

    UINT m_CbvSrvUavDescriptorSize = 0;
    UINT m_RtvDescriptorSize = 0;
    UINT m_DsvDescriptorSize = 0;

    ComPtr<ID3D12DescriptorHeap> m_RtvHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE m_SwapChainBufferView[SWAP_CHAIN_BUFFER_COUNT] = {};

    ComPtr<ID3D12Resource> m_DepthStencilBuffer;

    ComPtr<ID3D12DescriptorHeap> m_DsvHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE m_DepthStencilBufferView = {};

    ComPtr<ID3D12Fence> m_Fence;
    HANDLE m_hFenceEvent = nullptr;
    UINT64 m_CurrentFence = 0;

    D3D12_VIEWPORT m_ScreenViewport;
    D3D12_RECT m_ScissorRect;

protected:
    static D3DRenderer* m_pRenderer;

    HINSTANCE m_hInstance = nullptr;            // application instance handle
    HWND      m_hWnd = nullptr;                 // main window handle
    bool      m_bPaused = false;                // is the application paused?
    bool      m_bMinimized = false;             // is the application minimized?
    bool      m_bMaximized = false;             // is the application maximized?
    bool      m_bResizing = false;              // are the resize bars being dragged?
    bool      m_bFullscreenState = false;       // fullscreen enabled

    std::wstring m_strMainWnd = L"D3D12 Renderer";

    int m_nClientWidth = 800;
    int m_nClientHeight = 600;

private:
    GameTimer m_Timer;
};