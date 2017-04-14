#pragma once
#include "stdafx.h"
#include "Camera.h"
#include <CommonStates.h>

using namespace DirectX;

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
	void render(float deltaTime) override;
	bool loadBuffers();
	void unloadBuffers();
	bool loadShaders();
	void unloadShaders();
	void onResize() override;
	void onMouseDown(WPARAM btnState, int x, int y) override;
	void onMouseUp(WPARAM btnState, int x, int y) override;
	void onMouseMove(WPARAM btnState, int x, int y) override;
	void checkAndProcessKeyboardInput(float deltaTime);
	bool initDirectX() override;
	void cleanup() override;
	void fillDensityTexture();
	bool loadDensityFunctionShaders();
	bool loadTextures();
	void unloadTextures();

private:
	// Shader resources
	enum ShaderConstanBufferType
	{
		CB_Appliation,
		CB_Frame,
		CB_Object,
		NumConstantBuffers
	};

	// Vertex data for a colored cube.
	struct VertexPosColor
	{
		XMFLOAT3 Position;
		XMFLOAT3 Color;
	};
	struct VertexPos
	{
		XMFLOAT3 Position;
	};

	// Vertex buffer data
	ID3D11InputLayout* m_inputLayoutSimpleVS = nullptr;
	ID3D11Buffer* m_vertexBuffer = nullptr;
	ID3D11Buffer* m_indexBuffer = nullptr;

	VertexPosColor m_vertices[8] =
	{
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
		{ XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) }, // 1
		{ XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) }, // 3
		{ XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) }, // 4
		{ XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) }, // 5
		{ XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, // 6
		{ XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) }  // 7
	};

	VertexPos m_renderPortalVertices[6] =
	{
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f) }, // 0
		{ XMFLOAT3(-1.0f, 1.0f, 0.f) }, // 1
		{ XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f) }, // 3
		{ XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 4
		{ XMFLOAT3(1.0f, -1.0f, 0.0f) } // 5
	};

	WORD m_indices[36] =
	{
		0, 1, 2, 0, 2, 3,
		4, 6, 5, 4, 7, 6,
		4, 5, 1, 4, 1, 0,
		3, 2, 6, 3, 6, 7,
		1, 5, 6, 1, 6, 2,
		4, 0, 3, 4, 3, 7
	};

	// Shader data
	ID3D11VertexShader* m_simpleVS = nullptr;
	ID3D11PixelShader* m_simplePS = nullptr;
	ID3D11GeometryShader* m_simpleGS = nullptr;
	ID3D11Buffer* m_constantBuffers[NumConstantBuffers];

	POINT m_lastMousePos;
	Camera m_camera;
	// Demo parameters
	SimpleMath::Matrix m_worldMatrix;


	/** Density Variables */
	ID3D11Texture3D* m_densityTex3D = nullptr;
	ID3D11RenderTargetView* m_densityTex3D_RTV = nullptr;
	ID3D11ShaderResourceView* m_densityTex3D_SRV = nullptr;

	bool m_isDensityTextureCreated = false;
	ID3D11InputLayout* m_inputLayoutDensityVS = nullptr;
	ID3D11Buffer* m_renderPortalvertexBuffer = nullptr;
	ID3D11VertexShader* m_densityVS;
	ID3D11GeometryShader* m_densityGS;
	ID3D11PixelShader* m_densityPS;

	const size_t m_noiseTexCount = 8;
	std::unique_ptr<CommonStates> m_commonStates;
	ID3D11ShaderResourceView* m_noiseTexSRV[8];
	const std::wstring m_noiseTexPrefix = L"Textures/Noise/";
	const std::wstring m_noiseTexFilename[8] = {	L"lichen1_disp.dds", L"lichen2_disp.dds", L"lichen3_disp.dds", L"lichen4_disp.dds",  
													L"lichen5_disp.dds", L"lichen6_disp.dds", L"lichen7_disp.dds", L"lichen8_disp.dds" };
};