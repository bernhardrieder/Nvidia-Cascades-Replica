#pragma once

class RockVertexBufferGenerator
{
public:
	RockVertexBufferGenerator();
	~RockVertexBufferGenerator();

	bool Initialize(ID3D11Device* device);
	bool Generate(ID3D11DeviceContext* pDeviceContext, ID3D11ShaderResourceView* pDensityTexture3DShaderResourceView) const;
	ID3D11Buffer* GetVertexBuffer() const;
private:
	bool loadShaders(ID3D11Device* device);
	bool loadConstantBuffers(ID3D11Device* device);
	bool loadVertexBuffer(ID3D11Device* device);
	void unloadConstantBuffers();
	void unloadShaders();
	void unloadVertexBuffer();

	struct VertexShaderInput
	{
		DirectX::XMFLOAT2 UV;
	};
	struct GeometryShaderOutput
	{
		DirectX::XMFLOAT4 wsCoord_Ambo;
		DirectX::XMFLOAT3 wsNormal;
	};
	enum ShaderConstanBufferType
	{
		mc_lut_1,
		mc_lut_2,
		SliceInfos,
		NumConstantBuffers
	};
	struct CB_MC_LUT_1
	{
		uint32_t case_to_numpolys[256];
		DirectX::XMFLOAT4 cornerAmask0123[12];
		DirectX::XMFLOAT4 cornerAmask4567[12];
		DirectX::XMFLOAT4 cornerBmask0123[12];
		DirectX::XMFLOAT4 cornerBmask4567[12];
		DirectX::XMFLOAT3 vec_start[12];
		DirectX::XMFLOAT3 vec_dir[12];
	};
	struct CB_MC_LUT_2
	{
		DirectX::XMINT4 g_triTable[1280];
	};
	struct CB_SliceInfo
	{
		float slice_world_space_Y_coord[256];
	};

	ID3D11Buffer* m_constantBuffers[NumConstantBuffers];
	ID3D11VertexShader* m_vertexShader = nullptr;
	ID3D11InputLayout* m_vsInputLayout = nullptr;
	ID3D11GeometryShader* m_geometryShader = nullptr;
	ID3D11Buffer* m_vertexBuffer = nullptr;
	std::unique_ptr<DirectX::CommonStates> m_commonStates = nullptr;

#if _DEBUG
	const wchar_t* m_compiledVSPath = L"Shader/generate_rock_VB_VS_d.cso";
	const wchar_t* m_compiledGSPath = L"Shader/generate_rock_VB_GS_d.cso";
#else
	const wchar_t* m_compiledVSPath = L"Shader/generate_rock_VB_VS.cso";
	const wchar_t* m_compiledGSPath = L"Shader/generate_rock_VB_GS.cso";
#endif

};

