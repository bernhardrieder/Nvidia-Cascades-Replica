#pragma once
#include "Camera.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

class ParticleSystem
{
public:
	ParticleSystem();
	~ParticleSystem();
	ParticleSystem(const ParticleSystem& other) = delete;
	ParticleSystem(const ParticleSystem&& move) = delete;
	ParticleSystem& operator=(const ParticleSystem& other) = delete;
	ParticleSystem& operator=(const ParticleSystem&& move) = delete;

	void SetEmitterPosition(const XMFLOAT3& emitterPositionWorld);
	void SetEmitterDirection(const XMFLOAT3& emitterDirectionNormalized);

	bool Initialize(const std::wstring& particleNamePrefixInShaderFile, const std::wstring& textureFile, const unsigned& maxParticles, ID3D11Device* pDevice);
	void Update(ID3D11DeviceContext* pDeviceContext, const float& deltaTime, const float& gameTime, const Camera& camera);
	void Draw(ID3D11DeviceContext* pDeviceContext, ID3D11RenderTargetView* pRenderTarget, ID3D11DepthStencilView* pDepthStencilView, ID3D11RasterizerState* pRasterizerState, const D3D11_VIEWPORT* viewports, const size_t& numOfViewports);
	
	void Reset();
private:
	void drawPassEmitter(ID3D11DeviceContext* pDeviceContext);
	void drawPassDraw(ID3D11DeviceContext* pDeviceContext, ID3D11RenderTargetView* pRenderTarget, ID3D11DepthStencilView* pDepthStencilView, ID3D11RasterizerState* pRasterizerState, const D3D11_VIEWPORT* pViewports, const size_t& numOfViewports) const;

	bool loadShaders(ID3D11Device* pDevice);
	bool loadInitShaders(ID3D11Device* pDevice);
	bool loadDrawShaders(ID3D11Device* pDevice);
	bool createConstantBuffers(ID3D11Device* pDevice);
	bool createVertexBuffers(ID3D11Device* pDevice);
	bool createRandomTextureSRV(ID3D11Device* pDevice);
	bool createTextureSRV(ID3D11Device* pDevice, const std::wstring& filename);

	void releaseShaders();
	void releaseConstantBuffers();
	void releaseVertexBuffers();
	void releaseTextureSRVs();

	static D3D11_INPUT_ELEMENT_DESC* getParticleInputLayoutDesc(size_t& outCountOfElements);

private:
	struct ParticleShaderInput
	{
		XMFLOAT4 InitialPos;
		XMFLOAT4 InitialVel;
		XMFLOAT2 Size;
		float LifeTime;
		unsigned int Type;
	};
	enum ConstanBufferType
	{
		CB_PerFrame = 0,
		NumConstantBuffers
	};
	struct CbPerFrame
	{
		XMMATRIX ViewProj;
		XMFLOAT4 EyePositionWorld;
		XMFLOAT4 EmitterPositionWorld;
		XMFLOAT4 EmitterDirectionWorld;
		float GameTime;
		float DeltaTime;
	};

	unsigned m_maxParticles = 0;
	bool m_isEmitterAlive = false;
	bool m_isEmitterPositionSet = false;

	//emitter
	ID3D11VertexShader* m_vsEmit = nullptr;
	ID3D11GeometryShader* m_gsEmit = nullptr;
	//draw
	ID3D11VertexShader* m_vsDraw = nullptr;
	ID3D11GeometryShader* m_gsDraw = nullptr;
	ID3D11PixelShader* m_psDraw = nullptr;
	//general
	ID3D11InputLayout* m_vsInputLayout = nullptr;

	ID3D11SamplerState* m_samplerLinearWrap = nullptr;
	ID3D11DepthStencilState* m_depthStencilRead = nullptr;
	ID3D11BlendState* m_blendAdd = nullptr;

	ID3D11Buffer* m_emitterCreationVB = nullptr;
	ID3D11Buffer* m_emitterStreamOutVB = nullptr;
	ID3D11Buffer* m_emittedParticlesVB = nullptr;

	ID3D11Buffer* m_constantBuffers[NumConstantBuffers];
	CbPerFrame m_cbPerFrame;

	ID3D11ShaderResourceView* m_randomTextureSRV = nullptr;
	ID3D11ShaderResourceView* m_textureSRV = nullptr;
	std::unique_ptr<CommonStates> m_commonStates = nullptr;

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
