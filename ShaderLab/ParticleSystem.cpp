#include "stdafx.h"
#include "ParticleSystem.h"
#include <random>


ParticleSystem::ParticleSystem()
{
	m_isEmitterAlive = false;
	m_cbPerFrame.GameTime = 0.0f;
	m_cbPerFrame.DeltaTime = 0.0f;

	m_cbPerFrame.EyePositionWorld = { 0.0f, 0.0f, 0.0f, 0.f };
	m_cbPerFrame.EmitterPositionWorld = {0.0f, 0.0f, 0.0f, 0.f};
	m_cbPerFrame.EmitterDirectionWorld = {0.0f, 1.0f, 0.0f, 0.f};
}

ParticleSystem::~ParticleSystem()
{
	releaseShaders();
	releaseConstantBuffers();
	releaseVertexBuffers();
	releaseTextureSRVs(); 
}

void ParticleSystem::SetEmitterPosition(const XMFLOAT3& emitterPositionWorld)
{
	m_cbPerFrame.EmitterPositionWorld = { emitterPositionWorld.x, emitterPositionWorld.y, emitterPositionWorld.z, 0.f };
	m_isEmitterPositionSet = true;
}

void ParticleSystem::SetEmitterDirection(const XMFLOAT3& emitterDirectionNormalized)
{
	m_cbPerFrame.EmitterDirectionWorld = { emitterDirectionNormalized.x, emitterDirectionNormalized.y, emitterDirectionNormalized.z, 0.f };
}
 
bool ParticleSystem::Initialize(const std::wstring& particleNamePrefixInShaderFile, const std::wstring& textureFile, const unsigned& maxParticles, ID3D11Device* device)
{
	m_shaderFilePrefix = particleNamePrefixInShaderFile;
	m_maxParticles = maxParticles;

	if (!loadShaders(device))
	{
		MessageBox(nullptr, TEXT("ParticleSystem: Failed to load shaders!"), TEXT("Error"), MB_OK);
		return false;
	}
	if (!createConstantBuffers(device))
	{
		MessageBox(nullptr, TEXT("ParticleSystem: Failed to load constant buffers!"), TEXT("Error"), MB_OK);
		return false;
	}
	if (!createVertexBuffers(device))
	{
		MessageBox(nullptr, TEXT("ParticleSystem: Failed to load buffers!"), TEXT("Error"), MB_OK);
		return false;
	}
	if (!createRandomTextureSRV(device))
	{
		MessageBox(nullptr, TEXT("ParticleSystem: Failed to create random texture!"), TEXT("Error"), MB_OK);
		return false;
	}
	if(!createTextureSRV(device, textureFile))
	{
		MessageBox(nullptr, TEXT("ParticleSystem: Failed to create texture!"), TEXT("Error"), MB_OK);
		return false;
	}
	
	m_commonStates = std::make_unique<CommonStates>(device);
	m_samplerLinearWrap = m_commonStates->LinearWrap();	
	m_depthStencilRead = m_commonStates->DepthRead();
	m_blendAdd = m_commonStates->Additive();
	return true;
}

void ParticleSystem::Reset()
{
	m_isEmitterPositionSet = false;
	m_isEmitterAlive = false;
}

void ParticleSystem::Update(ID3D11DeviceContext* deviceContext, const float& deltaTime, const float& gameTime, const Camera& camera)
{
	//update constant buffer
	m_cbPerFrame.GameTime = gameTime;
	m_cbPerFrame.DeltaTime = deltaTime;
	m_cbPerFrame.ViewProj = camera.GetView() * camera.GetProj();
	auto camPos = camera.GetPosition();
	m_cbPerFrame.EyePositionWorld = {camPos.x, camPos.y, camPos.z, 0.f};

	deviceContext->UpdateSubresource(m_constantBuffers[CB_PerFrame], 0, nullptr, &m_cbPerFrame, 0, 0);
}

void ParticleSystem::Draw(ID3D11DeviceContext* deviceContext, ID3D11RenderTargetView* renderTarget, ID3D11DepthStencilView* depthStencilView, ID3D11RasterizerState* rasterizerState, const D3D11_VIEWPORT* viewports, const size_t& numOfViewports)
{
	if (!m_isEmitterPositionSet)
		return;

	deviceContext->IASetInputLayout(m_vsInputLayout);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	drawPassEmitter(deviceContext);
	drawPassDraw(deviceContext, renderTarget, depthStencilView, rasterizerState, viewports, numOfViewports);
}

