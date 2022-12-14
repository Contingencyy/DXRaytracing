#include "Pch.h"
#include "Graphics/Backend/Device.h"

Device::Device()
{
    EnableDebugLayer();

    CreateAdapter();
    CreateDevice();
}

Device::~Device()
{
}

void Device::CreateCommandQueue(CommandQueue& commandQueue, const D3D12_COMMAND_QUEUE_DESC& queueDesc)
{
    ComPtr<ID3D12CommandQueue> d3d12CommandQueue;
    DX_CALL(m_d3d12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&d3d12CommandQueue)));
    commandQueue.SetD3D12CommandQueue(d3d12CommandQueue);

    ComPtr<ID3D12Fence> d3d12Fence;
    DX_CALL(m_d3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&d3d12Fence)));
    commandQueue.SetD3D12Fence(d3d12Fence);
}

void Device::CreateCommandList(CommandList& commandList)
{
    ComPtr<ID3D12CommandAllocator> d3d12Allocator;
    DX_CALL(m_d3d12Device->CreateCommandAllocator(commandList.GetCommandListType(), IID_PPV_ARGS(&d3d12Allocator)));

    ComPtr<ID3D12GraphicsCommandList4> d3d12CommandList;
    DX_CALL(m_d3d12Device->CreateCommandList(0, commandList.GetCommandListType(), d3d12Allocator.Get(), nullptr, IID_PPV_ARGS(&d3d12CommandList)));

    commandList.SetD3D12CommandAllocator(d3d12Allocator);
    commandList.SetD3D12CommandList(d3d12CommandList);
}

void Device::CreateDescriptorHeap(const D3D12_DESCRIPTOR_HEAP_DESC& heapDesc, ComPtr<ID3D12DescriptorHeap>& heap)
{
    DX_CALL(m_d3d12Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap)));
}

void Device::CreatePipelineState(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& pipelineStateDesc, ComPtr<ID3D12PipelineState>& pipelineState)
{
    DX_CALL(m_d3d12Device->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&pipelineState)));
}

void Device::CreateRootSignature(const CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC& rootSignatureDesc, ComPtr<ID3D12RootSignature>& rootSignature)
{
    ComPtr<ID3DBlob> serializedRootSig;
    ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &serializedRootSig, &errorBlob);

    if (!SUCCEEDED(hr) || errorBlob)
    {
        LOG_ERR(static_cast<const char*>(errorBlob->GetBufferPointer()));
        errorBlob->Release();
    }

    DX_CALL(m_d3d12Device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
}

void Device::CreateBuffer(Buffer& buffer, D3D12_HEAP_TYPE bufferType, const D3D12_RESOURCE_DESC& bufferDesc, D3D12_RESOURCE_STATES initialState, std::size_t size)
{
    CD3DX12_HEAP_PROPERTIES heapProps(bufferType);

    ComPtr<ID3D12Resource> d3d12Resource;
    DX_CALL(m_d3d12Device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        initialState,
        nullptr,
        IID_PPV_ARGS(&d3d12Resource)
    ));

    buffer.SetD3D12Resource(d3d12Resource);
}

void Device::CreateTexture(Texture& texture, const D3D12_RESOURCE_DESC& textureDesc, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE* clearValue)
{
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    ComPtr<ID3D12Resource> d3d12Resource;
    DX_CALL(m_d3d12Device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        initialState,
        clearValue,
        IID_PPV_ARGS(&d3d12Resource)
    ));

    texture.SetD3D12Resource(d3d12Resource);
}

void Device::CreateRenderTargetView(Texture& texture, const D3D12_RENDER_TARGET_VIEW_DESC& rtvDesc, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
    m_d3d12Device->CreateRenderTargetView(texture.GetD3D12Resource().Get(), &rtvDesc, descriptor);
}

void Device::CreateDepthStencilView(Texture& texture, const D3D12_DEPTH_STENCIL_VIEW_DESC& dsvDesc, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
    m_d3d12Device->CreateDepthStencilView(texture.GetD3D12Resource().Get(), &dsvDesc, descriptor);
}

void Device::CreateConstantBufferView(Buffer& buffer, const D3D12_CONSTANT_BUFFER_VIEW_DESC& cbvDesc, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
    m_d3d12Device->CreateConstantBufferView(&cbvDesc, descriptor);
}

void Device::CreateShaderResourceView(Buffer& buffer, const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
    if (srvDesc.RaytracingAccelerationStructure.Location)
        m_d3d12Device->CreateShaderResourceView(nullptr, &srvDesc, descriptor);
    else
        m_d3d12Device->CreateShaderResourceView(buffer.GetD3D12Resource().Get(), &srvDesc, descriptor);
}

void Device::CreateShaderResourceView(Texture& texture, const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
    m_d3d12Device->CreateShaderResourceView(texture.GetD3D12Resource().Get(), &srvDesc, descriptor);
}

void Device::CreateUnorderedAccessView(Buffer& buffer, const D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
    m_d3d12Device->CreateUnorderedAccessView(buffer.GetD3D12Resource().Get(), nullptr, &uavDesc, descriptor);
}

