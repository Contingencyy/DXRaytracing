#include "Pch.h"
#include "Graphics/Renderpass.h"
#include "Graphics/Backend/PipelineState.h"

RenderPass::RenderPass(const RenderPassDesc& desc)
	: m_Desc(desc)
{
	for (uint32_t i = 0; i < ShaderType::NUM_SHADER_TYPES; ++i)
	{
		if (m_Desc.ShaderDesc[i].Filepath != "")
			m_Shader[i] = std::make_unique<Shader>(m_Desc.ShaderDesc[i]);
	}

	m_PipelineState = std::make_unique<PipelineState>(m_Desc.Name + " pipeline state", m_Shader[ShaderType::RAYGEN]->GetShaderByteCode(),
		m_Shader[ShaderType::MISS]->GetShaderByteCode(), m_Shader[ShaderType::CLOSEST_HIT]->GetShaderByteCode());
	m_ColorAttachment = std::make_shared<Texture>(m_Desc.Name + " color attachment", m_Desc.ColorAttachmentDesc);
	m_DepthStencilAttachment = std::make_unique<Texture>(m_Desc.Name + " depth stencil attachment", m_Desc.DepthStencilAttachmentDesc);
}

RenderPass::~RenderPass()
{
}

void RenderPass::ResizeAttachments(uint32_t width, uint32_t height)
{
	m_ColorAttachment->Resize(width, height);
	m_DepthStencilAttachment->Resize(width, height);
}
