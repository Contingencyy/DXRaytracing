#include "Pch.h"
#include "Graphics/Renderer.h"
#include "Graphics/RenderPass.h"
#include "Graphics/Buffer.h"
#include "Graphics/Backend/RenderBackend.h"
#include "Graphics/Backend/SwapChain.h"
#include "Graphics/Backend/CommandList.h"
#include "Graphics/Backend/DescriptorHeap.h"
#include "Graphics/Backend/Device.h"
#include "Application.h"
#include "Window.h"
#include "Scene/Camera.h"

#include "ResourceLoader.h"

struct ViewData
{
	glm::mat4 ViewProjection;
	glm::vec4 ViewOriginAndTanHalfFovY;
	glm::vec2 Resolution;
};

struct RendererInternalData
{
	// TLAS
	std::unique_ptr<Buffer> TLASInstanceBuffer;
	std::unique_ptr<Buffer> TLASScratchBuffer;
	std::unique_ptr<Buffer> TLASBuffer;
	uint64_t TLASSize = 0;

	// BLAS
	std::unique_ptr<Buffer> BLASScratchBuffer;
	std::unique_ptr<Buffer> BLASBuffer;

	// Test Vertex and index buffer
	std::shared_ptr<Buffer> VertexBuffer;
	std::shared_ptr<Buffer> IndexBuffer;

	std::unique_ptr<RenderPass> RenderPass;

	ViewData ViewData;
	std::unique_ptr<Buffer> ViewConstantBuffer;

	struct Resolution
	{
		uint32_t x = 0;
		uint32_t y = 0;
	} Resolution;

	bool VSync = true;
};

static RendererInternalData s_Data;

void Renderer::Initialize(uint32_t resX, uint32_t resY)
{
	s_Data.Resolution.x = resX;
	s_Data.Resolution.y = resY;

	RenderBackend::Initialize(Application::Get().GetWindow().GetHandle(), s_Data.Resolution.x, s_Data.Resolution.y);

	s_Data.ViewConstantBuffer = std::make_unique<Buffer>("View constant buffer", BufferDesc(BufferUsage::BUFFER_USAGE_CONSTANT, 1, sizeof(ViewData)));

	CreateRenderPasses();

	CreateBLAS();
	CreateTLAS();
}

void Renderer::Finalize()
{
	RenderBackend::Finalize();
}

void Renderer::BeginScene(const Camera& sceneCamera)
{
	// Set view data and view constant buffer data
	s_Data.ViewData.ViewProjection = sceneCamera.GetViewProjection();
	s_Data.ViewData.ViewOriginAndTanHalfFovY = glm::vec4(s_Data.ViewData.ViewProjection[3][0],
		s_Data.ViewData.ViewProjection[3][1], s_Data.ViewData.ViewProjection[3][2], 1.0f);
	s_Data.ViewData.Resolution = glm::vec2(s_Data.Resolution.x, s_Data.Resolution.y);

	s_Data.ViewConstantBuffer->SetBufferData(&s_Data.ViewData);
}

void Renderer::Render()
{
	auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);

	// Resource barrier to put output texture into UNORDERED_ACCESS state
	CD3DX12_RESOURCE_BARRIER outputUAVBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		s_Data.RenderPass->GetColorAttachment()->GetD3D12Resource().Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	commandList->ResourceBarrier(1, &outputUAVBarrier);

	// Set descriptor heap
	auto bindlessDescriptorHeap = RenderBackend::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, *bindlessDescriptorHeap);

	auto& pipelineState = s_Data.RenderPass->GetPipelineState();
	auto& shaderTable = pipelineState.GetShaderTable();
	uint32_t shaderTableRecordSize = pipelineState.GetShaderTableRecordSize();

	D3D12_DISPATCH_RAYS_DESC desc = {};
	desc.RayGenerationShaderRecord.StartAddress = shaderTable.GetD3D12Resource()->GetGPUVirtualAddress();
	desc.RayGenerationShaderRecord.SizeInBytes = shaderTableRecordSize;

	desc.MissShaderTable.StartAddress = shaderTable.GetD3D12Resource()->GetGPUVirtualAddress() + shaderTableRecordSize;
	desc.MissShaderTable.SizeInBytes = shaderTableRecordSize;
	desc.MissShaderTable.StrideInBytes = shaderTableRecordSize;

	desc.HitGroupTable.StartAddress = shaderTable.GetD3D12Resource()->GetGPUVirtualAddress() + (shaderTableRecordSize * 2);
	desc.HitGroupTable.SizeInBytes = shaderTableRecordSize;
	desc.HitGroupTable.StrideInBytes = shaderTableRecordSize;

	desc.Width = s_Data.Resolution.x;
	desc.Height = s_Data.Resolution.y;
	desc.Depth = 1;

	commandList->SetPipelineState(pipelineState);
	commandList->DispatchRays(desc);

	RenderBackend::ExecuteCommandList(commandList);
}

