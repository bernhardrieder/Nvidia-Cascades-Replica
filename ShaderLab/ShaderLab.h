#pragma once
#include "stdafx.h"
#include "Camera.h"
#include <CommonStates.h>
#include "RockVertexBufferGenerator.h"
#include "Density3DTextureGenerator.h"

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
	std::unique_ptr<DirectX::CommonStates> m_commonStates;

	POINT m_lastMousePos;
	Camera m_camera;
	// Demo parameters
	SimpleMath::Matrix m_worldMatrix;

	/** Density Variables */
	bool m_isDensityTextureGenerated = false;
	Density3DTextureGenerator m_densityTexGenerator;

	bool m_isRockVertexBufferGenerated = false;
	RockVertexBufferGenerator m_rockVBGenerator;
};