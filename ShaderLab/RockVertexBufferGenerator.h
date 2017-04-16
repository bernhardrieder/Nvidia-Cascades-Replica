#pragma once

class RockVertexBufferGenerator
{
public:
	RockVertexBufferGenerator();
	~RockVertexBufferGenerator();

	bool Initialize(ID3D11Device* device);
	bool Generate(ID3D11DeviceContext* pDeviceContext, ID3D11ShaderResourceView* pDensityTexture3DShaderResourceView) const;
	ID3D11Buffer* GetVertexBuffer() const;

	struct GeometryShaderOutput
	{
		DirectX::XMFLOAT4 wsCoord_Ambo;
		DirectX::XMFLOAT3 wsNormal;
	};
private:
	bool loadShaders(ID3D11Device* device);
	bool loadConstantBuffers(ID3D11Device* device);
	bool loadVertexBuffer(ID3D11Device* device);
	bool loadSamplerStates(ID3D11Device* device);
	bool loadAdditionalID3D11Resources(ID3D11Device* device);
	void unloadConstantBuffers();
	void unloadShaders();
	void unloadVertexBuffer();
	void unloadSamplerStates();
	void unloadAdditionalID3D11Resources();

	struct VertexShaderInput
	{
		DirectX::XMFLOAT2 UV;
	};

	enum ShaderConstanBufferType
	{
		mc_lut_1 = 0,
		mc_lut_2,
		SliceInfos,
		NumConstantBuffers
	};
	struct CB_MC_LUT_1
	{
		DirectX::XMINT4 case_to_numpolys[256];
		DirectX::XMFLOAT4 cornerAmask0123[12];
		DirectX::XMFLOAT4 cornerAmask4567[12];
		DirectX::XMFLOAT4 cornerBmask0123[12];
		DirectX::XMFLOAT4 cornerBmask4567[12];
		DirectX::XMFLOAT4 vec_start[12];
		DirectX::XMFLOAT4 vec_dir[12];
	};
	struct CB_MC_LUT_2
	{
		DirectX::XMINT4 g_triTable[1280];
	};
	struct CB_SliceInfo
	{
		DirectX::XMFLOAT4 slice_world_space_Y_coord[256];
	};

	uint32_t m_maxRenderedSlices = 50;
	ID3D11Buffer* m_constantBuffers[NumConstantBuffers];
	ID3D11VertexShader* m_vertexShader = nullptr;
	ID3D11InputLayout* m_vsInputLayout = nullptr;
	ID3D11GeometryShader* m_geometryShader = nullptr;
	ID3D11Buffer* m_vertexBuffer = nullptr;
	ID3D11Buffer* m_dummyVertexBuffer = nullptr;
	std::unique_ptr<DirectX::CommonStates> m_commonStates = nullptr;

	VertexShaderInput m_dummyVertices[96 * 96];
	// trilinearinterp; clamps on XY, wraps on Z.
	ID3D11SamplerState* m_samplerState = nullptr;

	ID3D11DepthStencilState* m_depthStencilState = nullptr;
	ID3D11RasterizerState* m_rasterizerState = nullptr;

#if _DEBUG
	const wchar_t* m_compiledVSPath = L"Shader/generate_rock_VB_VS_d.cso";
	const wchar_t* m_compiledGSPath = L"Shader/generate_rock_VB_GS_d.cso";
#else
	const wchar_t* m_compiledVSPath = L"Shader/generate_rock_VB_VS.cso";
	const wchar_t* m_compiledGSPath = L"Shader/generate_rock_VB_GS.cso";
#endif

};

