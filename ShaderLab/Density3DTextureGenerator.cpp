#include "stdafx.h"
#include "Density3DTextureGenerator.h"


Density3DTextureGenerator::Density3DTextureGenerator()
{
}


Density3DTextureGenerator::~Density3DTextureGenerator()
{
	unloadID3D11Resources();
	unloadShaders();
	unloadTextures();
}

bool Density3DTextureGenerator::Initialize(ID3D11Device* device)
{
	if (!loadID3D11Resources(device))
	{
		MessageBox(nullptr, TEXT("Density3DTextureGenerator: Failed to load ID3D11 resources."), TEXT("Error"), MB_OK);
		return false;
	}
	if (!loadShaders(device))
	{
		MessageBox(nullptr, TEXT("Density3DTextureGenerator: Failed to load shaders."), TEXT("Error"), MB_OK);
		return false;
	}
	if (!loadTextures(device))
	{
		MessageBox(nullptr, TEXT("Density3DTextureGenerator: Failed to load textures. Is Textures folder in execution folder?"), TEXT("Error"), MB_OK);
		return false;
	}
	m_commonStates = std::make_unique<CommonStates>(device);
	setViewport(96, 96);
	return true;
}

bool Density3DTextureGenerator::Generate(ID3D11DeviceContext* deviceContext)
{
	deviceContext->ClearRenderTargetView(m_tex3D_RTV, Colors::White);
	//m_deviceContext->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	const UINT vertexStride = sizeof(VertexPos);
	const UINT offset = 0;

	deviceContext->IASetInputLayout(m_inputLayoutVS);
	deviceContext->IASetVertexBuffers(0, 1, &m_renderPortalvertexBuffer, &vertexStride, &offset);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	deviceContext->VSSetShader(m_vertexShader, nullptr, 0);
	//m_deviceContext->VSSetConstantBuffers(0, 3, m_constantBuffers);

	deviceContext->GSSetShader(m_geometryShader, nullptr, 0);

	//rasterizer
	deviceContext->RSSetState(m_rasterizerState);
	deviceContext->RSSetViewports(1, &m_viewport);

	deviceContext->PSSetShader(m_pixelShader, nullptr, 0);
	deviceContext->PSSetShaderResources(0, m_noiseTexCount, m_noiseTexSRV);

	auto samplerState = m_commonStates->LinearWrap();
	deviceContext->PSSetSamplers(0, 1, &samplerState);

	//output merger
	deviceContext->OMSetRenderTargets(1, &m_tex3D_RTV, nullptr);
	//m_deviceContext->OMSetDepthStencilState(m_depthStencilState, 1);

	deviceContext->DrawInstanced(6, 256, 0, 0);
	//m_swapChain->Present(0, 0);


	/** RESET */
	deviceContext->VSSetShader(nullptr, nullptr, 0);
	deviceContext->GSSetShader(nullptr, nullptr, 0);
	deviceContext->PSSetShader(nullptr, nullptr, 0);
	//reset render target - otherwise the density SRV can't be used
	deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
	//release density rtv?

	return true;
}

bool Density3DTextureGenerator::loadID3D11Resources(ID3D11Device* device)
{
	D3D11_TEXTURE3D_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D11_TEXTURE3D_DESC));
	texDesc.Width = 96; // x axis
	texDesc.Height = 96; // y axis
	texDesc.Depth = 256; // z axis
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R16_FLOAT;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	//texDesc.CPUAccessFlags = 0;
	//texDesc.MiscFlags = 0;
	//
	//texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	HRESULT hr = device->CreateTexture3D(&texDesc, nullptr, &m_tex3D);
	if (FAILED(hr))
		return false;

	// Give the buffer a useful name
	//m_tex3D->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof("Density Texture"), "Density Texture");

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	ZeroMemory(&rtvDesc, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
	rtvDesc.Format = texDesc.Format;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
	
	hr = device->CreateRenderTargetView(m_tex3D, nullptr, &m_tex3D_RTV);
	if (FAILED(hr))
		return false;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	srvDesc.Texture3D.MipLevels = texDesc.MipLevels;
	srvDesc.Texture3D.MostDetailedMip = 0;

	hr = device->CreateShaderResourceView(m_tex3D, &srvDesc, &m_tex3D_SRV);
	if (FAILED(hr))
		return false;

	// Setup rasterizer state.
	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
	rasterizerDesc.AntialiasedLineEnable = FALSE;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.ScissorEnable = FALSE;
	rasterizerDesc.SlopeScaledDepthBias = 0.0f;

	// Create the rasterizer state object.
	hr = device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerState);
	if (FAILED(hr))
		return false;

	return true;
}

