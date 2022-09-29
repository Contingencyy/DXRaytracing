#pragma once

class Buffer;

struct Model
{
	std::shared_ptr<Buffer> VertexBuffer;
	std::shared_ptr<Buffer> IndexBuffer;
};

class ResourceLoader
{
public:
	static Model LoadGLTF(const std::string& filepath);

};
