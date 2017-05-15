#include "stdafx.h"
#include "ShaderLab.h"
#include <fstream>

using namespace DirectX;
using namespace DirectX::SimpleMath;

ShaderLab::ShaderLab(HINSTANCE hInstance, int nCmdShow) : D3D11App(hInstance, nCmdShow), m_rockSize(30, 100, 30)
{
	m_cbPerFrame.AppTime = 0;	
	
	m_cbPerApplication.DisplacementScale = 2.f;
	m_cbPerApplication.InitialStepIterations = 24;
	m_cbPerApplication.RefinementStepIterations = 12;
	m_cbPerApplication.ParallaxDepth = 15;
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

	auto fileName = m_textureFilesPath + L"flare0" + m_textureFilenameExtension;
	std::vector<std::wstring> fireParticlesFilenames = { fileName };
	if (!m_fireParticles.Initialize(L"Fire", m_device, m_deviceContext, fireParticlesFilenames, 500))
	{
		MessageBox(nullptr, TEXT("Failed to initialize fire ParticleSystem!"), TEXT("Error"), MB_OK);
		return false;
	}

	m_commonStates = std::make_unique<CommonStates>(m_device);
	m_camera.SetPosition(0., 0.0f, -20.f);
	onResize();
	m_camera.LookAt(Vector3::Up, Vector3(0, 10, 0) - m_camera.GetPosition());
	m_camera.UpdateViewMatrix();

	m_worldMatrix = Matrix::CreateWorld(Vector3(0, 0, 0), Vector3::Forward, Vector3::Up);
	m_worldMatrix *= Matrix::CreateScale(10, 10, 10);

	m_rockKdTreeRoot = std::make_unique<KDNode>();
	m_raycastHitSphere = DirectX::GeometricPrimitive::CreateSphere(m_deviceContext);

	return true;
}

void ShaderLab::update(float deltaTime)
{
	D3D11App::update(deltaTime);

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

	if(m_updateCbPerApplication)
	{
		m_deviceContext->UpdateSubresource(m_constantBuffers[CB_Application], 0, nullptr, &m_cbPerApplication, 0, 0);
		m_updateCbPerApplication = false;
	}

	m_fireParticles.Update(m_deviceContext, m_cbPerFrame.DeltaTime, m_cbPerFrame.AppTime, m_camera);
}

void ShaderLab::render()
{
	assert(m_device);
	assert(m_deviceContext);

	if (!m_isDensityTextureGenerated)
	{
		std::cout << "Create density 3D texture ...";
		m_isDensityTextureGenerated = m_densityTexGenerator.Generate(m_deviceContext);
		std::cout << " finished!\n";
	}

	if (m_isDensityTextureGenerated && !m_isRockVertexBufferGenerated)
	{
		std::cout << "Create rock vertexbuffer and extract triangles from vertexbuffer ...";
		m_isRockVertexBufferGenerated = m_rockVBGenerator.Generate(m_deviceContext, m_densityTexGenerator.GetTexture3DShaderResourceView());
		m_rockTrianglesTransformed = m_rockVBGenerator.extractTrianglesFromVertexBuffer(m_deviceContext, m_worldMatrix);
		std::cout << " finished!\n";

		std::vector<Triangle*> triPointer;
		for (auto& triangle : m_rockTrianglesTransformed)
			triPointer.push_back(&triangle);
		std::cout << "Build kdTree ...";
		m_rockKdTreeRoot.reset(KDNode::Build(triPointer, 0));
		std::cout << " finished!\n";
	}

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
	m_deviceContext->PSSetConstantBuffers(0, 2, m_constantBuffers);
	auto sampler = m_commonStates->LinearWrap();
	m_deviceContext->PSSetSamplers(0, 1, &m_lichenSampler);
	m_deviceContext->PSSetSamplers(1, 1, &sampler);
	m_deviceContext->PSSetShaderResources(0, 1, &m_texturesSRVs[14]);
	m_deviceContext->PSSetShaderResources(1, 1, &m_texturesSRVs[3]);
	m_deviceContext->PSSetShaderResources(2, 1, &m_texturesSRVs[18]);
	m_deviceContext->PSSetShaderResources(3, 1, &m_texturesDispSRVs[14]);
	m_deviceContext->PSSetShaderResources(4, 1, &m_texturesDispSRVs[3]);
	m_deviceContext->PSSetShaderResources(5, 1, &m_texturesDispSRVs[18]);
	m_deviceContext->PSSetShaderResources(6, 1, &m_texturesBumpSRVs[14]);
	m_deviceContext->PSSetShaderResources(7, 1, &m_texturesBumpSRVs[3]);
	m_deviceContext->PSSetShaderResources(8, 1, &m_texturesBumpSRVs[18]);

	m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);
	m_deviceContext->OMSetDepthStencilState(m_depthStencilState, 1);

	m_deviceContext->DrawAuto();

	m_fireParticles.Draw(m_deviceContext, m_renderTargetView, m_depthStencilView, m_rasterizerState, &m_viewport, 1);

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

	constantBufferDesc.ByteWidth = sizeof(CbPerApplication);
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
		{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(VertexPosNormal,LocalPosition), D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(VertexPosNormal,LocalVertexNormal), D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "NORMAL", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(VertexPosNormal,LocalSurfaceNormal), D3D11_INPUT_PER_VERTEX_DATA, 0 }
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
	m_cbPerApplication.Proj = m_camera.GetProj();
	m_deviceContext->UpdateSubresource(m_constantBuffers[CB_Application], 0, nullptr, &m_cbPerApplication, 0, 0);
}

