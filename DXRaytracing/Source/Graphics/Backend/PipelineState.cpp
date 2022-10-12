#include "Pch.h"
#include "Graphics/Shader.h"
#include "Graphics/Backend/PipelineState.h"
#include "Graphics/Backend/Device.h"
#include "Graphics/Backend/RootSignature.h"
#include "Graphics/Backend/RenderBackend.h"
#include "Graphics/Backend/DescriptorHeap.h"

struct DefaultRayPayload
{
	glm::vec3 Color;
};

PipelineState::PipelineState(const std::string& name, const D3D12_SHADER_BYTECODE& rayGenByteCode,
	const D3D12_SHADER_BYTECODE& missByteCode, const D3D12_SHADER_BYTECODE& closestHitByteCode)
{
	CreateRootSignatures();
	CreateStateObject(name, rayGenByteCode, missByteCode, closestHitByteCode);
	CreateShaderTable();
}

PipelineState::~PipelineState()
{
}

void PipelineState::CreateRootSignatures()
{
	auto device = RenderBackend::GetDevice();

	// Global Root Signature
	// This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
	{
		//CD3DX12_DESCRIPTOR_RANGE descriptorRange[3] = {};
		//descriptorRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, 0); // View constant buffer
		//descriptorRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, 2); // Output
		//descriptorRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, 7); // Acceleration structure
		//CD3DX12_ROOT_PARAMETER rootParameters[1] = {};
		//rootParameters[0].InitAsDescriptorTable(3, &descriptorRange[0]);
		//CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);

		//ComPtr<ID3DBlob> blob;
		//ComPtr<ID3DBlob> error;
		//DX_CALL(D3D12SerializeRootSignature(&globalRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error), error ? static_cast<wchar_t*>(error->GetBufferPointer()) : nullptr);
		//DX_CALL(device->GetD3D12Device()->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&m_GlobalRootSignature)));
	}
	
	// Local Root Signature
	// This is a root signature that enables a shader to have unique arguments that come from shader tables.
	{
		CD3DX12_DESCRIPTOR_RANGE descriptorRanges[6] = {};
		descriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, 1); // View constant buffer
		descriptorRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, 3); // Output
		descriptorRanges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, 11); // Acceleration structure
		descriptorRanges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1, 5); // Vertex buffer
		descriptorRanges[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 2, 6); // Index buffer
		descriptorRanges[5].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 3, 4); // Base color texture
		
		CD3DX12_ROOT_PARAMETER rootParameters[1] = {};
		rootParameters[0].InitAsDescriptorTable(6, &descriptorRanges[0]);

		CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
		localRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

		ComPtr<ID3DBlob> blob;
		ComPtr<ID3DBlob> error;

		HRESULT hr = D3D12SerializeRootSignature(&localRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error);
		if (!SUCCEEDED(hr))
		{
			ASSERT(false, static_cast<const char*>(error->GetBufferPointer()));
		}

		DX_CALL(device->GetD3D12Device()->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&m_LocalRootSignature)));
	}
}

