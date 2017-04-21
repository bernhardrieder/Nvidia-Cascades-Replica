#pragma once
#include "stdafx.h"

class Density3DTextureGenerator
{
public:
	Density3DTextureGenerator();
	~Density3DTextureGenerator();

	bool Initialize(ID3D11Device* device, DirectX::XMINT3 textureSize);
	bool Generate(ID3D11DeviceContext* pDeviceContext);
	ID3D11Texture3D* GetTexture3D() const { return m_tex3D; }
	ID3D11ShaderResourceView* GetTexture3DShaderResourceView() const { return m_tex3D_SRV; }

private:
	bool createID3D11Resources(ID3D11Device* device);
	bool loadShaders(ID3D11Device* device);
	void releaseID3D11Resources();
	void releaseShaders();
	void setViewport(FLOAT width, FLOAT height);

	struct VertexPos
	{
		DirectX::XMFLOAT3 Position;
	};
	VertexPos m_renderPortalVertices[6] =
	{
		{ DirectX::XMFLOAT3(-1.0f, -1.0f, 0.0f) }, // 0
		{ DirectX::XMFLOAT3(-1.0f, 1.0f, 0.f) }, // 1
		{ DirectX::XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
		{ DirectX::XMFLOAT3(-1.0f, -1.0f, 0.0f) }, // 3
		{ DirectX::XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 4
		{ DirectX::XMFLOAT3(1.0f, -1.0f, 0.0f) } // 5
	};

	DirectX::XMINT3 m_texSize;

	ID3D11RasterizerState* m_rasterizerState = nullptr;
	D3D11_VIEWPORT m_viewport = { 0 };

	ID3D11Texture3D* m_tex3D = nullptr;
	ID3D11RenderTargetView* m_tex3D_RTV = nullptr;
	ID3D11ShaderResourceView* m_tex3D_SRV = nullptr;

	ID3D11InputLayout* m_vertexShaderInputLayout = nullptr;
	ID3D11Buffer* m_renderPortalvertexBuffer = nullptr;
	ID3D11VertexShader* m_vertexShader = nullptr;
	ID3D11GeometryShader* m_geometryShader = nullptr;
	ID3D11PixelShader* m_pixelShader = nullptr;

#if _DEBUG
	const wchar_t* m_compiledVSPath = L"Shader/create_density_tex3d_VS_d.cso";
	const wchar_t* m_compiledGSPath = L"Shader/create_density_tex3d_GS_d.cso";
	const wchar_t* m_compiledPSPath = L"Shader/create_density_tex3d_PS_d.cso";
#else
	const wchar_t* m_compiledVSPath = L"Shader/create_density_tex3d_VS.cso";
	const wchar_t* m_compiledGSPath = L"Shader/create_density_tex3d_GS.cso";
	const wchar_t* m_compiledPSPath = L"Shader/create_density_tex3d_PS.cso";
#endif
};