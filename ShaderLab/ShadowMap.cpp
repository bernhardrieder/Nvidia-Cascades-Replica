#include "stdafx.h"
#include "ShadowMap.h"

ShadowMap::ShadowMap(const XMINT2& textureSize)
{
	m_viewport.TopLeftX = 0.0f;
	m_viewport.TopLeftY = 0.0f;
	m_viewport.Width = static_cast<float>(textureSize.x);
	m_viewport.Height = static_cast<float>(textureSize.y);
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;
}

ShadowMap::~ShadowMap()
{
	SafeRelease(m_depthMapSRV);
	SafeRelease(m_depthMapDSV);
	SafeRelease(m_buildVS);
}

bool ShadowMap::Initialize(ID3D11Device* const device)
{	
	// Use typeless format because the DSV is going to interpret
	// the bits as DXGI_FORMAT_D24_UNORM_S8_UINT, whereas the SRV is going to interpret
	// the bits as DXGI_FORMAT_R24_UNORM_X8_TYPELESS.
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = m_viewport.Width;
	texDesc.Height = m_viewport.Height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;
	ID3D11Texture2D* depthMapTexture = nullptr;
	HRESULT hr;
	if (FAILED(hr = device->CreateTexture2D(&texDesc, nullptr, &depthMapTexture)))
		return false;

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = 0;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	if (FAILED(hr = device->CreateDepthStencilView(depthMapTexture, &dsvDesc, &m_depthMapDSV)))
		return false;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = texDesc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;
	if (FAILED(hr = device->CreateShaderResourceView(depthMapTexture, &srvDesc, &m_depthMapSRV)))
		return false;

	// View saves a reference to the texture so we can release our reference.
	SafeRelease(depthMapTexture);

	if(!loadShader(device) || !createRasterizerState(device))
	{
		return false;
	}

	return true;
}

void ShadowMap::RenderIntoShadowMap(ID3D11DeviceContext* const deviceContext, ID3D11Buffer* const vertexBuffer, ID3D11InputLayout* const inputLayout, ID3D11Buffer* const cbPerFrame, ID3D11Buffer* const cbPerObject) const
{
	deviceContext->ClearDepthStencilView(m_depthMapDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

	const UINT vertexStride = sizeof(RockVertexBufferGenerator::GeometryShaderOutput);
	const UINT offset = 0;
	deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &vertexStride, &offset);
	deviceContext->IASetInputLayout(inputLayout);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	deviceContext->VSSetShader(m_buildVS, nullptr, 0);
	deviceContext->VSSetConstantBuffers(0, 1, &cbPerFrame);
	deviceContext->VSSetConstantBuffers(1, 1, &cbPerObject);

	deviceContext->GSSetShader(nullptr, nullptr, 0);
	
	deviceContext->RSSetViewports(1, &m_viewport);
	deviceContext->RSSetState(m_rasterizerStateDepth);

	deviceContext->PSSetShader(nullptr, nullptr, 0);

	// Set null render target because we are only going to draw to depth buffer.
	// Setting a null render target will disable color writes.
	ID3D11RenderTargetView* renderTargets = nullptr;
	deviceContext->OMSetRenderTargets(1, &renderTargets, m_depthMapDSV);
	deviceContext->OMSetDepthStencilState(nullptr, 0);

	deviceContext->DrawAuto();

	deviceContext->RSSetState(nullptr);
	deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
}

DirectX::SimpleMath::Matrix ShadowMap::CalculateShadowTransform(DirectX::BoundingSphere sceneBounds, DirectX::SimpleMath::Vector3 sunLightDirection)
{	
	// Only the first "main" light casts a shadow.
	Vector3 lightPos = -2.0f * sceneBounds.Radius * -sunLightDirection;
	Vector3 targetPos = Vector3::Zero;

	Matrix lightView = XMMatrixLookAtLH(lightPos, targetPos, Vector3::Up);

	// Transform bounding sphere to light space.
	Vector3 sphereCenterLS = Vector3::Transform(targetPos, lightView);

	// Ortho frustum in light space encloses scene.
	float l = sphereCenterLS.x - sceneBounds.Radius;
	float b = sphereCenterLS.y - sceneBounds.Radius;
	float n = sphereCenterLS.z - sceneBounds.Radius;
	float r = sphereCenterLS.x + sceneBounds.Radius;
	float t = sphereCenterLS.y + sceneBounds.Radius;
	float f = sphereCenterLS.z + sceneBounds.Radius;
	
	Matrix lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	Matrix T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	return lightView*lightProj*T;
}

ID3D11ShaderResourceView* ShadowMap::GetDepthMapSRV() const
{
	return m_depthMapSRV;
}

bool ShadowMap::loadShader(ID3D11Device* const device)
{
	ID3DBlob *vsBlob;

	HRESULT HR_VS;
	HR_VS = D3DReadFileToBlob(m_compiledVSPath, &vsBlob);

	if (FAILED(HR_VS) )
		return false;

	HR_VS = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_buildVS);

	if (FAILED(HR_VS))
		return false;

	SafeRelease(vsBlob);

	return true;
}

bool ShadowMap::createRasterizerState(ID3D11Device* const device)
{	
	// [From MSDN]
	// If the depth buffer currently bound to the output-merger stage has a UNORM format or
	// no depth buffer is bound the bias value is calculated like this:
	//
	// Bias = (float)DepthBias * r + SlopeScaledDepthBias * MaxDepthSlope;
	//
	// where r is the minimum representable value > 0 in the depth-buffer format converted to float32.
	// [/End MSDN]
	//
	// For a 24-bit depth buffer, r = 1 / 2^24.
	//
	// Example: DepthBias = 100000 ==> Actual DepthBias = 100000/2^24 = .006

	// You need to experiment with these values for your scene.

	// Setup rasterizer state.
	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));

	rasterizerDesc.DepthBias = 100000;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.SlopeScaledDepthBias = 1.0f;
	rasterizerDesc.DepthClipEnable = TRUE;

	//rasterizerDesc.AntialiasedLineEnable = FALSE;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	//rasterizerDesc.FrontCounterClockwise = FALSE;
	//rasterizerDesc.MultisampleEnable = FALSE;
	//rasterizerDesc.ScissorEnable = FALSE;

	HRESULT hr;
	// Create the rasterizer state object.
	if (FAILED(hr = device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerStateDepth)))
		return false;

	return true;
}
