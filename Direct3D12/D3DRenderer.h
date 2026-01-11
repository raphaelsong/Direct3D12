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
    static D3DRenderer* m_pRenderer;

    HINSTANCE m_hInstance = nullptr;         // application instance handle
    HWND      m_hWnd = nullptr;          // main window handle
    bool      m_bPaused = false;          // is the application paused?
    bool      m_bMinimized = false;           // is the application minimized?
    bool      m_bMaximized = false;           // is the application maximized?
    bool      m_bResizing = false;            // are the resize bars being dragged?
    bool      m_bFullscreenState = false;     // fullscreen enabled

    std::wstring m_strMainWnd = L"D3D12 Renderer";

    int m_nClientWidth = 800;
    int m_nClientHeight = 600;

private:
    GameTimer m_Timer;
};