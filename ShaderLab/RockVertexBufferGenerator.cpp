#include "stdafx.h"
#include "RockVertexBufferGenerator.h"
#include "LookUpTables.h"

//see https://msdn.microsoft.com/en-us/library/windows/desktop/bb205122(v=vs.85).aspx for geometry shader with stream output
// https://www.gamedev.net/blog/272/entry-1913400-using-d3d11-stream-out-for-debugging/

RockVertexBufferGenerator::RockVertexBufferGenerator()
{
}

RockVertexBufferGenerator::~RockVertexBufferGenerator()
{
	unloadShaders();
	unloadConstantBuffers();
	unloadVertexBuffer();
	unloadSamplerStates();
}

bool RockVertexBufferGenerator::Initialize(ID3D11Device* device)
{
	if (!loadShaders(device))
	{
		MessageBox(nullptr, TEXT("RockVertexBufferGenerator: Failed to load shaders!"), TEXT("Error"), MB_OK);
		return false;
	}
	if (!loadConstantBuffers(device))
	{
		MessageBox(nullptr, TEXT("RockVertexBufferGenerator: Failed to load constant buffers!"), TEXT("Error"), MB_OK);
		return false;
	}
	if(!loadVertexBuffer(device))
	{
		MessageBox(nullptr, TEXT("RockVertexBufferGenerator: Failed to load buffers!"), TEXT("Error"), MB_OK);
		return false;
	}
	if(!loadSamplerStates(device))
	{
		MessageBox(nullptr, TEXT("RockVertexBufferGenerator: Failed to load SamplerState!"), TEXT("Error"), MB_OK);
		return false;
	}
	m_commonStates = std::make_unique<CommonStates>(device);

	return true;
}

bool RockVertexBufferGenerator::Generate(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView* densityTexture3D) const
{
	CB_SliceInfo cb;
	//float step = 1.f / 256.f;
	//for (int i = 0; i < 256; ++i)
	//	cb.slice_world_space_Y_coord[i] = { 0.f, step*i, 0.f, 0.f };
	cb.g_depthStep = { test_depthStep, test_depthStep, test_depthStep, test_depthStep };
	cb.g_heightStep = { test_heightStep, test_heightStep, test_heightStep, test_heightStep };

	deviceContext->UpdateSubresource(m_constantBuffers[SliceInfos], 0, nullptr, &cb, 0, 0);

	const UINT vertexStride = sizeof(VertexShaderInput);
	UINT offset = 0;

	deviceContext->IASetInputLayout(m_vsInputLayout);
	deviceContext->IASetVertexBuffers(0, 1, &m_dummyVertexBuffer, &vertexStride, &offset);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	deviceContext->VSSetShader(m_vertexShader, nullptr, 0);
	deviceContext->VSSetConstantBuffers(0, 1, &m_constantBuffers[SliceInfos]);
	deviceContext->VSSetShaderResources(0, 1, &densityTexture3D);
	deviceContext->VSSetSamplers(0, 1, &m_samplerState);

	deviceContext->GSSetShader(m_geometryShader, nullptr, 0);
	deviceContext->GSSetConstantBuffers(0, 2, m_constantBuffers);
	deviceContext->GSSetShaderResources(0, 1, &densityTexture3D);
	deviceContext->GSSetSamplers(0, 1, &m_samplerState);
	deviceContext->RSSetState(nullptr);
	deviceContext->RSSetViewports(0, nullptr);

	deviceContext->PSSetShader(nullptr, nullptr, 0); 

	deviceContext->SOSetTargets(1, &m_vertexBuffer, &offset);

	deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
	deviceContext->OMSetDepthStencilState(nullptr, 0);

	deviceContext->DrawInstanced(96*96, m_maxRenderedSlices, 0, 0);

	/** RESET */
	deviceContext->VSSetShader(nullptr, nullptr, 0);
	deviceContext->GSSetShader(nullptr, nullptr, 0);
	deviceContext->PSSetShader(nullptr, nullptr, 0);
	//decouple vertexbuffer from SO
	deviceContext->SOSetTargets(0, nullptr, nullptr);

	return true;
}