void Device::CreateUnorderedAccessView(Texture& texture, const D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
    m_d3d12Device->CreateUnorderedAccessView(texture.GetD3D12Resource().Get(), nullptr, &uavDesc, descriptor);
}

uint32_t Device::GetDescriptorIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    return m_d3d12Device->GetDescriptorHandleIncrementSize(type);
}

void Device::CopyDescriptors(uint32_t numDescriptorRanges, const D3D12_CPU_DESCRIPTOR_HANDLE* destDescriptorRangeStarts, const uint32_t* destDescriptorRangeSizes, uint32_t numSrcDescriptorRanges, const D3D12_CPU_DESCRIPTOR_HANDLE* srcDescriptorRangeStarts, const uint32_t* srcDescriptorRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    m_d3d12Device->CopyDescriptors(numDescriptorRanges, destDescriptorRangeStarts, destDescriptorRangeSizes, numSrcDescriptorRanges, srcDescriptorRangeStarts, srcDescriptorRangeSizes, type);
}

void Device::EnableDebugLayer()
{
#if defined(_DEBUG)
    ComPtr<ID3D12Debug> d3d12DebugController0;
    ComPtr<ID3D12Debug1> d3d12DebugController1;

    DX_CALL(D3D12GetDebugInterface(IID_PPV_ARGS(&d3d12DebugController0)));
    d3d12DebugController0->EnableDebugLayer();

    if (GPU_VALIDATION_ENABLED)
    {
        DX_CALL(d3d12DebugController0->QueryInterface(IID_PPV_ARGS(&d3d12DebugController1)));
        d3d12DebugController1->SetEnableGPUBasedValidation(true);
        LOG_INFO("[Device] Enabled GPU based validation");
    }
#endif
}

void Device::CreateAdapter()
{
	ComPtr<IDXGIFactory4> dxgiFactory;
	UINT createFactoryFlags = 0;

#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	DX_CALL(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

	ComPtr<IDXGIAdapter1> dxgiAdapter1;

	SIZE_T maxDedicatedVideoMemory = 0;
	for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
		dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

		if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
			SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_0,
				__uuidof(ID3D12Device), nullptr)) && dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
		{
			maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
			DX_CALL(dxgiAdapter1.As(&m_dxgiAdapter));
		}
	}
}

void Device::CreateDevice()
{
    DX_CALL(D3D12CreateDevice(m_dxgiAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_d3d12Device)));

#if defined(_DEBUG)
    if (GPU_VALIDATION_ENABLED)
    {
        // Set up GPU validation
        ComPtr<ID3D12DebugDevice1> d3d12DebugDevice;
        DX_CALL(m_d3d12Device->QueryInterface(IID_PPV_ARGS(&d3d12DebugDevice)));

        D3D12_DEBUG_DEVICE_GPU_BASED_VALIDATION_SETTINGS debugValidationSettings = {};
        debugValidationSettings.MaxMessagesPerCommandList = 10;
        debugValidationSettings.DefaultShaderPatchMode = D3D12_GPU_BASED_VALIDATION_SHADER_PATCH_MODE_GUARDED_VALIDATION;
        debugValidationSettings.PipelineStateCreateFlags = D3D12_GPU_BASED_VALIDATION_PIPELINE_STATE_CREATE_FLAG_FRONT_LOAD_CREATE_GUARDED_VALIDATION_SHADERS;
        DX_CALL(d3d12DebugDevice->SetDebugParameter(D3D12_DEBUG_DEVICE_PARAMETER_GPU_BASED_VALIDATION_SETTINGS, &debugValidationSettings, sizeof(D3D12_DEBUG_DEVICE_GPU_BASED_VALIDATION_SETTINGS)));
    }

    // Set up info queue with filters
    ComPtr<ID3D12InfoQueue> pInfoQueue;

    if (SUCCEEDED(m_d3d12Device.As(&pInfoQueue)))
    {
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

        // Suppress whole categories of messages
        //D3D12_MESSAGE_CATEGORY categories[] = {};
        
        // Suppress messages based on their severity level
        D3D12_MESSAGE_SEVERITY severities[] =
        {
            D3D12_MESSAGE_SEVERITY_INFO
        };

        // Suppress individual messages by their ID
        D3D12_MESSAGE_ID denyIds[] =
        {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
            // This is a temporary fix for Windows 11 due to a bug that has not been fixed yet
            D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,
        };

        D3D12_INFO_QUEUE_FILTER newFilter = {};
        //newFilter.DenyList.NumCategories = _countof(categories);
        //newFilter.DenyList.pCategoryList = categories;
        newFilter.DenyList.NumSeverities = _countof(severities);
        newFilter.DenyList.pSeverityList = severities;
        newFilter.DenyList.NumIDs = _countof(denyIds);
        newFilter.DenyList.pIDList = denyIds;

        DX_CALL(pInfoQueue->PushStorageFilter(&newFilter));
    }
#endif
}
