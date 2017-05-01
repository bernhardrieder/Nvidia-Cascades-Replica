#include "stdafx.h"
#include "ShaderLab.h"
#include <fstream>
#include <minwinbase.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;

ShaderLab::ShaderLab(HINSTANCE hInstance, int nCmdShow) : D3D11App(hInstance, nCmdShow), m_rockSize(30, 100, 30)
{
	m_cbPerFrame.AppTime = 0;
}

ShaderLab::~ShaderLab()
{
	ShaderLab::cleanup();
	unloadShaders();
	releaseTextures();
	releaseSampler();
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

	if (!createTextures(m_device))
	{
		MessageBox(nullptr, TEXT("Failed to load textures. Is Assets/Textures folder in execution folder?"), TEXT("Error"), MB_OK);
		return false;
	}
	if(!createSampler(m_device))
	{
		MessageBox(nullptr, TEXT("Failed create SamplerStates!"), TEXT("Error"), MB_OK);
		return false;
	}


	m_commonStates = std::make_unique<CommonStates>(m_device);
	m_camera.SetPosition(0., 0.0f, -20.f);
	onResize();
	m_camera.LookAt(Vector3::Up, Vector3(0, 10, 0) - m_camera.GetPosition());
	m_camera.UpdateViewMatrix();

	m_worldMatrix = Matrix::CreateWorld(Vector3(0, 0, 0), Vector3::Forward, Vector3::Up);
	m_worldMatrix *= Matrix::CreateScale(10, 10, 10);

	return true;
}

void ShaderLab::update(float deltaTime)
{
	checkAndProcessKeyboardInput(deltaTime);


	m_cbPerFrame.View = m_camera.GetView();
	m_cbPerFrame.ViewProj = m_camera.GetView() * m_camera.GetProj();
	m_cbPerFrame.ScreenSize = { (float)m_windowWidth, (float)m_windowHeight, 0.f};
	m_cbPerFrame.WorldEyePosition = static_cast<XMFLOAT3>(m_camera.GetPosition());
	m_cbPerFrame.SunLightDirection = -MathHelper::SphericalToCartesian(1.0f, m_sunTheta, m_sunPhi);
	m_cbPerFrame.AppTime += deltaTime;
	m_cbPerFrame.DeltaTime = deltaTime;

	m_deviceContext->UpdateSubresource(m_constantBuffers[CB_Frame], 0, nullptr, &m_cbPerFrame, 0, 0);

	m_cbPerObject.World = m_worldMatrix;
	m_cbPerObject.WorldInverseTranspose = (m_worldMatrix.Invert()).Transpose();
	m_deviceContext->UpdateSubresource(m_constantBuffers[CB_Object], 0, nullptr, &m_cbPerObject, 0, 0);
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

	m_deviceContext->RSSetState(m_rasterizerState);
	m_deviceContext->RSSetViewports(1, &m_viewport);

	m_deviceContext->PSSetShader(m_simplePS, nullptr, 0);
	m_deviceContext->PSSetConstantBuffers(0, 1, &m_constantBuffers[CB_Frame]);
	auto sampler = m_commonStates->PointWrap();
	m_deviceContext->PSSetSamplers(0, 1, &m_lichenSampler);
	m_deviceContext->PSSetShaderResources(0, 1, &m_texturesSRVs[0]);
	m_deviceContext->PSSetShaderResources(1, 1, &m_texturesSRVs[3]);
	m_deviceContext->PSSetShaderResources(2, 1, &m_texturesSRVs[18]);


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
	constantBufferDesc.CPUAccessFlags = 0;
	constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	constantBufferDesc.ByteWidth = sizeof(XMMATRIX);
	HRESULT hr = m_device->CreateBuffer(&constantBufferDesc, nullptr, &m_constantBuffers[CB_Application]);
	if (FAILED(hr))
		return false;

	constantBufferDesc.ByteWidth = sizeof(CbPerFrame);
	hr = m_device->CreateBuffer(&constantBufferDesc, nullptr, &m_constantBuffers[CB_Frame]);
	if (FAILED(hr))
		return false;

	constantBufferDesc.ByteWidth = sizeof(CbPerObject);
	hr = m_device->CreateBuffer(&constantBufferDesc, nullptr, &m_constantBuffers[CB_Object]);
	if (FAILED(hr))
		return false;

	// Load the compiled vertex shader.
	ID3DBlob* vertexShaderBlob;

	hr = D3DReadFileToBlob(m_compiledVSPath, &vertexShaderBlob);
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

	hr = D3DReadFileToBlob(m_compiledPSPath, &pixelShaderBlob);
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
	SafeRelease(m_constantBuffers[CB_Application]);
	SafeRelease(m_constantBuffers[CB_Frame]);
	SafeRelease(m_constantBuffers[CB_Object]);
	SafeRelease(m_inputLayoutSimpleVS);
	SafeRelease(m_simpleVS);
	SafeRelease(m_simplePS);
}

void ShaderLab::onResize()
{
	D3D11App::onResize();
	// The window resized, so update the aspect ratio and recompute the projection matrix.
	m_camera.SetLens(.5f * XM_PI, AspectRatio(), .1f, 10000.0f);
	auto proj = m_camera.GetProj();
	m_deviceContext->UpdateSubresource(m_constantBuffers[CB_Application], 0, nullptr, &proj, 0, 0);
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

	//SUN ROTATION
	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
		m_sunTheta -= 1.0f*deltaTime;

	if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
		m_sunTheta += 1.0f*deltaTime;

	if (GetAsyncKeyState(VK_UP) & 0x8000)
		m_sunPhi += 1.0f*deltaTime;

	if (GetAsyncKeyState(VK_DOWN) & 0x8000)
		m_sunPhi -= 1.0f*deltaTime;

	m_sunPhi = MathHelper::Clamp(m_sunPhi, 0.1f, XM_PIDIV2);
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

bool ShaderLab::createTextures(ID3D11Device* device)
{
	m_texturesSRVs.resize(m_textureCount);
	for (int i = 1; i < m_textureCount + 1; ++i)
	{
		// Load the texture in.
		auto filename = (m_textureFilesPath + m_textureGenericFilename + std::to_wstring(i) + m_textureFilenameExtension);
		HRESULT hr = CreateDDSTextureFromFile(device, filename.c_str(), nullptr, &m_texturesSRVs[i - 1]);

		if (FAILED(hr))
			return false;
	}

	return true;
}

void ShaderLab::releaseTextures()
{
	for (auto a : m_texturesSRVs)
	{
		SafeRelease(a);
	}
}

bool ShaderLab::createSampler(ID3D11Device* device)
{
	D3D11_SAMPLER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_SAMPLER_DESC));
	desc.Filter = D3D11_FILTER_ANISOTROPIC;
	desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

	//from dxtk commonstates
	desc.MaxAnisotropy = 16;
	desc.MaxLOD = FLT_MAX;
	desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	

	HRESULT hr = device->CreateSamplerState(&desc, &m_lichenSampler);
	if (FAILED(hr))
		return false;
	return true;
}

void ShaderLab::releaseSampler()
{
	SafeRelease(m_lichenSampler);
}
