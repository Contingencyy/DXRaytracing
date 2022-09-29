struct DefaultRayPayload
{
	float3 Color;
};

struct BuiltinIntersectAttribs
{
	uint2 barycentrics;
};

[shader("closesthit")]
void main(inout DefaultRayPayload payload, BuiltinIntersectAttribs attribs)
{
	payload.Color = float3(0.8f, 0.8f, 0.1f);
}