void ParticleSystem::drawPassEmitter(ID3D11DeviceContext* deviceContext)
{
	const UINT stride = sizeof(ParticleShaderInput);
	const UINT offset = 0;

	if (!m_isEmitterAlive)
		deviceContext->IASetVertexBuffers(0, 1, &m_emitterCreationVB, &stride, &offset);
	else
		deviceContext->IASetVertexBuffers(0, 1, &m_emittedParticlesVB, &stride, &offset);

	deviceContext->VSSetShader(m_vsEmit, nullptr, 0);

	deviceContext->GSSetShader(m_gsEmit, nullptr, 0);
	deviceContext->GSSetConstantBuffers(0, 1, &m_constantBuffers[CB_PerFrame]);
	deviceContext->GSSetShaderResources(0, 1, &m_randomTextureSRV);
	deviceContext->GSSetSamplers(0, 1, &m_samplerLinearWrap);

	//stream out updated emitted particles and swap with actual VB afterwards
	deviceContext->SOSetTargets(1, &m_emitterStreamOutVB, &offset);

	//disable the rest for the emitter pass
	deviceContext->RSSetState(nullptr);
	deviceContext->RSSetViewports(0, nullptr);
	deviceContext->PSSetShader(nullptr, nullptr, 0);
	deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
	deviceContext->OMSetDepthStencilState(nullptr, 0);


	if (!m_isEmitterAlive)
	{
		deviceContext->Draw(1, 0);
		m_isEmitterAlive = true;
	}
	else
	{
		deviceContext->DrawAuto();
	}

	// unbind VB
	deviceContext->SOSetTargets(0, nullptr, nullptr);

	//now swap the before emitted particles for the actual draw call
	std::swap(m_emittedParticlesVB, m_emitterStreamOutVB);
}

void ParticleSystem::drawPassDraw(ID3D11DeviceContext* deviceContext, ID3D11RenderTargetView* renderTarget, ID3D11DepthStencilView* depthStencilView, ID3D11RasterizerState* rasterizerState, const D3D11_VIEWPORT* viewports, const size_t& numOfViewports) const
{	
	const UINT stride = sizeof(ParticleShaderInput);
	const UINT offset = 0;

	deviceContext->IASetVertexBuffers(0, 1, &m_emittedParticlesVB, &stride, &offset);

	deviceContext->VSSetShader(m_vsDraw, nullptr, 0);
	deviceContext->VSSetConstantBuffers(0, 1, &m_constantBuffers[CB_PerFrame]);

	deviceContext->GSSetShader(m_gsDraw, nullptr, 0);
	deviceContext->GSSetConstantBuffers(0, 1, &m_constantBuffers[CB_PerFrame]);

	deviceContext->RSSetState(rasterizerState);
	deviceContext->RSSetViewports(numOfViewports, viewports);

	deviceContext->PSSetShader(m_psDraw, nullptr, 0);
	deviceContext->PSSetShaderResources(1, 1, &m_textureSRV);
	deviceContext->PSSetSamplers(0, 1, &m_samplerLinearWrap);

	deviceContext->OMSetRenderTargets(1, &renderTarget, depthStencilView);
	deviceContext->OMSetDepthStencilState(m_depthStencilRead, 1);
	const FLOAT blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	deviceContext->OMSetBlendState(m_blendAdd, blendFactor, 0xffffffff);

	deviceContext->DrawAuto();

	//reset blendstate
	deviceContext->OMSetBlendState(nullptr, {}, 0xffffffff);
}


bool ParticleSystem::loadShaders(ID3D11Device* device)
{
	bool result = true;
	result &= loadInitShaders(device);
	result &= loadDrawShaders(device);
	return result;
}

