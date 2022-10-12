#pragma once

class Buffer;
class Texture;

struct Model
{
	std::shared_ptr<Buffer> VertexBuffer;
	std::shared_ptr<Buffer> IndexBuffer;
	std::vector<std::shared_ptr<Texture>> Textures;
};

class ResourceLoader
{
public:
	static Model LoadGLTF(const std::string& filepath);

};
