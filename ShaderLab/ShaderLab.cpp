#include "stdafx.h"
#include "ShaderLab.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

ShaderLab::ShaderLab(HINSTANCE hInstance, int nCmdShow): D3D11App(hInstance, nCmdShow)
{
}

ShaderLab::~ShaderLab()
{
	ShaderLab::cleanup();
	unloadContent();
	unloadShaders();
}

bool ShaderLab::Initialize()
{
	if (!D3D11App::Initialize())
		return false;
	if (!loadContent())
	{
		MessageBox(nullptr, TEXT("Failed to load content."), TEXT("Error"), MB_OK);
		return false;
	}
	if (!loadShaders())
	{
		MessageBox(nullptr, TEXT("Failed to load shaders."), TEXT("Error"), MB_OK);
		return false;
	}

	m_camera.SetPosition(0.f, 0.0f, -20.f);
	onResize();

	m_worldMatrix = XMMatrixRotationAxis(XMVectorSet(0, 1, 1, 0), XMConvertToRadians(90));
	m_worldMatrix *= Matrix::CreateTranslation(2.f, 0.f, 0.f);
	return true;
}

void ShaderLab::update(float deltaTime)
{
	checkAndProcessKeyboardInput(deltaTime);

	m_deviceContext->UpdateSubresource(m_constantBuffers[CB_Frame], 0, nullptr, &m_camera.GetView(), 0, 0);

	static float angle = 0.0f;
	angle += 90.0f * deltaTime;
	XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);

	m_worldMatrix *= Matrix::CreateTranslation(-4.f, 0.f, 0.f);
	//m_worldMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));
	m_deviceContext->UpdateSubresource(m_constantBuffers[CB_Object], 0, nullptr, &m_worldMatrix, 0, 0);
}

void ShaderLab::render(float deltaTime)
{
	assert(m_device);
	assert(m_deviceContext);

	if (!m_isDensityTextureCreated)
		fillDensityTexture();

	//Clear!
	m_deviceContext->ClearRenderTargetView(m_renderTargetView, DirectX::Colors::CornflowerBlue);
	m_deviceContext->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	const UINT vertexStride = sizeof(VertexPosColor);
	const UINT offset = 0;

	m_deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &vertexStride, &offset);
	m_deviceContext->IASetInputLayout(m_inputLayoutSimpleVS);
	m_deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R16_UINT, 0);
	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_deviceContext->VSSetShader(m_simpleVS, nullptr, 0);
	m_deviceContext->VSSetConstantBuffers(0, 3, m_constantBuffers);

	m_deviceContext->GSSetShader(m_simpleGS, nullptr, 0);

	m_deviceContext->RSSetState(m_rasterizerState);
	m_deviceContext->RSSetViewports(1, &m_viewport);

	m_deviceContext->PSSetShader(m_simplePS, nullptr, 0);

	//m_deviceContext->OMSetRenderTargets(1, &m_densityTex3D_RTV, m_depthStencilView);
	m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);
	m_deviceContext->OMSetDepthStencilState(m_depthStencilState, 1);

	m_deviceContext->DrawIndexed(_countof(m_indices), 0, 0);

	m_worldMatrix *= Matrix::CreateTranslation(4.f, 0.f, 0.f);
	m_deviceContext->UpdateSubresource(m_constantBuffers[CB_Object], 0, nullptr, &m_worldMatrix, 0, 0);
	m_deviceContext->VSSetConstantBuffers(0, 3, m_constantBuffers);
	m_deviceContext->GSSetShader(nullptr, nullptr, 0);
	m_deviceContext->DrawIndexed(_countof(m_indices), 0, 0);


	//Present Frame!!
	m_swapChain->Present(0, 0);
}