bool ParticleSystem::loadInitShaders(ID3D11Device* device)
{
	ID3DBlob *vsBlob, *gsBlob;

	std::wstring compiledVSPath = m_shaderFolder + m_shaderFilePrefix + m_shaderFileSuffixes.EmitVS + m_shaderFileSuffixes.Extension;
	std::wstring compiledGSPath = m_shaderFolder + m_shaderFilePrefix + m_shaderFileSuffixes.EmitGSWithSO + m_shaderFileSuffixes.Extension;

	HRESULT HR_VS, HR_GS;
	HR_VS = D3DReadFileToBlob(compiledVSPath.c_str(), &vsBlob);
	HR_GS = D3DReadFileToBlob(compiledGSPath.c_str(), &gsBlob);

	if (FAILED(HR_VS) || FAILED(HR_GS))
		return false;

	HR_VS = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vsEmit);

	D3D11_SO_DECLARATION_ENTRY pDecl[] =
	{
		// stream num, semantic name, semantic index, start component, component count, output slot
		{0, "SV_POSITION", 0, 0, 4, 0},
		{0, "VELOCITY", 0, 0, 4, 0},
		{0, "SIZE", 0, 0, 2, 0},
		{0, "AGE", 0, 0, 1, 0},
		{0, "TYPE", 0, 0, 1, 0}
	};

	HR_GS = device->CreateGeometryShaderWithStreamOutput(gsBlob->GetBufferPointer(), gsBlob->GetBufferSize(), pDecl, _countof(pDecl), NULL, 0, D3D11_SO_NO_RASTERIZED_STREAM, NULL, &m_gsEmit);

	if (FAILED(HR_VS) || FAILED(HR_GS))
		return false;

	std::pair<D3D11_INPUT_ELEMENT_DESC*, size_t> layout;
	layout.first = getParticleInputLayoutDesc(layout.second);
	HR_VS = device->CreateInputLayout(layout.first, layout.second, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_vsInputLayout);
	if (FAILED(HR_VS))
		return false;

	SafeRelease(vsBlob);
	SafeRelease(gsBlob);

	return true;
}

bool ParticleSystem::loadDrawShaders(ID3D11Device* device)
{
	ID3DBlob *vsBlob, *gsBlob, *psBlob;

	std::wstring compiledVSPath = m_shaderFolder + m_shaderFilePrefix + m_shaderFileSuffixes.DrawVS + m_shaderFileSuffixes.Extension;
	std::wstring compiledGSPath = m_shaderFolder + m_shaderFilePrefix + m_shaderFileSuffixes.DrawGS + m_shaderFileSuffixes.Extension;
	std::wstring compiledPSPath = m_shaderFolder + m_shaderFilePrefix + m_shaderFileSuffixes.DrawPS + m_shaderFileSuffixes.Extension;

	HRESULT HR_VS, HR_GS, HR_PS;
	HR_VS = D3DReadFileToBlob(compiledVSPath.c_str(), &vsBlob);
	HR_GS = D3DReadFileToBlob(compiledGSPath.c_str(), &gsBlob);
	HR_PS = D3DReadFileToBlob(compiledPSPath.c_str(), &psBlob);

	if (FAILED(HR_VS) || FAILED(HR_GS) || FAILED(HR_PS))
		return false;

	HR_VS = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vsDraw);
	HR_GS = device->CreateGeometryShader(gsBlob->GetBufferPointer(), gsBlob->GetBufferSize(), nullptr, &m_gsDraw);
	HR_PS = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_psDraw);

	if (FAILED(HR_VS) || FAILED(HR_GS) || FAILED(HR_PS))
		return false;

	std::pair<D3D11_INPUT_ELEMENT_DESC*, size_t> layout;
	layout.first = getParticleInputLayoutDesc(layout.second);
	HR_VS = device->CreateInputLayout(layout.first, layout.second, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_vsInputLayout);
	if (FAILED(HR_VS))
		return false;

	SafeRelease(vsBlob);
	SafeRelease(gsBlob);
	SafeRelease(psBlob);

	return true;
}

