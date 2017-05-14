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
	releaseShaders();
	releaseConstantBuffers();
	releaseVertexBuffer();
	releaseSamplerStates();
}

bool RockVertexBufferGenerator::Initialize(ID3D11Device* device, XMINT3 size)
{
	size = {size.x, size.z, size.y};
	m_densityTextureSize = size;
	m_densityTextureSizeStep = { 2.0f / static_cast<float>(size.x), 2.0f / static_cast<float>(size.y), 2.0f / static_cast<float>(size.z) };
	if (!loadShaders(device))
	{
		MessageBox(nullptr, TEXT("RockVertexBufferGenerator: Failed to load shaders!"), TEXT("Error"), MB_OK);
		return false;
	}
	if (!createConstantBuffers(device))
	{
		MessageBox(nullptr, TEXT("RockVertexBufferGenerator: Failed to load constant buffers!"), TEXT("Error"), MB_OK);
		return false;
	}
	if(!createVertexBuffer(device))
	{
		MessageBox(nullptr, TEXT("RockVertexBufferGenerator: Failed to load buffers!"), TEXT("Error"), MB_OK);
		return false;
	}
	if(!createSamplerStates(device))
	{
		MessageBox(nullptr, TEXT("RockVertexBufferGenerator: Failed to load SamplerState!"), TEXT("Error"), MB_OK);
		return false;
	}
	m_commonStates = std::make_unique<CommonStates>(device);

	return true;
}

bool RockVertexBufferGenerator::Generate(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView* densityTexture3D) const
{	
	UINT vertexStride = sizeof(VertexShaderInput);
	UINT offset = 0;

	deviceContext->IASetInputLayout(m_vertexShaderInputLayout);
	deviceContext->IASetVertexBuffers(0, 1, &m_dummyVertexBuffer, &vertexStride, &offset);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	deviceContext->VSSetShader(m_vertexShader, nullptr, 0);
	deviceContext->VSSetConstantBuffers(0, 1, &m_constantBuffers[Steps]);
	deviceContext->VSSetShaderResources(0, 1, &densityTexture3D);
	deviceContext->VSSetSamplers(0, 1, &m_samplerState);

	deviceContext->GSSetShader(m_geometryShader, nullptr, 0);
	deviceContext->GSSetConstantBuffers(0, 1, &m_constantBuffers[Steps]);
	deviceContext->GSSetConstantBuffers(1, 1, &m_constantBuffers[MarchingCubesLookUpTables]);
	deviceContext->GSSetShaderResources(0, 1, &densityTexture3D);
	deviceContext->GSSetSamplers(0, 1, &m_samplerState);

	deviceContext->RSSetState(nullptr);
	deviceContext->RSSetViewports(0, nullptr);

	deviceContext->PSSetShader(nullptr, nullptr, 0); 

	deviceContext->SOSetTargets(1, &m_vertexBuffer, &offset);

	deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
	deviceContext->OMSetDepthStencilState(nullptr, 0);

	deviceContext->Begin(m_deviceStats);
	deviceContext->DrawInstanced(m_densityTextureSize.x*m_densityTextureSize.z, m_densityTextureSize.y, 0, 0);
	deviceContext->End(m_deviceStats);

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
		{ 0, "NORMAL", 0, 0, 3, 0 },
		{ 0, "NORMAL", 1, 0, 3, 0 },
	};

	HR_GS = device->CreateGeometryShaderWithStreamOutput(gsBlob->GetBufferPointer(), gsBlob->GetBufferSize(), pDecl, _countof(pDecl), NULL, 0, D3D11_SO_NO_RASTERIZED_STREAM, NULL, &m_geometryShader);

	if (FAILED(HR_VS) || FAILED(HR_GS))
		return false;

	// Create the input layout for the vertex shader.
	D3D11_INPUT_ELEMENT_DESC vertexLayoutDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }

	};

	HR_VS = device->CreateInputLayout(vertexLayoutDesc, _countof(vertexLayoutDesc), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_vertexShaderInputLayout);
	if (FAILED(HR_VS))
		return false;

	SafeRelease(vsBlob);
	SafeRelease(gsBlob);
	return true;
}

