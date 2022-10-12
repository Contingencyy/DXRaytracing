struct DefaultRayPayload
{
	float3 Color;
};

struct Vertex
{
	float3 Position;
	float2 TexCoord;
	float3 Normal;
};

StructuredBuffer<Vertex> vertexBuffer : register(t0, space1);
StructuredBuffer<uint> indexBuffer : register(t0, space2);
Texture2D baseColorTexture : register(t0, space3);

uint3 GetIndices(uint triangleIndex)
{
	uint baseIndex = (triangleIndex * 3);
	return uint3(indexBuffer.Load(baseIndex), indexBuffer.Load(baseIndex + 1), indexBuffer.Load(baseIndex + 2));
}

[shader("closesthit")]
void main(inout DefaultRayPayload payload, BuiltInTriangleIntersectionAttributes attribs)
{
	uint3 indices = GetIndices(PrimitiveIndex());
	float3 weights = float3(1.0f - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
	
	// Get vertex attribs
	Vertex vertex;
	vertex.Position = float3(0.0f, 0.0f, 0.0f);
	vertex.TexCoord = float2(0.0f, 0.0f);
	vertex.Normal = float3(0.0f, 0.0f, 0.0);

	for (uint i = 0; i < 3; i++)
	{
		Vertex loadedVertex = vertexBuffer.Load(indices[i]);
		vertex.Position += loadedVertex.Position * weights[i];
		vertex.TexCoord += loadedVertex.TexCoord * weights[i];
		vertex.Normal += loadedVertex.Normal * weights[i];
	}
	
	// Mod the frag coord to imitate wrapping behaviour of samplers
	vertex.TexCoord = fmod(vertex.TexCoord, 1.0f);

	int2 coord = floor(vertex.TexCoord * 2048);
	float3 color = baseColorTexture.Load(int3(coord, 0)).rgb;
	payload.Color = color;
}