void Density3DTextureGenerator::unloadID3D11Resources()
{
	SafeRelease(m_rasterizerState);
	SafeRelease(m_tex3D);
	SafeRelease(m_tex3D_RTV);
	SafeRelease(m_tex3D_SRV);
}

bool Density3DTextureGenerator::loadShaders(ID3D11Device* device)
{
	ID3DBlob *vsBlob, *gsBlob, *psBlob;

	HRESULT HR_VS, HR_GS, HR_PS;
	HR_VS = D3DReadFileToBlob(m_compiledVSPath, &vsBlob);
	HR_GS = D3DReadFileToBlob(m_compiledGSPath, &gsBlob);
	HR_PS = D3DReadFileToBlob(m_compiledPSPath, &psBlob);

	if (FAILED(HR_VS) || FAILED(HR_GS) || FAILED(HR_PS))
		return false;

	HR_VS = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vertexShader);
	HR_GS = device->CreateGeometryShader(gsBlob->GetBufferPointer(), gsBlob->GetBufferSize(), nullptr, &m_geometryShader);
	HR_PS = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pixelShader);

	if (FAILED(HR_VS) || FAILED(HR_GS) || FAILED(HR_PS))
		return false;

	// Create the input layout for the vertex shader.
	D3D11_INPUT_ELEMENT_DESC vertexLayoutDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(VertexPos,Position), D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	HR_VS = device->CreateInputLayout(vertexLayoutDesc, _countof(vertexLayoutDesc), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayoutVS);
	if (FAILED(HR_VS))
		return false;

	SafeRelease(vsBlob);
	SafeRelease(gsBlob);
	SafeRelease(psBlob);


	// Create an initialize the vertex buffer.
	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(D3D11_BUFFER_DESC));
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.ByteWidth = sizeof(VertexPos) * _countof(m_renderPortalVertices);
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA resourceData;
	ZeroMemory(&resourceData, sizeof(D3D11_SUBRESOURCE_DATA));
	resourceData.pSysMem = m_renderPortalVertices;

	HRESULT hr = device->CreateBuffer(&vertexBufferDesc, &resourceData, &m_renderPortalvertexBuffer);
	if (FAILED(hr))
		return false;

	return true;
}

bool Density3DTextureGenerator::loadTextures(ID3D11Device* device)
{
	for (int i = 0; i < m_noiseTexCount; ++i)
	{
		// Load the texture in.
		auto filename = (m_noiseTexPathPrefix + m_noiseTexFilename[i]);
		HRESULT hr = CreateDDSTextureFromFile(device, filename.c_str(), nullptr, &m_noiseTexSRV[i]);

		if (FAILED(hr))
			return false;
	}

	return true;
}

void Density3DTextureGenerator::unloadShaders()
{
	SafeRelease(m_inputLayoutVS);
	SafeRelease(m_vertexShader);
	SafeRelease(m_geometryShader);
	SafeRelease(m_pixelShader);
}

void Density3DTextureGenerator::unloadTextures()
{
	for (int i = 0; i < m_noiseTexCount; ++i)
	{
		SafeRelease(m_noiseTexSRV[i]);
	}
}

void Density3DTextureGenerator::setViewport(FLOAT width, FLOAT height)
{
	m_viewport.Width = width;
	m_viewport.Height = height;
	m_viewport.TopLeftX = 0.0f;
	m_viewport.TopLeftY = 0.0f;
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;
}
