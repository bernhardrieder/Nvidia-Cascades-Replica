#pragma once
#include "Camera.h"

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
	void SetEmitPos(const XMFLOAT3& emitPosW);
	void SetEmitDir(const XMFLOAT3& emitDirW);

	bool Initialize(const std::wstring& particleNamePrefixInShaderFile, ID3D11Device* device, ID3D11DeviceContext* context, const std::vector<std::wstring>& filenamesOfUniformTextures, unsigned maxParticles);

	void Reset();
	void Update(ID3D11DeviceContext* deviceContext, const float& dt, const float& gameTime, const Camera& camera);
	void Draw(ID3D11DeviceContext* dc, ID3D11RenderTargetView* renderTarget, ID3D11DepthStencilView* depthStencilView, ID3D11RasterizerState* pRasterizerState, const D3D11_VIEWPORT* pViewports, const size_t& numOfViewports);

private:
	void drawPassEmitter(ID3D11DeviceContext* dc);
	void drawPassDraw(ID3D11DeviceContext* dc, ID3D11RenderTargetView* renderTarget, ID3D11DepthStencilView* depthStencilView, ID3D11RasterizerState* pRasterizerState, const D3D11_VIEWPORT* pViewports, const size_t& numOfViewports);

	bool loadShaders(ID3D11Device* device);
	bool loadInitShaders(ID3D11Device* device);
	bool loadDrawShaders(ID3D11Device* device);
	static D3D11_INPUT_ELEMENT_DESC* getParticleInputLayoutDesc(size_t& outCountOfElements);
	void releaseShaders();
	bool createConstantBuffers(ID3D11Device* device);
	void releaseConstantBuffers();
	bool createVertexBuffers(ID3D11Device* device);
	void releaseVertexBuffers();
	bool createRandomTexture(ID3D11Device* device);
	void releaseRandomTexture();
	bool createTextureArray(ID3D11Device* device, ID3D11DeviceContext* context, const std::vector<std::wstring>& filenames);
	void releaseTextureArray();

	struct Particle
	{
		XMFLOAT4 InitialPos;
		XMFLOAT4 InitialVel;
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
	bool m_emitPosSet = false;

	//emitter
	ID3D11VertexShader* m_vsInit = nullptr;
	ID3D11GeometryShader* m_gsInit = nullptr;
	//draw
	ID3D11VertexShader* m_vsDraw = nullptr;
	ID3D11GeometryShader* m_gsDraw = nullptr;
	ID3D11PixelShader* m_psDraw = nullptr;
	//general
	ID3D11InputLayout* m_vsInputLayout = nullptr;

	ID3D11SamplerState* m_samplerLinearWrap = nullptr;

	ID3D11Buffer* mInitVB;
	ID3D11Buffer* mDrawVB;
	ID3D11Buffer* mStreamOutVB;
	ID3D11Buffer* m_constantBuffers[NumConstantBuffers];
	CbPerFrame m_cbPerFrame;

	//ID3D11ShaderResourceView* mTexArraySRV;
	ID3D11ShaderResourceView* mRandomTexSRV;
	ID3D11ShaderResourceView* m_TexSRV;
	std::unique_ptr<CommonStates> m_commonStates;

	std::wstring m_shaderFolder = L"Shader/";
	std::wstring m_shaderFilePrefix;
	const struct ShaderFileSuffixes
	{
		const std::wstring EmitVS = L"Particle_Emit_VS";
		const std::wstring EmitGSWithSO = L"Particle_Emit_GSWithSO";
		const std::wstring DrawVS = L"Particle_Draw_VS";
		const std::wstring DrawGS = L"Particle_Draw_GS";
		const std::wstring DrawPS = L"Particle_Draw_PS";
#if _DEBUG
		const std::wstring Extension = L"_d.cso";
#else
		const std::wstring Extension = L".cso";
#endif
	} m_shaderFileSuffixes;
};
