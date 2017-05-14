#pragma once
#include "stdafx.h"
#include "Camera.h"
#include <CommonStates.h>
#include "RockVertexBufferGenerator.h"
#include "Density3DTextureGenerator.h"
#include <SimpleMath.h>
#include "CollisionDetectionHelper.h"
#include "KdTree.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

class ShaderLab : public D3D11App
{
public:
	ShaderLab(HINSTANCE hInstance, int nCmdShow);
	ShaderLab(const ShaderLab& rhs) = delete;
	ShaderLab& operator=(const ShaderLab& rhs) = delete;
	~ShaderLab() override;

	bool Initialize() override;

private:	
	void update(float deltaTime) override;
	void render() override;
	bool loadShaders();
	void unloadShaders();
	void onResize() override;
	void checkAndProcessKeyboardInput(float deltaTime) override;
	void checkAndProcessMouseInput(float deltaTime) override;
	bool initDirectX() override;
	void cleanup() override;

	bool createTextures(ID3D11Device* device);
	void releaseTextures();
	bool createSampler(ID3D11Device* device);
	void releaseSampler();

	HitResult raycast(int sx, int sy, DirectX::SimpleMath::Ray& outRay);
private:
	// Shader resources
	enum ShaderConstanBufferType
	{
		CB_Application,
		CB_Frame,
		CB_Object,
		NumConstantBuffers
	};

	// Vertex data for a colored cube.
	struct VertexPosNormal
	{
		DirectX::XMFLOAT4 LocalPosition;
		DirectX::XMFLOAT3 LocalVertexNormal;
		DirectX::XMFLOAT3 LocalSurfaceNormal;
	};

	struct CbPerApplication
	{
		XMMATRIX Proj;
		float DisplacementScale;
		float InitialStepIterations;
		float RefinementStepIterations;
		float ParallaxDepth;
	};

	struct CbPerFrame
	{
		XMMATRIX View;
		XMMATRIX ViewProj;
		XMVECTOR SunLightDirection;
		XMFLOAT3 WorldEyePosition;
		float AppTime;
		XMFLOAT3 ScreenSize;
		float DeltaTime;

	};

	struct CbPerObject
	{
		XMMATRIX World;
		XMMATRIX WorldInverseTranspose;
	};

	CbPerApplication m_cbPerApplication;
	bool m_updateCbPerApplication = false;
	CbPerFrame m_cbPerFrame;
	CbPerObject m_cbPerObject;
	float m_sunTheta = -1.4f* DirectX::XM_PI;
	float m_sunPhi = DirectX::XM_PIDIV2;

	// Vertex buffer data
	ID3D11InputLayout* m_inputLayoutSimpleVS = nullptr;

	// Shader data
	ID3D11VertexShader* m_simpleVS = nullptr;
	ID3D11PixelShader* m_simplePS = nullptr;

#if _DEBUG
	const wchar_t* m_compiledVSPath = L"Shader/drawRockVS_d.cso";
	const wchar_t* m_compiledPSPath = L"Shader/drawRockPS_d.cso";
#else
	const wchar_t* m_compiledVSPath = L"Shader/drawRockVS.cso";
	const wchar_t* m_compiledPSPath = L"Shader/drawRockPS.cso";
#endif

	ID3D11Buffer* m_constantBuffers[NumConstantBuffers];
	std::unique_ptr<DirectX::CommonStates> m_commonStates;

	Camera m_camera;
	SimpleMath::Matrix m_worldMatrix = SimpleMath::Matrix::Identity;;

	DirectX::XMINT3 m_rockSize;

	bool m_isDensityTextureGenerated = false;
	Density3DTextureGenerator m_densityTexGenerator;

	bool m_isRockVertexBufferGenerated = false;
	RockVertexBufferGenerator m_rockVBGenerator;
	
	ID3D11SamplerState* m_lichenSampler = nullptr;
	const size_t m_textureCount = 19;
	std::vector<ID3D11ShaderResourceView*> m_texturesSRVs;
	std::vector<ID3D11ShaderResourceView*> m_texturesDispSRVs;
	std::vector<ID3D11ShaderResourceView*> m_texturesBumpSRVs;
	std::wstring m_textureFilesPath = L"Assets/Textures/";
	std::wstring m_textureDispFilesPath = L"Assets/Textures/disp/";
	std::wstring m_textureBumpFilesPath = L"Assets/Textures/bump/";
	std::wstring m_textureGenericFilename = L"lichen";
	std::wstring m_textureFilenameExtension = L".dds";

	//Raycasting
	std::unique_ptr<KDNode> m_rockKdTreeRoot;
	std::vector<Triangle> m_rockTrianglesTransformed;
	HitResult m_raycastHitResult;
	std::unique_ptr<DirectX::GeometricPrimitive> m_raycastHitSphere;
};