//
bool ShaderLab::loadContent()
{
	assert(m_device);

	// Create an initialize the vertex buffer.
	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(D3D11_BUFFER_DESC));
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.ByteWidth = sizeof(VertexPosColor) * _countof(m_vertices);
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA resourceData;
	ZeroMemory(&resourceData, sizeof(D3D11_SUBRESOURCE_DATA));
	resourceData.pSysMem = m_vertices;

	HRESULT hr = m_device->CreateBuffer(&vertexBufferDesc, &resourceData, &m_vertexBuffer);
	if (FAILED(hr))
		return false;

	// Create and initialize the index buffer.
	D3D11_BUFFER_DESC indexBufferDesc;
	ZeroMemory(&indexBufferDesc, sizeof(D3D11_BUFFER_DESC));
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.ByteWidth = sizeof(WORD) * _countof(m_indices);
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	resourceData.pSysMem = m_indices;

	hr = m_device->CreateBuffer(&indexBufferDesc, &resourceData, &m_indexBuffer);
	if (FAILED(hr))
		return false;

	return true;
}

void ShaderLab::unloadContent()
{
	SafeRelease(m_indexBuffer);
	SafeRelease(m_vertexBuffer);
}

bool ShaderLab::loadShaders()
{
	// Create the constant buffers for the variables defined in the vertex shader.
	D3D11_BUFFER_DESC constantBufferDesc;
	ZeroMemory(&constantBufferDesc, sizeof(D3D11_BUFFER_DESC));

	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.ByteWidth = sizeof(XMMATRIX);
	constantBufferDesc.CPUAccessFlags = 0;
	constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	HRESULT hr = m_device->CreateBuffer(&constantBufferDesc, nullptr, &m_constantBuffers[CB_Appliation]);
	if (FAILED(hr))
		return false;
	hr = m_device->CreateBuffer(&constantBufferDesc, nullptr, &m_constantBuffers[CB_Frame]);
	if (FAILED(hr))
		return false;
	hr = m_device->CreateBuffer(&constantBufferDesc, nullptr, &m_constantBuffers[CB_Object]);
	if (FAILED(hr))
		return false;

	// Load the compiled vertex shader.
	ID3DBlob* vertexShaderBlob;
#if _DEBUG
	LPCWSTR compiledVertexShaderObject = L"SimpleVertexShader_d.cso";
#else
	LPCWSTR compiledVertexShaderObject = L"SimpleVertexShader.cso";
#endif

	hr = D3DReadFileToBlob(compiledVertexShaderObject, &vertexShaderBlob);
	if (FAILED(hr))
		return false;

	hr = m_device->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), nullptr, &m_simpleVS);
	if (FAILED(hr))
		return false;

	// Create the input layout for the vertex shader.
	D3D11_INPUT_ELEMENT_DESC vertexLayoutDesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(VertexPosColor,Position), D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(VertexPosColor,Color), D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	hr = m_device->CreateInputLayout(vertexLayoutDesc, _countof(vertexLayoutDesc), vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), &m_inputLayoutSimpleVS);
	if (FAILED(hr))
		return false;

	SafeRelease(vertexShaderBlob);

	// Load the compiled pixel shader.
	ID3DBlob* pixelShaderBlob;
#if _DEBUG
	LPCWSTR compiledPixelShaderObject = L"SimplePixelShader_d.cso";
#else
	LPCWSTR compiledPixelShaderObject = L"SimplePixelShader.cso";
#endif

	hr = D3DReadFileToBlob(compiledPixelShaderObject, &pixelShaderBlob);
	if (FAILED(hr))
		return false;

	hr = m_device->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), nullptr, &m_simplePS);
	if (FAILED(hr))
		return false;

	SafeRelease(pixelShaderBlob);

	ID3DBlob* geometryShaderBlob;
#if _DEBUG
	LPCWSTR compiledGeometryShaderObject = L"SimpleGeometryShader_d.cso";
#else
	LPCWSTR compiledGeometryShaderObject = L"SimpleGeometryShader.cso";
#endif

	hr = D3DReadFileToBlob(compiledGeometryShaderObject, &geometryShaderBlob);
	if (FAILED(hr))
		return false;

	hr = m_device->CreateGeometryShader(geometryShaderBlob->GetBufferPointer(), geometryShaderBlob->GetBufferSize(), nullptr, &m_simpleGS);
	if (FAILED(hr))
		return false;

	SafeRelease(geometryShaderBlob);

	if (!loadDensityFunctionShaders())
		return false;

	return true;
}

