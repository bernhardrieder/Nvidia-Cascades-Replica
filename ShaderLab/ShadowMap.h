#pragma once

class ShadowMap
{
public:
	ShadowMap(const DirectX::XMINT2& textureSize);
	ShadowMap(const ShadowMap& rhs) = delete;
	ShadowMap(const ShadowMap&& move) = delete;
	ShadowMap& operator=(const ShadowMap& rhs) = delete;
	ShadowMap& operator=(const ShadowMap&& move) = delete;
	~ShadowMap();

	bool Initialize(ID3D11Device* const device);
	void Update(ID3D11DeviceContext* const deviceContext, DirectX::BoundingSphere sceneBounds, DirectX::SimpleMath::Vector3 sunLightDirection);
	void RenderIntoShadowMap(ID3D11DeviceContext* const deviceContext, ID3D11Buffer* const vertexBuffer, ID3D11InputLayout* const inputLayout, const DirectX::SimpleMath::Matrix& world);
	ID3D11ShaderResourceView* GetDepthMapSRV() const;
	const DirectX::SimpleMath::Matrix& GetShadowTransform() const;

private:
	bool loadShader(ID3D11Device* const device);
	bool createRasterizerState(ID3D11Device* const device);
	bool createConstantBuffer(ID3D11Device* const device);

	void calculateShadowTransform(DirectX::BoundingSphere sceneBounds, DirectX::SimpleMath::Vector3 sunLightDirection);

	enum ShaderConstanBufferType
	{
		CB_Frame,
		CB_Object,
		NumConstantBuffers
	};
	struct CbPerFrame
	{
		XMMATRIX LightViewProj;
	};

	struct CbPerObject
	{
		XMMATRIX World;
	};

	ID3D11ShaderResourceView* m_depthMapSRV = nullptr;
	ID3D11DepthStencilView* m_depthMapDSV = nullptr;
	ID3D11RasterizerState* m_rasterizerStateDepth = nullptr;
	ID3D11Buffer* m_constantBuffers[NumConstantBuffers];
	CbPerFrame m_cbPerFrame;
	CbPerObject m_cbPerObject;

	D3D11_VIEWPORT m_viewport;

#if _DEBUG
	const wchar_t* m_compiledVSPath = L"Shader/build_shadowmap_VS_d.cso";
#else
	const wchar_t* m_compiledVSPath = L"Shader/build_shadowmap_VS.cso";
#endif

	ID3D11VertexShader* m_buildVS = nullptr;

	DirectX::SimpleMath::Matrix m_shadowTransform;
};