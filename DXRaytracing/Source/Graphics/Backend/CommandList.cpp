#include "Pch.h"
#include "Graphics/Buffer.h"
#include "Graphics/Backend/CommandList.h"
#include "Graphics/Backend/Device.h"
#include "Graphics/Backend/DescriptorHeap.h"

CommandList::CommandList(std::shared_ptr<Device> device, D3D12_COMMAND_LIST_TYPE type)
	: m_d3d12CommandListType(type), m_Device(device)
{
	m_Device->CreateCommandList(*this);
	
	m_RootSignature = nullptr;
	for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_DescriptorHeaps[i] = nullptr;
	}
}

CommandList::~CommandList()
{
}

void CommandList::ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const float* clearColor)
{
	m_d3d12CommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

void CommandList::ClearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE dsv, float depth)
{
	m_d3d12CommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

void CommandList::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, const DescriptorHeap& descriptorHeap)
{
	ID3D12DescriptorHeap* d3d12DescriptorHeap = descriptorHeap.GetD3D12DescriptorHeap().Get();
	if (m_DescriptorHeaps[type] != d3d12DescriptorHeap)
	{
		m_DescriptorHeaps[type] = d3d12DescriptorHeap;

		uint32_t numDescriptorHeaps = 0;
		ID3D12DescriptorHeap* descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};

		for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		{
			ID3D12DescriptorHeap* descriptorHeap = m_DescriptorHeaps[i];
			if (descriptorHeap)
				descriptorHeaps[numDescriptorHeaps++] = descriptorHeap;
		}

		m_d3d12CommandList->SetDescriptorHeaps(numDescriptorHeaps, descriptorHeaps);
	}
}

void CommandList::SetRootDescriptorTable(uint32_t rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor)
{
	m_d3d12CommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, baseDescriptor);
}

void CommandList::SetPipelineState(const PipelineState& pipelineState)
{
	m_d3d12CommandList->SetPipelineState1(pipelineState.GetStateObject().Get());
}

void CommandList::BuildRaytracingAccelerationStructure(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& buildDesc)
{
	m_d3d12CommandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
}

void CommandList::DispatchRays(const D3D12_DISPATCH_RAYS_DESC& dispatchRayDesc)
{
	m_d3d12CommandList->DispatchRays(&dispatchRayDesc);
}

void CommandList::CopyBuffer(Buffer& intermediateBuffer, Buffer& destBuffer, const void* bufferData)
{
	std::size_t alignedBufferSize = intermediateBuffer.GetByteSize();

	if (alignedBufferSize > 0 && bufferData != nullptr)
	{
		CD3DX12_RESOURCE_BARRIER copyDestBarrier = CD3DX12_RESOURCE_BARRIER::Transition(destBuffer.GetD3D12Resource().Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		ResourceBarrier(1, &copyDestBarrier);

		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = bufferData;
		subresourceData.RowPitch = alignedBufferSize;
		subresourceData.SlicePitch = subresourceData.RowPitch;

		UpdateSubresources(m_d3d12CommandList.Get(), destBuffer.GetD3D12Resource().Get(),
			intermediateBuffer.GetD3D12Resource().Get(), 0, 0, 1, &subresourceData);

		CD3DX12_RESOURCE_BARRIER commonBarrier = CD3DX12_RESOURCE_BARRIER::Transition(destBuffer.GetD3D12Resource().Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
		ResourceBarrier(1, &commonBarrier);

		TrackObject(intermediateBuffer.GetD3D12Resource());
		TrackObject(destBuffer.GetD3D12Resource());
	}
}

void CommandList::CopyBufferRegion(Buffer& intermediateBuffer, std::size_t intermediateOffset, Buffer& destBuffer, std::size_t destOffset, std::size_t numBytes)
{
	ASSERT(intermediateOffset + numBytes <= intermediateBuffer.GetByteSize(), "Intermediate offset is bigger than the intermediate buffer byte size");
	ASSERT(destOffset + numBytes <= destBuffer.GetByteSize(), "Destination offset is bigger than the destination buffer byte size");

	if (numBytes > 0)
	{
		CD3DX12_RESOURCE_BARRIER copyDestBarrier = CD3DX12_RESOURCE_BARRIER::Transition(destBuffer.GetD3D12Resource().Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		ResourceBarrier(1, &copyDestBarrier);

		m_d3d12CommandList->CopyBufferRegion(destBuffer.GetD3D12Resource().Get(), destOffset, intermediateBuffer.GetD3D12Resource().Get(), intermediateOffset, numBytes);

		CD3DX12_RESOURCE_BARRIER commonBarrier = CD3DX12_RESOURCE_BARRIER::Transition(destBuffer.GetD3D12Resource().Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
		ResourceBarrier(1, &commonBarrier);

		TrackObject(intermediateBuffer.GetD3D12Resource());
		TrackObject(destBuffer.GetD3D12Resource());
	}
}

void CommandList::CopyTexture(Buffer& intermediateBuffer, Texture& destTexture, const void* textureData)
{
	TextureDesc textureDesc = destTexture.GetTextureDesc();

	if (textureData != nullptr)
	{
		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = textureData;
		subresourceData.RowPitch = static_cast<std::size_t>(textureDesc.Width * 4);
		subresourceData.SlicePitch = subresourceData.RowPitch * textureDesc.Height;

		UpdateSubresources(m_d3d12CommandList.Get(), destTexture.GetD3D12Resource().Get(),
			intermediateBuffer.GetD3D12Resource().Get(), 0, 0, 1, &subresourceData);

		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(destTexture.GetD3D12Resource().Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
		ResourceBarrier(1, &barrier);

		TrackObject(intermediateBuffer.GetD3D12Resource());
		TrackObject(destTexture.GetD3D12Resource());
	}
}

void CommandList::ResolveTexture(const Texture& srcTexture, const Texture& destTexture)
{
	m_d3d12CommandList->CopyResource(destTexture.GetD3D12Resource().Get(), srcTexture.GetD3D12Resource().Get());

	TrackObject(destTexture.GetD3D12Resource());
	TrackObject(srcTexture.GetD3D12Resource());

	//m_d3d12CommandList->ResolveSubresource();
}

void CommandList::ResourceBarrier(uint32_t numBarriers, const D3D12_RESOURCE_BARRIER* barriers)
{
	m_d3d12CommandList->ResourceBarrier(numBarriers, barriers);
}

void CommandList::TrackObject(ComPtr<ID3D12Object> object)
{
	m_TrackedObjects.push_back(object);
}

void CommandList::ReleaseTrackedObjects()
{
	m_TrackedObjects.clear();
}

void CommandList::Close()
{
	m_d3d12CommandList->Close();
}

void CommandList::Reset()
{
	DX_CALL(m_d3d12CommandAllocator->Reset());
	DX_CALL(m_d3d12CommandList->Reset(m_d3d12CommandAllocator.Get(), nullptr));

	ReleaseTrackedObjects();

	m_RootSignature = nullptr;

	for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_DescriptorHeaps[i] = nullptr;
	}
}
