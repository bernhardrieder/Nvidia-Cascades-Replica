#pragma once

using namespace DirectX;
using namespace DirectX::SimpleMath;

class ParticleSystem
{
public:
	ParticleSystem();
	~ParticleSystem();
	ParticleSystem(const ParticleSystem&& move) = delete;
	ParticleSystem(const ParticleSystem& rhs) = delete;
	ParticleSystem& operator=(const ParticleSystem& rhs) = delete;

	// Time elapsed since the system was reset.
	//float GetAge()const;

	//void SetEyePos(const XMFLOAT4& eyePosW);
	void SetEmitPos(const XMFLOAT4& emitPosW);
	void SetEmitDir(const XMFLOAT4& emitDirW);

	bool Initialize(const std::wstring& particleNamePrefixInShaderFile, ID3D11Device* device, ID3D11ShaderResourceView* texArraySRV, ID3D11ShaderResourceView* randomTexSRV, unsigned maxParticles);

	void Reset();
	void Update(ID3D11DeviceContext* deviceContext, const float& dt, const float& gameTime, const Camera& camera);
	void Draw(ID3D11DeviceContext* dc, const Camera& cam);

private:
	bool loadShaders(ID3D11Device* device);
	bool loadInitShaders(ID3D11Device* device);
	bool loadDrawShaders(ID3D11Device* device);
	static D3D11_INPUT_ELEMENT_DESC* getParticleInputLayoutDesc(size_t& outCountOfElements);
	void releaseShaders();
	bool createConstantBuffers(ID3D11Device* device);
	void releaseConstantBuffers();
	bool createVertexBuffers(ID3D11Device* device);
	void releaseVertexBuffers();

	struct Particle
	{
		XMFLOAT3 InitialPos;
		XMFLOAT3 InitialVel;
		XMFLOAT2 Size;
		float Age;
		unsigned int Type;
	};
	enum ConstanBufferType
	{
		PerFrame = 0,
		NumConstantBuffers
	};
	struct CbPerFrame
	{
		XMMATRIX ViewProj;
		XMFLOAT4 EyePosW;
		XMFLOAT4 EmitPosW;
		XMFLOAT4 EmitDirW;
		float GameTime;
		float DeltaTime;
	};
	unsigned mMaxParticles;
	bool mFirstRun;
	float mAge;

	//init
	ID3D11VertexShader* m_vsInit = nullptr;
	ID3D11GeometryShader* m_gsInit = nullptr;
	//draw
	ID3D11VertexShader* m_vsDraw = nullptr;
	ID3D11GeometryShader* m_gsDraw = nullptr;
	ID3D11PixelShader* m_psDraw = nullptr;
	//general
	ID3D11InputLayout* m_vsInputLayout = nullptr;

	ID3D11Buffer* mInitVB;
	ID3D11Buffer* mDrawVB;
	ID3D11Buffer* mStreamOutVB;
	ID3D11Buffer* m_constantBuffers[NumConstantBuffers];
	CbPerFrame m_cbPerFrame;

	ID3D11ShaderResourceView* mTexArraySRV;
	ID3D11ShaderResourceView* mRandomTexSRV;

	std::wstring m_shaderFilePrefix;
	const struct ShaderFileSuffixes
	{
		const std::wstring InitVS = L"Particle_Init_VS.hlsl";
		const std::wstring InitGSWithSO = L"Particle_Init_GSWithSO.hlsl";
		const std::wstring DrawVS = L"Particle_Draw_VS.hlsl";
		const std::wstring DrawGS = L"Particle_Draw_GS.hlsl";
		const std::wstring DrawPS = L"Particle_Draw_PS.hlsl";
	} m_shaderFileSuffixes;
};
