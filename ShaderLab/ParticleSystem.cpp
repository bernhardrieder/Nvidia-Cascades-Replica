#include "stdafx.h"
#include "ParticleSystem.h"


ParticleSystem::ParticleSystem()
{
	mFirstRun = true;
	mGameTime = 0.0f;
	mTimeStep = 0.0f;
	mAge = 0.0f;

	mEyePosW = Vector3(0.0f, 0.0f, 0.0f);
	mEmitPosW = Vector3(0.0f, 0.0f, 0.0f);
	mEmitDirW = Vector3(0.0f, 1.0f, 0.0f);
}

ParticleSystem::~ParticleSystem()
{
	releaseShaders();
	releaseConstantBuffers();
	releaseVertexBuffers();
}

void ParticleSystem::SetEyePos(const Vector3& eyePosW)
{
	mEyePosW = eyePosW;
}

void ParticleSystem::SetEmitPos(const Vector3& emitPosW)
{
	mEmitPosW = emitPosW;
}

void ParticleSystem::SetEmitDir(const Vector3& emitDirW)
{
	mEmitDirW = emitDirW;
}

bool ParticleSystem::Initialize(const std::wstring& particleNamePrefixInShaderFile, ID3D11Device* device, ID3D11ShaderResourceView* texArraySRV, ID3D11ShaderResourceView* randomTexSRV, unsigned maxParticles)
{
	m_shaderFilePrefix = particleNamePrefixInShaderFile;
	mMaxParticles = maxParticles;
	mTexArraySRV = texArraySRV;
	mRandomTexSRV = randomTexSRV;

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

	return true;
}

void ParticleSystem::Reset()
{
	mFirstRun = true;
	mAge = 0.0f;
}

void ParticleSystem::Update(float dt, float gameTime)
{
	mGameTime = gameTime;
	mTimeStep = dt;

	mAge += dt;
}

void ParticleSystem::Draw(ID3D11DeviceContext* dc, const Camera& cam)
{
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

	std::wstring compiledVSPath = m_shaderFilePrefix + m_shaderFileSuffixes.InitVS;
	std::wstring compiledGSPath = m_shaderFilePrefix + m_shaderFileSuffixes.InitGSWithSO;

	HRESULT HR_VS, HR_GS;
	HR_VS = D3DReadFileToBlob(compiledVSPath.c_str(), &vsBlob);
	HR_GS = D3DReadFileToBlob(compiledGSPath.c_str(), &gsBlob);

	if (FAILED(HR_VS) || FAILED(HR_GS))
		return false;

	HR_VS = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vsInit);

	D3D11_SO_DECLARATION_ENTRY pDecl[] =
	{
		// stream num, semantic name, semantic index, start component, component count, output slot
		{ 0, "SV_POSITION", 0, 0, 4, 0 },
		{ 0, "VELOCITY", 0, 0, 3, 0 },
		{ 0, "SIZE", 0, 0, 2, 0 },
		{ 0, "AGE", 0, 0, 1, 0 },
		{ 0, "TYPE", 0, 0, 1, 0 }
	};

	HR_GS = device->CreateGeometryShaderWithStreamOutput(gsBlob->GetBufferPointer(), gsBlob->GetBufferSize(), pDecl, _countof(pDecl), NULL, 0, D3D11_SO_NO_RASTERIZED_STREAM, NULL, &m_gsInit);

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

	std::wstring compiledVSPath = m_shaderFilePrefix + m_shaderFileSuffixes.DrawVS;
	std::wstring compiledGSPath = m_shaderFilePrefix + m_shaderFileSuffixes.DrawGS;
	std::wstring compiledPSPath = m_shaderFilePrefix + m_shaderFileSuffixes.DrawPS;

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
		{ "SV_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Particle,InitialPos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "VELOCITY", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Particle,InitialVel), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Particle,Size), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "AGE", 0, DXGI_FORMAT_R32_FLOAT, 0, offsetof(Particle,Age), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TYPE", 0, DXGI_FORMAT_R32_UINT, 0, offsetof(Particle,Type), D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	outCountOfElements = _countof(layoutDesc);
	return layoutDesc;
}

void ParticleSystem::releaseShaders()
{
	SafeRelease(m_vsInit);
	SafeRelease(m_gsInit);
	SafeRelease(m_vsDraw);
	SafeRelease(m_gsDraw);
	SafeRelease(m_psDraw);
	SafeRelease(m_vsInputLayout);
}

//todo
bool ParticleSystem::createConstantBuffers(ID3D11Device* device)
{
}

//todo
void ParticleSystem::releaseConstantBuffers()
{
}

bool ParticleSystem::createVertexBuffers(ID3D11Device* device)
{
	//
	// Create the buffer to kick-off the particle system.
	//

	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(D3D11_BUFFER_DESC));
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(Particle) * 1;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// The initial particle emitter has type 0 and age 0.  The rest
	// of the particle attributes do not apply to an emitter.
	Particle p;
	ZeroMemory(&p, sizeof(Particle));
	p.Age = 0.0f;
	p.Type = 0;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = &p;

	if (FAILED(device->CreateBuffer(&vertexBufferDesc, &initData, &mInitVB)))
		return false;

	//
	// Create the ping-pong buffers for stream-out and drawing.
	//
	vertexBufferDesc.ByteWidth = sizeof(Particle) * mMaxParticles;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;

	if (FAILED(device->CreateBuffer(&vertexBufferDesc, NULL, &mDrawVB)))
		return false;
	if (FAILED(device->CreateBuffer(&vertexBufferDesc, NULL, &mStreamOutVB)))
		return false;

	return true;
}

void ParticleSystem::releaseVertexBuffers()
{
	SafeRelease(mInitVB);
	SafeRelease(mDrawVB);
	SafeRelease(mStreamOutVB);
}