void Renderer::EndScene()
{
	RenderBackend::GetSwapChain()->ResolveToBackBuffer(*s_Data.RenderPass->GetColorAttachment());
	RenderBackend::GetSwapChain()->SwapBuffers(s_Data.VSync);
}

void Renderer::OnWindowResize(uint32_t width, uint32_t height)
{
	width = std::max(1u, width);
	height = std::max(1u, height);

	s_Data.Resolution.x = std::max(1u, width);
	s_Data.Resolution.y = std::max(1u, height);

	RenderBackend::Resize(width, height);

	s_Data.RenderPass->ResizeAttachments(width, height);
}

void Renderer::ToggleVSync()
{
	s_Data.VSync = !s_Data.VSync;
}

glm::vec2 Renderer::GetResolution()
{
	return glm::vec2(s_Data.Resolution.x, s_Data.Resolution.y);
}

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
}

void Renderer::CreateRenderPasses()
{
	RenderPassDesc rpDesc = {};

	ShaderDesc sDesc = {};
	sDesc.Filepath = "Resources/Shaders/RaygenDefault.hlsl";
	sDesc.EntryPoint = "";
	sDesc.Target = "lib_6_3";
	rpDesc.ShaderDesc[ShaderType::RAYGEN] = sDesc;

	sDesc.Filepath = "Resources/Shaders/MissDefault.hlsl";
	rpDesc.ShaderDesc[ShaderType::MISS] = sDesc;

	sDesc.Filepath = "Resources/Shaders/ClosestHitDefault.hlsl";
	rpDesc.ShaderDesc[ShaderType::CLOSEST_HIT] = sDesc;

	rpDesc.ColorAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_READ | TextureUsage::TEXTURE_USAGE_WRITE,
		TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM, s_Data.Resolution.x, s_Data.Resolution.y);
	rpDesc.DepthStencilAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_DEPTH, TextureFormat::TEXTURE_FORMAT_DEPTH32,
		s_Data.Resolution.x, s_Data.Resolution.y);
	rpDesc.Name = "Default Render Pass";

	s_Data.RenderPass = std::make_unique<RenderPass>(rpDesc);
}

