#include "stdafx.h"
#include "D3D11App.h"

using namespace DirectX;

D3D11App::D3D11App()
{
}


D3D11App::~D3D11App()
{
	UnloadContent();
	Cleanup();
}

/**
* Initialize the DirectX device and swap chain.
*/
int D3D11App::Init(HWND windowHandle, BOOL vSync)
{
	// A window handle must have been created already.
	assert(windowHandle != 0);

	m_EnableVSync = vSync;

	RECT clientRect;
	GetClientRect(windowHandle, &clientRect);

	// Compute the exact client dimensions. This will be used
	// to initialize the render targets for our swap chain.
	unsigned int clientWidth = clientRect.right - clientRect.left;
	unsigned int clientHeight = clientRect.bottom - clientRect.top;

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));

	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Width = clientWidth;
	swapChainDesc.BufferDesc.Height = clientHeight;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate = QueryRefreshRate(clientWidth, clientHeight, vSync);
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = windowHandle;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Windowed = TRUE;

	UINT createDeviceFlags = 0;
#if _DEBUG
	createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG;
#endif

	// These are the feature levels that we will accept.
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	// This will be the feature level that 
	// is used to create our device and swap chain.
	D3D_FEATURE_LEVEL featureLevel;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
		nullptr, createDeviceFlags, featureLevels, _countof(featureLevels),
		D3D11_SDK_VERSION, &swapChainDesc, &m_d3dSwapChain, &m_d3dDevice, &featureLevel,
		&m_d3dDeviceContext);

	if (hr == E_INVALIDARG)
	{
		hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
			nullptr, createDeviceFlags, &featureLevels[1], _countof(featureLevels) - 1,
			D3D11_SDK_VERSION, &swapChainDesc, &m_d3dSwapChain, &m_d3dDevice, &featureLevel,
			&m_d3dDeviceContext);
	}

	if (FAILED(hr))
	{
		return -1;
	}

	// The Direct3D device and swap chain were successfully created.
	// Now we need to initialize the buffers of the swap chain.
	// Next initialize the back buffer of the swap chain and associate it to a 
	// render target view.
	ID3D11Texture2D* backBuffer;
	hr = m_d3dSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
	if (FAILED(hr))
	{
		return -1;
	}

	hr = m_d3dDevice->CreateRenderTargetView(backBuffer, nullptr, &m_d3dRenderTargetView);
	if (FAILED(hr))
	{
		return -1;
	}

	SafeRelease(backBuffer);

	// Create the depth buffer for use with the depth/stencil view.
	D3D11_TEXTURE2D_DESC depthStencilBufferDesc;
	ZeroMemory(&depthStencilBufferDesc, sizeof(D3D11_TEXTURE2D_DESC));

	depthStencilBufferDesc.ArraySize = 1;
	depthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilBufferDesc.CPUAccessFlags = 0; // No CPU access required.
	depthStencilBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilBufferDesc.Width = clientWidth;
	depthStencilBufferDesc.Height = clientHeight;
	depthStencilBufferDesc.MipLevels = 1;
	depthStencilBufferDesc.SampleDesc.Count = 1;
	depthStencilBufferDesc.SampleDesc.Quality = 0;
	depthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	hr = m_d3dDevice->CreateTexture2D(&depthStencilBufferDesc, nullptr, &m_d3dDepthStencilBuffer);
	if (FAILED(hr))
	{
		return -1;
	}

	hr = m_d3dDevice->CreateDepthStencilView(m_d3dDepthStencilBuffer, nullptr, &m_d3dDepthStencilView);
	if (FAILED(hr))
	{
		return -1;
	}

	// Setup depth/stencil state.
	D3D11_DEPTH_STENCIL_DESC depthStencilStateDesc;
	ZeroMemory(&depthStencilStateDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));

	depthStencilStateDesc.DepthEnable = TRUE;
	depthStencilStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilStateDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilStateDesc.StencilEnable = FALSE;

	hr = m_d3dDevice->CreateDepthStencilState(&depthStencilStateDesc, &m_d3dDepthStencilState);

	// Setup rasterizer state.
	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));

	rasterizerDesc.AntialiasedLineEnable = FALSE;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.ScissorEnable = FALSE;
	rasterizerDesc.SlopeScaledDepthBias = 0.0f;

	// Create the rasterizer state object.
	hr = m_d3dDevice->CreateRasterizerState(&rasterizerDesc, &m_d3dRasterizerState);
	if (FAILED(hr))
	{
		return -1;
	}

	// Initialize the viewport to occupy the entire client area.
	m_Viewport.Width = static_cast<float>(clientWidth);
	m_Viewport.Height = static_cast<float>(clientHeight);
	m_Viewport.TopLeftX = 0.0f;
	m_Viewport.TopLeftY = 0.0f;
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;

	return 0;
}

