#include "stdafx.h"
#include "D3D11App.h"
#include "resource.h"

using namespace DirectX;

D3D11App* D3D11App::m_app = nullptr; 

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Forward hwnd on because we can get messages (e.g., WM_CREATE)
	// before CreateWindow returns, and thus before mhMainWnd is valid.
	return D3D11App::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

D3D11App::D3D11App(HINSTANCE hInstance, int nCmdShow) : m_hInst(hInstance) ,m_nCmdShow(nCmdShow), m_deltaTime(0)
{
	assert(m_app == nullptr);
	m_app = this;
}


D3D11App::~D3D11App()
{
	D3D11App::cleanup();
}

D3D11App* D3D11App::GetApp()
{
	return m_app;
}

bool D3D11App::Initialize()
{
	if (!XMVerifyCPUSupport())
	{
		MessageBox(nullptr, TEXT("Failed to verify DirectX Math library support."), TEXT("Error"), MB_OK);
		return false;
	}

	if (!initWin32Application(m_hInst, m_nCmdShow))
	{
		MessageBox(nullptr, TEXT("Failed to create application window."), TEXT("Error"), MB_OK);
		return false;
	}

	if (!initDirectX())
	{
		MessageBox(nullptr, TEXT("Failed to initalize DirectX."), TEXT("Error"), MB_OK);
		return false;
	}
	return true;
}

int D3D11App::Run()
{
	MSG msg = { 0 };

	static DWORD previousTime = timeGetTime();
	static const float targetFramerate = 30.0f;
	static const float maxTimeStep = 1.0f / targetFramerate;

	// Main message loop:
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			DWORD currentTime = timeGetTime();
			if(!m_paused)
				m_deltaTime = (currentTime - previousTime) / 1000.0f;
			else
				m_deltaTime = 0.0f;

			previousTime = currentTime;

			// Cap the delta time to the max time step (useful if your 
			// debugging and you don't want the deltaTime value to explode.
			m_deltaTime = std::min<float>(m_deltaTime, maxTimeStep);

			update(m_deltaTime);
			render(m_deltaTime);
		}
	}

	return static_cast<int>(msg.wParam);
}

void D3D11App::render(float deltaTime)
{
	assert(m_device);
	assert(m_deviceContext);

	//Clear!
	m_deviceContext->ClearRenderTargetView(m_renderTargetView, Colors::CornflowerBlue);
	m_deviceContext->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	m_deviceContext->RSSetState(m_rasterizerState);
	m_deviceContext->RSSetViewports(1, &m_viewport);

	m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);
	m_deviceContext->OMSetDepthStencilState(m_depthStencilState, 1);

	//Present Frame!!
	m_swapChain->Present(0, 0);
}

void D3D11App::cleanup()
{
	SafeRelease(m_depthStencilView);
	SafeRelease(m_renderTargetView);
	SafeRelease(m_depthStencilBuffer);
	SafeRelease(m_depthStencilState);
	SafeRelease(m_rasterizerState);
	SafeRelease(m_swapChain);
	SafeRelease(m_deviceContext);
	SafeRelease(m_device);
}

