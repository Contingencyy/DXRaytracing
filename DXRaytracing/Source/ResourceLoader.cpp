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
		for (auto& prim : mesh.primitives)
		{
			uint32_t vertexPosIndex = prim.attributes.find("POSITION")->second;
			tinygltf::Accessor& posAccessor = tinygltf.accessors[vertexPosIndex];
			totalVertexBufferSize += posAccessor.count * posAccessor.ByteStride(tinygltf.bufferViews[posAccessor.bufferView]);

			uint32_t indicesIndex = prim.indices;
			tinygltf::Accessor& indexAccessor = tinygltf.accessors[indicesIndex];
			totalIndexBufferSize += indexAccessor.count * indexAccessor.ByteStride(tinygltf.bufferViews[indexAccessor.bufferView]);
		}
	}

	std::vector<unsigned char> vertexData;
	std::vector<unsigned char> indexData;

	vertexData.resize(totalVertexBufferSize);
	indexData.resize(totalIndexBufferSize);

	std::size_t currentVertexByteOffset = 0;
	std::size_t currentIndexByteOffset = 0;

	for (auto& mesh : tinygltf.meshes)
	{
		for (auto& prim : mesh.primitives)
		{
			// Copy vertex positions
			uint32_t vertexPosIndex = prim.attributes.find("POSITION")->second;
			tinygltf::Accessor& posAccessor = tinygltf.accessors[vertexPosIndex];
			tinygltf::BufferView& posBufferView = tinygltf.bufferViews[posAccessor.bufferView];
			tinygltf::Buffer& posBuffer = tinygltf.buffers[posBufferView.buffer];

			const unsigned char* pData = &posBuffer.data[0] + posBufferView.byteOffset + posAccessor.byteOffset;
			ASSERT(posBufferView.byteOffset + posAccessor.byteOffset < posBuffer.data.size(), "Byte offset for attribute POSITION exceeded the total buffer size");

			std::size_t verticesByteSize = posAccessor.count * posAccessor.ByteStride(posBufferView);
			memcpy(&vertexData[currentVertexByteOffset], pData, verticesByteSize);
			currentVertexByteOffset += verticesByteSize;

			// Copy indices
			uint32_t indicesIndex = prim.indices;
			tinygltf::Accessor& indexAccessor = tinygltf.accessors[indicesIndex];
			tinygltf::BufferView& indexBufferView = tinygltf.bufferViews[indexAccessor.bufferView];
			tinygltf::Buffer& indexBuffer = tinygltf.buffers[indexBufferView.buffer];

			pData = &indexBuffer.data[0] + indexBufferView.byteOffset + indexAccessor.byteOffset;
			ASSERT(indexBufferView.byteOffset + indexAccessor.byteOffset < indexBuffer.data.size(), "Byte offset for INDICES exceeded the total buffer size");

			std::size_t indicesByteSize = indexAccessor.count * indexAccessor.ByteStride(indexBufferView);
			memcpy(&indexData[currentIndexByteOffset], pData, indicesByteSize);
			currentIndexByteOffset += indicesByteSize;
		}
	}

	model.VertexBuffer = std::make_shared<Buffer>("Model vertex buffer", BufferDesc(BufferUsage::BUFFER_USAGE_VERTEX,
		totalVertexBufferSize / sizeof(glm::vec3), sizeof(glm::vec3)), &vertexData[0]);
	model.IndexBuffer = std::make_shared<Buffer>("Model index buffer", BufferDesc(BufferUsage::BUFFER_USAGE_INDEX,
		totalIndexBufferSize / sizeof(WORD), sizeof(WORD)), &indexData[0]);

	return model;
}
