#pragma once

class RockVertexBufferGenerator
{
public:
	RockVertexBufferGenerator();
	~RockVertexBufferGenerator();

	bool Initialize(ID3D11Device* device, DirectX::XMINT3 densityTextureSize);
	bool Generate(ID3D11DeviceContext* pDeviceContext, ID3D11ShaderResourceView* pDensityTexture3DShaderResourceView) const;
	ID3D11Buffer* GetVertexBuffer() const;

	struct GeometryShaderOutput
	{
		DirectX::XMFLOAT4 WorldSpacePosition;
		DirectX::XMFLOAT3 WorldSpaceNormal;
	};
private:
	struct VertexShaderInput
	{
		DirectX::XMFLOAT4 VertexPosition;
	};

	bool loadShaders(ID3D11Device* device);
	bool createConstantBuffers(ID3D11Device* device);
	bool createVertexBuffer(ID3D11Device* device);
	bool createSamplerStates(ID3D11Device* device);
	void releaseConstantBuffers();
	void releaseShaders();
	void releaseVertexBuffer();
	void releaseSamplerStates();

	int createDummyVertices(VertexShaderInput** outVertices) const;

	enum ShaderConstanBufferType
	{
		MarchingCubesLookUpTables = 0,
		Steps,
		NumConstantBuffers
	};
	struct CB_Steps
	{
		DirectX::XMFLOAT4 cornerStep[8];
		DirectX::XMFLOAT4 dataStep;
	};

	struct CB_MC_LookUpTables
	{
		DirectX::XMINT4 case_to_numpolys[256];
		DirectX::XMINT4 g_triTable[1280];
	};

	DirectX::XMINT3 m_densityTextureSize;
	DirectX::XMFLOAT3 m_densityTextureSizeStep;
	ID3D11Buffer* m_constantBuffers[NumConstantBuffers];
	ID3D11VertexShader* m_vertexShader = nullptr;
	ID3D11InputLayout* m_vertexShaderInputLayout = nullptr;
	ID3D11GeometryShader* m_geometryShader = nullptr;
	ID3D11Buffer* m_vertexBuffer = nullptr;
	std::unique_ptr<DirectX::CommonStates> m_commonStates = nullptr;

	ID3D11Buffer* m_dummyVertexBuffer = nullptr;
	VertexShaderInput* m_dummyVertices = nullptr;
	int m_dummyVertexCount = 0;
	// trilinearinterp; clamps on XY, wraps on Z.
	ID3D11SamplerState* m_samplerState = nullptr;


#if _DEBUG
	const wchar_t* m_compiledVSPath = L"Shader/generate_rock_VB_VS_d.cso";
	const wchar_t* m_compiledGSPath = L"Shader/generate_rock_VB_GS_d.cso";
#else
	const wchar_t* m_compiledVSPath = L"Shader/generate_rock_VB_VS.cso";
	const wchar_t* m_compiledGSPath = L"Shader/generate_rock_VB_GS.cso";
#endif
};

