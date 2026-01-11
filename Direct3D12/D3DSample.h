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
};