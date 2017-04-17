#include "stdafx.h"
#include "ShaderLab.h"
#include <fstream>
#include <minwinbase.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;

ShaderLab::ShaderLab(HINSTANCE hInstance, int nCmdShow): D3D11App(hInstance, nCmdShow)
{
}

ShaderLab::~ShaderLab()
{
	ShaderLab::cleanup();
	unloadBuffers();
	unloadShaders();
}

bool ShaderLab::Initialize()
{
	if (!D3D11App::Initialize())
		return false;
	if (!loadBuffers())
	{
		MessageBox(nullptr, TEXT("Failed to load content."), TEXT("Error"), MB_OK);
		return false;
	}
	if (!loadShaders())
	{
		MessageBox(nullptr, TEXT("Failed to load shaders."), TEXT("Error"), MB_OK);
		return false;
	}
	if (!m_densityTexGenerator.Initialize(m_device))
		return false;
	if (!m_rockVBGenerator.Initialize(m_device))
		return false;


	m_commonStates = std::make_unique<CommonStates>(m_device);
	m_camera.SetPosition(0., 0.0f,-20.f);
	onResize();
	m_camera.LookAt(Vector3::Up, Vector3(0, 10, 0) - m_camera.GetPosition());
	m_camera.UpdateViewMatrix();

	m_worldMatrix = Matrix::CreateWorld(Vector3(0, 0, 0), Vector3::Forward, Vector3::Up);
	m_worldMatrix *= Matrix::CreateScale(10,10,10);
	return true;
}

void ShaderLab::update(float deltaTime)
{
	checkAndProcessKeyboardInput(deltaTime);

	m_deviceContext->UpdateSubresource(m_constantBuffers[CB_Frame], 0, nullptr, &m_camera.GetView(), 0, 0);

	static float angle = 0.0f;
	angle += 90.0f * deltaTime;
	XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);

	//m_worldMatrix *= Matrix::CreateTranslation(-4.f, 0.f, 0.f);
	//m_worldMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));
	m_deviceContext->UpdateSubresource(m_constantBuffers[CB_Object], 0, nullptr, &m_worldMatrix, 0, 0);
}

void ShaderLab::render(float deltaTime)
{
	assert(m_device);
	assert(m_deviceContext);

	if (!m_isDensityTextureGenerated)
		m_isDensityTextureGenerated = m_densityTexGenerator.Generate(m_deviceContext);
	

	//m_deviceContext->ClearRenderTargetView(m_renderTargetView, DirectX::Colors::CornflowerBlue);
	//m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);
	//auto densityTex = m_densityTexGenerator.GetTexture3DShaderResourceView();
	if (m_isDensityTextureGenerated && !m_isRockVertexBufferGenerated)
		m_isRockVertexBufferGenerated = m_rockVBGenerator.Generate(m_deviceContext, m_densityTexGenerator.GetTexture3DShaderResourceView());

	//std::unique_ptr<BasicEffect> effect;
	//effect = std::make_unique<BasicEffect>(m_device); 
	//effect->SetWorld(m_worldMatrix);
	//effect->SetView(m_camera.GetView());
	//effect->SetProjection(m_camera.GetProj());
	//effect->SetTextureEnabled(false);
	//effect->EnableDefaultLighting();
	//TSET
	//m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);
	//m_deviceContext->OMSetDepthStencilState(m_depthStencilState, 1);
	//m_swapChain->Present(0, 0);
	//return;
	/** SAMPLE CODE */
	//Clear!
	m_deviceContext->ClearRenderTargetView(m_renderTargetView, DirectX::Colors::CornflowerBlue);
	m_deviceContext->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);


	//const UINT vertexStride = sizeof(VertexPosColor);
	const UINT vertexStride = sizeof(RockVertexBufferGenerator::GeometryShaderOutput);
	const UINT offset = 0;

	//m_deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &vertexStride, &offset);
	auto vb = m_rockVBGenerator.GetVertexBuffer();
	m_deviceContext->IASetVertexBuffers(0, 1, &vb, &vertexStride, &offset);
	m_deviceContext->IASetInputLayout(m_inputLayoutSimpleVS);
	//m_deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R16_UINT, 0);
	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_deviceContext->VSSetShader(m_simpleVS, nullptr, 0);
	m_deviceContext->VSSetConstantBuffers(0, 3, m_constantBuffers);

	m_deviceContext->GSSetShader(nullptr, nullptr, 0);
	//m_deviceContext->GSSetShader(m_simpleGS, nullptr, 0);
	//m_deviceContext->GSSetConstantBuffers(0, 3, m_constantBuffers);
	//auto densSRV = m_densityTexGenerator.GetTexture3DShaderResourceView();
	//m_deviceContext->GSSetShaderResources(0, 1, &densSRV);
	//auto samplerState = m_commonStates->LinearWrap();
	//m_deviceContext->GSSetSamplers(0, 1, &samplerState);

	auto wireframe = m_commonStates->Wireframe();
	auto test = m_commonStates->CullNone();
	m_deviceContext->RSSetState(wireframe);
	//m_deviceContext->RSSetState(m_rasterizerState);
	m_deviceContext->RSSetViewports(1, &m_viewport);

	m_deviceContext->PSSetShader(m_simplePS, nullptr, 0);

	//m_deviceContext->OMSetRenderTargets(1, &m_tex3D_RTV, m_depthStencilView);
	m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);

	auto depth = m_commonStates->DepthNone();
	//m_deviceContext->OMSetDepthStencilState(depth, 1);
	m_deviceContext->OMSetDepthStencilState(m_depthStencilState, 1);
	
	//m_deviceContext->DrawIndexed(_countof(m_indices), 0, 0);
	m_deviceContext->DrawAuto();
	//m_deviceContext->DrawInstanced(96 * 96 * 50 * 15, 1, 0, 0);
	//m_deviceContext->Draw(96 * 96 * 3 * 15, 0);

	//m_worldMatrix *= Matrix::CreateTranslation(4.f, 0.f, 0.f);

	//m_deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R16_UINT, 0);
	//m_deviceContext->UpdateSubresource(m_constantBuffers[CB_Object], 0, nullptr, &m_worldMatrix, 0, 0);
	//m_deviceContext->VSSetConstantBuffers(0, 3, m_constantBuffers);
	//m_deviceContext->GSSetShader(nullptr, nullptr, 0);
	//m_deviceContext->DrawIndexed(_countof(m_indices), 0, 0);


	//Present Frame!!
	m_swapChain->Present(0, 0);
}