void Renderer::CreateBLAS()
{
	// Set test data for vertex and index buffer
	/*glm::vec3 vertices[4] = {
		{ -0.5f, 0.5f, 1.0f },
		{ 0.5f, 0.5f, 1.0f },
		{ 0.5f, -0.5f, 1.0f },
		{ -0.5f, -0.5f, 1.0f },
	};

	WORD indices[6] = {
		0, 1, 2,
		2, 3, 0
	};

	s_Data.VertexBuffer = std::make_shared<Buffer>("AS test triangle vertex buffer", BufferDesc(BufferUsage::BUFFER_USAGE_VERTEX,
		4, sizeof(glm::vec3)), &vertices);
	s_Data.IndexBuffer = std::make_shared<Buffer>("AS test triangle index buffer", BufferDesc(BufferUsage::BUFFER_USAGE_INDEX,
		6, sizeof(WORD)), &indices);*/

	Model model = ResourceLoader::LoadGLTF("Resources/Models/Sponza_OLD/Sponza.gltf");
	s_Data.VertexBuffer = model.VertexBuffer;
	s_Data.IndexBuffer = model.IndexBuffer;

	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
	geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geometryDesc.Triangles.VertexBuffer.StartAddress = s_Data.VertexBuffer->GetD3D12Resource()->GetGPUVirtualAddress();
	geometryDesc.Triangles.VertexBuffer.StrideInBytes = s_Data.VertexBuffer->GetBufferDesc().ElementSize;
	geometryDesc.Triangles.VertexCount = s_Data.VertexBuffer->GetBufferDesc().NumElements;
	geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geometryDesc.Triangles.IndexBuffer = s_Data.IndexBuffer->GetD3D12Resource()->GetGPUVirtualAddress();
	geometryDesc.Triangles.IndexFormat = s_Data.IndexBuffer->GetBufferDesc().ElementSize == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
	geometryDesc.Triangles.IndexCount = s_Data.IndexBuffer->GetBufferDesc().NumElements;
	geometryDesc.Triangles.Transform3x4 = 0;
	geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS ASInputs = {};
	ASInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	ASInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	ASInputs.pGeometryDescs = &geometryDesc;
	ASInputs.NumDescs = 1;
	ASInputs.Flags = buildFlags;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO ASPreBuildInfo = {};
	RenderBackend::GetDevice()->GetD3D12Device()->GetRaytracingAccelerationStructurePrebuildInfo(&ASInputs, &ASPreBuildInfo);

	ASPreBuildInfo.ScratchDataSizeInBytes = MathHelper::AlignUp(ASPreBuildInfo.ScratchDataSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
	ASPreBuildInfo.ResultDataMaxSizeInBytes = MathHelper::AlignUp(ASPreBuildInfo.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);

	// BLAS scratch buffer
	s_Data.BLASScratchBuffer = std::make_unique<Buffer>("BLAS scratch buffer", BufferDesc(BufferUsage::BUFFER_USAGE_WRITE,
		1, ASPreBuildInfo.ScratchDataSizeInBytes, std::max(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT)));

	// BLAS buffer
	s_Data.BLASBuffer = std::make_unique<Buffer>("BLAS buffer", BufferDesc(BufferUsage::BUFFER_USAGE_WRITE | BufferUsage::BUFFER_USAGE_RAYTRACING_ACCELERATION_STRUCTURE,
		1, ASPreBuildInfo.ResultDataMaxSizeInBytes, std::max(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT)));

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
	buildDesc.Inputs = ASInputs;
	buildDesc.ScratchAccelerationStructureData = s_Data.BLASScratchBuffer->GetD3D12Resource()->GetGPUVirtualAddress();
	buildDesc.DestAccelerationStructureData = s_Data.BLASBuffer->GetD3D12Resource()->GetGPUVirtualAddress();

	auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
	commandList->BuildRaytracingAccelerationStructure(buildDesc);

	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = s_Data.BLASBuffer->GetD3D12Resource().Get();
	uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

	commandList->ResourceBarrier(1, &uavBarrier);
	RenderBackend::ExecuteCommandList(commandList);
}

void Renderer::CreateTLAS()
{
	// Describe the TLAS geometry instance(s)
	D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
	instanceDesc.InstanceID = 0;
	instanceDesc.InstanceContributionToHitGroupIndex = 0;
	instanceDesc.InstanceMask = 0xFF;
	instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
	instanceDesc.AccelerationStructure = s_Data.BLASBuffer->GetD3D12Resource()->GetGPUVirtualAddress();
	instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE;

	s_Data.TLASInstanceBuffer = std::make_unique<Buffer>("TLAS instance buffer", BufferDesc(BufferUsage::BUFFER_USAGE_UPLOAD, 1, sizeof(instanceDesc)));
	s_Data.TLASInstanceBuffer->SetBufferData(&instanceDesc);

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS ASInputs = {};
	ASInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	ASInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	ASInputs.InstanceDescs = s_Data.TLASInstanceBuffer->GetD3D12Resource()->GetGPUVirtualAddress();
	ASInputs.NumDescs = 1;
	ASInputs.Flags = buildFlags;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO ASPreBuildInfo = {};
	RenderBackend::GetDevice()->GetD3D12Device()->GetRaytracingAccelerationStructurePrebuildInfo(&ASInputs, &ASPreBuildInfo);

	ASPreBuildInfo.ResultDataMaxSizeInBytes = MathHelper::AlignUp(ASPreBuildInfo.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
	ASPreBuildInfo.ScratchDataSizeInBytes = MathHelper::AlignUp(ASPreBuildInfo.ScratchDataSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);

	s_Data.TLASSize = ASPreBuildInfo.ResultDataMaxSizeInBytes;

	// TLAS scratch buffer
	s_Data.TLASScratchBuffer = std::make_unique<Buffer>("TLAS scratch buffer", BufferDesc(BufferUsage::BUFFER_USAGE_WRITE,
		1, ASPreBuildInfo.ScratchDataSizeInBytes, std::max(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT)));

	// TLAS buffer
	s_Data.TLASBuffer = std::make_unique<Buffer>("TLAS buffer", BufferDesc(BufferUsage::BUFFER_USAGE_WRITE | BufferUsage::BUFFER_USAGE_RAYTRACING_ACCELERATION_STRUCTURE,
		1, ASPreBuildInfo.ResultDataMaxSizeInBytes, std::max(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT)));

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
	buildDesc.Inputs = ASInputs;
	buildDesc.ScratchAccelerationStructureData = s_Data.TLASScratchBuffer->GetD3D12Resource()->GetGPUVirtualAddress();
	buildDesc.DestAccelerationStructureData = s_Data.TLASBuffer->GetD3D12Resource()->GetGPUVirtualAddress();

	auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
	commandList->BuildRaytracingAccelerationStructure(buildDesc);

	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = s_Data.TLASBuffer->GetD3D12Resource().Get();
	uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

	commandList->ResourceBarrier(1, &uavBarrier);
	RenderBackend::ExecuteCommandList(commandList);
}
