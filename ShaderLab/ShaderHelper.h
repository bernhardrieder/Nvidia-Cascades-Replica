#pragma once
#include "stdafx.h"

namespace ShaderHelper
{
	inline bool LoadVertexShader(ID3D11Device* device, const wchar_t* compiledFilename, D3D11_INPUT_ELEMENT_DESC* inputDesc, const size_t& inputDescCount, ID3D11VertexShader* outVertexShader, ID3D11InputLayout* outInputLayout)
	{
		ID3DBlob* blob;

		HRESULT hr = D3DReadFileToBlob(compiledFilename, &blob);
		if (FAILED(hr))
			return false;

		hr = device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &outVertexShader);
		if (FAILED(hr))
			return false;

		hr = device->CreateInputLayout(inputDesc, inputDescCount, blob->GetBufferPointer(), blob->GetBufferSize(), &outInputLayout);
		if (FAILED(hr))
			return false;

		SafeRelease(blob);
		return true;
	}

	inline bool LoadPixelShader(ID3D11Device* device, const wchar_t* compiledFilename, ID3D11PixelShader* outPixelShader)
	{
		ID3DBlob* blob;

		HRESULT hr = D3DReadFileToBlob(compiledFilename, &blob);
		if (FAILED(hr))
			return false;

		hr = device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &outPixelShader);
		if (FAILED(hr))
			return false;

		SafeRelease(blob);
		return true;
	}

	inline bool LoadGeometryShader(ID3D11Device* device, const wchar_t* compiledFilename, ID3D11GeometryShader* outGeometryShader)
	{
		ID3DBlob* blob;

		HRESULT hr = D3DReadFileToBlob(compiledFilename, &blob);
		if (FAILED(hr))
			return false;

		hr = device->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(), NULL, &outGeometryShader);
		if (FAILED(hr))
			return false;

		SafeRelease(blob);
		return true;
	}

	inline bool LoadGeometryShaderWithStreamOutput(ID3D11Device* device, const wchar_t* compiledFilename, D3D11_SO_DECLARATION_ENTRY* soDecl, const size_t& soDeclCount, ID3D11GeometryShader* outGeometryShader)
	{
		ID3DBlob* blob;

		HRESULT hr = D3DReadFileToBlob(compiledFilename, &blob);
		if (FAILED(hr))
			return false;

		hr = device->CreateGeometryShaderWithStreamOutput(blob->GetBufferPointer(), blob->GetBufferSize(), soDecl, soDeclCount, NULL, 0, D3D11_SO_NO_RASTERIZED_STREAM, NULL, &outGeometryShader);
		if (FAILED(hr))
			return false;

		SafeRelease(blob);
		return true;
	}
}