ID3D11Buffer* RockVertexBufferGenerator::GetVertexBuffer() const
{
	return m_vertexBuffer;
}

bool RockVertexBufferGenerator::loadShaders(ID3D11Device* device)
{
	ID3DBlob *vsBlob, *gsBlob;

	HRESULT HR_VS, HR_GS;
	HR_VS = D3DReadFileToBlob(m_compiledVSPath, &vsBlob);
	HR_GS = D3DReadFileToBlob(m_compiledGSPath, &gsBlob);

	if (FAILED(HR_VS) || FAILED(HR_GS))
		return false;

	HR_VS = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vertexShader);
	
	D3D11_SO_DECLARATION_ENTRY pDecl[] =
	{
		// stream num, semantic name, semantic index, start component, component count, output slot
		{ 0, "SV_POSITION", 0, 0, 4, 0 },   
		{ 0, "NORMAL", 0, 0, 3, 0 }
	};

	HR_GS = device->CreateGeometryShaderWithStreamOutput(gsBlob->GetBufferPointer(), gsBlob->GetBufferSize(), pDecl, _countof(pDecl), NULL, 0, D3D11_SO_NO_RASTERIZED_STREAM, NULL, &m_geometryShader);

	if (FAILED(HR_VS) || FAILED(HR_GS))
		return false;

	// Create the input layout for the vertex shader.
	D3D11_INPUT_ELEMENT_DESC vertexLayoutDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }

	};

	HR_VS = device->CreateInputLayout(vertexLayoutDesc, _countof(vertexLayoutDesc), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_vsInputLayout);
	if (FAILED(HR_VS))
		return false;

	SafeRelease(vsBlob);
	SafeRelease(gsBlob);
	return true;
}

bool RockVertexBufferGenerator::loadConstantBuffers(ID3D11Device* device)
{
	CB_MC_LUT_1 mcLut1Data;
	memcpy(mcLut1Data.case_to_numpolys, g_mcCaseToNumpolysLUT, sizeof g_mcCaseToNumpolysLUT);
	memcpy(mcLut1Data.cornerAmask0123, g_mcCornerAmask0123LUT, sizeof g_mcCornerAmask0123LUT);
	memcpy(mcLut1Data.cornerAmask4567, g_mcCornerAmask4567LUT, sizeof g_mcCornerAmask4567LUT);
	memcpy(mcLut1Data.cornerBmask0123, g_mcCornerBmask0123LUT, sizeof g_mcCornerBmask0123LUT);
	memcpy(mcLut1Data.cornerBmask4567, g_mcCornerBmask4567LUT, sizeof g_mcCornerBmask4567LUT);
	memcpy(mcLut1Data.vec_start, g_mcVecStartLUT, sizeof g_mcVecStartLUT);
	memcpy(mcLut1Data.vec_dir, g_mcVecDirLUT, sizeof g_mcVecDirLUT);

	CB_MC_LUT_2 mcLut2Data;
	memcpy(mcLut2Data.g_triTable, g_mcTriLUT, sizeof g_mcTriLUT);

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	//bufferDesc.ByteWidth = 5248; // from shader errors!
	bufferDesc.ByteWidth = sizeof(CB_MC_LUT_1);
	bufferDesc.CPUAccessFlags = 0; 
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.StructureByteStride = 0;
	bufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA cbInitData;
	cbInitData.pSysMem = &mcLut1Data;
	cbInitData.SysMemPitch = 0;
	cbInitData.SysMemSlicePitch = 0;
	
	HRESULT hrCB1 = device->CreateBuffer(&bufferDesc, &cbInitData, &m_constantBuffers[mc_lut_1]);
	
	bufferDesc.ByteWidth = sizeof(CB_MC_LUT_2);
	cbInitData.pSysMem = &mcLut2Data;
	HRESULT hrCB2 = device->CreateBuffer(&bufferDesc, &cbInitData, &m_constantBuffers[mc_lut_2]);
	

	//bufferDesc.ByteWidth = 4096; // from shader error
	bufferDesc.ByteWidth = sizeof(CB_SliceInfo);
	HRESULT hrCB3 = device->CreateBuffer(&bufferDesc, nullptr, &m_constantBuffers[SliceInfos]);
	
	if (FAILED(hrCB1) || FAILED(hrCB2) || FAILED (hrCB3))
		return false;

	return true;
}