bool RockVertexBufferGenerator::createConstantBuffers(ID3D11Device* device)
{
	CB_MC_LookUpTables mcLutData;
	memcpy(mcLutData.case_to_numpolys, g_mcCaseToNumpolysLUT, sizeof g_mcCaseToNumpolysLUT);
	memcpy(mcLutData.g_triTable, g_mcTriLUT, sizeof g_mcTriLUT);

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.ByteWidth = sizeof(CB_MC_LookUpTables);
	bufferDesc.CPUAccessFlags = 0; 
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.StructureByteStride = 0;
	bufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA cbInitData;
	cbInitData.pSysMem = &mcLutData;
	cbInitData.SysMemPitch = 0;
	cbInitData.SysMemSlicePitch = 0;
	
	HRESULT hr = device->CreateBuffer(&bufferDesc, &cbInitData, &m_constantBuffers[MarchingCubesLookUpTables]);
	
	if (FAILED(hr))
		return false;

	//Create cornerStep buffer
	bufferDesc.ByteWidth = sizeof(CB_Steps);
	CB_Steps stepBufferData;

	const XMFLOAT3& step = m_densityTextureSizeStep;
	stepBufferData.cornerStep[0] = { 0.0f, 0.0f, 0.0f, 1 };
	stepBufferData.cornerStep[1] = { step.x, 0.0f, 0.0f, 1 };
	stepBufferData.cornerStep[2] = { step.x, step.y, 0.0f, 1 };
	stepBufferData.cornerStep[3] = { 0.0f, step.y, 0.0f, 1 };
	stepBufferData.cornerStep[4] = { 0.0f, 0.0f, step.z, 1 };
	stepBufferData.cornerStep[5] = { step.x, 0.0f, step.z, 1 };
	stepBufferData.cornerStep[6] = { step.x, step.y, step.z, 1 };
	stepBufferData.cornerStep[7] = { 0.0f, step.y, step.z, 1 };
	stepBufferData.dataStep = { 1.0f / (float)m_densityTextureSize.x, 1.f / (float)m_densityTextureSize.y, 1.f / (float)m_densityTextureSize.z, 1.f };


	cbInitData.pSysMem = &stepBufferData;
	hr = device->CreateBuffer(&bufferDesc, &cbInitData, &m_constantBuffers[Steps]);

	if (FAILED(hr))
		return false;

	return true;
}

bool RockVertexBufferGenerator::createVertexBuffer(ID3D11Device* device)
{
	m_dummyVertexCount = createDummyVertices(&m_dummyVertices);
	
	// Create an initialize the vertex buffer.
	D3D11_BUFFER_DESC dummyVBDesc;
	ZeroMemory(&dummyVBDesc, sizeof(D3D11_BUFFER_DESC));
	dummyVBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	dummyVBDesc.ByteWidth = sizeof(VertexShaderInput) * m_dummyVertexCount;
	dummyVBDesc.CPUAccessFlags = 0;
	dummyVBDesc.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA resourceData;
	ZeroMemory(&resourceData, sizeof(D3D11_SUBRESOURCE_DATA));
	resourceData.pSysMem = m_dummyVertices;

	HRESULT hr = device->CreateBuffer(&dummyVBDesc, &resourceData, &m_dummyVertexBuffer);
	if (FAILED(hr))
		return false;

	// Create an initialize the vertex buffer.
	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(D3D11_BUFFER_DESC));
	vertexBufferDesc.BindFlags = D3D11_BIND_STREAM_OUTPUT | D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.ByteWidth = sizeof(GeometryShaderOutput) * m_densityTextureSize.x*m_densityTextureSize.z* m_densityTextureSize.y *18; //max 15 vertices per case!
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	hr = device->CreateBuffer(&vertexBufferDesc, NULL, &m_vertexBuffer);

	if (FAILED(hr))
		return false;

	//created staging vertex buffer for reading buffer on the cpu
	vertexBufferDesc.BindFlags = 0;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	vertexBufferDesc.Usage = D3D11_USAGE_STAGING;

	if (FAILED(hr = device->CreateBuffer(&vertexBufferDesc, NULL, &m_stagedVertexBuffer)))
		return false;

	//create devicestats for querying size of stream output
	D3D11_QUERY_DESC queryDesc;
	queryDesc.Query = D3D11_QUERY_PIPELINE_STATISTICS;
	queryDesc.MiscFlags = 0;
	if (FAILED(hr = device->CreateQuery(&queryDesc, &m_deviceStats)))
		return false;

	return true;
}