void ShaderLab::checkAndProcessKeyboardInput(float deltaTime)
{
	auto state = m_keyboard->GetState();
	if (state.Escape)
	{
		PostQuitMessage(0);
	}

	m_keyboardStateTracker.Update(state);
	
	//m_keyboardStateTracker.IsKeyPressed(Keyboard::Keys::W) --> returns true if button is pressed (the first time)
	//state.W --> return true if button is held
	static float cameraSpeed = 10.f;
	if (state.W)
		m_camera.Walk(cameraSpeed * deltaTime);
	if (state.S)
		m_camera.Walk(-cameraSpeed * deltaTime);
	if (state.A)
		m_camera.Strafe(-cameraSpeed * deltaTime);
	if (state.D)
		m_camera.Strafe(cameraSpeed * deltaTime);
	m_camera.UpdateViewMatrix();

	//SUN ROTATION
	m_sunPhi = MathHelper::Clamp(m_sunPhi, 0.1f, XM_PIDIV2);
	if (state.Up)
		m_sunPhi += 1.0f*deltaTime;
	if (state.Down)
		m_sunPhi -= 1.0f*deltaTime;
	if (state.Left)
		m_sunTheta -= 1.0f*deltaTime;
	if (state.Right)
		m_sunTheta += 1.0f*deltaTime;

	//per app (mostly parallax elements) constant buffer
	if (state.O)
	{
		m_cbPerApplication.DisplacementScale -= 0.1f * deltaTime;
		if (m_cbPerApplication.DisplacementScale < 0.0f)
			m_cbPerApplication.DisplacementScale = 0.f;
	}
	if(state.P)
		m_cbPerApplication.DisplacementScale += 0.1f * deltaTime;
	if (m_keyboardStateTracker.IsKeyPressed(Keyboard::Keys::K))
	{
		if (m_cbPerApplication.InitialStepIterations >= 4)
		{
			m_cbPerApplication.InitialStepIterations -= 2;
			m_cbPerApplication.RefinementStepIterations = m_cbPerApplication.InitialStepIterations / 2;
		}
	}
	if (m_keyboardStateTracker.IsKeyPressed(Keyboard::Keys::L))
	{
		m_cbPerApplication.InitialStepIterations += 2;
		m_cbPerApplication.RefinementStepIterations = m_cbPerApplication.InitialStepIterations / 2;
	}
	if (state.N)
	{
		m_cbPerApplication.ParallaxDepth -= 1.f * deltaTime;
		if (m_cbPerApplication.ParallaxDepth < 0.0f)
			m_cbPerApplication.ParallaxDepth = 0.f;
	}
	if (state.M)
		m_cbPerApplication.ParallaxDepth += 1.f * deltaTime;

	if (state.O || state.P || state.K || state.L ||	state.N || state.M)
		m_updateCbPerApplication = true;
}

