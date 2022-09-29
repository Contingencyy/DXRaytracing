#pragma once
#include "Graphics/Texture.h"
#include "Graphics/RenderPass.h"

class RootSignature;
class Shader;
class Buffer;

class PipelineState
{
public:
	PipelineState(const std::string& name, const D3D12_SHADER_BYTECODE& rayGenByteCode,
		const D3D12_SHADER_BYTECODE& missByteCode, const D3D12_SHADER_BYTECODE& closestHitByteCode);
	~PipelineState();

	const Buffer& GetShaderTable() const { return *m_ShaderTable; }
	uint32_t GetShaderTableRecordSize() const { return m_ShaderTableRecordSize; }

	ComPtr<ID3D12StateObject> GetStateObject() const { return m_d3d12StateObject; };
	D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const { return m_d3d12PrimitiveToplogy; }
	
private:
	void CreateRootSignatures();
	void CreateStateObject(const std::string& name, const D3D12_SHADER_BYTECODE& rayGenByteCode,
		const D3D12_SHADER_BYTECODE& missByteCode, const D3D12_SHADER_BYTECODE& closestHitByteCode);
	void CreateShaderTable();

private:
	ComPtr<ID3D12StateObject> m_d3d12StateObject;
	ComPtr<ID3D12StateObjectProperties> m_d3d12StateProperties;
	D3D12_PRIMITIVE_TOPOLOGY m_d3d12PrimitiveToplogy;

	std::unique_ptr<Buffer> m_ShaderTable;
	uint32_t m_ShaderTableRecordSize = 0;

	ComPtr<ID3D12RootSignature> m_LocalRootSignature;
	ComPtr<ID3D12RootSignature> m_GlobalRootSignature;

};
