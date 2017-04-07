#include "stdafx.h"
#include "ShaderLab.h"

bool ShaderLab::Initialize()
{
	if (!D3D11App::Initialize())
		return false;
	if (!loadContentAndShaders())
	{
		MessageBox(nullptr, TEXT("Failed to load content/shaders."), TEXT("Error"), MB_OK);
		return false;
	}
	return true;
}

void ShaderLab::update(float deltaTime)
{
}

void ShaderLab::render(float deltaTime)
{
	D3D11App::render(deltaTime);
}

bool ShaderLab::loadContentAndShaders()
{
	return true;
}

void ShaderLab::unloadContentAndShaders()
{
}

void ShaderLab::cleanup()
{
	D3D11App::cleanup();
}