void ShaderLab::checkAndProcessMouseInput(float deltaTime)
{
	auto state = m_mouse->GetState();
	auto lastState = m_mouseStateTracker.GetLastState();
	m_mouseStateTracker.Update(state);
	if (m_mouseStateTracker.leftButton == m_mouseStateTracker.HELD)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = DirectX::XMConvertToRadians(0.10f*static_cast<float>(state.x - lastState.x));
		float dy = DirectX::XMConvertToRadians(0.10f*static_cast<float>(state.y - lastState.y));

		m_camera.Pitch(dy);
		m_camera.RotateY(dx);
		m_camera.UpdateViewMatrix();
	}
	if (m_mouseStateTracker.rightButton == m_mouseStateTracker.PRESSED)
	{
		Ray resultRay;
		m_raycastHitResult = raycast(state.x, state.y, resultRay);
		m_fireParticles.Reset();
		if (m_raycastHitResult.IsHit)
		{
			std::cout << "Raycast hit object! Position: (" << std::to_string(m_raycastHitResult.ImpactPoint.x) << ", " << std::to_string(m_raycastHitResult.ImpactPoint.y) << ", " << std::to_string(m_raycastHitResult.ImpactPoint.z) << ")\n";
			m_fireParticles.SetEmitPos(m_raycastHitResult.ImpactPoint);
		}
	
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

bool ShaderLab::createTextures(ID3D11Device* device)
{
	m_texturesSRVs.resize(m_textureCount);
	m_texturesDispSRVs.resize(m_textureCount);
	m_texturesBumpSRVs.resize(m_textureCount);
	for (int i = 1; i < m_textureCount + 1; ++i)
	{
		// Load the texture in.
		auto filename = (m_textureFilesPath + m_textureGenericFilename + std::to_wstring(i) + m_textureFilenameExtension);
		auto filenameDisp = (m_textureDispFilesPath + m_textureGenericFilename + std::to_wstring(i) + L"_disp" + m_textureFilenameExtension);
		auto filenameBump = (m_textureBumpFilesPath + m_textureGenericFilename + std::to_wstring(i) + L"_bump" + m_textureFilenameExtension);

		HRESULT hr = CreateDDSTextureFromFile(device, filename.c_str(), nullptr, &m_texturesSRVs[i - 1]);
		HRESULT hrDisp = CreateDDSTextureFromFile(device, filenameDisp.c_str(), nullptr, &m_texturesDispSRVs[i - 1]);
		HRESULT hrBump = CreateDDSTextureFromFile(device, filenameBump.c_str(), nullptr, &m_texturesBumpSRVs[i - 1]);

		if (FAILED(hr) || FAILED(hrDisp) || FAILED(hrBump))
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
	for (auto a : m_texturesDispSRVs)
	{
		SafeRelease(a);
	}
	for (auto a : m_texturesBumpSRVs)
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

HitResult ShaderLab::raycast(int sx, int sy, Ray& outRay)
{	
	// Compute picking ray in view space.
	float vx = (((+2.0f*sx) / m_windowWidth) - 1.0f) / m_camera.GetProj()(0, 0);
	float vy = (((-2.0f*sy) / m_windowHeight) + 1.0f) / m_camera.GetProj()(1, 1);

	Matrix invView = m_camera.GetView().Invert();

	// Ray definition in view space.
	//Vector3 rayOrigin = Vector3(invView._41, invView._42, invView._43); //current eye position in world space
	//or define like this -> Vector3::Zero is view space origin and we transform it to world space with the inverse of the view matrix
	Vector3 rayOrigin = Vector3::Transform(Vector3::Zero, invView);

	Vector3 rayDir = Vector3(vx, vy, 1.0f);
	//convert to world space
	rayDir = Vector3::TransformNormal(rayDir, invView);
	rayDir.Normalize();

	outRay = Ray(rayOrigin, rayDir);
	return KDNode::RayCast(m_rockKdTreeRoot.get(), outRay);
}
