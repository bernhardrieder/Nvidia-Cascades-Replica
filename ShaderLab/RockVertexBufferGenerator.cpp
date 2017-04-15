#include "stdafx.h"
#include "RockVertexBufferGenerator.h"


RockVertexBufferGenerator::RockVertexBufferGenerator()
{
}

RockVertexBufferGenerator::~RockVertexBufferGenerator()
{
	unloadShaders();
	unloadConstantBuffers();
	unloadVertexBuffer();
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
	m_commonStates = std::make_unique<CommonStates>(device);

	return true;
}

bool RockVertexBufferGenerator::Generate(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView* densityTexture3D) const
{
	//TODO: do stuff



	UINT offset[1] = { 0 };
	deviceContext->SOSetTargets(1, &m_vertexBuffer, offset);
	//m_isVBGenerated = true;
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
	
	//TODO: create GS with SO
	//D3D11_SO_DECLARATION_ENTRY pDecl[] =
	//{
	//	// semantic name, semantic index, start component, component count, output slot
	//	{ 0, "SV_POSITION", 0, 0, 4, 0 },   // output all components of position
	//	{ 0, "TEXCOORD0", 0, 0, 3, 0 },     // output the first 3 of the normal
	//	{ 0, "TEXCOORD1", 0, 0, 2, 0 }     // output the first 2 texture coordinates
	//};
	D3D11_SO_DECLARATION_ENTRY pDecl[] =
	{
		// semantic name, semantic index, start component, component count, output slot
		{ 0, "POSITION", 0, 0, 4, 0 },   
		{ 0, "NORMAL", 0, 0, 3, 0 }
	};
	HR_GS = device->CreateGeometryShaderWithStreamOutput(gsBlob->GetBufferPointer(), gsBlob->GetBufferSize(), pDecl, sizeof(pDecl), NULL, 0, 0, NULL, &m_geometryShader);

	if (FAILED(HR_VS) || FAILED(HR_GS))
		return false;

	// Create the input layout for the vertex shader.
	D3D11_INPUT_ELEMENT_DESC vertexLayoutDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(VertexShaderInput,UV), D3D11_INPUT_PER_VERTEX_DATA, 0 }
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
	//TODO: fill data
	CB_MC_LUT_2 mcLut2Data;
	//TODO: fill data

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.ByteWidth = sizeof(CB_MC_LUT_1);
	bufferDesc.CPUAccessFlags = 0; //D3D11_CPU_ACCESS_WRITE
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
	
	bufferDesc.ByteWidth = sizeof(CB_SliceInfo);
	HRESULT hrCB3 = device->CreateBuffer(&bufferDesc, nullptr, &m_constantBuffers[SliceInfos]);
	
	if (FAILED(hrCB1) || FAILED(hrCB2) || FAILED (hrCB3))
		return false;

	return true;
}

bool RockVertexBufferGenerator::loadVertexBuffer(ID3D11Device* device)
{
	// Create an initialize the vertex buffer.
	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(D3D11_BUFFER_DESC));
	vertexBufferDesc.BindFlags = D3D11_BIND_STREAM_OUTPUT;
	//TODO: check and set right num of outputs!
	vertexBufferDesc.ByteWidth = sizeof(GeometryShaderOutput) * 10000;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	HRESULT hr = device->CreateBuffer(&vertexBufferDesc, NULL, &m_vertexBuffer);

	if (FAILED(hr))
		return false;

	return true;
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
