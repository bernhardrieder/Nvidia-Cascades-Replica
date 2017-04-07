#include "stdafx.h"

#pragma once
class D3D11App
{
public:
	D3D11App();
	~D3D11App();

	int Init(HWND hwnd, BOOL vSync);
	bool LoadContent();
	void Update(float deltaTime);
	void Render();

private:
	static DXGI_RATIONAL QueryRefreshRate(UINT screenWidth, UINT screenHeight, BOOL vsync);

	void Clear(const FLOAT clearColor[4], FLOAT clearDepth, UINT8 clearStencil) const;
	void Present(bool vSync) const;
	void UnloadContent();
	void Cleanup();

	// Direct3D device and swap chain.
	ID3D11Device* m_d3dDevice = nullptr;
	ID3D11DeviceContext* m_d3dDeviceContext = nullptr;
	IDXGISwapChain* m_d3dSwapChain = nullptr;

	// Render target view for the back buffer of the swap chain.
	ID3D11RenderTargetView* m_d3dRenderTargetView = nullptr;
	// Depth/stencil view for use as a depth buffer.
	ID3D11DepthStencilView* m_d3dDepthStencilView = nullptr;
	// A texture to associate to the depth stencil view.
	ID3D11Texture2D* m_d3dDepthStencilBuffer = nullptr;

	// Define the functionality of the depth/stencil stages.
	ID3D11DepthStencilState* m_d3dDepthStencilState = nullptr;
	// Define the functionality of the rasterizer stage.
	ID3D11RasterizerState* m_d3dRasterizerState = nullptr;
	D3D11_VIEWPORT m_Viewport = { 0 };

	// Demo parameters
	DirectX::XMMATRIX g_WorldMatrix;
	DirectX::XMMATRIX g_ViewMatrix;
	DirectX::XMMATRIX g_ProjectionMatrix;

	bool m_EnableVSync;
};

