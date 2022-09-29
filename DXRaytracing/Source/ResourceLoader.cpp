#include "Pch.h"
#include "ResourceLoader.h"
#include "Graphics/Buffer.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_USE_CPP14
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#include "tinygltf/tiny_gltf.h"

Model ResourceLoader::LoadGLTF(const std::string& filepath)
{
	tinygltf::Model tinygltf;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	bool result = loader.LoadASCIIFromFile(&tinygltf, &err, &warn, filepath);
	if (!warn.empty())
		LOG_WARN(warn);
	if (!err.empty())
		LOG_ERR(err);

	ASSERT(result, "Failed to parse glTF model: " + filepath);
	
	Model model;
	std::size_t totalVertexBufferSize = 0, totalIndexBufferSize = 0;

	for (auto& mesh : tinygltf.meshes)
	{
		for (auto& primitive : mesh.primitives)
		{
			uint32_t vertexPositionsIndex = primitive.attributes.find("POSITION")->second;

			tinygltf::Accessor& vertexAccessor = tinygltf.accessors[vertexPositionsIndex];
			tinygltf::Accessor& indexAccessor = tinygltf.accessors[primitive.indices];

			tinygltf::BufferView& vertexBufferView = tinygltf.bufferViews[vertexAccessor.bufferView];
			tinygltf::BufferView& indexBufferView = tinygltf.bufferViews[indexAccessor.bufferView];

			totalVertexBufferSize += vertexBufferView.byteLength;
			totalIndexBufferSize += indexBufferView.byteLength;
		}
	}

	unsigned char* vertexData = new unsigned char[totalVertexBufferSize];
	unsigned char* indexData = new unsigned char[totalIndexBufferSize];

	std::size_t currentVertexDataOffset = 0;
	std::size_t currentIndexDataOffset = 0;

	model.VertexBuffer = std::make_shared<Buffer>("Model vertex buffer", BufferDesc(BufferUsage::BUFFER_USAGE_VERTEX, totalVertexBufferSize / sizeof(glm::vec3), sizeof(glm::vec3)));
	model.IndexBuffer = std::make_shared<Buffer>("Model index buffer", BufferDesc(BufferUsage::BUFFER_USAGE_INDEX, totalIndexBufferSize / sizeof(WORD), sizeof(WORD)));

	for (auto& mesh : tinygltf.meshes)
	{
		for (auto& primitive : mesh.primitives)
		{
			uint32_t vertexPositionsIndex = primitive.attributes.find("POSITION")->second;

			tinygltf::Accessor& vertexAccessor = tinygltf.accessors[vertexPositionsIndex];
			tinygltf::Accessor& indexAccessor = tinygltf.accessors[primitive.indices];

			tinygltf::BufferView& vertexBufferView = tinygltf.bufferViews[vertexAccessor.bufferView];
			tinygltf::BufferView& indexBufferView = tinygltf.bufferViews[indexAccessor.bufferView];

			tinygltf::Buffer& vertexBuffer = tinygltf.buffers[vertexBufferView.buffer];
			tinygltf::Buffer& indexBuffer = tinygltf.buffers[indexBufferView.buffer];

			const unsigned char* vertexDataPtr = &vertexBuffer.data[0] + vertexBufferView.byteOffset;
			memcpy(vertexData + currentVertexDataOffset, vertexDataPtr, vertexBufferView.byteLength);
			currentVertexDataOffset += vertexBufferView.byteLength;

			const unsigned char* indexDataPtr = &indexBuffer.data[0] + indexBufferView.byteOffset;
			memcpy(indexData + currentIndexDataOffset, indexDataPtr, indexBufferView.byteLength);
			currentIndexDataOffset += indexBufferView.byteLength;
		}
	}

	model.VertexBuffer->SetBufferData(vertexData);
	model.IndexBuffer->SetBufferData(indexData);

	delete[] vertexData;
	delete[] indexData;

	return model;
}
