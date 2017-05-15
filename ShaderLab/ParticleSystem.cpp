#include "stdafx.h"
#include "ParticleSystem.h"


ParticleSystem::ParticleSystem()
{
	mFirstRun = true;
	m_cbPerFrame.GameTime = 0.0f;
	m_cbPerFrame.DeltaTime = 0.0f;
	mAge = 0.0f;

	m_cbPerFrame.EyePosW = {0.0f, 0.0f, 0.0f, 0.f};
	m_cbPerFrame.EmitPosW = {0.0f, 0.0f, 0.0f, 0.f};
	m_cbPerFrame.EmitDirW = {0.0f, 1.0f, 0.0f, 0.f};

}

ParticleSystem::~ParticleSystem()
{
	releaseShaders();
	releaseConstantBuffers();
	releaseVertexBuffers();
	releaseRandomTexture(); 
	releaseTextureArray();
}

//void ParticleSystem::SetEyePos(const XMFLOAT4& eyePosW)
//{
//	m_cbPerFrame.EyePosW = eyePosW;
//}

void ParticleSystem::SetEmitPos(const XMFLOAT3& emitPosW)
{
	m_cbPerFrame.EmitPosW = { emitPosW.x, emitPosW.y, emitPosW.z, 0.f };
	m_emitPosSet = true;
}

void ParticleSystem::SetEmitDir(const XMFLOAT3& emitDirW)
{
	m_cbPerFrame.EmitDirW = { emitDirW.x, emitDirW.y, emitDirW.z, 0.f };
}

bool ParticleSystem::Initialize(const std::wstring& particleNamePrefixInShaderFile, ID3D11Device* device, ID3D11DeviceContext* context, const std::vector<std::wstring>& filenamesOfUniformTextures, unsigned maxParticles)
{
	m_shaderFilePrefix = particleNamePrefixInShaderFile;
	mMaxParticles = maxParticles;

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
	if (!createRandomTexture(device))
	{
		MessageBox(nullptr, TEXT("ParticleSystem: Failed to create random texture!"), TEXT("Error"), MB_OK);
		return false;
	}
	if(!createTextureArray(device, context, filenamesOfUniformTextures))
	{
		MessageBox(nullptr, TEXT("ParticleSystem: Failed to create texture array!"), TEXT("Error"), MB_OK);
		return false;
	}

	m_commonStates = std::make_unique<CommonStates>(device);
	return true;
}

void ParticleSystem::Reset()
{
	m_emitPosSet = false;
	mFirstRun = true;
	mAge = 0.0f;
}

void ParticleSystem::Update(ID3D11DeviceContext* deviceContext, const float& dt, const float& gameTime, const Camera& camera)
{
	m_cbPerFrame.GameTime = gameTime;
	m_cbPerFrame.DeltaTime = dt;
	m_cbPerFrame.ViewProj = camera.GetView() * camera.GetProj();
	auto camPos = camera.GetPosition();
	m_cbPerFrame.EyePosW = {camPos.x, camPos.y, camPos.z, 0.f};

	deviceContext->UpdateSubresource(m_constantBuffers[PerFrame], 0, nullptr, &m_cbPerFrame, 0, 0);

	mAge += dt;
}

