#include "Pch.h"
#include "ResourceLoader.h"
#include "Graphics/Buffer.h"
#include "Graphics/Texture.h"

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

	for (uint32_t matIndex = 0; matIndex < tinygltf.materials.size(); ++matIndex)
	{
		auto& material = tinygltf.materials[matIndex];
		int baseColorTextureIndex = material.pbrMetallicRoughness.baseColorTexture.index;
		int normalTextureIndex = material.normalTexture.index;

		if (baseColorTextureIndex >= 0)
		{
			uint32_t baseColorImageIndex = tinygltf.textures[baseColorTextureIndex].source;
			model.Textures.push_back(std::make_shared<Texture>("Albedo texture", TextureDesc(TextureUsage::TEXTURE_USAGE_READ, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
				tinygltf.images[baseColorImageIndex].width, tinygltf.images[baseColorImageIndex].height), &tinygltf.images[baseColorImageIndex].image[0]));
		}
		else
		{
			uint32_t whiteTextureData = 0xFFFFFFFF;
			model.Textures.push_back(std::make_shared<Texture>("Albedo texture", TextureDesc(TextureUsage::TEXTURE_USAGE_READ, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
				1, 1), &whiteTextureData));
		}

		// Test
		break;
	}

	std::size_t totalVerticesCount = 0, totalIndicesCount = 0;

	std::size_t totalVertexCount = 0;
	std::size_t totalIndexCount = 0;

	for (auto& mesh : tinygltf.meshes)
	{
		for (auto& prim : mesh.primitives)
		{
			uint32_t vertexPosIndex = prim.attributes.find("POSITION")->second;
			const tinygltf::Accessor& vertexPosAccessor = tinygltf.accessors[vertexPosIndex];
			totalVertexCount += vertexPosAccessor.count;

			uint32_t indicesIndex = prim.indices;
			const tinygltf::Accessor& indexAccessor = tinygltf.accessors[indicesIndex];
			totalIndexCount += indexAccessor.count;
		}
	}

	struct Vertex
	{
		glm::vec3 Position;
		glm::vec2 TexCoord;
		glm::vec3 Normal;
	};

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	vertices.reserve(totalVertexCount);
	indices.reserve(totalIndexCount);

	for (auto& mesh : tinygltf.meshes)
	{
		for (auto& prim : mesh.primitives)
		{
			// Get vertex position data
			auto vertexPosAttrib = prim.attributes.find("POSITION");
			ASSERT(vertexPosAttrib != prim.attributes.end(), "GLTF primitive does not contain vertex attribute POSITION");

			uint32_t vertexPosIndex = vertexPosAttrib->second;
			const tinygltf::Accessor& vertexPosAccessor = tinygltf.accessors[vertexPosIndex];
			const tinygltf::BufferView& vertexPosBufferView = tinygltf.bufferViews[vertexPosAccessor.bufferView];
			const tinygltf::Buffer& vertexPosBuffer = tinygltf.buffers[vertexPosBufferView.buffer];

			const float* pVertexPosData = reinterpret_cast<const float*>(&vertexPosBuffer.data[0] + vertexPosBufferView.byteOffset + vertexPosAccessor.byteOffset);
			ASSERT(vertexPosAccessor.count * vertexPosAccessor.ByteStride(vertexPosBufferView) + vertexPosBufferView.byteOffset + vertexPosAccessor.byteOffset <= vertexPosBuffer.data.size(),
				"Byte offset for vertex attribute POSITION exceeded total buffer size");

			// Get vertex tex coord data
			auto vertexTexCoordAttrib = prim.attributes.find("TEXCOORD_0");
			ASSERT(vertexTexCoordAttrib != prim.attributes.end(), "GLTF primitive does not contain vertex attribute TEXCOORD_0");

			uint32_t vertexTexCoordIndex = vertexTexCoordAttrib->second;
			const tinygltf::Accessor& vertexTexCoordAccessor = tinygltf.accessors[vertexTexCoordIndex];
			const tinygltf::BufferView& vertexTexCoordBufferView = tinygltf.bufferViews[vertexTexCoordAccessor.bufferView];
			const tinygltf::Buffer& vertexTexCoordBuffer = tinygltf.buffers[vertexTexCoordBufferView.buffer];

			const float* pVertexTexCoordData = reinterpret_cast<const float*>(&vertexTexCoordBuffer.data[0] + vertexTexCoordBufferView.byteOffset + vertexTexCoordAccessor.byteOffset);
			ASSERT(vertexTexCoordAccessor.count* vertexTexCoordAccessor.ByteStride(vertexTexCoordBufferView) + vertexTexCoordBufferView.byteOffset + vertexTexCoordAccessor.byteOffset <= vertexTexCoordBuffer.data.size(),
				"Byte offset for vertex attribute TEXCOORD_0 exceeded total buffer size");

			// Get vertex normal data
			auto vertexNormalAttrib = prim.attributes.find("NORMAL");
			ASSERT(vertexNormalAttrib != prim.attributes.end(), "GLTF primitive does not contain vertex attribute NORMAL");

			uint32_t vertexNormalIndex = vertexNormalAttrib->second;
			const tinygltf::Accessor& vertexNormalAccessor = tinygltf.accessors[vertexNormalIndex];
			const tinygltf::BufferView& vertexNormalBufferView = tinygltf.bufferViews[vertexNormalAccessor.bufferView];
			const tinygltf::Buffer& vertexNormalBuffer = tinygltf.buffers[vertexNormalBufferView.buffer];

			const float* pVertexNormalData = reinterpret_cast<const float*>(&vertexNormalBuffer.data[0] + vertexNormalBufferView.byteOffset + vertexNormalAccessor.byteOffset);
			ASSERT(vertexNormalAccessor.count* vertexNormalAccessor.ByteStride(vertexNormalBufferView) + vertexNormalBufferView.byteOffset + vertexNormalAccessor.byteOffset <= vertexNormalBuffer.data.size(),
				"Byte offset for vertex attribute NormalITION exceeded total buffer size");

			// Set attribute indices
			uint32_t posIndex = 0;
			uint32_t texCoordIndex = 0;
			uint32_t normalIndex = 0;

			// Construct a vertex from the primitive attributes data and add it to all vertices
			for (uint32_t i = 0; i < vertexPosAccessor.count; ++i)
			{
				Vertex v = {};
				v.Position = glm::vec3(pVertexPosData[posIndex], pVertexPosData[posIndex + 1], pVertexPosData[posIndex + 2]);
				v.TexCoord = glm::vec2(pVertexTexCoordData[texCoordIndex], pVertexTexCoordData[texCoordIndex + 1]);
				v.Normal = glm::vec3(pVertexNormalData[normalIndex], pVertexNormalData[normalIndex + 1], pVertexNormalData[normalIndex + 2]);
				vertices.push_back(v);
				
				posIndex += 3;
				texCoordIndex += 2;
				normalIndex += 3;
			}

			// Get index data
			uint32_t indicesIndex = prim.indices;
			const tinygltf::Accessor& indexAccessor = tinygltf.accessors[indicesIndex];
			const tinygltf::BufferView& indexBufferView = tinygltf.bufferViews[indexAccessor.bufferView];
			const tinygltf::Buffer& indexBuffer = tinygltf.buffers[indexBufferView.buffer];

			const WORD* pIndexData = reinterpret_cast<const WORD*>(&indexBuffer.data[0] + indexBufferView.byteOffset + indexAccessor.byteOffset);
			ASSERT(indexAccessor.count * indexAccessor.ByteStride(indexBufferView) + indexBufferView.byteOffset + indexAccessor.byteOffset,
				"Byte offset for indices exceeded total buffer size");

			// Get indices for current primitive/mesh and add it to all indices
			std::size_t numIndices = 0;

			for (uint32_t i = 0; i < indexAccessor.count; ++i)
			{
				uint32_t index = static_cast<uint32_t>(pIndexData[i]);
				indices.push_back(index);
				numIndices++;
			}
		}
	}

	model.VertexBuffer = std::make_shared<Buffer>("Vertex buffer", BufferDesc(BufferUsage::BUFFER_USAGE_VERTEX | BufferUsage::BUFFER_USAGE_READ, vertices.size(), sizeof(Vertex)), &vertices[0]);
	model.IndexBuffer = std::make_shared<Buffer>("Index buffer", BufferDesc(BufferUsage::BUFFER_USAGE_INDEX | BufferUsage::BUFFER_USAGE_READ, indices.size(), sizeof(uint32_t)), &indices[0]);

	LOG_INFO("[ResourceManager] Loaded model: " + filepath);

	return model;
}
