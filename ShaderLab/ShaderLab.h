#pragma once
#include "stdafx.h"
#include "Camera.h"
#include <CommonStates.h>
#include "RockVertexBufferGenerator.h"
#include "Density3DTextureGenerator.h"
#include <SimpleMath.h>

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
	bool loadShaders();
	void unloadShaders();
	void onResize() override;
	void onMouseDown(WPARAM btnState, int x, int y) override;
	void onMouseUp(WPARAM btnState, int x, int y) override;
	void onMouseMove(WPARAM btnState, int x, int y) override;
	void checkAndProcessKeyboardInput(float deltaTime);
	bool initDirectX() override;
	void cleanup() override;

	bool createTextures(ID3D11Device* device);
	void releaseTextures();

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
	struct VertexPosNormal
	{
		XMFLOAT3 Position;
		XMFLOAT3 Normal;
	};

	// Vertex buffer data
	ID3D11InputLayout* m_inputLayoutSimpleVS = nullptr;

	// Shader data
	ID3D11VertexShader* m_simpleVS = nullptr;
	ID3D11PixelShader* m_simplePS = nullptr;
	ID3D11GeometryShader* m_simpleGS = nullptr;
	ID3D11Buffer* m_constantBuffers[NumConstantBuffers];
	std::unique_ptr<DirectX::CommonStates> m_commonStates;

	POINT m_lastMousePos;
	Camera m_camera;
	SimpleMath::Matrix m_worldMatrix = SimpleMath::Matrix::Identity;;

	DirectX::XMINT3 m_rockSize;

	bool m_isDensityTextureGenerated = false;
	Density3DTextureGenerator m_densityTexGenerator;

	bool m_isRockVertexBufferGenerated = false;
	RockVertexBufferGenerator m_rockVBGenerator;

	
	const size_t m_textureCount = 5;
	std::vector<ID3D11ShaderResourceView*> m_texturesSRVs;
	std::wstring m_textureFilesPath = L"Assets/Textures/";
	std::wstring m_textureGenericFilename = L"lichen";
	std::wstring m_textureFilenameExtension = L".dds";

};