//
bool ShaderLab::loadBuffers()
{
	//assert(m_device);

	//// Create an initialize the vertex buffer.
	//D3D11_BUFFER_DESC vertexBufferDesc;
	//ZeroMemory(&vertexBufferDesc, sizeof(D3D11_BUFFER_DESC));
	//vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	//vertexBufferDesc.ByteWidth = sizeof(VertexPosNormal) * _countof(m_vertices);
	//vertexBufferDesc.CPUAccessFlags = 0;
	//vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	//D3D11_SUBRESOURCE_DATA resourceData;
	//ZeroMemory(&resourceData, sizeof(D3D11_SUBRESOURCE_DATA));
	//resourceData.pSysMem = m_vertices;

	//HRESULT hr = m_device->CreateBuffer(&vertexBufferDesc, &resourceData, &m_vertexBuffer);
	//if (FAILED(hr))
	//	return false;

	//// Create and initialize the index buffer.
	//D3D11_BUFFER_DESC indexBufferDesc;
	//ZeroMemory(&indexBufferDesc, sizeof(D3D11_BUFFER_DESC));
	//indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	//indexBufferDesc.ByteWidth = sizeof(WORD) * _countof(m_indices);
	//indexBufferDesc.CPUAccessFlags = 0;
	//indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	//resourceData.pSysMem = m_indices;

	//hr = m_device->CreateBuffer(&indexBufferDesc, &resourceData, &m_indexBuffer);
	//if (FAILED(hr))
	//	return false;

	return true;
}

void ShaderLab::unloadBuffers()
{
	//SafeRelease(m_indexBuffer);
	//SafeRelease(m_vertexBuffer);
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
	LPCWSTR compiledVertexShaderObject = L"Shader/SimpleVertexShader_d.cso";
#else
	LPCWSTR compiledVertexShaderObject = L"Shader/SimpleVertexShader.cso";
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
		{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(VertexPosNormal,Position), D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(VertexPosNormal,Normal), D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	hr = m_device->CreateInputLayout(vertexLayoutDesc, _countof(vertexLayoutDesc), vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), &m_inputLayoutSimpleVS);
	if (FAILED(hr))
		return false;

	SafeRelease(vertexShaderBlob);

	// Load the compiled pixel shader.
	ID3DBlob* pixelShaderBlob;
#if _DEBUG
	LPCWSTR compiledPixelShaderObject = L"Shader/SimplePixelShader_d.cso";
#else
	LPCWSTR compiledPixelShaderObject = L"Shader/SimplePixelShader.cso";
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
	LPCWSTR compiledGeometryShaderObject = L"Shader/SimpleGeometryShader_d.cso";
#else
	LPCWSTR compiledGeometryShaderObject = L"Shader/SimpleGeometryShader.cso";
#endif

	hr = D3DReadFileToBlob(compiledGeometryShaderObject, &geometryShaderBlob);
	if (FAILED(hr))
		return false;

	hr = m_device->CreateGeometryShader(geometryShaderBlob->GetBufferPointer(), geometryShaderBlob->GetBufferSize(), nullptr, &m_simpleGS);
	if (FAILED(hr))
		return false;

	SafeRelease(geometryShaderBlob);
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
	m_camera.SetLens(.5f * pi, AspectRatio(), .1f, 10000.0f);
	auto proj = m_camera.GetProj();
	m_deviceContext->UpdateSubresource(m_constantBuffers[CB_Appliation], 0, nullptr, &proj, 0, 0);
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

	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
	{
		//0.00109200052 --> old
		//0.00003520004410
		//0.000432299683
		m_rockVBGenerator.test_depthStep -= 0.0001f*deltaTime;
		m_isRockVertexBufferGenerated = false;
	} 
	else if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
	{
		m_rockVBGenerator.test_depthStep += 0.0001f*deltaTime;
		m_isRockVertexBufferGenerated = false;
	}

	if (GetAsyncKeyState(VK_UP) & 0x8000)
	{
		m_rockVBGenerator.test_heightStep += 0.001f*deltaTime;
		m_isRockVertexBufferGenerated = false;
	}
	else if (GetAsyncKeyState(VK_DOWN) & 0x8000)
	{
		m_rockVBGenerator.test_heightStep -= 0.001f*deltaTime;
		m_isRockVertexBufferGenerated = false;
	}
}

bool ShaderLab::initDirectX()
{
	if (!D3D11App::initDirectX())
		return false;

	return true;
}

void ShaderLab::cleanup()
{
	D3D11App::cleanup();
}