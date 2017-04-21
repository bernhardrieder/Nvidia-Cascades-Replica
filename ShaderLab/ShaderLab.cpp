#include "stdafx.h"
#include "ShaderLab.h"
#include <fstream>
#include <minwinbase.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;

ShaderLab::ShaderLab(HINSTANCE hInstance, int nCmdShow) : D3D11App(hInstance, nCmdShow), m_rockSize( 30, 50, 30 )
{
}

ShaderLab::~ShaderLab()
{
	ShaderLab::cleanup();
	unloadShaders();
}

bool ShaderLab::Initialize()
{
	if (!D3D11App::Initialize())
		return false;
	if (!loadShaders())
	{
		MessageBox(nullptr, TEXT("Failed to load shaders."), TEXT("Error"), MB_OK);
		return false;
	}

	if (!m_densityTexGenerator.Initialize(m_device, m_rockSize))
		return false;
	if (!m_rockVBGenerator.Initialize(m_device, m_rockSize))
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
	
	auto viewMatrix = m_camera.GetView();
	m_deviceContext->UpdateSubresource(m_constantBuffers[CB_Frame], 0, nullptr, &viewMatrix, 0, 0);
	m_deviceContext->UpdateSubresource(m_constantBuffers[CB_Object], 0, nullptr, &m_worldMatrix, 0, 0);
}

void ShaderLab::render(float deltaTime)
{
	assert(m_device);
	assert(m_deviceContext);

	if (!m_isDensityTextureGenerated)
		m_isDensityTextureGenerated = m_densityTexGenerator.Generate(m_deviceContext);
	
	if (m_isDensityTextureGenerated && !m_isRockVertexBufferGenerated)
		m_isRockVertexBufferGenerated = m_rockVBGenerator.Generate(m_deviceContext, m_densityTexGenerator.GetTexture3DShaderResourceView());

	m_deviceContext->ClearRenderTargetView(m_renderTargetView, DirectX::Colors::CornflowerBlue);
	m_deviceContext->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	const UINT vertexStride = sizeof(RockVertexBufferGenerator::GeometryShaderOutput);
	const UINT offset = 0;

	auto vb = m_rockVBGenerator.GetVertexBuffer();
	m_deviceContext->IASetVertexBuffers(0, 1, &vb, &vertexStride, &offset);
	m_deviceContext->IASetInputLayout(m_inputLayoutSimpleVS);
	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_deviceContext->VSSetShader(m_simpleVS, nullptr, 0);
	m_deviceContext->VSSetConstantBuffers(0, 3, m_constantBuffers);

	m_deviceContext->GSSetShader(nullptr, nullptr, 0);

	auto wireframe = m_commonStates->Wireframe();
	m_deviceContext->RSSetState(wireframe);
	//m_deviceContext->RSSetState(m_rasterizerState);
	m_deviceContext->RSSetViewports(1, &m_viewport);

	m_deviceContext->PSSetShader(m_simplePS, nullptr, 0);

	m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);
	m_deviceContext->OMSetDepthStencilState(m_depthStencilState, 1);
	
	m_deviceContext->DrawAuto();

	//Present Frame!!
	m_swapChain->Present(0, 0);
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
	static float cameraSpeed = 10.f;
	if (GetAsyncKeyState('W') & 0x8000)
		m_camera.Walk(cameraSpeed * deltaTime);

	if (GetAsyncKeyState('S') & 0x8000)
		m_camera.Walk(-cameraSpeed * deltaTime);

	if (GetAsyncKeyState('A') & 0x8000)
		m_camera.Strafe(-cameraSpeed * deltaTime);

	if (GetAsyncKeyState('D') & 0x8000)
		m_camera.Strafe(cameraSpeed * deltaTime);

	m_camera.UpdateViewMatrix();

	//TODO: ADD rock control FEATURE! -> density & vb generator needs reinit!
	//rock control
	//bool buttoLeftPressed = GetAsyncKeyState(VK_LEFT) & 0x8000;
	//bool buttonRightPressed = GetAsyncKeyState(VK_RIGHT) & 0x8000;
	//bool buttonUpPressed = GetAsyncKeyState(VK_UP) & 0x8000;
	//bool buttonDownPressed = GetAsyncKeyState(VK_DOWN) & 0x8000;
	//if (buttoLeftPressed)
	//{
	//	m_rockSize.x -= 1;
	//	m_rockSize.z -= 1;
	//} 
	//else if (buttonRightPressed)
	//{
	//	m_rockSize.x += 1;
	//	m_rockSize.z += 1;
	//}

	//if (buttonUpPressed)
	//{
	//	m_rockSize.y += 1;
	//}
	//else if (buttonDownPressed)
	//{
	//	m_rockSize.y -= 1;
	//}
	//if(buttoLeftPressed || buttonRightPressed || buttonUpPressed || buttonDownPressed)
	//{
	//	m_isDensityTextureGenerated = false;
	//	m_isRockVertexBufferGenerated = false;
	//}
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