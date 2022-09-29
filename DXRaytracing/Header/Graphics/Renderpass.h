#pragma once
#include "Graphics/Texture.h"
#include "Graphics/Shader.h"

class PipelineState;
class Shader;
class Texture;

enum ShaderType : uint32_t
{
	RAYGEN,
	INTERSECT,
	CLOSEST_HIT,
	ANY_HIT,
	MISS,
	NUM_SHADER_TYPES
};

struct RenderPassDesc
{
	ShaderDesc ShaderDesc[NUM_SHADER_TYPES];

	TextureDesc ColorAttachmentDesc;
	TextureDesc DepthStencilAttachmentDesc;

	std::string Name;
};

class RenderPass
{
public:
	RenderPass(const RenderPassDesc& desc);
	~RenderPass();

	void ResizeAttachments(uint32_t width, uint32_t height);

	const PipelineState& GetPipelineState() const { return *m_PipelineState; }
	std::shared_ptr<Texture> GetColorAttachment() const { return m_ColorAttachment; }
	std::shared_ptr<Texture> GetDepthStencilAttachment() const { return m_DepthStencilAttachment; }

private:
	RenderPassDesc m_Desc;

	std::unique_ptr<Shader> m_Shader[ShaderType::NUM_SHADER_TYPES];
	std::unique_ptr<PipelineState> m_PipelineState;

	std::shared_ptr<Texture> m_ColorAttachment;
	std::shared_ptr<Texture> m_DepthStencilAttachment;

};
