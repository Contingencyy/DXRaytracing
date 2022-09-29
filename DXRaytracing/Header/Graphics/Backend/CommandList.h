#pragma once
#include "Graphics/Buffer.h"
#include "Graphics/Texture.h"
#include "Graphics/Backend/PipelineState.h"
#include "Graphics/Backend/RootSignature.h"

class DescriptorHeap;
class DynamicDescriptorHeap;
class Device;

class CommandList
{
public:
	CommandList(std::shared_ptr<Device> device, D3D12_COMMAND_LIST_TYPE type);
	~CommandList();

	void ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const float* clearColor);
	void ClearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE dsv, float depth = 1.0f);

	void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, const DescriptorHeap& descriptorHeap);
	void SetRootDescriptorTable(uint32_t rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor);
	void SetPipelineState(const PipelineState& pipelineState);

	void BuildRaytracingAccelerationStructure(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& buildDesc);
	void DispatchRays(const D3D12_DISPATCH_RAYS_DESC& dispatchRayDesc);

	void CopyBuffer(Buffer& intermediateBuffer, Buffer& destBuffer, const void* bufferData);
	void CopyBufferRegion(Buffer& intermediateBuffer, std::size_t intermediateOffset, Buffer& destBuffer, std::size_t destOffset, std::size_t numBytes);
	void CopyTexture(Buffer& intermediateBuffer, Texture& destTexture, const void* textureData);
	void ResolveTexture(const Texture& srcTexture, const Texture& destTexture);

	void ResourceBarrier(uint32_t numBarriers, const D3D12_RESOURCE_BARRIER* barriers);
	void TrackObject(ComPtr<ID3D12Object> object);
	void ReleaseTrackedObjects();

	void Close();
	void Reset();

	void SetD3D12CommandAllocator(ComPtr<ID3D12CommandAllocator> d3d12CommandAllocator) { m_d3d12CommandAllocator = d3d12CommandAllocator; }
	void SetD3D12CommandList(ComPtr<ID3D12GraphicsCommandList4> d3d12CommandList) { m_d3d12CommandList = d3d12CommandList; }

	D3D12_COMMAND_LIST_TYPE GetCommandListType() const { return m_d3d12CommandListType; }
	ComPtr<ID3D12GraphicsCommandList4> GetGraphicsCommandList() const { return m_d3d12CommandList; }

private:
	ComPtr<ID3D12GraphicsCommandList4> m_d3d12CommandList;
	D3D12_COMMAND_LIST_TYPE m_d3d12CommandListType;
	ComPtr<ID3D12CommandAllocator> m_d3d12CommandAllocator;

	std::shared_ptr<Device> m_Device;

	std::vector<ComPtr<ID3D12Object>> m_TrackedObjects;

	ID3D12RootSignature* m_RootSignature;
	ID3D12DescriptorHeap* m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

};