bool D3D11App::LoadContent()
{
//	assert(m_d3dDevice);
//
//	// Create an initialize the vertex buffer.
//	D3D11_BUFFER_DESC vertexBufferDesc;
//	ZeroMemory(&vertexBufferDesc, sizeof(D3D11_BUFFER_DESC));
//
//	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
//	vertexBufferDesc.ByteWidth = sizeof(VertexPosColor) * _countof(g_Vertices);
//	vertexBufferDesc.CPUAccessFlags = 0;
//	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
//
//	D3D11_SUBRESOURCE_DATA resourceData;
//	ZeroMemory(&resourceData, sizeof(D3D11_SUBRESOURCE_DATA));
//
//	resourceData.pSysMem = g_Vertices;
//
//	HRESULT hr = g_d3dDevice->CreateBuffer(&vertexBufferDesc, &resourceData, &g_d3dVertexBuffer);
//	if (FAILED(hr))
//	{
//		return false;
//	}
//
//	// Create and initialize the index buffer.
//	D3D11_BUFFER_DESC indexBufferDesc;
//	ZeroMemory(&indexBufferDesc, sizeof(D3D11_BUFFER_DESC));
//
//	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
//	indexBufferDesc.ByteWidth = sizeof(WORD) * _countof(g_Indicies);
//	indexBufferDesc.CPUAccessFlags = 0;
//	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
//	resourceData.pSysMem = g_Indicies;
//
//	hr = g_d3dDevice->CreateBuffer(&indexBufferDesc, &resourceData, &g_d3dIndexBuffer);
//	if (FAILED(hr))
//	{
//		return false;
//	}
//
//	// Create the constant buffers for the variables defined in the vertex shader.
//	D3D11_BUFFER_DESC constantBufferDesc;
//	ZeroMemory(&constantBufferDesc, sizeof(D3D11_BUFFER_DESC));
//
//	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
//	constantBufferDesc.ByteWidth = sizeof(XMMATRIX);
//	constantBufferDesc.CPUAccessFlags = 0;
//	constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
//
//	hr = g_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &g_d3dConstantBuffers[CB_Appliation]);
//	if (FAILED(hr))
//	{
//		return false;
//	}
//	hr = g_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &g_d3dConstantBuffers[CB_Frame]);
//	if (FAILED(hr))
//	{
//		return false;
//	}
//	hr = g_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &g_d3dConstantBuffers[CB_Object]);
//	if (FAILED(hr))
//	{
//		return false;
//	}
//
//	// Load the shaders
//	//g_d3dVertexShader = LoadShader<ID3D11VertexShader>( L"../data/shaders/SimpleVertexShader.hlsl", "SimpleVertexShader", "latest" );
//	//g_d3dPixelShader = LoadShader<ID3D11PixelShader>( L"../data/shaders/SimplePixelShader.hlsl", "SimplePixelShader", "latest" );
//
//	// Load the compiled vertex shader.
//	ID3DBlob* vertexShaderBlob;
//#if _DEBUG
//	LPCWSTR compiledVertexShaderObject = L"SimpleVertexShader_d.cso";
//#else
//	LPCWSTR compiledVertexShaderObject = L"SimpleVertexShader.cso";
//#endif
//
//	hr = D3DReadFileToBlob(compiledVertexShaderObject, &vertexShaderBlob);
//	if (FAILED(hr))
//	{
//		return false;
//	}
//
//	hr = g_d3dDevice->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), nullptr, &g_d3dVertexShader);
//	if (FAILED(hr))
//	{
//		return false;
//	}
//
//	// Create the input layout for the vertex shader.
//	D3D11_INPUT_ELEMENT_DESC vertexLayoutDesc[] =
//	{
//		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(VertexPosColor,Position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
//		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(VertexPosColor,Color), D3D11_INPUT_PER_VERTEX_DATA, 0 }
//	};
//
//	hr = g_d3dDevice->CreateInputLayout(vertexLayoutDesc, _countof(vertexLayoutDesc), vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), &g_d3dInputLayout);
//	if (FAILED(hr))
//	{
//		return false;
//	}
//
//	SafeRelease(vertexShaderBlob);
//
//	// Load the compiled pixel shader.
//	ID3DBlob* pixelShaderBlob;
//#if _DEBUG
//	LPCWSTR compiledPixelShaderObject = L"SimplePixelShader_d.cso";
//#else
//	LPCWSTR compiledPixelShaderObject = L"SimplePixelShader.cso";
//#endif
//
//	hr = D3DReadFileToBlob(compiledPixelShaderObject, &pixelShaderBlob);
//	if (FAILED(hr))
//	{
//		return false;
//	}
//
//	hr = g_d3dDevice->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), nullptr, &g_d3dPixelShader);
//	if (FAILED(hr))
//	{
//		return false;
//	}
//
//	SafeRelease(pixelShaderBlob);
//
//	// Setup the projection matrix.
//	RECT clientRect;
//	GetClientRect(g_WindowHandle, &clientRect);
//
//	// Compute the exact client dimensions.
//	// This is required for a correct projection matrix.
//	float clientWidth = static_cast<float>(clientRect.right - clientRect.left);
//	float clientHeight = static_cast<float>(clientRect.bottom - clientRect.top);
//
//	g_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), clientWidth / clientHeight, 0.1f, 100.0f);
//
//	g_d3dDeviceContext->UpdateSubresource(g_d3dConstantBuffers[CB_Appliation], 0, nullptr, &g_ProjectionMatrix, 0, 0);
//
//	return true;
return true;
}

