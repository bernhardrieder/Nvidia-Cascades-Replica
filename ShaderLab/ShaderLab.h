#pragma once

class ShaderLab : public D3D11App
{
public:
	ShaderLab(HINSTANCE hInstance, int nCmdShow) : D3D11App(hInstance, nCmdShow){}
	ShaderLab(const ShaderLab& rhs) = delete;
	ShaderLab& operator=(const ShaderLab& rhs) = delete;

	bool Initialize() override;

private:
	void update(float deltaTime) override;
	void render(float deltaTime) override;
	bool loadContentAndShaders();
	void unloadContentAndShaders();
	void cleanup() override;
};