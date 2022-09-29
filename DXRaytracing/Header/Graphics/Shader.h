#pragma once

class RootSignature;

struct ShaderDesc
{
	std::string Filepath;
	std::string EntryPoint;
	std::string Target;
};

class Shader
{
public:
	Shader(const ShaderDesc& desc);
	~Shader();

	D3D12_SHADER_BYTECODE GetShaderByteCode() const { return m_ShaderByteCode; }

private:
	void Compile();

private:
	ShaderDesc m_Desc;

	ComPtr<IDxcBlob> m_ShaderByteBlob;
	D3D12_SHADER_BYTECODE m_ShaderByteCode;

};