void D3D11App::Update(float deltaTime)
{
	XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 1);
	XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
	XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
	g_ViewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);
	//m_d3dDeviceContext->UpdateSubresource(m_d3dConstantBuffers[CB_Frame], 0, nullptr, &g_ViewMatrix, 0, 0);


	static float angle = 0.0f;
	angle += 90.0f * deltaTime;
	XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);

	g_WorldMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));
	//m_d3dDeviceContext->UpdateSubresource(m_d3dConstantBuffers[CB_Object], 0, nullptr, &g_WorldMatrix, 0, 0);
}

void D3D11App::Render()
{
	assert(m_d3dDevice);
	assert(m_d3dDeviceContext);

	Clear(DirectX::Colors::CornflowerBlue, 1.0f, 0);

	m_d3dDeviceContext->RSSetState(m_d3dRasterizerState);
	m_d3dDeviceContext->RSSetViewports(1, &m_Viewport);

	m_d3dDeviceContext->OMSetRenderTargets(1, &m_d3dRenderTargetView, m_d3dDepthStencilView);
	m_d3dDeviceContext->OMSetDepthStencilState(m_d3dDepthStencilState, 1);

	Present(m_EnableVSync);
}

void D3D11App::Present(bool vSync) const
{
	if (vSync)
	{
		m_d3dSwapChain->Present(1, 0);
	}
	else
	{
		m_d3dSwapChain->Present(0, 0);
	}
}