void ParticleSystem::Draw(ID3D11DeviceContext* dc, ID3D11RenderTargetView* renderTarget)
{
	if (!m_emitPosSet)
		return;

	dc->IASetInputLayout(m_vsInputLayout);
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	UINT stride = sizeof(Particle);
	UINT offset = 0;

	// On the first pass, use the initialization VB.  Otherwise, use
	// the VB that contains the current particle list.
	if (mFirstRun)
		dc->IASetVertexBuffers(0, 1, &mInitVB, &stride, &offset);
	else
		dc->IASetVertexBuffers(0, 1, &mDrawVB, &stride, &offset);

	//
	// Draw the current particle list using stream-out only to update them.  
	// The updated vertices are streamed-out to the target VB. 
	//
	dc->SOSetTargets(1, &mStreamOutVB, &offset);

	/*
	technique11 StreamOutTech
	{
		pass P0
		{
			SetVertexShader( CompileShader( vs_5_0, StreamOutVS() ) );
			SetGeometryShader( gsStreamOut );

			// disable pixel shader for stream-out only
			SetPixelShader(NULL);

			// we must also disable the depth buffer for stream-out only
			SetDepthStencilState( DisableDepth, 0 );
		}
	}
	*/
	dc->VSSetShader(m_vsInit, nullptr, 0);

	dc->GSSetShader(m_gsInit, nullptr, 0);
	dc->GSSetConstantBuffers(0, 1, &m_constantBuffers[PerFrame]);
	dc->GSSetShaderResources(0, 1, &mRandomTexSRV);
	
	//dc->RSSetState(nullptr);
	//dc->RSSetViewports(0, nullptr);
	//dc->PSSetShader(nullptr, nullptr, 0);
	//dc->OMSetRenderTargets(0, nullptr, nullptr);
	//dc->OMSetDepthStencilState(nullptr, 0);


	auto depthNone = m_commonStates->DepthNone();
	dc->OMSetDepthStencilState(depthNone, 1);

	if (mFirstRun)
	{
		dc->Draw(1, 0);
		mFirstRun = false;
	}
	else
	{
		dc->DrawAuto();
	}

	// done streaming-out--unbind the vertex buffer
	ID3D11Buffer* bufferArray[1] = { 0 };
	dc->SOSetTargets(1, bufferArray, &offset);

	// ping-pong the vertex buffers
	std::swap(mDrawVB, mStreamOutVB);


	//
	// Draw the updated particle system we just streamed-out. 
	//
	dc->IASetVertexBuffers(0, 1, &mDrawVB, &stride, &offset);
	/*
	technique11 DrawTech
	{
		pass P0
		{
			SetVertexShader(   CompileShader( vs_5_0, DrawVS() ) );
			SetGeometryShader( CompileShader( gs_5_0, DrawGS() ) );
			SetPixelShader(    CompileShader( ps_5_0, DrawPS() ) );

			SetBlendState(AdditiveBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xffffffff);
			SetDepthStencilState( NoDepthWrites, 0 );
		}
	}
	*/

	dc->VSSetShader(m_vsDraw, nullptr, 0);
	dc->VSSetConstantBuffers(0, 1, &m_constantBuffers[PerFrame]);

	dc->GSSetShader(m_gsDraw, nullptr, 0);
	dc->GSSetConstantBuffers(0, 1, &m_constantBuffers[PerFrame]);

	dc->PSSetShader(m_psDraw, nullptr, 0);
	//dc->PSSetShaderResources(1, 1, &mTexArraySRV);
	dc->PSSetShaderResources(1, 1, &m_TexSRV);

	auto depthRead = m_commonStates->DepthRead();
	dc->OMSetRenderTargets(1, &renderTarget, nullptr);
	dc->OMSetDepthStencilState(depthRead, 1);

	auto blendAdd = m_commonStates->Additive();
	FLOAT blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	dc->OMSetBlendState(blendAdd, blendFactor, 0xffffffff);

	//todo: set rendertarget????????
	dc->DrawAuto();
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

	std::wstring compiledVSPath = m_shaderFolder + m_shaderFilePrefix + m_shaderFileSuffixes.InitVS + m_shaderFileSuffixes.Extension;
	std::wstring compiledGSPath = m_shaderFolder + m_shaderFilePrefix + m_shaderFileSuffixes.InitGSWithSO + m_shaderFileSuffixes.Extension;

	HRESULT HR_VS, HR_GS;
	HR_VS = D3DReadFileToBlob(compiledVSPath.c_str(), &vsBlob);
	HR_GS = D3DReadFileToBlob(compiledGSPath.c_str(), &gsBlob);

	if (FAILED(HR_VS) || FAILED(HR_GS))
		return false;

	HR_VS = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vsInit);

	D3D11_SO_DECLARATION_ENTRY pDecl[] =
	{
		// stream num, semantic name, semantic index, start component, component count, output slot
		{0, "SV_POSITION", 0, 0, 4, 0},
		{0, "VELOCITY", 0, 0, 4, 0},
		{0, "SIZE", 0, 0, 2, 0},
		{0, "AGE", 0, 0, 1, 0},
		{0, "TYPE", 0, 0, 1, 0}
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
		{"SV_POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Particle,InitialPos), D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"VELOCITY", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Particle,InitialVel), D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Particle,Size), D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"AGE", 0, DXGI_FORMAT_R32_FLOAT, 0, offsetof(Particle,Age), D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TYPE", 0, DXGI_FORMAT_R32_UINT, 0, offsetof(Particle,Type), D3D11_INPUT_PER_VERTEX_DATA, 0}
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

	if (FAILED(device->CreateBuffer(&bufferDesc, nullptr, &m_constantBuffers[PerFrame])))
		return false;

	return true;
}

