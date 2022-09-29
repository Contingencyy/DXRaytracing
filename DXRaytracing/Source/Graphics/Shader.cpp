#include "Pch.h"
#include "Graphics/Shader.h"
#include "Graphics/Backend/RootSignature.h"

static ComPtr<IDxcLibrary> library;
static ComPtr<IDxcCompiler> compiler;

Shader::Shader(const ShaderDesc& desc)
    : m_Desc(desc)
{
    if (!library)
        DX_CALL(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library)));
    if (!compiler)
        DX_CALL(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));

    Compile();
}

Shader::~Shader()
{
}

void Shader::Compile()
{
    std::vector<LPCWSTR> args;
    //args.emplace_back(DXC_ARG_SKIP_VALIDATION);

#ifdef _DEBUG
    args.emplace_back(DXC_ARG_DEBUG);
    args.emplace_back(DXC_ARG_SKIP_OPTIMIZATIONS);
#endif

    uint32_t codePage = 0;
    ComPtr<IDxcBlobEncoding> sourceBlob;
    DX_CALL(library->CreateBlobFromFile(StringHelper::StringToWString(m_Desc.Filepath).c_str(), &codePage, &sourceBlob));

    // Include handler if necessary
    //ComPtr<IDxcIncludeHandler> dxcIncludeHandler;
    //DX_CALL(library->CreateIncludeHandler(&dxcIncludeHandler));

    ComPtr<IDxcOperationResult> result;
    HRESULT hr = compiler->Compile(
        sourceBlob.Get(),
        StringHelper::StringToWString(m_Desc.Filepath).c_str(),
        StringHelper::StringToWString(m_Desc.EntryPoint).c_str(),
        StringHelper::StringToWString(m_Desc.Target).c_str(),
        args.data(), args.size(),
        NULL, 0,
        NULL,
        &result
    );

    ASSERT(hr == S_OK, "Failed to compile shader");
    result->GetStatus(&hr);

    if (FAILED(hr))
    {
        IDxcBlobEncoding* error;
        hr = result->GetErrorBuffer(&error);
        ASSERT(hr == S_OK, "Failed to gete shader compiler error buffer");

        printf(static_cast<const char*>(error->GetBufferPointer()));
        error->Release();
    }

    result->GetResult(&m_ShaderByteBlob);
    ASSERT(hr == S_OK, "Failed to get shader blob result");

    m_ShaderByteCode.pShaderBytecode = m_ShaderByteBlob->GetBufferPointer();
    m_ShaderByteCode.BytecodeLength = m_ShaderByteBlob->GetBufferSize();
}
