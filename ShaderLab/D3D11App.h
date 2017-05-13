#pragma once

#define MAX_LOADSTRING 100

class D3D11App
{
protected:
	D3D11App(HINSTANCE hInstance, int nCmdShow);
	D3D11App(const D3D11App& rhs) = delete;
	D3D11App& operator=(const D3D11App& rhs) = delete;
	virtual ~D3D11App();

public:
	static D3D11App* GetApp();

	virtual bool Initialize();
	int Run();
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	float AspectRatio() const;
	void SetViewportDimensions(FLOAT width, FLOAT height);
	D3D11_VIEWPORT const* GetViewport() const;

protected:
	//called in Gameloop by Run()
	virtual void update(float deltaTime);
	virtual void checkAndProcessKeyboardInput(float deltaTime) = 0;
	virtual void checkAndProcessMouseInput(float deltaTime) = 0;
	//called in Gameloop by Run()
	virtual void render(float deltaTime);
	virtual void cleanup();
	virtual void onResize();

	BOOL initWin32Application(HINSTANCE hInstance, int nCmdShow);
	ATOM registerWin32Class(HINSTANCE hInstance);
	virtual bool initDirectX();

	static DXGI_RATIONAL QueryRefreshRate(UINT screenWidth, UINT screenHeight, BOOL vsync);

protected:
	static D3D11App* m_app;

	// current instance
	HINSTANCE m_hInst;
	HWND m_windowHandle;
	// the main window class name
	WCHAR m_szWindowClass[MAX_LOADSTRING];
	// The title bar text
	WCHAR m_szTitle[MAX_LOADSTRING];
	int m_nCmdShow;

	// Direct3D device and swap chain.
	ID3D11Device* m_device = nullptr;
	ID3D11DeviceContext* m_deviceContext = nullptr;
	IDXGISwapChain* m_swapChain = nullptr;

	// Render target view for the back buffer of the swap chain.
	ID3D11RenderTargetView* m_renderTargetView = nullptr;
	// Depth/stencil view for use as a depth buffer.
	ID3D11DepthStencilView* m_depthStencilView = nullptr;
	// A texture to associate to the depth stencil view.
	ID3D11Texture2D* m_depthStencilBuffer = nullptr;

	// Define the functionality of the depth/stencil stages.
	ID3D11DepthStencilState* m_depthStencilState = nullptr;
	// Define the functionality of the rasterizer stage.
	ID3D11RasterizerState* m_rasterizerState = nullptr;
	D3D11_VIEWPORT m_viewport = { 0 };

	int m_windowHeight = 600;
	int m_windowWidth = 800;
	float m_deltaTime = 0.f;
	bool m_paused = false;
	bool m_minimized = false;
	bool m_maximized = true;
	bool m_resizing = false;

	// Input Modules
	std::unique_ptr<DirectX::Keyboard> m_keyboard;
	DirectX::Keyboard::KeyboardStateTracker m_keyboardStateTracker;
	std::unique_ptr<DirectX::Mouse> m_mouse;
	DirectX::Mouse::ButtonStateTracker m_mouseStateTracker;
};