void D3D11App::onResize()
{
	assert(m_deviceContext);
	assert(m_device);
	assert(m_swapChain);

	// Release the old views, as they hold references to the buffers we will be destroying.  Also release the old depth/stencil buffer.
	SafeRelease(m_renderTargetView);
	SafeRelease(m_depthStencilView);
	SafeRelease(m_depthStencilBuffer);

	// Resize the swap chain and recreate the render target view.
	//todo: error check
	m_swapChain->ResizeBuffers(1, m_windowWidth, m_windowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
	ID3D11Texture2D* backBuffer;
	//todo: error check
	m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
	//todo: error check
	m_device->CreateRenderTargetView(backBuffer, 0, &m_renderTargetView);
	SafeRelease(backBuffer);

	// Recreate the depth buffer for use with the depth/stencil view.
	D3D11_TEXTURE2D_DESC depthStencilBufferDesc;
	depthStencilBufferDesc.ArraySize = 1;
	depthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilBufferDesc.CPUAccessFlags = 0; // No CPU access required.
	depthStencilBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilBufferDesc.Width = m_windowWidth;
	depthStencilBufferDesc.Height = m_windowHeight;
	depthStencilBufferDesc.MipLevels = 1;
	depthStencilBufferDesc.SampleDesc.Count = 1;
	depthStencilBufferDesc.SampleDesc.Quality = 0;
	depthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilBufferDesc.MiscFlags = 0;

	//todo: error check
	m_device->CreateTexture2D(&depthStencilBufferDesc, 0, &m_depthStencilBuffer);
	//todo: error check
	m_device->CreateDepthStencilView(m_depthStencilBuffer, 0, &m_depthStencilView);

	// Initialize the viewport to occupy the entire client area.
	setViewportDimensions(static_cast<FLOAT>(m_windowWidth), static_cast<FLOAT>(m_windowHeight));
}

bool D3D11App::initDirectX()
{
	// A window handle must have been created already.
	assert(m_windowHandle != 0);

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));

	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Width = m_windowWidth;
	swapChainDesc.BufferDesc.Height = m_windowHeight;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate = QueryRefreshRate(m_windowWidth, m_windowHeight, false);
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = m_windowHandle;
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
		D3D11_SDK_VERSION, &swapChainDesc, &m_swapChain, &m_device, &featureLevel,
		&m_deviceContext);

	if (hr == E_INVALIDARG)
	{
		hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
			nullptr, createDeviceFlags, &featureLevels[1], _countof(featureLevels) - 1,
			D3D11_SDK_VERSION, &swapChainDesc, &m_swapChain, &m_device, &featureLevel,
			&m_deviceContext);
	}

	if (FAILED(hr))
		return false;

	// The Direct3D device and swap chain were successfully created.
	// Now we need to initialize the buffers of the swap chain.
	// Next initialize the back buffer of the swap chain and associate it to a 
	// render target view.
	ID3D11Texture2D* backBuffer;
	hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
	if (FAILED(hr))
		return false;

	hr = m_device->CreateRenderTargetView(backBuffer, nullptr, &m_renderTargetView);
	if (FAILED(hr))
		return false;

	SafeRelease(backBuffer);

	// Create the depth buffer for use with the depth/stencil view.
	D3D11_TEXTURE2D_DESC depthStencilBufferDesc;
	ZeroMemory(&depthStencilBufferDesc, sizeof(D3D11_TEXTURE2D_DESC));

	depthStencilBufferDesc.ArraySize = 1;
	depthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilBufferDesc.CPUAccessFlags = 0; // No CPU access required.
	depthStencilBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilBufferDesc.Width = m_windowWidth;
	depthStencilBufferDesc.Height = m_windowHeight;
	depthStencilBufferDesc.MipLevels = 1;
	depthStencilBufferDesc.SampleDesc.Count = 1;
	depthStencilBufferDesc.SampleDesc.Quality = 0;
	depthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	hr = m_device->CreateTexture2D(&depthStencilBufferDesc, nullptr, &m_depthStencilBuffer);
	if (FAILED(hr))
		return false;

	hr = m_device->CreateDepthStencilView(m_depthStencilBuffer, nullptr, &m_depthStencilView);
	if (FAILED(hr))
		return false;

	// Setup depth/stencil state.
	D3D11_DEPTH_STENCIL_DESC depthStencilStateDesc;
	ZeroMemory(&depthStencilStateDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));

	depthStencilStateDesc.DepthEnable = TRUE;
	depthStencilStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilStateDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilStateDesc.StencilEnable = FALSE;

	hr = m_device->CreateDepthStencilState(&depthStencilStateDesc, &m_depthStencilState);

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
	hr = m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerState);
	if (FAILED(hr))
		return false;

	// Initialize the viewport to occupy the entire client area.
	setViewportDimensions(static_cast<FLOAT>(m_windowWidth), static_cast<FLOAT>(m_windowHeight));

	return true;
}

void D3D11App::setViewportDimensions(FLOAT width, FLOAT height)
{
	m_viewport.Width = width;
	m_viewport.Height = height;
	m_viewport.TopLeftX = 0.0f;
	m_viewport.TopLeftY = 0.0f;
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;
}

ATOM D3D11App::registerWin32Class(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = MainWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SHADERLAB));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_SHADERLAB);
	wcex.lpszClassName = m_szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

BOOL D3D11App::initWin32Application(HINSTANCE hInstance, int nCmdShow)
{
	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, m_szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_SHADERLAB, m_szWindowClass, MAX_LOADSTRING);
	registerWin32Class(hInstance);

	RECT windowRect = { 0, 0, m_windowWidth, m_windowHeight };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	m_windowHandle = CreateWindowW(m_szWindowClass, m_szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top, nullptr, nullptr, hInstance, nullptr);

	if (!m_windowHandle)
	{
		return FALSE;
	}

	ShowWindow(m_windowHandle, nCmdShow);
	UpdateWindow(m_windowHandle);

	return TRUE;
}

LRESULT D3D11App::MsgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//  WM_COMMAND  - process the application menu
	//  WM_PAINT    - Paint the main window
	//  WM_DESTROY  - post a quit message and return
	switch (message)
	{
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code that uses hdc here...
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
			m_paused = true;
		else
			m_paused = false;
		return 0;
	case WM_SIZE:
		// Save the new client area dimensions.
		m_windowWidth = LOWORD(lParam);
		m_windowHeight = HIWORD(lParam);
		if (m_device)
		{
			if (wParam == SIZE_MINIMIZED)
			{
				m_paused = true;
				m_minimized = true;
				m_maximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				m_paused = false;
				m_minimized = false;
				m_maximized = true;
				onResize();
			}
			else if (wParam == SIZE_RESTORED)
			{
				if (m_minimized)
				{
					m_paused = false;
					m_minimized = false;
					onResize();
				}
				else if (m_maximized)
				{
					m_paused = false;
					m_maximized = false;
					onResize();
				}
				else if (m_resizing)
				{
					//do nothing!!
				}
				else
				{
					onResize();
				}
			}
		}
		return 0;
	case WM_ENTERSIZEMOVE:
		m_paused = true;
		m_resizing = true;
		return 0;
	case WM_EXITSIZEMOVE:
		m_paused = false;
		m_resizing = false;
		onResize();
		return 0;
	case WM_GETMINMAXINFO:
		//prevent window from becoming too small
		reinterpret_cast<MINMAXINFO*>(lParam)->ptMinTrackSize.x = 200;
		reinterpret_cast<MINMAXINFO*>(lParam)->ptMinTrackSize.y = 200;
		return 0;
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		onMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		onMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		onMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

float D3D11App::AspectRatio() const
{
	return static_cast<float>(m_windowWidth) / static_cast<float>(m_windowHeight);
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