void ParticleSystem::releaseConstantBuffers()
{
	for (auto& buffer : m_constantBuffers)
		SafeRelease(buffer);
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

bool ParticleSystem::createRandomTexture(ID3D11Device* device)
{
	//code from frank luna book - but its dx11 code in general
	XMFLOAT4 randomValues[1024];

	for (int i = 0; i < 1024; ++i)
	{
		randomValues[i].x = MathHelper::RandF(-1.0f, 1.0f);
		randomValues[i].y = MathHelper::RandF(-1.0f, 1.0f);
		randomValues[i].z = MathHelper::RandF(-1.0f, 1.0f);
		randomValues[i].w = MathHelper::RandF(-1.0f, 1.0f);
	}

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = randomValues;
	initData.SysMemPitch = 1024 * sizeof(XMFLOAT4);
	initData.SysMemSlicePitch = 0;

	// Create the texture.
	D3D11_TEXTURE1D_DESC texDesc;
	texDesc.Width = 1024;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.Usage = D3D11_USAGE_IMMUTABLE;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;
	texDesc.ArraySize = 1;

	ID3D11Texture1D* randomTex = 0;
	if (FAILED(device->CreateTexture1D(&texDesc, &initData, &randomTex)))
		return false;

	// Create the resource view.
	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = texDesc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
	viewDesc.Texture1D.MipLevels = texDesc.MipLevels;
	viewDesc.Texture1D.MostDetailedMip = 0;

	if (FAILED(device->CreateShaderResourceView(randomTex, &viewDesc, &mRandomTexSRV)))
		return false;

	SafeRelease(randomTex);

	return true;
}

void ParticleSystem::releaseRandomTexture()
{
	SafeRelease(mRandomTexSRV);
}

bool ParticleSystem::createTextureArray(ID3D11Device* device, ID3D11DeviceContext* context, const std::vector<std::wstring>& filenames)
{
	HRESULT hr;
	UINT size = filenames.size();

	std::vector<ID3D11Texture2D*> srcTex(size);
	for (size_t i = 0; i < size; ++i)
		if (FAILED(hr = DirectX::CreateDDSTextureFromFile(device, filenames[i].c_str(), reinterpret_cast<ID3D11Resource**>(&srcTex[i]), &m_TexSRV)))
			return false;
	
	
	//
	// Create the texture array.  Each element in the texture 
	// array has the same format/dimensions.
	//

	//D3D11_TEXTURE2D_DESC texElementDesc;
	//srcTex[0]->GetDesc(&texElementDesc);

	//D3D11_TEXTURE2D_DESC texArrayDesc;
	//texArrayDesc.Width = texElementDesc.Width;
	//texArrayDesc.Height = texElementDesc.Height;
	//texArrayDesc.MipLevels = texElementDesc.MipLevels;
	//texArrayDesc.ArraySize = size;
	//texArrayDesc.Format = texElementDesc.Format;
	//texArrayDesc.SampleDesc.Count = 1;
	//texArrayDesc.SampleDesc.Quality = 0;
	//texArrayDesc.Usage = D3D11_USAGE_DEFAULT;
	//texArrayDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	//texArrayDesc.CPUAccessFlags = 0;
	//texArrayDesc.MiscFlags = 0;


	//ID3D11Texture2D* texArray = 0;
	//if (FAILED(hr = device->CreateTexture2D(&texArrayDesc, NULL, &texArray)))
	//	return false;

	////
	//// Copy individual texture elements into texture array.
	////

	//// for each texture element...
	//for (UINT texElement = 0; texElement < size; ++texElement)
	//{
	//	// for each mipmap level...
	//	for (UINT mipLevel = 0; mipLevel < texElementDesc.MipLevels; ++mipLevel)
	//	{
	//		D3D11_MAPPED_SUBRESOURCE mappedTex2D;
	//		if (FAILED(hr = context->Map(srcTex[texElement], mipLevel, D3D11_MAP_READ, 0, &mappedTex2D)))
	//			return false;

	//		context->UpdateSubresource(texArray,
	//			D3D11CalcSubresource(mipLevel, texElement, texElementDesc.MipLevels),
	//			0, mappedTex2D.pData, mappedTex2D.RowPitch, mappedTex2D.DepthPitch);

	//		context->Unmap(srcTex[texElement], mipLevel);
	//	}
	//}

	//// Create a resource view to the texture array.
	//D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	//viewDesc.Format = texArrayDesc.Format;
	//viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	//viewDesc.Texture2DArray.MostDetailedMip = 0;
	//viewDesc.Texture2DArray.MipLevels = texArrayDesc.MipLevels;
	//viewDesc.Texture2DArray.FirstArraySlice = 0;
	//viewDesc.Texture2DArray.ArraySize = size;

	//if (FAILED(hr = device->CreateShaderResourceView(texArray, &viewDesc, &mTexArraySRV)))
	//	return false;

	//// Cleanup
	//SafeRelease(texArray);
	//for (UINT i = 0; i < size; ++i)
	//	SafeRelease(srcTex[i]);

	return true;
}

void ParticleSystem::releaseTextureArray()
{
	//SafeRelease(mTexArraySRV);
	SafeRelease(m_TexSRV);
}