void ShaderLab::unloadShaders()
{
	SafeRelease(m_constantBuffers[CB_Appliation]);
	SafeRelease(m_constantBuffers[CB_Frame]);
	SafeRelease(m_constantBuffers[CB_Object]);
	SafeRelease(m_inputLayoutSimpleVS);
	SafeRelease(m_simpleVS);
	SafeRelease(m_simplePS);
	SafeRelease(m_simpleGS);
}

void ShaderLab::onResize()
{
	static float pi = 3.1415926535f; // cmath include isn't working?!?!
	D3D11App::onResize();
	// The window resized, so update the aspect ratio and recompute the projection matrix.
	m_camera.SetLens(0.10f * pi, AspectRatio(), .1f, 1000.0f);
	m_deviceContext->UpdateSubresource(m_constantBuffers[CB_Appliation], 0, nullptr, &m_camera.GetProj(), 0, 0);
}

void ShaderLab::onMouseDown(WPARAM btnState, int x, int y)
{
	D3D11App::onMouseDown(btnState, x, y);
	m_lastMousePos.x = x;
	m_lastMousePos.y = y;

	SetCapture(m_windowHandle);
}

void ShaderLab::onMouseUp(WPARAM btnState, int x, int y)
{
	D3D11App::onMouseUp(btnState, x, y);
	ReleaseCapture();
}

void ShaderLab::onMouseMove(WPARAM btnState, int x, int y)
{
	D3D11App::onMouseMove(btnState, x, y);
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = DirectX::XMConvertToRadians(0.10f * static_cast<float>(x - m_lastMousePos.x));
		float dy = DirectX::XMConvertToRadians(0.10f * static_cast<float>(y - m_lastMousePos.y));

		m_camera.Pitch(dy);
		m_camera.RotateY(dx);
	}

	m_lastMousePos.x = x;
	m_lastMousePos.y = y;
}

void ShaderLab::checkAndProcessKeyboardInput(float deltaTime)
{
	if (GetAsyncKeyState('W') & 0x8000)
		m_camera.Walk(10.0f * deltaTime);

	if (GetAsyncKeyState('S') & 0x8000)
		m_camera.Walk(-10.0f * deltaTime);

	if (GetAsyncKeyState('A') & 0x8000)
		m_camera.Strafe(-10.0f * deltaTime);

	if (GetAsyncKeyState('D') & 0x8000)
		m_camera.Strafe(10.0f * deltaTime);

	m_camera.UpdateViewMatrix();
}

bool ShaderLab::initDirectX()
{
	if (!D3D11App::initDirectX())
		return false;

	D3D11_TEXTURE3D_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D11_TEXTURE3D_DESC));
	texDesc.Width = 96; // x axis
	texDesc.Height = 96; // y axis
	texDesc.Depth = 256; // z axis
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R16_FLOAT;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	//texDesc.CPUAccessFlags = 0;
	//texDesc.MiscFlags = 0;
	//
	//texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	HRESULT hr = m_device->CreateTexture3D(&texDesc, nullptr, &m_densityTex3D);
	if (FAILED(hr))
		return false;

	// Give the buffer a useful name
	//m_densityTex3D->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof("Density Texture"), "Density Texture");

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	ZeroMemory(&rtvDesc, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
	rtvDesc.Format = texDesc.Format;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;

	hr = m_device->CreateRenderTargetView(m_densityTex3D, nullptr, &m_densityTex3D_RTV);
	if (FAILED(hr))
		return false;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	srvDesc.Texture3D.MipLevels = 0;

	hr = m_device->CreateShaderResourceView(m_densityTex3D, nullptr, &m_densityTex3D_SRV);
	if (FAILED(hr))
		return false;

	return true;
}

void ShaderLab::cleanup()
{
	D3D11App::cleanup();
	SafeRelease(m_densityTex3D);
	SafeRelease(m_densityTex3D_RTV);
	SafeRelease(m_densityTex3D_SRV);
}