D3D11_INPUT_ELEMENT_DESC* ParticleSystem::getParticleInputLayoutDesc(size_t& outCountOfElements)
{
	static D3D11_INPUT_ELEMENT_DESC layoutDesc[] =
	{
		{"SV_POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(ParticleShaderInput,InitialPos), D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"VELOCITY", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(ParticleShaderInput,InitialVel), D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(ParticleShaderInput,Size), D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"AGE", 0, DXGI_FORMAT_R32_FLOAT, 0, offsetof(ParticleShaderInput,LifeTime), D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TYPE", 0, DXGI_FORMAT_R32_UINT, 0, offsetof(ParticleShaderInput,Type), D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
	outCountOfElements = _countof(layoutDesc);
	return layoutDesc;
}

bool ParticleSystem::createConstantBuffers(ID3D11Device* device)
{
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.ByteWidth = sizeof(CbPerFrame);
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.StructureByteStride = 0;
	bufferDesc.MiscFlags = 0;

	HRESULT hr;
	if (FAILED(hr = device->CreateBuffer(&bufferDesc, nullptr, &m_constantBuffers[CB_PerFrame])))
		return false;

	return true;
}

bool ParticleSystem::createVertexBuffers(ID3D11Device* device)
{
	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(D3D11_BUFFER_DESC));
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(ParticleShaderInput) * 1;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// the initial particle emitter has type 0 (emitter) and lifetime 0 - lifetime will be used for emitting
	// the rest of the particle attributes do not apply to an emitter
	ParticleShaderInput p;
	ZeroMemory(&p, sizeof(ParticleShaderInput));
	p.LifeTime = 0.0f;
	p.Type = 0;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = &p;

	HRESULT hr;
	if (FAILED(hr = device->CreateBuffer(&vertexBufferDesc, &initData, &m_emitterCreationVB)))
		return false;

	// double buffering for emitted particles stream-out and drawing
	vertexBufferDesc.ByteWidth = sizeof(ParticleShaderInput) * m_maxParticles;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;

	if (FAILED(hr = device->CreateBuffer(&vertexBufferDesc, NULL, &m_emittedParticlesVB)))
		return false;
	if (FAILED(hr = device->CreateBuffer(&vertexBufferDesc, NULL, &m_emitterStreamOutVB)))
		return false;

	return true;
}

bool ParticleSystem::createRandomTextureSRV(ID3D11Device* device)
{
	std::random_device rd;
	std::mt19937_64 mt(rd());
	std::uniform_real_distribution<float> dist(-1, 1);

	XMFLOAT4 randomValues[1024];
	for (int i = 0; i < 1024; ++i)
	{
		randomValues[i].x = dist(mt);
		randomValues[i].y = dist(mt);
		randomValues[i].z = dist(mt);
		randomValues[i].w = dist(mt);
	}

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = randomValues;
	initData.SysMemPitch = 1024 * sizeof(XMFLOAT4);
	initData.SysMemSlicePitch = 0;

	D3D11_TEXTURE1D_DESC texDesc;
	texDesc.Width = 1024;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.Usage = D3D11_USAGE_IMMUTABLE;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;
	texDesc.ArraySize = 1;

	HRESULT hr;
	ID3D11Texture1D* randomTex = nullptr;
	if (FAILED(hr = device->CreateTexture1D(&texDesc, &initData, &randomTex)))
		return false;

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = texDesc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
	viewDesc.Texture1D.MipLevels = texDesc.MipLevels;
	viewDesc.Texture1D.MostDetailedMip = 0;

	if (FAILED(hr = device->CreateShaderResourceView(randomTex, &viewDesc, &m_randomTextureSRV)))
		return false;

	SafeRelease(randomTex);

	return true;
}

bool ParticleSystem::createTextureSRV(ID3D11Device* device, const std::wstring& filename)
{
	HRESULT hr;
	if (FAILED(hr = DirectX::CreateDDSTextureFromFile(device, filename.c_str(), nullptr, &m_textureSRV)))
		return false;
	
	return true;
}

void ParticleSystem::releaseShaders()
{
	SafeRelease(m_vsEmit);
	SafeRelease(m_gsEmit);
	SafeRelease(m_vsDraw);
	SafeRelease(m_gsDraw);
	SafeRelease(m_psDraw);
	SafeRelease(m_vsInputLayout);
}

void ParticleSystem::releaseConstantBuffers()
{
	for (auto& buffer : m_constantBuffers)
		SafeRelease(buffer);
}

void ParticleSystem::releaseVertexBuffers()
{
	SafeRelease(m_emitterCreationVB);
	SafeRelease(m_emitterStreamOutVB);
	SafeRelease(m_emittedParticlesVB);
}

void ParticleSystem::releaseTextureSRVs()
{
	SafeRelease(m_randomTextureSRV);
	SafeRelease(m_textureSRV);
}