void D3D11App::UnloadContent()
{
	//SafeRelease(g_d3dConstantBuffers[CB_Appliation]);
	//SafeRelease(g_d3dConstantBuffers[CB_Frame]);
	//SafeRelease(g_d3dConstantBuffers[CB_Object]);
	//SafeRelease(g_d3dIndexBuffer);
	//SafeRelease(g_d3dVertexBuffer);
	//SafeRelease(g_d3dInputLayout);
	//SafeRelease(g_d3dVertexShader);
	//SafeRelease(g_d3dPixelShader);
}

void D3D11App::Cleanup()
{
	SafeRelease(m_d3dDepthStencilView);
	SafeRelease(m_d3dRenderTargetView);
	SafeRelease(m_d3dDepthStencilBuffer);
	SafeRelease(m_d3dDepthStencilState);
	SafeRelease(m_d3dRasterizerState);
	SafeRelease(m_d3dSwapChain);
	SafeRelease(m_d3dDeviceContext);
	SafeRelease(m_d3dDevice);
}

// This function was inspired by:
// http://www.rastertek.com/dx11tut03.html
DXGI_RATIONAL D3D11App::QueryRefreshRate(UINT screenWidth, UINT screenHeight, BOOL vsync)
{
	DXGI_RATIONAL refreshRate = { 0, 1 };
	if (vsync)
	{
		IDXGIFactory* factory;
		IDXGIAdapter* adapter;
		IDXGIOutput* adapterOutput;
		DXGI_MODE_DESC* displayModeList;

		// Create a DirectX graphics interface factory.
		HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&factory));
		if (FAILED(hr))
		{
			MessageBox(0,
				TEXT("Could not create DXGIFactory instance."),
				TEXT("Query Refresh Rate"),
				MB_OK);

			throw new std::exception("Failed to create DXGIFactory.");
		}

		hr = factory->EnumAdapters(0, &adapter);
		if (FAILED(hr))
		{
			MessageBox(0,
				TEXT("Failed to enumerate adapters."),
				TEXT("Query Refresh Rate"),
				MB_OK);

			throw new std::exception("Failed to enumerate adapters.");
		}

		hr = adapter->EnumOutputs(0, &adapterOutput);
		if (FAILED(hr))
		{
			MessageBox(0,
				TEXT("Failed to enumerate adapter outputs."),
				TEXT("Query Refresh Rate"),
				MB_OK);

			throw new std::exception("Failed to enumerate adapter outputs.");
		}

		UINT numDisplayModes;
		hr = adapterOutput->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numDisplayModes, nullptr);
		if (FAILED(hr))
		{
			MessageBox(0,
				TEXT("Failed to query display mode list."),
				TEXT("Query Refresh Rate"),
				MB_OK);

			throw new std::exception("Failed to query display mode list.");
		}

		displayModeList = new DXGI_MODE_DESC[numDisplayModes];
		assert(displayModeList);

		hr = adapterOutput->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numDisplayModes, displayModeList);
		if (FAILED(hr))
		{
			MessageBox(0,
				TEXT("Failed to query display mode list."),
				TEXT("Query Refresh Rate"),
				MB_OK);

			throw new std::exception("Failed to query display mode list.");
		}

		// Now store the refresh rate of the monitor that matches the width and height of the requested screen.
		for (UINT i = 0; i < numDisplayModes; ++i)
		{
			if (displayModeList[i].Width == screenWidth && displayModeList[i].Height == screenHeight)
			{
				refreshRate = displayModeList[i].RefreshRate;
			}
		}

		delete[] displayModeList;
		SafeRelease(adapterOutput);
		SafeRelease(adapter);
		SafeRelease(factory);
	}

	return refreshRate;
}

// Clear the color and depth buffers.
void D3D11App::Clear(const FLOAT clearColor[4], FLOAT clearDepth, UINT8 clearStencil) const
{
	m_d3dDeviceContext->ClearRenderTargetView(m_d3dRenderTargetView, clearColor);
	m_d3dDeviceContext->ClearDepthStencilView(m_d3dDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, clearDepth, clearStencil);
}