void ShaderLab::fillDensityTexture()
{
	setViewportDimensions(96, 96);
	m_deviceContext->ClearRenderTargetView(m_densityTex3D_RTV, Colors::White);
	//m_deviceContext->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	const UINT vertexStride = sizeof(VertexPos);
	const UINT offset = 0;

	m_deviceContext->IASetInputLayout(m_inputLayoutDensityVS);
	m_deviceContext->IASetVertexBuffers(0, 1, &m_renderPortalvertexBuffer, &vertexStride, &offset);
	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	m_deviceContext->VSSetShader(m_densityVS, nullptr, 0);
	//m_deviceContext->VSSetConstantBuffers(0, 3, m_constantBuffers);

	m_deviceContext->GSSetShader(m_densityGS, nullptr, 0);

	//rasterizer
	m_deviceContext->RSSetState(m_rasterizerState);
	m_deviceContext->RSSetViewports(1, &m_viewport);

	m_deviceContext->PSSetShader(m_densityPS, nullptr, 0);

	//output merger
	m_deviceContext->OMSetRenderTargets(1, &m_densityTex3D_RTV, nullptr);
	//m_deviceContext->OMSetDepthStencilState(m_depthStencilState, 1);

	m_deviceContext->DrawInstanced(6, 256, 0, 0);
	//m_swapChain->Present(0, 0);


	setViewportDimensions(static_cast<FLOAT>(m_windowWidth), static_cast<FLOAT>(m_windowHeight));
	//m_isDensityTextureCreated = true;
}

bool ShaderLab::loadDensityFunctionShaders()
{
	ID3DBlob *vsBlob, *gsBlob, *psBlob;

#if _DEBUG
	LPCWSTR compiledVS = L"create_density_tex3d_VS_d.cso";
	LPCWSTR compiledGS = L"create_density_tex3d_GS_d.cso";
	LPCWSTR compilesPS = L"create_density_tex3d_PS_d.cso";
#else
	LPCWSTR compiledVS = L"create_density_tex3d_VS.cso";
	LPCWSTR compiledGS = L"create_density_tex3d_GS.cso";
	LPCWSTR compilesPS = L"create_density_tex3d_PS.cso";
#endif

	HRESULT HR_VS, HR_GS, HR_PS;
	HR_VS = D3DReadFileToBlob(compiledVS, &vsBlob);
	HR_GS = D3DReadFileToBlob(compiledGS, &gsBlob);
	HR_PS = D3DReadFileToBlob(compilesPS, &psBlob);

	if (FAILED(HR_VS) || FAILED(HR_GS) || FAILED(HR_PS))
		return false;

	HR_VS = m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_densityVS);
	HR_GS = m_device->CreateGeometryShader(gsBlob->GetBufferPointer(), gsBlob->GetBufferSize(), nullptr, &m_densityGS);
	HR_PS = m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_densityPS);

	if (FAILED(HR_VS) || FAILED(HR_GS) || FAILED(HR_PS))
		return false;

	// Create the input layout for the vertex shader.
	D3D11_INPUT_ELEMENT_DESC vertexLayoutDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(VertexPos,Position), D3D11_INPUT_PER_VERTEX_DATA, 0 }
		//{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(VertexPosColor,Color), D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	HR_VS = m_device->CreateInputLayout(vertexLayoutDesc, _countof(vertexLayoutDesc), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayoutDensityVS);
	if (FAILED(HR_VS))
		return false;

	SafeRelease(vsBlob);
	SafeRelease(gsBlob);
	SafeRelease(psBlob);


	// Create an initialize the vertex buffer.
	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(D3D11_BUFFER_DESC));
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.ByteWidth = sizeof(VertexPos) * _countof(m_renderPortalVertices);
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA resourceData;
	ZeroMemory(&resourceData, sizeof(D3D11_SUBRESOURCE_DATA));
	resourceData.pSysMem = m_renderPortalVertices;

	HRESULT hr = m_device->CreateBuffer(&vertexBufferDesc, &resourceData, &m_renderPortalvertexBuffer);
	if (FAILED(hr))
		return false;

	return true;
}