void PipelineState::CreateStateObject(const std::string& name, const D3D12_SHADER_BYTECODE& rayGenByteCode,
	const D3D12_SHADER_BYTECODE& missByteCode, const D3D12_SHADER_BYTECODE& closestHitByteCode)
{
	auto device = RenderBackend::GetDevice();

	// Subobjects need to be associated with DXIL exports (i.e. shaders) either by way of default or explicit associations.
	// Default association applies to every exported shader entrypoint that doesn't have any of the same type of subobject associated with it.
	// This simple sample utilizes default shader association except for local root signature subobject
	// which has an explicit association specified purely for demonstration purposes.
	// Subobjects:
	// - Ray generation shader
	// - Miss shader
	// - Closest hit shader
	// - Hit group
	// - root signature and association
	// - Shader config and association
	// - Global root signature
	// - RT state

	CD3DX12_STATE_OBJECT_DESC raytracingPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

	// DXIL library
	// This contains the shaders and their entrypoints for the state object.
	// Since shaders are not considered a subobject, they need to be passed in via DXIL library subobjects.
	// Also define which shader exports to surface from the library.
	// If no shader exports are defined for a DXIL library subobject, all shaders will be surfaced.
	// In this sample, this could be omitted for convenience since the sample uses all shaders in the library.
	auto rayGenLib = raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
	rayGenLib->SetDXILLibrary(&rayGenByteCode);
	// Name: Unique identifier to be used elsewhere
	// ExportToRename: Entrypoint name in shader code
	rayGenLib->DefineExport(L"RayGenShader_Default", L"main");

	auto missLib = raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
	missLib->SetDXILLibrary(&missByteCode);
	missLib->DefineExport(L"MissShader_Default", L"main");

	auto closestHitLib = raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
	closestHitLib->SetDXILLibrary(&closestHitByteCode);
	closestHitLib->DefineExport(L"ClosestHitShader_Default", L"main");

	// Triangle hit group
	// A hit group specifies closest hit, any hit and intersection shaders to be executed when a ray intersects the geometry's triangle/AABB.
	// In this sample, we only use triangle geometry with a closest hit shader, so others are not set.
	auto hitGroup = raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
	hitGroup->SetClosestHitShaderImport(L"ClosestHitShader_Default");
	hitGroup->SetHitGroupExport(L"HitGroupTriangle_Default");
	hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

	// Shader config
	// Defines the maximum sizes in bytes for the ray payload and attribute structure.
	auto shaderConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
	UINT payloadSize = 4 * sizeof(float);   // float4 color
	UINT attributeSize = 2 * sizeof(float); // float2 barycentrics
	shaderConfig->Config(payloadSize, attributeSize);

	// Shader payload association
	auto shaderAssociation = raytracingPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
	const wchar_t* shaderExports[] = { L"RayGenShader_Default", L"MissShader_Default", L"HitGroupTriangle_Default" };
	shaderAssociation->AddExports(shaderExports, 3);
	shaderAssociation->SetSubobjectToAssociate(*shaderConfig);

	// Local root signature and shader association
	// This is a root signature that enables a shader to have unique arguments that come from shader tables.
	auto localRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
	localRootSignature->SetRootSignature(m_LocalRootSignature.Get());

	//// Shader association
	auto rootSignatureAssociation = raytracingPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
	const wchar_t* rootSigExports[] = { L"RayGenShader_Default", L"HitGroupTriangle_Default", L"MissShader_Default" };
	rootSignatureAssociation->AddExports(rootSigExports, 3);
	rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);

	// Global root signature
	// This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
	/*auto globalRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
	globalRootSignature->SetRootSignature(m_GlobalRootSignature.Get());*/

	// Pipeline config
	// Defines the maximum TraceRay() recursion depth.
	auto pipelineConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
	// PERFOMANCE TIP: Set max recursion depth as low as needed 
	// as drivers may apply optimization strategies for low recursion depths. 
	UINT maxRecursionDepth = 1; // ~ primary rays only. 
	pipelineConfig->Config(maxRecursionDepth);

	// Create the state object.
	DX_CALL(device->GetD3D12Device()->CreateStateObject(raytracingPipeline, IID_PPV_ARGS(&m_d3d12StateObject)));
	DX_CALL(m_d3d12StateObject->QueryInterface(IID_PPV_ARGS(&m_d3d12StateProperties)));
}

void PipelineState::CreateShaderTable()
{
	/*
		Shader table layout:
		- ray generation shader
		- miss shader
		- closest hit shader

		All shader records in the shader table must have the same size.
		32 bytes  - D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
		+ 8 bytes - CBV_SRV_UAV descriptor table pointer
		= 40 bytes
		Need to align this to 64 bytes, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT.
	*/

	uint32_t shaderIdSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	uint32_t shaderTableSize = 0;

	m_ShaderTableRecordSize = shaderIdSize;
	m_ShaderTableRecordSize += 8; // CBV_SRV_UAV descriptor heap
	m_ShaderTableRecordSize = MathHelper::AlignUp(m_ShaderTableRecordSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

	shaderTableSize = m_ShaderTableRecordSize * 3;
	shaderTableSize = MathHelper::AlignUp(shaderTableSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

	uint32_t currentOffset = 0;
	m_ShaderTable = std::make_unique<Buffer>("Shader table buffer", BufferDesc(BufferUsage::BUFFER_USAGE_UPLOAD, 1, shaderTableSize));
	m_ShaderTable->SetBufferData(m_d3d12StateProperties->GetShaderIdentifier(L"RayGenShader_Default"), shaderIdSize);

	auto gpuBaseDescriptor = RenderBackend::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->GetGPUBaseDescriptor();
	m_ShaderTable->SetBufferDataAtOffset(&gpuBaseDescriptor, sizeof(D3D12_GPU_DESCRIPTOR_HANDLE), currentOffset + shaderIdSize);

	currentOffset += m_ShaderTableRecordSize;
	m_ShaderTable->SetBufferDataAtOffset(m_d3d12StateProperties->GetShaderIdentifier(L"MissShader_Default"), shaderIdSize, currentOffset);

	currentOffset += m_ShaderTableRecordSize;
	m_ShaderTable->SetBufferDataAtOffset(m_d3d12StateProperties->GetShaderIdentifier(L"HitGroupTriangle_Default"), shaderIdSize, currentOffset);
	m_ShaderTable->SetBufferDataAtOffset(&gpuBaseDescriptor, sizeof(D3D12_GPU_DESCRIPTOR_HANDLE), currentOffset + shaderIdSize);
}