bool RockVertexBufferGenerator::createSamplerStates(ID3D11Device* device)
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
	
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(D3D11_SAMPLER_DESC));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	//sampDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	//sampDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = 0;

	return true;
}


int RockVertexBufferGenerator::createDummyVertices(VertexShaderInput** outVertices) const
{
	int size = (m_densityTextureSize.x + 1) * (m_densityTextureSize.z + 1);

	*outVertices = new VertexShaderInput[size];
	int index = 0;
	for (float z = -1; z < 1.0f; z += m_densityTextureSizeStep.z)
	{
		for (float x = -1; x < 1.0f; x += m_densityTextureSizeStep.x)
		{
			(*outVertices)[index] = { XMFLOAT4(x, 1, z, 1) };
			index++;
		}
	}

	return size;
}

std::vector<Triangle> RockVertexBufferGenerator::extractTrianglesFromVertexBuffer(ID3D11DeviceContext* pDeviceContext, const Matrix& triangleSRT) const
{
	//see https://www.gamedev.net/blog/272/entry-1913400-using-d3d11-stream-out-for-debugging/
	std::vector<Triangle> triangles;
	pDeviceContext->CopyResource(m_stagedVertexBuffer, m_vertexBuffer);

	D3D11_MAPPED_SUBRESOURCE data;
	if (SUCCEEDED(pDeviceContext->Map(m_stagedVertexBuffer, 0, D3D11_MAP_READ, 0, &data)))
	{
		GeometryShaderOutput *pRaw = reinterpret_cast< GeometryShaderOutput*>(data.pData);

		D3D11_QUERY_DATA_PIPELINE_STATISTICS stats;
		while (S_OK != pDeviceContext->GetData(m_deviceStats, &stats, m_deviceStats->GetDataSize(), 0));
		
		for (size_t i = 0; i < stats.GSPrimitives*3;)
		{
			bool notANumberDetected = false;
			Vector3 vertices[3];
			for(auto& vertex : vertices)
			{
				if (pRaw[i].LocalPosition.x != pRaw[i].LocalPosition.x || pRaw[i].LocalPosition.y != pRaw[i].LocalPosition.y || pRaw[i].LocalPosition.z != pRaw[i].LocalPosition.z)
					notANumberDetected = true;

				vertex = Vector3(pRaw[i].LocalPosition.x, pRaw[i].LocalPosition.y, pRaw[i].LocalPosition.z);
				vertex = XMVector3Transform(vertex, triangleSRT);
				++i;
			}

			if(!notANumberDetected)
				triangles.push_back(Triangle(vertices[0], vertices[1], vertices[2]));
		}

		pDeviceContext->Unmap(m_stagedVertexBuffer, 0);
	}

	return triangles;
}

void RockVertexBufferGenerator::releaseSamplerStates()
{
	SafeRelease(m_samplerState);
}

void RockVertexBufferGenerator::releaseVertexBuffer()
{
	SafeRelease(m_vertexBuffer);
	SafeRelease(m_dummyVertexBuffer);
	SafeRelease(m_stagedVertexBuffer);
	SafeRelease(m_deviceStats);
	delete[] m_dummyVertices;
}

void RockVertexBufferGenerator::releaseConstantBuffers()
{
	for (auto a : m_constantBuffers)
		SafeRelease(a);
}

void RockVertexBufferGenerator::releaseShaders()
{
	SafeRelease(m_vertexShader);
	SafeRelease(m_vertexShaderInputLayout);
	SafeRelease(m_geometryShader);
}