bool RockVertexBufferGenerator::loadVertexBuffer(ID3D11Device* device)
{
	float step = 1.f/95.0f;
	int currentIndex = 0;
	int maxIndex = 96 * 96;

	for (float x = 0.f; x <= 1.f && currentIndex != maxIndex; x += step)
	{
		for(float y = 0.f; y <= 1.f && currentIndex != maxIndex; y += step)
		{
			m_dummyVertices[currentIndex++].UV = {x,y, 0.f};
			if(currentIndex == maxIndex)
				m_dummyVertices[currentIndex-1].UV = { 1.f, 1.f, 0.f };
		}
	}

	// Create an initialize the vertex buffer.
	D3D11_BUFFER_DESC dummyVBDesc;
	ZeroMemory(&dummyVBDesc, sizeof(D3D11_BUFFER_DESC));
	dummyVBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	dummyVBDesc.ByteWidth = sizeof(VertexShaderInput) * _countof(m_dummyVertices);
	dummyVBDesc.CPUAccessFlags = 0;
	dummyVBDesc.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA resourceData;
	ZeroMemory(&resourceData, sizeof(D3D11_SUBRESOURCE_DATA));
	resourceData.pSysMem = m_dummyVertices;
	resourceData.SysMemPitch = 0;
	resourceData.SysMemSlicePitch = 0;

	HRESULT hr = device->CreateBuffer(&dummyVBDesc, &resourceData, &m_dummyVertexBuffer);
	if (FAILED(hr))
		return false;

	// Create an initialize the vertex buffer.
	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(D3D11_BUFFER_DESC));
	vertexBufferDesc.BindFlags = D3D11_BIND_STREAM_OUTPUT | D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.ByteWidth = sizeof(GeometryShaderOutput) * 96*96* m_maxRenderedSlices *15; //max 15 vertices per case!
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	hr = device->CreateBuffer(&vertexBufferDesc, NULL, &m_vertexBuffer);

	if (FAILED(hr))
		return false;

	return true;
}

bool RockVertexBufferGenerator::loadSamplerStates(ID3D11Device* device)
{
	D3D11_SAMPLER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_SAMPLER_DESC));
	desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

	//from dxtk commonstates
	desc.MaxAnisotropy = (device->GetFeatureLevel() > D3D_FEATURE_LEVEL_9_1) ? D3D11_MAX_MAXANISOTROPY : 2;
	desc.MaxLOD = FLT_MAX;
	desc.ComparisonFunc = D3D11_COMPARISON_NEVER;

	HRESULT hr = device->CreateSamplerState(&desc, &m_samplerState);
	if (FAILED(hr))
		return false;
	
	return true;
}

void RockVertexBufferGenerator::unloadSamplerStates()
{
	SafeRelease(m_samplerState);
}

void RockVertexBufferGenerator::unloadVertexBuffer()
{
	SafeRelease(m_vertexBuffer);
}

void RockVertexBufferGenerator::unloadConstantBuffers()
{
	for (auto a : m_constantBuffers)
		SafeRelease(a);
}

void RockVertexBufferGenerator::unloadShaders()
{
	SafeRelease(m_vertexShader);
	SafeRelease(m_vsInputLayout);
	SafeRelease(m_geometryShader);
}
